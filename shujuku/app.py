import os
from datetime import date, datetime

import streamlit as st
import pymysql
import pandas as pd

# =============================
# 0) SDK 导入检查
# =============================
ZAI_AVAILABLE = False
try:
    from zai import ZhipuAiClient

    ZAI_AVAILABLE = True
except Exception:
    ZAI_AVAILABLE = False

try:
    from zhipuai import ZhipuAI  # type: ignore
except Exception:
    ZhipuAI = None

# ==============================================
# 1. 全局配置
# ==============================================
st.set_page_config(
    page_title="校园集市 Pro",
    page_icon="🎓",
    layout="wide",
    initial_sidebar_state="expanded"
)


def safe_get_secret(key: str, default=""):
    try:
        return st.secrets.get(key, default)
    except Exception:
        return default


# 读取配置
ZHIPU_API_KEY = safe_get_secret("ZHIPU_API_KEY", "") or os.getenv("ZHIPU_API_KEY", "")

DB_HOST = safe_get_secret("DB_HOST", "localhost") or os.getenv("DB_HOST", "localhost")
DB_USER = safe_get_secret("DB_USER", "root") or os.getenv("DB_USER", "root")
DB_PASSWORD = safe_get_secret("DB_PASSWORD", "root") or os.getenv("DB_PASSWORD", "root")
DB_NAME = safe_get_secret("DB_NAME", "campus_market") or os.getenv("DB_NAME", "campus_market")
DB_PORT = int(safe_get_secret("DB_PORT", "3306") or os.getenv("DB_PORT", "3306"))


# ==============================================
# 2. 数据库连接工具
# ==============================================
def get_connection():
    return pymysql.connect(
        host=DB_HOST,
        user=DB_USER,
        password=DB_PASSWORD,
        db=DB_NAME,
        port=DB_PORT,
        charset="utf8mb4",
        cursorclass=pymysql.cursors.DictCursor,
        autocommit=False,
    )


def fetch_one(sql: str, params=None):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(sql, params or ())
            row = cur.fetchone()
        conn.commit()
        return row
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


def fetch_all(sql: str, params=None):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(sql, params or ())
            rows = cur.fetchall()
        conn.commit()
        return rows
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


def get_all_users():
    return fetch_all("select user_id, username, balance from users order by user_id")


def get_user_by_username_password(username: str, password: str):
    return fetch_one(
        "select user_id, username, balance from users where username=%s and password=%s",
        (username, password),
    )


def get_categories():
    return fetch_all("select category_id, category_name from categories order by category_id")


# ==============================================
# 3. 核心业务逻辑
# ==============================================
def publish_product(title, description, price, category_id, seller_id):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                """
                insert into products(title, description, price, category_id, seller_id, status, create_time)
                values(%s, %s, %s, %s, %s, '待售', now())
                """,
                (title, description, price, category_id, seller_id),
            )
        conn.commit()
        return True
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


def down_shelf_product(product_id, seller_id):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                """
                update products
                set status='已下架'
                where product_id=%s and seller_id=%s and status='待售'
                """,
                (product_id, seller_id),
            )
            affected = cur.rowcount
        conn.commit()
        return affected == 1
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


def delete_product(product_id, seller_id):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                """
                delete from products
                where product_id=%s and seller_id=%s and status in ('待售','已下架')
                """,
                (product_id, seller_id),
            )
            affected = cur.rowcount
        conn.commit()
        return affected == 1
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


def buy_product(product_id, buyer_id):
    """
    事务操作：保证扣款和商品状态变更原子性
    """
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            # 锁定商品行
            cur.execute(
                "select product_id, price, seller_id, status from products where product_id=%s for update",
                (product_id,),
            )
            p = cur.fetchone()
            if not p:
                conn.rollback()
                return False, "商品不存在"
            if p["status"] != "待售":
                conn.rollback()
                return False, "手慢了，商品已被抢走或下架"
            if int(p["seller_id"]) == int(buyer_id):
                conn.rollback()
                return False, "不能购买自己发布的商品"

            price = float(p["price"])
            seller_id = int(p["seller_id"])

            # 锁定买家余额
            cur.execute("select balance from users where user_id=%s for update", (buyer_id,))
            b = cur.fetchone()
            if not b or float(b["balance"]) < price:
                conn.rollback()
                return False, "余额不足"

            # 执行转账
            cur.execute("update users set balance = balance - %s where user_id=%s", (price, buyer_id))
            cur.execute("update users set balance = balance + %s where user_id=%s", (price, seller_id))

            # 更新商品状态
            cur.execute(
                "update products set buyer_id=%s, status='已售' where product_id=%s",
                (buyer_id, product_id),
            )

        conn.commit()
        return True, "购买成功"
    except Exception as e:
        conn.rollback()
        return False, f"系统繁忙，请稍后重试：{e}"
    finally:
        conn.close()


