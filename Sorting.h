// Name: Muhammad Musab Sial
// Reg No: 20255888
#ifndef SORTING_H
#define SORTING_H

#include "Structures.h"

extern long long sortComparisons;

bool comesBefore(SortItem& a, SortItem& b);
void insertionSort(SortItem* items, int n);
void mergeParts(SortItem* items, SortItem* temp, int left, int mid, int right);
void mergeSortRecursive(SortItem* items, SortItem* temp, int left, int right);
void mergeSort(SortItem* items, int n);
void swapItems(SortItem& a, SortItem& b);
int partitionItems(SortItem* items, int low, int high);
void quickSortRecursive(SortItem* items, int low, int high);
void quickSort(SortItem* items, int n);
bool isSorted(SortItem* items, int n);

#endif
