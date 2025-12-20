#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <limits>
#include <functional>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <string>

using namespace std;

struct Edge {
    int to;
    int w;
};

class Graph {
public:
    int n;
    bool directed;
    vector<vector<Edge> > adj;

    Graph(int n = 0, bool directed = false) : n(n), directed(directed), adj(n) {}

    void addEdge(int u, int v, int w = 1) {
        adj[u].push_back((Edge){v, w});
        if (!directed) adj[v].push_back((Edge){u, w});
    }

    // ---------- (1) 输出邻接矩阵 ----------
    void printAdjMatrix() const {
        const int INF = 1000000000;
        vector<vector<int> > mat(n, vector<int>(n, INF));
        for (int i = 0; i < n; ++i) mat[i][i] = 0;
        for (int u = 0; u < n; ++u) {
            for (size_t k = 0; k < adj[u].size(); ++k) {
                Edge e = adj[u][k];
                if (e.w < mat[u][e.to]) mat[u][e.to] = e.w;
            }
        }

        cout << "邻接矩阵 (INF 表示无边):\n";
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (mat[i][j] == INF) cout << "INF ";
                else cout << mat[i][j] << " ";
            }
            cout << "\n";
        }
    }

    // ---------- (2) BFS ----------
    void bfs(int s, const vector<char> &label) const {
        vector<bool> vis(n, false);
        queue<int> q;
        q.push(s);
        vis[s] = true;
        cout << "BFS: ";
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            cout << label[u] << " ";
            for (size_t k = 0; k < adj[u].size(); ++k) {
                int v = adj[u][k].to;
                if (!vis[v]) {
                    vis[v] = true;
                    q.push(v);
                }
            }
        }
        cout << "\n";
    }

    // ---------- (2) DFS ----------
    void dfsUtil(int u, vector<bool> &vis, const vector<char> &label) const {
        vis[u] = true;
        cout << label[u] << " ";
        for (size_t k = 0; k < adj[u].size(); ++k) {
            int v = adj[u][k].to;
            if (!vis[v]) dfsUtil(v, vis, label);
        }
    }

    void dfs(int s, const vector<char> &label) const {
        vector<bool> vis(n, false);
        cout << "DFS: ";
        dfsUtil(s, vis, label);
        cout << "\n";
    }

    // ---------- (3) Dijkstra 最短路径 ----------
    void dijkstra(int s, const vector<char> &label) const {
        const int INF = 1000000000;
        vector<int> dist(n, INF), pre(n, -1);
        typedef pair<int,int> P;
        priority_queue<P, vector<P>, greater<P> > pq;

        dist[s] = 0;
        pq.push(P(0, s));

        while (!pq.empty()) {
            P top = pq.top();
            pq.pop();
            int d = top.first;
            int u = top.second;

            if (d != dist[u]) continue;
            for (size_t k = 0; k < adj[u].size(); ++k) {
                Edge e = adj[u][k];
                int v = e.to;
                int w = e.w;
                if (dist[v] > dist[u] + w) {
                    dist[v] = dist[u] + w;
                    pre[v] = u;
                    pq.push(P(dist[v], v));
                }
            }
        }

        cout << "从 " << label[s] << " 出发的最短路径：\n";
        for (int i = 0; i < n; ++i) {
            cout << label[s] << " -> " << label[i] << " : ";
            if (dist[i] == INF) {
                cout << "不可达\n";
            } else {
                cout << dist[i] << " , 路径: ";
                vector<int> path;
                for (int v = i; v != -1; v = pre[v]) path.push_back(v);
                reverse(path.begin(), path.end());
                for (size_t k = 0; k < path.size(); ++k) {
                    cout << label[path[k]] << " ";
                }
                cout << "\n";
            }
        }
    }

    // ---------- (3) Prim 最小生成树 ----------
    void primMST(int start, const vector<char> &label) const {
        const int INF = 1000000000;
        vector<int> low(n, INF), pre(n, -1);
        vector<bool> inMST(n, false);

        low[start] = 0;
        for (int i = 0; i < n; ++i) {
            int u = -1;
            for (int j = 0; j < n; ++j) {
                if (!inMST[j] && (u == -1 || low[j] < low[u])) u = j;
            }
            if (u == -1 || low[u] == INF) break;
            inMST[u] = true;

            for (size_t k = 0; k < adj[u].size(); ++k) {
                Edge e = adj[u][k];
                int v = e.to;
                int w = e.w;
                if (!inMST[v] && w < low[v]) {
                    low[v] = w;
                    pre[v] = u;
                }
            }
        }

        int total = 0;
        cout << "Prim 最小生成树的边：\n";
        for (int v = 0; v < n; ++v) {
            if (v == start) continue;
            if (pre[v] != -1) {
                cout << label[pre[v]] << " - " << label[v]
                     << " (w=" << low[v] << ")\n";
                total += low[v];
            }
        }
        cout << "总权值 = " << total << "\n";
    }

    // ---------- (4) 双连通分量 & 关节点 ----------
    void biconnectedComponents(const vector<char> &label) const {
        if (directed) {
            cerr << "双连通分量只对无向图定义。\n";
            return;
        }

        int timeDfs = 0;
        vector<int> disc(n, -1), low(n, -1), parent(n, -1);
        vector<bool> isAP(n, false);
        vector<pair<int,int> > st;
        vector<vector<pair<int,int> > > bccs;

        // 用 std::function + lambda 实现递归
        std::function<void(int)> dfsBC;
        dfsBC = [&](int u) {
            disc[u] = low[u] = ++timeDfs;
            int child = 0;
            for (size_t i = 0; i < adj[u].size(); ++i) {
                int v = adj[u][i].to;
                if (disc[v] == -1) {
                    parent[v] = u;
                    child++;
                    st.push_back(make_pair(u, v));
                    dfsBC(v);
                    if (low[v] < low[u]) low[u] = low[v];

                    if ((parent[u] == -1 && child > 1) ||
                        (parent[u] != -1 && low[v] >= disc[u])) {
                        isAP[u] = true;
                        vector<pair<int,int> > comp;
                        while (!st.empty()) {
                            pair<int,int> e2 = st.back();
                            st.pop_back();
                            comp.push_back(e2);
                            if (e2.first == u && e2.second == v) break;
                        }
                        bccs.push_back(comp);
                    }
                } else if (v != parent[u] && disc[v] < disc[u]) {
                    if (disc[v] < low[u]) low[u] = disc[v];
                    st.push_back(make_pair(u, v));
                }
            }
        };

        for (int i = 0; i < n; ++i) {
            if (disc[i] == -1) {
                dfsBC(i);
                if (!st.empty()) {
                    vector<pair<int,int> > comp;
                    while (!st.empty()) {
                        comp.push_back(st.back());
                        st.pop_back();
                    }
                    bccs.push_back(comp);
                }
            }
        }

        cout << "双连通分量（顶点集合）：\n";
        int id = 1;
        for (size_t i = 0; i < bccs.size(); ++i) {
            cout << "BCC " << id++ << ": ";
            set<int> vs;
            for (size_t j = 0; j < bccs[i].size(); ++j) {
                vs.insert(bccs[i][j].first);
                vs.insert(bccs[i][j].second);
            }
            for (set<int>::iterator it = vs.begin(); it != vs.end(); ++it) {
                cout << label[*it] << " ";
            }
            cout << "\n";
        }

        cout << "关节点: ";
        bool any = false;
        for (int i = 0; i < n; ++i) {
            if (isAP[i]) {
                cout << label[i] << " ";
                any = true;
            }
        }
        if (!any) cout << "无";
        cout << "\n";
    }
};