def publish_task(title, description, reward, requester_id):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("select balance from users where user_id=%s", (requester_id,))
            u = cur.fetchone()
            if not u or float(u["balance"]) < float(reward):
                conn.rollback()
                return False, "余额不足，请先充值"

            cur.execute(
                """
                insert into tasks(title, description, reward, requester_id, runner_id, status, create_time)
                values(%s, %s, %s, %s, null, '待接单', now())
                """,
                (title, description, reward, requester_id),
            )
        conn.commit()
        return True, "任务发布成功"
    except Exception as e:
        conn.rollback()
        return False, f"发布异常：{e}"
    finally:
        conn.close()


def accept_task(task_id, runner_id):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                """
                update tasks
                set runner_id=%s, status='进行中'
                where task_id=%s and status='待接单' and runner_id is null
                """,
                (runner_id, task_id),
            )
            ok = (cur.rowcount == 1)
        conn.commit()
        return ok, ("接单成功，请尽快完成任务" if ok else "接单失败，任务已被他人抢单")
    except Exception as e:
        conn.rollback()
        return False, f"接单异常：{e}"
    finally:
        conn.close()


def complete_task(task_id, operator_user_id):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                "select task_id, reward, requester_id, runner_id, status from tasks where task_id=%s for update",
                (task_id,),
            )
            t = cur.fetchone()
            if not t:
                conn.rollback()
                return False, "任务不存在"

            # 权限校验
            if int(t["requester_id"]) != int(operator_user_id):
                conn.rollback()
                return False, "权限不足：只有发布人可以确认完成"

            if t["status"] != "进行中":
                conn.rollback()
                return False, "任务状态不正确"

            reward = float(t["reward"])
            requester_id = int(t["requester_id"])
            runner_id = int(t["runner_id"])

            # 结算流程
            cur.execute("update users set balance=balance-%s where user_id=%s", (reward, requester_id))
            cur.execute("update users set balance=balance+%s where user_id=%s", (reward, runner_id))
            cur.execute("update tasks set status='已完成' where task_id=%s", (task_id,))

        conn.commit()
        return True, "结算完成，赏金已到账"
    except Exception as e:
        conn.rollback()
        return False, f"结算异常：{e}"
    finally:
        conn.close()


def register_user(username, password, phone):
    """注册新用户，默认赠送1000元初始资金"""
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            # 1. 检查用户名是否已存在
            cur.execute("select user_id from users where username=%s", (username,))
            if cur.fetchone():
                return False, "用户名已存在，请换一个试试"

            # 2. 插入新用户 (注意：这里默认给 balance 设为 1000.00)
            cur.execute(
                """
                insert into users (username, password, phone, balance, created_at)
                values (%s, %s, %s, 1000.00, now())
                """,
                (username, password, phone)
            )
        conn.commit()
        return True, "注册成功！已赠送 ¥1000 初始体验金"
    except Exception as e:
        conn.rollback()
        return False, f"注册失败：{e}"
    finally:
        conn.close()

