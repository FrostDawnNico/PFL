// 对比：O(n) 遍历 和 O(n²) 双层循环
#include <iostream>
using namespace std;
#include <vector>
#include <algorithm>

// O(1) 常量时间
// 获取数组中指定索引的元素
int getElement(const std::vector<int>& arr, int index) {
    // 无论数组有多大，通过下标直接计算内存地址获取元素，只执行一次操作
    return arr[index]; 
}

// O(log n) 二分查找时间
// 示例：二分查找（要求数组已排序）
int binarySearch(const std::vector<int>& arr, int target) {
    int left = 0, right = arr.size() - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2; // 防止溢出的写法
        if (arr[mid] == target) return mid;  // 找到目标
        if (arr[mid] < target) left = mid + 1; // 目标在右半区
        else right = mid - 1;                 // 目标在左半区
    }
    return -1; // 未找到
}

// O(n) 线性时间
void printArr(int arr[], int n) {
    for (int i = 0; i < n; i++) cout << arr[i] << " ";
}

//O(n log n) 
//通常出现在高效的分治排序算法中。比如快速排序、归并排序，它们将数组递归拆分（log n 层），并在每一层进行线性的合并或划分操作（n）。
// 示例：使用 C++ 标准库的高效排序
void sortArray(std::vector<int>& arr) {
    // std::sort 的底层通常是快速排序、堆排序和插入排序的结合，
    // 平均时间复杂度稳定在 O(n log n)
    std::sort(arr.begin(), arr.end());
}


// O(n²) 平方时间  
// 冒泡排序
void bubbleSort(int arr[], int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n-i-1; j++)
            if (arr[j] > arr[j+1]) swap(arr[j], arr[j+1]);
}

int main() {
    int arr[] = {3,1,2};

    std::vector<int> nums = {10, 20, 30, 40, 50};
    std::cout << "O(1) 示例结果: " << getElement(nums, 2) << std::endl; // 输出 30

    std::vector<int> nums1 = {1, 3, 5, 7, 9, 11, 13};
    std::cout << "O(log n) 示例结果: " << binarySearch(nums1, 7) << std::endl; // 输出索引 3

    printArr(arr, 3);

    sortArray(nums);

    bubbleSort(arr, 3);
    return 0;
}