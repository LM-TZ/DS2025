#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <map>
#include <vector>
#include <cctype>
#include <algorithm>
#include <cstring>
using namespace std;

//Bitmap类
class Bitmap {
private:
    unsigned char* M;
    int N;
    
public:
    Bitmap(int n = 8) {
        M = new unsigned char[N = (n + 7) / 8];
        memset(M, 0, N);
    }
    
    ~Bitmap() { delete[] M; }
    
    void set(int k) {
        M[k >> 3] |= (0x80 >> (k & 0x07));
    }
    
    bool test(int k) {
        return M[k >> 3] & (0x80 >> (k & 0x07));
    }
};

//HuffCode类
class HuffCode {
private:
    Bitmap* bitmap;
    int length;
    
public:
    HuffCode() : length(0) {
        bitmap = new Bitmap(256);
    }
    
    ~HuffCode() { delete bitmap; }
    
    void append(bool bit) {
        if (bit) bitmap->set(length);
        length++;
    }
    
    string toString() const {
        string result;
        for (int i = 0; i < length; i++) {
            result += bitmap->test(i) ? '1' : '0';
        }
        return result;
    }
};

//二叉树节点 
struct HuffNode {
    char ch;
    int weight;
    HuffNode *left, *right;
    
    HuffNode(char c = '^', int w = 0) : ch(c), weight(w), left(NULL), right(NULL) {}
};

//Huffman树类
class HuffTree {
private:
    HuffNode* root;
    
    // 递归生成编码
    void generateCodes(HuffNode* node, string code, map<char, string>& table) {
        if (!node) return;
        
        if (!node->left && !node->right) {  // 叶子节点
            table[node->ch] = code.empty() ? "0" : code;
            return;
        }
        
        generateCodes(node->left, code + "0", table);
        generateCodes(node->right, code + "1", table);
    }
    
    // 递归释放内存
    void destroy(HuffNode* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
    
public:
    HuffTree() : root(NULL) {}
    ~HuffTree() { destroy(root); }
    
    // 构建Huffman树
    void build(map<char, int>& freq) {
        // 定义优先队列的比较函数
        struct Compare {
            bool operator()(HuffNode* a, HuffNode* b) {
                return a->weight > b->weight;
            }
        };
        
        priority_queue<HuffNode*, vector<HuffNode*>, Compare> pq;
        
        // 将所有字符加入优先队列
        for (map<char, int>::iterator it = freq.begin(); it != freq.end(); ++it) {
            pq.push(new HuffNode(it->first, it->second));
        }
        
        // 构建Huffman树
        while (pq.size() > 1) {
            HuffNode* left = pq.top(); pq.pop();
            HuffNode* right = pq.top(); pq.pop();
            
            HuffNode* parent = new HuffNode('^', left->weight + right->weight);
            parent->left = left;
            parent->right = right;
            pq.push(parent);
        }
        
        root = pq.top();
    }
    
    // 生成编码表
    map<char, string> getCodes() {
        map<char, string> table;
        generateCodes(root, "", table);
        return table;
    }
    
    // 打印树结构
    void print(HuffNode* node = NULL, string prefix = "", bool isLeft = true) {
        if (node == NULL) node = root;
        if (!node) return;
        
        cout << prefix;
        cout << (isLeft ? "├── " : "└── ");
        
        if (!node->left && !node->right) {
            cout << "[" << node->ch << ": " << node->weight << "]" << endl;
        } else {
            cout << "[^: " << node->weight << "]" << endl;
        }
        
        if (node->left) print(node->left, prefix + (isLeft ? "│   " : "    "), true);
        if (node->right) print(node->right, prefix + (isLeft ? "│   " : "    "), false);
    }
};

//主程序
int main() {
    // 读取文件
    ifstream file("I have a dream.txt");
    if (!file) {
        cout << "无法打开文件！" << endl;
        return 1;
    }
    
    string text, line;
    while (getline(file, line)) {
        text += line;
    }
    file.close();
    
    cout << "文件读取成功，共 " << text.length() << " 个字符\n" << endl;
    
    // 统计字母频率
    map<char, int> freq;
    int totalLetters = 0;
    
    for (size_t i = 0; i < text.length(); i++) {
        char c = tolower(text[i]);
        if (c >= 'a' && c <= 'z') {
            freq[c]++;
            totalLetters++;
        }
    }
    
    // 显示频率
    cout << "========== 字母频率统计 ==========" << endl;
    for (char c = 'a'; c <= 'z'; c++) {
        if (freq[c] > 0) {
            cout << c << ": " << freq[c] << endl;
        }
    }
    
    // 构建Huffman树
    HuffTree tree;
    tree.build(freq);
    
    // 生成编码表
    map<char, string> codes = tree.getCodes();
    
    cout << "\n========== Huffman编码表 ==========" << endl;
    for (char c = 'a'; c <= 'z'; c++) {
        if (codes.count(c)) {
            cout << c << ": " << codes[c] << endl;
        }
    }
    
    // 显示Huffman树
    cout << "\n========== Huffman树结构 ==========" << endl;
    tree.print();
    
    // 对单词编码
    string words[] = {"dream", "freedom", "justice", "nation", "equal"};
    
    cout << "\n========== 单词编码 ==========" << endl;
    for (int i = 0; i < 5; i++) {
        cout << words[i] << ": ";
        for (size_t j = 0; j < words[i].length(); j++) {
            cout << codes[words[i][j]] << " ";
        }
        cout << endl;
    }
    
    // 计算压缩率
    int originalBits = totalLetters * 8;
    int compressedBits = 0;
    for (map<char, int>::iterator it = freq.begin(); it != freq.end(); ++it) {
        compressedBits += it->second * codes[it->first].length();
    }
    
    cout << "\n========== 压缩效果 ==========" << endl;
    cout << "原始大小: " << originalBits << " bits" << endl;
    cout << "压缩后: " << compressedBits << " bits" << endl;
    cout << "压缩率: " << ((1.0 - (double)compressedBits / originalBits) * 100) << "%" << endl;
    
    return 0;
}
