#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <string>
using namespace std;
//复数类定义
class Complex {
private:
double real, imag;
public:
Complex(double r = 0.0, double i = 0.0) : real(r), imag(i) {}
double getReal() const { return real; }
double getImag() const { return imag; }
double getMod() const { return sqrt(real * real + imag * imag); }
bool operator==(const Complex& other) const {
return real == other.real && imag == other.imag;
}
bool operator<(const Complex& other) const {
double m1 = getMod(), m2 = other.getMod();
if (m1 != m2) return m1 < m2;
return real < other.real;
}
friend ostream& operator<<(ostream& os, const Complex& c) {
os << fixed << setprecision(1) << "(" << c.real << " + " << c.imag << "i)";
return os;
}
};
void shuffle(vector<Complex>& v) {
int n = v.size();
for (int i = n - 1; i > 0; --i) {
int j = rand() % (i + 1);
swap(v[i], v[j]);
}
}
// 打印前 10 个
void showVector(const vector<Complex>& v, int limit = 10) {
int sz = min(limit, (int)v.size());
cout << "Vector (first " << sz << "): ";
for (int i = 0; i < sz; ++i) cout << v[i] << " ";
cout << endl << "Size: " << v.size() << endl;
}
// 查找
int findComplex(const vector<Complex>& vec, const Complex& target) {
for (size_t i = 0; i < vec.size(); ++i)
if (vec[i] == target) return i;
return -1;
}
// 起泡排序
void bubbleSort(vector<Complex>& vec) {
int n = vec.size();
for (int i = 0; i < n - 1; ++i)
for (int j = 0; j < n - 1 - i; ++j)
if (vec[j + 1] < vec[j])
swap(vec[j], vec[j + 1]);
}
// 归并排序
void mergeSort(vector<Complex>& vec, int left, int right) {
if (left >= right) return;
int mid = left + (right - left) / 2;
mergeSort(vec, left, mid);
mergeSort(vec, mid + 1, right);
vector<Complex> temp(right - left + 1);
int i = left, j = mid + 1, k = 0;
while (i <= mid && j <= right)
temp[k++] = (vec[i] < vec[j]) ? vec[i++] : vec[j++];
while (i <= mid) temp[k++] = vec[i++];
while (j <= right) temp[k++] = vec[j++];
for (int p = 0; p < k; ++p) vec[left + p] = temp[p];
}
void mergeSort(vector<Complex>& vec) {
if (!vec.empty()) mergeSort(vec, 0, vec.size() - 1);
}
// 二分找下界
int findmod(const vector<Complex>& vec, double key) {
int left = 0, right = vec.size();
while (left < right) {
int mid = left + (right - left) / 2;
if (vec[mid].getMod() < key) left = mid + 1;
else right = mid;
}
return left;
}
// 区间查找
vector<Complex> rangeFind(const vector<Complex>& vec, double m1, double
m2) {
vector<Complex> res;
int start = findmod(vec, m1);
int end = findmod(vec, m2);
for (int i = start; i < end; ++i)
res.push_back(vec[i]);
return res;
}
// 生成随机向量
vector<Complex> generateRandomVector(int size) {
vector<Complex> res;
for (int i = 0; i < size; ++i) {
double r = rand() % 201 - 100;
double im = rand() % 201 - 100;
res.push_back(Complex(r, im));
}
return res;
}
int main() {
srand(time(NULL));
cout << fixed << setprecision(1);
// 生成无序向量并测试
vector<Complex> v = generateRandomVector(100);
cout << "Original vector:" << endl;
showVector(v);
//测试置乱
shuffle(v);
cout << "置乱后: ";
int limit = min(10, (int)v.size());
for (int i = 0; i < limit; ++i) cout << v[i] << " ";
cout << " ... (Size: " << v.size() << ")" << endl;
//测试查找
if (!v.empty()) {
Complex target = v[0];
int idx = findComplex(v, target);
cout << "查找 " << target << " 在位置: " << idx << endl;
}
//测试插入
v.insert(v.begin() + 2, Complex(99.0, 99.0));
cout << "插入后: ";
for (int i = 0; i < min(10, (int)v.size()); ++i) cout << v[i] << " ";
cout << " ... (Size: " << v.size() << ")" << endl;
//测试删除
v.erase(v.begin() + 1);
cout << "删除后: ";
for (int i = 0; i < min(10, (int)v.size()); ++i) cout << v[i] << " ";
cout << " ... (Size: " << v.size() << ")" << endl;
//测试唯一化
sort(v.begin(), v.end());
auto it = unique(v.begin(), v.end());
v.erase(it, v.end());
cout << "唯一化后: ";
for (int i = 0; i < min(10, (int)v.size()); ++i) cout << v[i] << " ";
cout << " ... (Size: " << v.size() << ")" << endl;
//排序算法比较
v = generateRandomVector(100);
sort(v.begin(), v.end());
cout << "\nSorted vector for (2) and (3):" << endl;
showVector(v);
//创建顺序，乱序，逆序
vector<vector<Complex>> cases(3);
cases[0] = v;//顺序
cases[1] = v; shuffle(cases[1]);//乱序
cases[2] = v; reverse(cases[2].begin(), cases[2].end());//逆序
string labels[] = {"sorted", "shuffled", "reversed"};
for (int i = 0; i < 3; ++i) {
auto vec = cases[i];
//测试冒泡排序
clock_t t = clock();
bubbleSort(vec);
double bt = (double)(clock() - t) / CLOCKS_PER_SEC;
cout << "\nBubble on " << labels[i] << ": " << bt << "s";
//测试归并排序
vec = cases[i];
t = clock();
mergeSort(vec);
double mt = (double)(clock() - t) / CLOCKS_PER_SEC;
cout << "\nMerge on " << labels[i] << ": " << mt << "s" << endl;
}
//测试区间查找
double m1 = 2.0, m2 = 5.0;
auto sub = rangeFind(v, m1, m2);
cout << "\nRange find [" << m1 << ", " << m2 << "):" << endl;
showVector(sub);
return 0;
}