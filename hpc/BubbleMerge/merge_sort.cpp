//to run code
//g++ -fopenmp merge_sort.cpp -o merge_sort
//./merge_sort 50 20

#include <omp.h>
#include <stdlib.h>
#include <chrono>
#include <array>
#include <functional>
#include <iostream>
#include <string>
#include <vector> 

auto start = std::chrono::high_resolution_clock::now();
auto stop = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

using namespace std;

void p_mergesort(int *a, int i, int j);
void s_mergesort(int *a, int i, int j);
void merge(int *a, int i1, int j1, int i2, int j2);

void p_mergesort(int *a, int i, int j) {
if (i < j) {
int mid;
if ((j - i) > 1000) {
mid = (i + j) / 2;
#pragma omp task firstprivate(a, i, mid)
p_mergesort(a, i, mid);
#pragma omp task firstprivate(a, mid, j)
p_mergesort(a, mid + 1, j);
#pragma omp taskwait
merge(a, i, mid, mid + 1, j);
} else {
s_mergesort(a, i, j);
}
}
}

void parallel_mergesort(int *a, int i, int j) {
#pragma omp parallel num_threads(16)
{
#pragma omp single
p_mergesort(a, i, j);
}
}

void s_mergesort(int *a, int i, int j) {
int mid;
if (i < j) {
mid = (i + j) / 2;
s_mergesort(a, i, mid);
s_mergesort(a, mid + 1, j);
merge(a, i, mid, mid + 1, j);
}
}

void merge(int *a, int i1, int j1, int i2, int j2) {
    int size = j2 - i1 + 1;
    int* temp = new int[size]; // allocate required size dynamically

    int i = i1, j = i2, k = 0;

    while (i <= j1 && j <= j2) {
        temp[k++] = (a[i] < a[j]) ? a[i++] : a[j++];
    }

    while (i <= j1) temp[k++] = a[i++];
    while (j <= j2) temp[k++] = a[j++];

    for (i = i1, k = 0; i <= j2; i++, k++) {
        a[i] = temp[k];
    }

    delete[] temp; // free memory
}



std::string bench_traverse(std::function<void()> traverse_fn) {
    auto start = std::chrono::high_resolution_clock::now();
    traverse_fn();
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    return std::to_string(duration.count());
}

int main(int argc, const char **argv) {

    int n, rand_max;

    // Check if command-line arguments are provided
    if (argc < 3) {
        std::cout << "Specify array length and maximum random value\n";

        // Prompt user for input if arguments are not provided
        std::cout << "Enter array length: ";
        std::cin >> n;

        std::cout << "Enter maximum random value: ";
        std::cin >> rand_max;
    } else {
        // Parse array length and maximum random value from arguments
        n = stoi(argv[1]);
        rand_max = stoi(argv[2]);
    }

    int *a = new int[n];
    int *b = new int[n];

    // Generate the random array
    for (int i = 0; i < n; i++) {
        a[i] = rand() % rand_max;
    }

    // Copy array a to b
    std::copy(a, a + n, b);

    // Output generated array details
    std::cout << "Generated random array of length " << n 
              << " with elements between 0 and " << rand_max << "\n\n";

    // Sequential Merge Sort
    std::cout << "Sequential merge sort: " 
              << bench_traverse([&] { s_mergesort(a, 0, n-1); }) 
              << "ms\n";

    cout << "Sorted array is ready =>\n";
    // Uncomment to print sorted array if needed
    // for (int i = 0; i < n; i++) {
    //     cout << a[i] << ", ";
    // }
    cout << "\n\n";

    omp_set_num_threads(16);  // Set the number of threads for parallel processing

    // Parallel Merge Sort
    std::cout << "Parallel (16) merge sort: " 
              << bench_traverse([&] { parallel_mergesort(b, 0, n-1); }) 
              << "ms\n";

    // Uncomment to print sorted parallel array if needed
    // cout << "Sorted array is =>\n";
    // for (int i = 0; i < n; i++) {
    //     cout << b[i] << ", ";
    // }

    // Clean up dynamically allocated memory
    delete[] a;
    delete[] b;

    return 0;
}
