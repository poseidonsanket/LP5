//to run code
//g++ -fopenmp bubble_sort.cpp -o bubble_sort
//./bubble_sort 50 20

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

void s_bubble(int *, int);
void p_bubble(int *, int);
void swap(int &, int &);

void s_bubble(int *a, int n) {
    for (int i = 0; i < n; i++) {
        int first = i % 2;
        for (int j = first; j < n - 1; j += 2) 
        {
            if (a[j] > a[j + 1]) 
            {
                swap(a[j], a[j + 1]);
            }
        }
    }
}

void p_bubble(int *a, int n) {
    for (int i = 0; i < n; i++) 
    {
        int first = i % 2;
        #pragma omp parallel for shared(a, first) num_threads(16)
        for (int j = first; j < n - 1; j += 2) 
        {
            if (a[j] > a[j + 1]) 
            {
                swap(a[j], a[j + 1]);
            }
        }
    }
}

void swap(int &a, int &b) {
    int test;
    test = a;
    a = b;
    b = test;
}


std::string bench_traverse(std::function<void()> traverse_fn) {
    auto start = std::chrono::high_resolution_clock::now();
    traverse_fn();
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    return std::to_string(duration.count());
}

#include <iostream>
#include <cstdlib>
#include <algorithm> // For std::copy
#include <omp.h>     // For OpenMP functions
#include <string>    // For std::stoi

using namespace std;

// Assuming the bubble sort functions s_bubble and p_bubble are defined elsewhere

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

    // Assuming bench_traverse is defined and returns execution time
    std::cout << "Sequential Bubble sort: " 
              << bench_traverse([&] { s_bubble(a, n); }) 
              << "ms\n";

    cout << "Sorted array is ready =>\n";
    // Uncomment to print sorted array if needed
    // for (int i = 0; i < n; i++) {
    //     cout << a[i] << ", ";
    // }
    cout << "\n\n";

    omp_set_num_threads(16);  // Set the number of threads for parallel processing

    // Parallel Bubble Sort
    std::cout << "Parallel (16) Bubble sort: " 
              << bench_traverse([&] { p_bubble(b, n); }) 
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

