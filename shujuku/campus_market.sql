create database campus_market character set utf8mb4 collate utf8mb4_unicode_ci;
use campus_market;

-- 创建用户表
create table users (
    user_id int auto_increment primary key comment '用户id',
    username varchar(50) not null unique comment '用户名',
    password varchar(50) not null comment '密码',
    phone varchar(20) comment '联系电话',
    balance decimal(10, 2) default 0.00 comment '钱包余额',
    created_at datetime default current_timestamp comment '注册时间'
);

-- 创建商品类目表
create table categories (
    category_id int auto_increment primary key,
    category_name varchar(50) not null unique
);

-- 创建商品表
create table products (
    product_id int primary key auto_increment,  -- 商品id，自动增加
    title varchar(100) not null,                -- 标题
    description varchar(255),                   -- 描述（用简短的varchar代替text）
    price decimal(10, 2) not null,              -- 价格
    category_id int not null,                   -- 分类id  ✅(改动1：加 not null)
    seller_id int not null,                     -- 卖家id
    buyer_id int,                               -- 买家id
    status varchar(20) default '待售',           -- 状态（待售/已售）
    create_time datetime default current_timestamp,  -- 创建时间 ✅(改动2：加默认当前时间)

    -- 外键约束
    foreign key (category_id) references categories(category_id),
    foreign key (seller_id) references users(user_id),
    foreign key (buyer_id) references users(user_id)
);

-- 创建跑腿任务表 (tasks)
create table tasks (
    task_id int primary key auto_increment,    -- 任务id
    title varchar(100) not null,               -- 任务标题
    description varchar(255),                  -- 任务详情
    reward decimal(10, 2) not null,            -- 跑腿费
    requester_id int not null,                 -- 发布人id
    runner_id int,                             -- 接单人id
    status varchar(20) default '待接单',        -- 状态(待接单/进行中/已完成)
    create_time datetime default current_timestamp,  -- 创建时间 ✅(改动3：加默认当前时间)

    foreign key (requester_id) references users(user_id),
    foreign key (runner_id) references users(user_id)
);

-- 6. 插入模拟数据
-- 插入用户
insert into users (username, password, phone, balance, created_at) values 
('zhang_san', '123456', '13800000001', 500.00, '2023-10-01 10:00:00'),
('li_si', '123456', '13800000002', 120.50, '2023-10-02 11:00:00'),
('wang_wu', '123456', '13800000003', 0.00, '2023-10-03 12:00:00');

-- 插入分类
insert into categories (category_name) values 
('电子数码'), 
('书籍资料'), 
('生活用品'), 
('运动器材');

-- 插入商品
insert into products (title, description, price, category_id, seller_id, status, create_time) 
values ('iPhone 11 九成新', '换新机了闲置出', 2000.00, 1, 1, '待售', '2023-10-20 14:00:00');

insert into products (title, description, price, category_id, seller_id, buyer_id, status, create_time) 
values ('肖秀荣考研精讲', '上面有笔记', 15.00, 2, 2, 3, '已售', '2023-10-21 15:30:00');

-- 插入跑腿任务
insert into tasks (title, description, reward, requester_id, status, create_time) 
values ('帮买一杯霸王茶姬', '送到男生宿舍7栋302', 5.00, 3, '待接单', '2023-10-27 09:00:00');

insert into tasks (title, description, reward, requester_id, runner_id, status, create_time) 
values ('顺丰快递取件', '很重，最好有车', 10.00, 1, 2, '进行中', '2023-10-27 10:30:00');

-- 商品下架（注意：%s 是 python 占位符，sql 脚本不能直接跑）
-- ✅(改动4：改成一个能直接执行的示例)
update products
set status='已下架'
where product_id=1 and seller_id=1 and status='待售';

-- 数据可视化
select c.category_name, count(*) as cnt
from products p
left join categories c on p.category_id=c.category_id
where p.status='待售'
group by c.category_name;

select status, count(*) as cnt
from products
group by status;

select status, count(*) as cnt
from tasks
group by status;