# ==============================================
# 4. AI 助手服务
# ==============================================
def call_ai_agent(prompt_text, enable_search=False, debug=False, engine="search_pro"):
    if not ZHIPU_API_KEY:
        return "系统提示：未配置 AI 服务密钥，请联系管理员。"

    today = date.today().isoformat()
    system_text = (
        f"你是一个智能校园助手，基于智谱AI的GLM-4模型构建。今天的日期是 {today}。"
        "无论联网搜索结果如何，当被问及身份时，你必须回答你是智谱AI的GLM模型。"
        "请用简练、友好的语气回答同学的问题。"
    )

    user_text = prompt_text
    if enable_search:
        user_text = (
            "请基于联网搜索结果回答用户问题。若无法获取信息请如实告知。\n"
            f"用户问题：{prompt_text}"
        )

    try:
        if ZAI_AVAILABLE:
            client = ZhipuAiClient(api_key=ZHIPU_API_KEY)
            tools = [{"type": "web_search",
                      "web_search": {"enable": True, "search_result": True}}] if enable_search else None

            resp = client.chat.completions.create(
                model="glm-4.6",
                messages=[{"role": "system", "content": system_text}, {"role": "user", "content": user_text}],
                tools=tools,
            )
        else:
            if ZhipuAI is None:
                return "AI SDK 未安装。"
            client = ZhipuAI(api_key=ZHIPU_API_KEY)
            resp = client.chat.completions.create(
                model="glm-4.6",
                messages=[{"role": "system", "content": system_text}, {"role": "user", "content": user_text}],
            )

        # 调试模式下显示原始响应
        if debug:
            try:
                st.json(resp.model_dump())
            except:
                st.write(resp)

        try:
            return (resp.choices[0].message.content or "").strip()
        except:
            return str(resp)

    except Exception as e:
        return f"AI 服务暂时不可用: {str(e)}"


# ==============================================
# 5. 前端界面
# ==============================================
st.title("🎓 校园集市 Pro")
st.markdown("欢迎使用一站式校园生活服务平台")

with st.sidebar:
    st.header("👤 用户中心")
    # 增加了 "新用户注册" 选项
    mode = st.radio("模式选择", ["账号登录", "新用户注册", "快捷演示"], horizontal=True)

    # --- 模式1：账号登录 ---
    if mode == "账号登录":
        st.subheader("🔑 登录")
        u = st.text_input("学号/用户名", key="login_u")
        p = st.text_input("密码", type="password", key="login_p")
        if st.button("立即登录", type="primary", use_container_width=True):
            row = get_user_by_username_password(u.strip(), p)
            if row:
                st.session_state["user_id"] = row["user_id"]
                st.session_state["username"] = row["username"]
                st.success(f"欢迎回来，{row['username']}")
                st.rerun()
            else:
                st.error("账号或密码错误")

    # --- 模式2：新用户注册 (新增部分) ---
    elif mode == "新用户注册":
        st.subheader("📝 注册")
        with st.form("reg_form"):
            new_u = st.text_input("设置用户名", placeholder="字母或数字")
            new_p = st.text_input("设置密码", type="password")
            confirm_p = st.text_input("确认密码", type="password")
            new_phone = st.text_input("手机号 (选填)", placeholder="用于联系跑腿")

            if st.form_submit_button("提交注册", use_container_width=True):
                if not new_u or not new_p:
                    st.error("用户名和密码不能为空")
                elif new_p != confirm_p:
                    st.error("两次输入的密码不一致")
                else:
                    # 调用刚才写的注册函数
                    ok, msg = register_user(new_u.strip(), new_p, new_phone)
                    if ok:
                        st.success(msg)
                        # 注册成功后，提示用户去登录，或者你可以自动帮他登录（这里简单点，让他手动登）
                        st.info("请切换到【账号登录】页签进行登录")
                    else:
                        st.error(msg)

    # --- 模式3：快捷演示 ---
    else:
        st.subheader("🚀 演示模式")
        st.caption("无需密码，直接模拟身份进入")
        users = get_all_users()
        if not users:
            st.warning("暂无用户数据")
        else:
            options = {f'{x["username"]} (余额: ¥{x["balance"]})': x for x in users}
            pick = st.selectbox("选择演示账号", list(options.keys()))
            if st.button("一键进入系统", type="primary", use_container_width=True):
                x = options[pick]
                st.session_state["user_id"] = x["user_id"]
                st.session_state["username"] = x["username"]
                st.rerun()

    # --- 底部：显示当前登录状态 ---
    if "user_id" in st.session_state:
        st.divider()
        st.markdown(f"当前用户：**{st.session_state['username']}**")
        # 实时查余额
        bal_row = fetch_one("select balance from users where user_id=%s", (st.session_state["user_id"],))
        balance_show = bal_row["balance"] if bal_row else 0.00
        st.metric("我的钱包", f'¥ {balance_show:.2f}')

        if st.button("退出登录", use_container_width=True):
            del st.session_state["user_id"]
            if "username" in st.session_state:
                del st.session_state["username"]
            st.rerun()

