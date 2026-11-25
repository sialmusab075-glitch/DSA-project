// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "Sorting.h"

long long sortComparisons = 0;

bool comesBefore(SortItem& a, SortItem& b) {
    sortComparisons++;
    if (a.key1 != b.key1) {
        return a.key1 > b.key1;
    }
    if (a.key2 != b.key2) {
        return a.key2 < b.key2;
    }
    return a.index < b.index;
}

void insertionSort(SortItem* items, int n) {
    for (int i = 1; i < n; i++) {
        SortItem current = items[i];
        int j = i - 1;
        while (j >= 0 && comesBefore(current, items[j])) {
            items[j + 1] = items[j];
            j--;
        }
        items[j + 1] = current;
    }
}

void mergeParts(SortItem* items, SortItem* temp, int left, int mid, int right) {
    int i = left;
    int j = mid + 1;
    int k = left;
    while (i <= mid && j <= right) {
        if (comesBefore(items[j], items[i])) {
            temp[k] = items[j];
            j++;
        } else {
            temp[k] = items[i];
            i++;
        }
        k++;
    }
    while (i <= mid) {
        temp[k] = items[i];
        i++;
        k++;
    }
    while (j <= right) {
        temp[k] = items[j];
        j++;
        k++;
    }
    for (int x = left; x <= right; x++) {
        items[x] = temp[x];
    }
}

void mergeSortRecursive(SortItem* items, SortItem* temp, int left, int right) {
    if (left >= right) {
        return;
    }
    int mid = left + (right - left) / 2;
    mergeSortRecursive(items, temp, left, mid);
    mergeSortRecursive(items, temp, mid + 1, right);
    mergeParts(items, temp, left, mid, right);
}

void mergeSort(SortItem* items, int n) {
    if (n < 2) {
        return;
    }
    SortItem* temp = new SortItem[n];
    mergeSortRecursive(items, temp, 0, n - 1);
    delete[] temp;
}

void swapItems(SortItem& a, SortItem& b) {
    SortItem temp = a;
    a = b;
    b = temp;
}

int partitionItems(SortItem* items, int low, int high) {
    int mid = low + (high - low) / 2;
    swapItems(items[mid], items[high]);
    SortItem pivot = items[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (comesBefore(items[j], pivot)) {
            i++;
            swapItems(items[i], items[j]);
        }
    }
    swapItems(items[i + 1], items[high]);
    return i + 1;
}

void quickSortRecursive(SortItem* items, int low, int high) {
    if (low < high) {
        int p = partitionItems(items, low, high);
        quickSortRecursive(items, low, p - 1);
        quickSortRecursive(items, p + 1, high);
    }
}

void quickSort(SortItem* items, int n) {
    if (n < 2) {
        return;
    }
    quickSortRecursive(items, 0, n - 1);
}

bool isSorted(SortItem* items, int n) {
    for (int i = 1; i < n; i++) {
        if (comesBefore(items[i], items[i - 1])) {
            return false;
        }
    }
    return true;
}