int main() {
    // ================= 图 1 =================
    // A,B,C,D,E,F,G,H => 0..7
    int n1 = 8;
    Graph g1(n1, false);
    vector<char> label1(n1);
    for (int i = 0; i < n1; ++i) label1[i] = 'A' + i;

    // 图1的边（无向带权图）
    g1.addEdge(0, 1, 4);   // A-B
    g1.addEdge(1, 2, 12);  // B-C
    g1.addEdge(0, 3, 6);   // A-D
    g1.addEdge(3, 6, 2);   // D-G
    g1.addEdge(0, 6, 7);   // A-G
    g1.addEdge(2, 3, 9);   // C-D
    g1.addEdge(2, 4, 1);   // C-E
    g1.addEdge(3, 4, 13);  // D-E
    g1.addEdge(4, 6, 11);  // E-G
    g1.addEdge(4, 7, 8);   // E-H
    g1.addEdge(4, 5, 5);   // E-F
    g1.addEdge(2, 5, 2);   // C-F
    g1.addEdge(5, 7, 3);   // F-H
    g1.addEdge(6, 7, 14);  // G-H
    g1.addEdge(2, 7, 10);  // C-H

    cout << "===== 图1 =====\n";
    g1.printAdjMatrix();
    int s1 = 0;   // 从 A 开始
    g1.bfs(s1, label1);
    g1.dfs(s1, label1);
    g1.dijkstra(s1, label1);
    g1.primMST(s1, label1);

    // ================= 图 2 =================
    // A..L => 0..11
    int n2 = 12;
    Graph g2(n2, false);
    vector<char> label2(n2);
    for (int i = 0; i < n2; ++i) label2[i] = 'A' + i;

    // 图2的边（无向无权图，权值统一写 1）
    // 垂直
    g2.addEdge(0, 4, 1);   // A-E
    g2.addEdge(4, 8, 1);   // E-I
    g2.addEdge(1, 5, 1);   // B-F
    g2.addEdge(5, 9, 1);   // F-J
    g2.addEdge(2, 6, 1);   // C-G
    g2.addEdge(6, 10, 1);  // G-K
    g2.addEdge(3, 7, 1);   // D-H
    g2.addEdge(7, 11, 1);  // H-L

    // 水平
    g2.addEdge(0, 1, 1);   // A-B
    g2.addEdge(2, 3, 1);   // C-D
    g2.addEdge(4, 5, 1);   // E-F
    g2.addEdge(5, 6, 1);   // F-G
    g2.addEdge(6, 7, 1);   // G-H
    g2.addEdge(8, 9, 1);   // I-J
    g2.addEdge(9, 10, 1);  // J-K
    g2.addEdge(10, 11, 1); // K-L

    // 斜边
    g2.addEdge(2, 5, 1);   // C-F
    g2.addEdge(5, 8, 1);   // F-I
    g2.addEdge(5, 10, 1);  // F-K
    g2.addEdge(2, 7, 1);   // C-H

    cout << "\n===== 图2 =====\n";
    g2.biconnectedComponents(label2);

    return 0;
}