if "user_id" not in st.session_state:
    st.info("👋 请先在左侧侧边栏登录系统。")
    st.stop()

current_user_id = int(st.session_state["user_id"])

# 标签页导航
tab1, tab2, tab3, tab4, tab5 = st.tabs(
    ["🛒 跳蚤市场", "📦 个人中心", "🏃 跑腿大厅", "📊 数据看板", "🤖 智能助手"]
)

# -----------------
# Tab 1: 市场
# -----------------
with tab1:
    st.subheader("🛒 最新闲置好物")

    col_search, col_filter = st.columns([3, 1])
    with col_search:
        search_kw = st.text_input("🔍 搜索商品", placeholder="输入关键词...")

    base_sql = """
        select
            p.product_id, p.title, p.description, p.price,
            c.category_name, u.username as seller_name,
            p.seller_id, p.status, p.create_time
        from products p
        join users u on p.seller_id = u.user_id
        left join categories c on p.category_id = c.category_id
        where p.status='待售'
    """
    params = []
    if search_kw:
        base_sql += " and p.title like %s"
        params.append(f"%{search_kw}%")
    base_sql += " order by p.create_time desc"

    rows = fetch_all(base_sql, tuple(params))
    df = pd.DataFrame(rows)

    if df.empty:
        st.info("暂时没有符合条件的商品。")
    else:
        st.dataframe(
            df[["title", "price", "category_name", "seller_name", "create_time", "product_id"]],
            column_config={
                "title": "商品名称",
                "price": st.column_config.NumberColumn("价格", format="¥ %.2f"),
                "category_name": "分类",
                "seller_name": "卖家",
                "create_time": "发布时间",
                "product_id": "ID"
            },
            use_container_width=True,
            hide_index=True
        )

        st.markdown("#### 🛍️ 购买操作")
        pid = st.selectbox("选择要购买的商品ID", df["product_id"].tolist())
        if st.button("💳 立即购买", type="primary"):
            ok, msg = buy_product(int(pid), current_user_id)
            (st.success if ok else st.error)(msg)
            if ok:
                st.rerun()

# -----------------
# Tab 2: 个人中心
# -----------------
with tab2:
    col_a, col_b = st.columns(2)

    with col_a:
        st.subheader("📤 发布闲置")
        with st.form("pub_form"):
            title = st.text_input("商品标题", placeholder="如：九成新机械键盘")
            desc = st.text_area("详细描述")
            price = st.number_input("出售价格 (¥)", min_value=0.0, step=1.0)

            cats = get_categories()
            cat_map = {f'{c["category_name"]}': c["category_id"] for c in cats} if cats else {}
            cat_pick = st.selectbox("选择分类", list(cat_map.keys())) if cats else None

            if st.form_submit_button("立即发布"):
                if title and cat_pick:
                    publish_product(title, desc, price, cat_map[cat_pick], current_user_id)
                    st.success("发布成功！")
                    st.rerun()
                else:
                    st.error("请填写完整信息")

    with col_b:
        st.subheader("📦 我的发布记录")
        my_rows = fetch_all(
            "select product_id, title, price, status, create_time from products where seller_id=%s order by create_time desc",
            (current_user_id,)
        )
        my_df = pd.DataFrame(my_rows)

        if not my_df.empty:
            st.dataframe(
                my_df,
                column_config={"title": "标题", "price": "价格", "status": "状态", "create_time": "时间"},
                use_container_width=True,
                hide_index=True
            )

            st.markdown("#### 商品管理")
            sel_pid = st.selectbox("选择操作商品", my_df["product_id"].tolist())
            c1, c2 = st.columns(2)
            with c1:
                if st.button("📉 下架商品"):
                    ok = down_shelf_product(sel_pid, current_user_id)
                    (st.success if ok else st.error)("操作完成")
                    st.rerun()
            with c2:
                if st.button("🗑️ 删除记录"):
                    ok = delete_product(sel_pid, current_user_id)
                    (st.success if ok else st.error)("删除成功" if ok else "只能删除待售或已下架商品")
                    st.rerun()
        else:
            st.info("暂无发布记录")

