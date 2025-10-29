#include <iostream>
#include <vector>
#include <stack>
#include <cstdlib>
#include <ctime>
using namespace std;
// (1) 计算最大矩形面积函数
int largestArea(vector<int>& heights) {
int n = heights.size();
if (n == 0) return 0;
stack<int> s;
vector<int> h = heights;
h.insert(h.begin(), 0);
h.push_back(0);
int maxArea = 0;
for (int i = 0; i < h.size(); ++i) {
while (!s.empty() && h[s.top()] > h[i]) {
int height = h[s.top()]; s.pop();
int left = s.empty() ? -1 : s.top();
int width = i - left - 1;
maxArea = max(maxArea, height * width);
}
s.push(i);
}
return maxArea;
}
int main() {
srand(time(nullptr));
for (int test = 0; test < 10; ++test) {
// 随机生成 n
int n = (test == 0) ? 1 :
(test == 1) ? 100000 :
(rand() % 1000 + 1);
vector<int> heights(n);
for (int i = 0; i < n; ++i) {
heights[i] = rand() % 10001; // 0 ~ 10000
}
int area = largestArea(heights);
cout << "输入: heights = [" << n << " 个元素]\n";
cout << "输出: " << area << "\n";
cout << "解释: 最大的矩形面积为 " << area << "\n";
}
return 0;
}