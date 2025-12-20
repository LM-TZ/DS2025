#include<iostream>
#include<vector>
#include<algorithm>
#include<numeric>
#include<stdlib.h>
#include<cmath>
#include<ctime>

using namespace std;

struct Box{
    float x1, y1, x2, y2;
    float score;
};

// 计算面积
float Area(const Box& b){
    return max(0.0f,b.x2-b.x1)*max(0.0f,b.y2-b.y1);
}

// 计算iou
float IoU(const Box& b1, const Box& b2){
    // 确定交集的界限
    float x_1 = max(b1.x1,b2.x1);
    float y_1 = max(b1.y1,b2.y1);
    float x_2 = min(b1.x2,b2.x2);
    float y_2 = min(b1.y2,b2.y2);

    // 计算交集面积
    float inter_area = max(0.0f,x_2-x_1)*max(0.0f,y_2-y_1);

    // 计算并集面积
    float union_area = Area(b1)+Area(b2)-inter_area;
    
    return union_area > 0 ? inter_area / union_area : 0.0f;
}

//NMS算法
void NMS(vector<Box>& boxes, float threshold){
    vector<bool> is_suppressed(boxes.size(), false);
    
    for(int i=0; i<boxes.size(); i++){
        if(is_suppressed[i] == true){
            continue;
        }
        
        for(int j=i+1; j<boxes.size(); j++){
            if(is_suppressed[j] == false){
                if(IoU(boxes[i], boxes[j]) > threshold){
                    is_suppressed[j] = true;
                }
            }
        }
    }
}

// 实现排序算法
// 冒泡排序 
void BubbleSort(vector<Box>& boxes){
    int n = boxes.size();
    for(int i=0;i<n-1;i++){
        for(int j=0;j<n-i-1;j++){
            if(boxes[j].score<boxes[j+1].score){
                swap(boxes[j],boxes[j+1]);
            }
        }
    }
}

// 选择排序
void SelectionSort(vector<Box>& boxes){
    int n = boxes.size();
    for(int i=0;i<n-1;i++){
        int max_idx = i;
        for(int j=i+1;j<n;j++){
            if(boxes[j].score>boxes[max_idx].score){
                max_idx = j;
            }
        }
        if(max_idx!=i){
            swap(boxes[i],boxes[max_idx]);
        }
    }
}

// 插入排序
void InsertionSort(vector<Box>& boxes){
    int n = boxes.size();
    for(int i=1;i<n;i++){
        Box key = boxes[i];
        int j = i-1;
        while(j>=0&&boxes[j].score<key.score){
            boxes[j+1] = boxes[j];
            j--;
        }
        boxes[j+1] = key;
    }
}

// 快速排序
void QuickSort(vector<Box>& boxes,int left,int right){
    if(left>=right){
        return;
    }
    float pivot = boxes[left+(right-left)/2].score;
    int i = left-1;
    int j = right+1;
    while(i<j){
        do{i++;}while(boxes[i].score>pivot);
        do{j--;}while(boxes[j].score<pivot);
        if(i<j){
            swap(boxes[i],boxes[j]);
        }
    }
    QuickSort(boxes,left,j);
    QuickSort(boxes,j+1,right);
}

// 生成数据：随机分布
vector<Box> random1(int n){
    vector<Box> boxes;
    for(int i=0;i<n;i++){
        Box b;
        b.x1 = rand()%800;
        b.y1 = rand()%600;
        b.x2 = b.x1 + rand()%50 + 20;
        b.y2 = b.y1 + rand()%50 + 20;
        b.score = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        boxes.push_back(b);
    }
    return boxes;
}

//聚集分布
vector<Box> cluster1(int n){
    vector<Box> boxes;
    for(int i=0;i<n;i++){
        Box b;
        int center = rand() % 5; 
        int center_x = center * 150 + 50;
        int center_y = center * 100 + 50;

        b.x1 = center_x + rand()%60 - 30; 
        b.y1 = center_y + rand()%60 - 30;
        b.x2 = b.x1 + rand()%50 + 20;
        b.y2 = b.y1 + rand()%50 + 20;
        b.score = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        boxes.push_back(b);
    }
    return boxes;
}

// 主程序
int main(){
    srand(static_cast<unsigned int>(time(0)));
    int sizes[] = {100, 1000, 5000};
    for(int type=0; type<2; type++){
        if(type == 0) cout << "\nRandom Distribution (随机分布)" << endl;
        else cout << "\nCluster Distribution (聚集分布)" << endl;
        for(int k=0; k<3; k++){
            int n = sizes[k];
            cout << "数据大小" << n << endl;

            vector<Box> boxes;
            if(type == 0) boxes = random1(n);
            else boxes = cluster1(n);

            // 冒泡排序
            // 数据量太大时跳过冒泡
            if(n <= 2000){
                vector<Box> boxes_bubble = boxes;
                clock_t start_bubble = clock();
                BubbleSort(boxes_bubble);
                NMS(boxes_bubble, 0.5);
                clock_t end_bubble = clock();
                cout << "Bubble Sort" << double(end_bubble - start_bubble) / CLOCKS_PER_SEC << " s" << endl;
            }

            // 选择排序
            if(n <= 5000){
                vector<Box> boxes_selection = boxes;
                clock_t start_selection = clock();
                SelectionSort(boxes_selection);
                NMS(boxes_selection, 0.5);
                clock_t end_selection = clock();
                cout << "Selection Sort" << double(end_selection - start_selection) / CLOCKS_PER_SEC << " s" << endl;
            }

            // 插入排序
            if(n <= 5000){
                vector<Box> boxes_insertion = boxes;
                clock_t start_insertion = clock();
                InsertionSort(boxes_insertion);
                NMS(boxes_insertion, 0.5);
                clock_t end_insertion = clock();
                cout << "Insertion Sort" << double(end_insertion - start_insertion) / CLOCKS_PER_SEC << " s" << endl;
            }

            // 快速排序
            vector<Box> boxes_quick = boxes;
            clock_t start_quick = clock();
            QuickSort(boxes_quick, 0, boxes_quick.size() - 1);
            NMS(boxes_quick, 0.5);
            clock_t end_quick = clock();
            cout << "Quick Sort" << double(end_quick - start_quick) / CLOCKS_PER_SEC << " s" << endl;
            
            cout << "----------------" << endl;
        }
    }
    return 0;
}