# -----------------
# Tab 3: 跑腿
# -----------------
with tab3:
    st.subheader("🏃 悬赏大厅")

    # 发布任务区
    with st.expander("➕ 发布新求助", expanded=False):
        with st.form("task_form"):
            c1, c2 = st.columns([3, 1])
            t_title = c1.text_input("需要什么帮助？", placeholder="如：帮取中通快递")
            reward = c2.number_input("跑腿费 (¥)", min_value=1.0, value=2.0)
            t_desc = st.text_area("备注详情", placeholder="如：送到男生宿舍7栋...")

            if st.form_submit_button("发布悬赏"):
                ok, msg = publish_task(t_title, t_desc, reward, current_user_id)
                (st.success if ok else st.error)(msg)
                st.rerun()

    # 任务列表
    task_rows = fetch_all("""
        select t.task_id, t.title, t.reward, t.status, u.username as requester
        from tasks t join users u on t.requester_id = u.user_id
        order by t.create_time desc
    """)
    task_df = pd.DataFrame(task_rows)

    st.dataframe(
        task_df,
        column_config={
            "task_id": "ID", "title": "任务内容", "reward": st.column_config.NumberColumn("赏金", format="¥ %.2f"),
            "status": "当前状态", "requester": "发布人"
        },
        use_container_width=True,
        hide_index=True
    )

    if not task_df.empty:
        st.markdown("#### 任务操作")
        tid = st.selectbox("选择任务", task_df["task_id"].tolist())

        c1, c2 = st.columns(2)
        with c1:
            if st.button("🤝 我来接单"):
                ok, msg = accept_task(tid, current_user_id)
                (st.success if ok else st.error)(msg)
                st.rerun()
        with c2:
            if st.button("✅ 确认完成 (仅发布人可用)"):
                ok, msg = complete_task(tid, current_user_id)
                (st.success if ok else st.error)(msg)
                st.rerun()

# -----------------
# Tab 4: 看板
# -----------------
with tab4:
    st.subheader("📊 平台运营数据")

    col1, col2, col3 = st.columns(3)

    # 图表 1
    cat_df = pd.DataFrame(fetch_all("""
        select c.category_name, count(*) as num 
        from products p join categories c on p.category_id=c.category_id 
        where p.status='待售' group by c.category_name
    """))
    with col1:
        st.markdown("**📦 分类库存占比**")
        if not cat_df.empty:
            st.bar_chart(cat_df.set_index("category_name"))
        else:
            st.caption("暂无数据")

    # 图表 2
    stat_df = pd.DataFrame(fetch_all("select status, count(*) as num from products group by status"))
    with col2:
        st.markdown("**📈 商品流转状态**")
        if not stat_df.empty:
            st.bar_chart(stat_df.set_index("status"), color="#FFAA00")
        else:
            st.caption("暂无数据")

    # 图表 3
    task_stat_df = pd.DataFrame(fetch_all("select status, count(*) as num from tasks group by status"))
    with col3:
        st.markdown("**🏃 跑腿任务完成度**")
        if not task_stat_df.empty:
            st.bar_chart(task_stat_df.set_index("status"), color="#00CC96")
        else:
            st.caption("暂无数据")

# -----------------
# Tab 5: AI 助手
# -----------------
with tab5:
    st.subheader("🤖 智能校园助手")
    st.caption("基于 GLM-4 模型，支持联网查询校园资讯、商品比价等。")

    col_chat, col_setting = st.columns([4, 1])

    with col_setting:
        use_net = st.toggle("启用联网搜索", value=True)
        show_debug = st.checkbox("调试模式", value=False)

    prompt = st.chat_input("有问题？问问 AI 吧...")
    if prompt:
        with st.chat_message("user"):
            st.write(prompt)

        with st.chat_message("assistant"):
            with st.spinner("正在思考中..."):
                reply = call_ai_agent(prompt, enable_search=use_net, debug=show_debug)
                st.write(reply)