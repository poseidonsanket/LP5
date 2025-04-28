// To compile:
// g++ -std=c++17 -fopenmp Bubble+merge.cpp -o combined_sorts
// To run:
// ./combined_sorts <array_length> <max_random_value>

#include <omp.h>
#include <cstdlib>
#include <chrono>
#include <functional>
#include <iostream>
#include <algorithm>
#include <string>

using namespace std;

// Sequential Merge Sort declarations
void s_mergesort(int *a, int i, int j);
// Parallel Merge Sort declarations
void p_mergesort(int *a, int i, int j);
void parallel_mergesort(int *a, int i, int j);
void merge_ranges(int *a, int i1, int j1, int i2, int j2);

// Bubble Sort declarations
void s_bubble(int *a, int n);
void p_bubble(int *a, int n);
void swap_vals(int &a, int &b);

// Benchmark helper
string bench_traverse(function<void()> fn) {
    auto start = chrono::high_resolution_clock::now();
    fn();
    auto stop = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::milliseconds>(stop - start);
    return to_string(dur.count());
}

// Sequential merge sort
void s_mergesort(int *a, int i, int j) {
    if (i < j) {
        int mid = (i + j) / 2;
        s_mergesort(a, i, mid);
        s_mergesort(a, mid + 1, j);
        merge_ranges(a, i, mid, mid + 1, j);
    }
}

// Parallel merge sort task
void p_mergesort(int *a, int i, int j) {
    if (i < j) {
        int mid = (i + j) / 2;
        if ((j - i) > 1000) {
            #pragma omp task firstprivate(a, i, mid)
            p_mergesort(a, i, mid);
            #pragma omp task firstprivate(a, mid, j)
            p_mergesort(a, mid + 1, j);
            #pragma omp taskwait
            merge_ranges(a, i, mid, mid + 1, j);
        } else {
            s_mergesort(a, i, j);
        }
    }
}

void parallel_mergesort(int *a, int i, int j) {
    #pragma omp parallel
    {
        #pragma omp single
        p_mergesort(a, i, j);
    }
}

// Merge two sorted subarrays
void merge_ranges(int *a, int i1, int j1, int i2, int j2) {
    int size = j2 - i1 + 1;
    int *temp = new int[size];
    int i = i1, j = i2, k = 0;
    while (i <= j1 && j <= j2) {
        temp[k++] = (a[i] < a[j]) ? a[i++] : a[j++];
    }
    while (i <= j1) temp[k++] = a[i++];
    while (j <= j2) temp[k++] = a[j++];
    for (i = i1, k = 0; i <= j2; ++i, ++k) {
        a[i] = temp[k];
    }
    delete[] temp;
}

// Sequential bubble sort
void s_bubble(int *a, int n) {
    for (int i = 0; i < n; i++) {
        int first = i % 2;
        for (int j = first; j < n - 1; j += 2) {
            if (a[j] > a[j + 1]) swap_vals(a[j], a[j + 1]);
        }
    }
}

// Parallel bubble sort
void p_bubble(int *a, int n) {
    for (int i = 0; i < n; i++) {
        int first = i % 2;
        #pragma omp parallel for shared(a, first)
        for (int j = first; j < n - 1; j += 2) {
            if (a[j] > a[j + 1]) swap_vals(a[j], a[j + 1]);
        }
    }
}

void swap_vals(int &a, int &b) {
    int tmp = a;
    a = b;
    b = tmp;
}

int main(int argc, char **argv) {
    int n, rand_max;
    if (argc >= 3) {
        n = stoi(argv[1]);
        rand_max = stoi(argv[2]);
    } else {
        cout << "Enter array length: ";
        cin >> n;
        cout << "Enter max random value: ";
        cin >> rand_max;
    }

    // Allocate arrays
    int *orig = new int[n];
    int *mseq = new int[n];
    int *mpar = new int[n];
    int *bseq = new int[n];
    int *bpar = new int[n];

    // Generate random data
    for (int i = 0; i < n; i++) orig[i] = rand() % rand_max;
    copy(orig, orig + n, mseq);
    copy(orig, orig + n, mpar);
    copy(orig, orig + n, bseq);
    copy(orig, orig + n, bpar);

    cout << "Generated array of length " << n << " with max value " << rand_max << "\n\n";

    // Sequential Merge Sort
    cout << "Sequential Merge Sort: "
         << bench_traverse([&](){ s_mergesort(mseq, 0, n-1); })
         << " ms\n";

    // Parallel Merge Sort
    omp_set_num_threads(16);
    cout << "Parallel Merge Sort (16 threads): "
         << bench_traverse([&](){ parallel_mergesort(mpar, 0, n-1); })
         << " ms\n\n";

    // Sequential Bubble Sort
    cout << "Sequential Bubble Sort: "
         << bench_traverse([&](){ s_bubble(bseq, n); })
         << " ms\n";

    // Parallel Bubble Sort
    omp_set_num_threads(16);
    cout << "Parallel Bubble Sort (16 threads): "
         << bench_traverse([&](){ p_bubble(bpar, n); })
         << " ms\n";

    // Clean up
    delete[] orig;
    delete[] mseq;
    delete[] mpar;
    delete[] bseq;
    delete[] bpar;

    return 0;
}
