#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <iomanip>
#include <new>
#include "omp.h"

using namespace std;

// ==================== FUNCTION DECLARATIONS ====================

// Sequential (single-thread) operations for baseline comparison
long long sequential_sum(int* arr, int size);
int sequential_min(int* arr, int size);

// Parallel operations with specified number of threads
long long parallel_sum(int* arr, int size, int num_threads);
int parallel_min(int* arr, int size, int num_threads);

// ==================== MAIN ====================

int main() {
    // Seed random number generator
    srand(static_cast<unsigned>(time(nullptr)));

    // ----------------------------------------------------------
    // STEP 1: User input - number of arrays to create
    // ----------------------------------------------------------
    int num_arrays;
    cout << "============================================================" << endl;
    cout << "        OpenMP Nested Parallelism - Array Benchmark" << endl;
    cout << "============================================================" << endl;
    cout << endl;
    cout << "Available processors: " << omp_get_num_procs() << endl;
    cout << endl;
    cout << "Enter the number of arrays to create in parallel" << endl;
    cout << "(enter 0 for a single large array of 2 000 000 000 elements): ";
    cin >> num_arrays;
    cout << endl;

    // Special case: 0 -> create 1 array of 2 000 000 000 elements
    bool special_mode = false;
    if (num_arrays == 0) {
        special_mode = true;
        num_arrays = 1;
        cout << "  >>> Special mode: 1 array of 2 000 000 000 elements (~7.5 GB RAM)" << endl;
        cout << endl;
    }
    else if (num_arrays < 0) {
        cout << "Error: Number of arrays must be >= 0!" << endl;
        return 1;
    }

    // Enable nested parallelism
    omp_set_nested(1);

    // ----------------------------------------------------------
    // STEP 2: Parallel array creation (nested parallelism)
    //   - Outer level: each thread creates one array
    //   - Inner level: parallel fill with 10s
    // ----------------------------------------------------------

    // Arrays of pointers, sizes, and -1 positions
    int** arrays = new int* [num_arrays];
    int* sizes = new int[num_arrays];
    int* neg_positions = new int[num_arrays];
    double* create_times = new double[num_arrays];

    // Pre-generate random sizes and positions (rand() is not thread-safe)
    if (special_mode) {
        // Special mode: single array of 2 000 000 000 elements
        sizes[0] = 200000000; // 2 billion (fits in int, max int = 2 147 483 647)
    }
    else {
        for (int i = 0; i < num_arrays; i++) {
            // rand() on MSVC returns 0..32767, combine 3 calls for full range
            long long big_rand = ((long long)rand() << 30) | ((long long)rand() << 15) | (long long)rand();
            int raw_size = 200000000 + (int)(big_rand % (500000000 - 200000000 + 1));
            // Make it even
            if (raw_size % 2 != 0) {
                raw_size++;
            }
            sizes[i] = raw_size;
        }
    }

    // Pre-generate random positions for -1 (full range 0..size-1)
    // rand() on MSVC returns 0..32767, so we combine 3 calls for full int range
    for (int i = 0; i < num_arrays; i++) {
        long long big_rand = ((long long)rand() << 30) | ((long long)rand() << 15) | (long long)rand();
        neg_positions[i] = (int)(big_rand % (long long)sizes[i]);
        if (neg_positions[i] < 0) neg_positions[i] += sizes[i];
    }

    cout << "============================================================" << endl;
    cout << "  PHASE 1: Parallel Array Creation (Nested Parallelism)" << endl;
    cout << "============================================================" << endl;
    cout << endl;

    double phase1_start = omp_get_wtime();

    // Outer parallel region: each thread creates one array
#pragma omp parallel for num_threads(num_arrays) schedule(dynamic)
    for (int i = 0; i < num_arrays; i++) {
        double t_start = omp_get_wtime();
        int thread_id = omp_get_thread_num();

        int arr_size = sizes[i];

        // Allocate the array using size_t to handle large sizes properly
        // nothrow to avoid exceptions on allocation failure
        arrays[i] = new(nothrow) int[(size_t)arr_size];

        if (arrays[i] == nullptr) {
#pragma omp critical
            {
                cout << "  ERROR: Failed to allocate array #" << i + 1
                    << " (" << arr_size << " elements, "
                    << ((long long)arr_size * sizeof(int)) / (1024LL * 1024LL) << " MB)" << endl;
                cout << "  Make sure you have enough RAM and are building for x64!" << endl;
            }
            sizes[i] = 0;
        }
        else {
            // neg_positions[i] was already pre-generated above (thread-safe)

            // Inner parallel region: fill the array with 10s using nested threads
            int inner_threads = omp_get_num_procs();
            if (inner_threads < 2) inner_threads = 2;

#pragma omp parallel for num_threads(inner_threads) schedule(static)
            for (int j = 0; j < arr_size; j++) {
                arrays[i][j] = 10;
            }

            // Insert -1 at the random position
            arrays[i][neg_positions[i]] = -1;

            double t_end = omp_get_wtime();
            create_times[i] = t_end - t_start;

            // Output info (use critical to avoid mixed output)
#pragma omp critical
            {
                cout << "  Array #" << i + 1 << ":" << endl;
                cout << "    Thread ID:        " << thread_id << endl;
                cout << "    Size:             " << sizes[i] << " elements ("
                    << ((long long)sizes[i] * sizeof(int)) / (1024LL * 1024LL) << " MB)" << endl;
                cout << "    Position of -1:   " << neg_positions[i] << endl;
                cout << "    Inner threads:    " << inner_threads << endl;
                cout << "    Creation time:    " << fixed << setprecision(4) << create_times[i] << " seconds" << endl;
                cout << endl;
            }
        }
    }

    double phase1_end = omp_get_wtime();
    cout << "  Total Phase 1 time: " << fixed << setprecision(4)
        << phase1_end - phase1_start << " seconds" << endl;
    cout << endl;

    // ----------------------------------------------------------
    // STEP 3: Find the longest array (skip failed allocations)
    // ----------------------------------------------------------
    int longest_idx = -1;
    for (int i = 0; i < num_arrays; i++) {
        if (arrays[i] != nullptr && sizes[i] > 0) {
            if (longest_idx == -1 || sizes[i] > sizes[longest_idx]) {
                longest_idx = i;
            }
        }
    }

    if (longest_idx == -1) {
        cout << "ERROR: No arrays were successfully allocated!" << endl;
        cout << "Make sure you build for x64 (not x86) and have enough RAM." << endl;
        delete[] arrays;
        delete[] sizes;
        delete[] neg_positions;
        delete[] create_times;
        return 1;
    }

    int* target_arr = arrays[longest_idx];
    int target_size = sizes[longest_idx];

    cout << "============================================================" << endl;
    cout << "  Longest array: #" << longest_idx + 1
        << " (size = " << target_size
        << ", -1 at position " << neg_positions[longest_idx] << ")" << endl;
    cout << "============================================================" << endl;
    cout << endl;

    // ----------------------------------------------------------
    // STEP 4: Baseline (1 thread) using the SAME parallel functions
    //         to ensure fair comparison (same compiler optimizations)
    // ----------------------------------------------------------
    cout << "============================================================" << endl;
    cout << "  PHASE 2: Baseline (1 thread, same code path as parallel)" << endl;
    cout << "============================================================" << endl;
    cout << endl;

    double seq_sum_start = omp_get_wtime();
    long long seq_sum_result = parallel_sum(target_arr, target_size, 1);
    double seq_sum_end = omp_get_wtime();
    double seq_sum_time = seq_sum_end - seq_sum_start;

    cout << "  [SUM] 1 thread:" << endl;
    cout << "    Array size:   " << target_size << endl;
    cout << "    Sum:          " << seq_sum_result << endl;
    cout << "    Time:         " << fixed << setprecision(6) << seq_sum_time << " seconds" << endl;
    cout << endl;

    double seq_min_start = omp_get_wtime();
    int seq_min_result = parallel_min(target_arr, target_size, 1);
    double seq_min_end = omp_get_wtime();
    double seq_min_time = seq_min_end - seq_min_start;

    cout << "  [MIN] 1 thread:" << endl;
    cout << "    Array size:   " << target_size << endl;
    cout << "    Min:          " << seq_min_result << endl;
    cout << "    Time:         " << fixed << setprecision(6) << seq_min_time << " seconds" << endl;
    cout << endl;

    // ----------------------------------------------------------
    // STEP 5: Parallel sections - sum & min with 2-20 threads (step 2)
    //         Using nested parallelism: outer = 2 sections,
    //         inner = parallel for with varying thread counts
    // ----------------------------------------------------------
    cout << "============================================================" << endl;
    cout << "  PHASE 3: Parallel Sections (Nested Parallelism)" << endl;
    cout << "    2 sections: SUM and MIN, each with 2-20 threads (step 2)" << endl;
    cout << "============================================================" << endl;
    cout << endl;

    const int THREAD_STEPS = 10; // 2,4,6,8,10,12,14,16,18,20
    double sum_times[10];
    long long sum_results[10];
    double min_times[10];
    int min_results[10];

    double phase3_start = omp_get_wtime();

    // Two sections running in parallel: SUM section and MIN section
#pragma omp parallel sections num_threads(2)
    {
        // ============ SECTION 1: Sum of all elements ============
#pragma omp section
        {
            for (int step = 0; step < THREAD_STEPS; step++) {
                int nthreads = (step + 1) * 2; // 2, 4, 6, ..., 20
                double t1 = omp_get_wtime();

                long long local_sum = parallel_sum(target_arr, target_size, nthreads);

                double t2 = omp_get_wtime();
                sum_times[step] = t2 - t1;
                sum_results[step] = local_sum;

#pragma omp critical
                {
                    cout << "  [SUM] " << setw(2) << nthreads << " threads:"
                        << "  size=" << target_size
                        << "  sum=" << local_sum
                        << "  time=" << fixed << setprecision(6) << sum_times[step] << " s" << endl;
                }
            }
        }

        // ============ SECTION 2: Minimum element ============
#pragma omp section
        {
            for (int step = 0; step < THREAD_STEPS; step++) {
                int nthreads = (step + 1) * 2; // 2, 4, 6, ..., 20
                double t1 = omp_get_wtime();

                int local_min = parallel_min(target_arr, target_size, nthreads);

                double t2 = omp_get_wtime();
                min_times[step] = t2 - t1;
                min_results[step] = local_min;

#pragma omp critical
                {
                    cout << "  [MIN] " << setw(2) << nthreads << " threads:"
                        << "  size=" << target_size
                        << "  min=" << local_min
                        << "  time=" << fixed << setprecision(6) << min_times[step] << " s" << endl;
                }
            }
        }
    }

    double phase3_end = omp_get_wtime();
    double phase3_total = phase3_end - phase3_start;

    cout << endl;

    // ----------------------------------------------------------
    // STEP 6: Results comparison - speedup analysis
    // ----------------------------------------------------------
    cout << "============================================================" << endl;
    cout << "  RESULTS: SUM - Speedup Comparison" << endl;
    cout << "============================================================" << endl;
    cout << endl;
    cout << "  Sequential (1 thread): sum=" << seq_sum_result
        << "  time=" << fixed << setprecision(6) << seq_sum_time << " s" << endl;
    cout << endl;

    // Find best SUM time
    int best_sum_idx = 0;
    for (int i = 1; i < THREAD_STEPS; i++) {
        if (sum_times[i] < sum_times[best_sum_idx]) best_sum_idx = i;
    }

    cout << "  " << setw(10) << "Threads"
        << setw(18) << "Time (s)"
        << setw(18) << "Speedup (x)" << endl;
    cout << "  " << string(46, '-') << endl;

    for (int i = 0; i < THREAD_STEPS; i++) {
        int nthreads = (i + 1) * 2;
        double speedup = seq_sum_time / sum_times[i];
        cout << "  " << setw(10) << nthreads
            << setw(18) << fixed << setprecision(6) << sum_times[i]
            << setw(18) << fixed << setprecision(2) << speedup;
        if (i == best_sum_idx) cout << "  <-- FASTEST";
        cout << endl;
    }

    double best_sum_speedup = seq_sum_time / sum_times[best_sum_idx];
    int best_sum_threads = (best_sum_idx + 1) * 2;

    cout << endl;
    cout << "  >>> Best SUM: " << best_sum_threads << " threads, "
        << fixed << setprecision(6) << sum_times[best_sum_idx] << " s, "
        << "speedup " << fixed << setprecision(2) << best_sum_speedup << "x "
        << "vs sequential" << endl;
    cout << endl;

    cout << "============================================================" << endl;
    cout << "  RESULTS: MIN - Speedup Comparison" << endl;
    cout << "============================================================" << endl;
    cout << endl;
    cout << "  Sequential (1 thread): min=" << seq_min_result
        << "  time=" << fixed << setprecision(6) << seq_min_time << " s" << endl;
    cout << endl;

    // Find best MIN time
    int best_min_idx = 0;
    for (int i = 1; i < THREAD_STEPS; i++) {
        if (min_times[i] < min_times[best_min_idx]) best_min_idx = i;
    }

    cout << "  " << setw(10) << "Threads"
        << setw(18) << "Time (s)"
        << setw(18) << "Speedup (x)" << endl;
    cout << "  " << string(46, '-') << endl;

    for (int i = 0; i < THREAD_STEPS; i++) {
        int nthreads = (i + 1) * 2;
        double speedup = seq_min_time / min_times[i];
        cout << "  " << setw(10) << nthreads
            << setw(18) << fixed << setprecision(6) << min_times[i]
            << setw(18) << fixed << setprecision(2) << speedup;
        if (i == best_min_idx) cout << "  <-- FASTEST";
        cout << endl;
    }

    double best_min_speedup = seq_min_time / min_times[best_min_idx];
    int best_min_threads = (best_min_idx + 1) * 2;

    cout << endl;
    cout << "  >>> Best MIN: " << best_min_threads << " threads, "
        << fixed << setprecision(6) << min_times[best_min_idx] << " s, "
        << "speedup " << fixed << setprecision(2) << best_min_speedup << "x "
        << "vs sequential" << endl;
    cout << endl;

    // ----------------------------------------------------------
    // STEP 7: Overall timing summary
    // ----------------------------------------------------------
    cout << "============================================================" << endl;
    cout << "  OVERALL TIMING SUMMARY" << endl;
    cout << "============================================================" << endl;
    cout << endl;
    cout << "  Phase 1 (array creation):       " << fixed << setprecision(4)
        << phase1_end - phase1_start << " s" << endl;
    cout << "  Sequential SUM (1 thread):      " << fixed << setprecision(6)
        << seq_sum_time << " s" << endl;
    cout << "  Sequential MIN (1 thread):      " << fixed << setprecision(6)
        << seq_min_time << " s" << endl;
    cout << "  Phase 3 (parallel sections):    " << fixed << setprecision(4)
        << phase3_total << " s" << endl;
    cout << "    Best SUM: " << best_sum_threads << " thr -> "
        << fixed << setprecision(6) << sum_times[best_sum_idx] << " s ("
        << fixed << setprecision(2) << best_sum_speedup << "x faster)" << endl;
    cout << "    Best MIN: " << best_min_threads << " thr -> "
        << fixed << setprecision(6) << min_times[best_min_idx] << " s ("
        << fixed << setprecision(2) << best_min_speedup << "x faster)" << endl;
    cout << endl;

    // ----------------------------------------------------------
    // Cleanup: free all allocated arrays
    // ----------------------------------------------------------
    for (int i = 0; i < num_arrays; i++) {
        if (arrays[i] != nullptr) {
            delete[] arrays[i];
        }
    }
    delete[] arrays;
    delete[] sizes;
    delete[] neg_positions;
    delete[] create_times;

    cout << "============================================================" << endl;
    cout << "  Program finished. Memory freed." << endl;
    cout << "============================================================" << endl;

    return 0;
}

// ==================== SEQUENTIAL OPERATIONS ====================

long long sequential_sum(int* arr, int size) {
    long long sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

int sequential_min(int* arr, int size) {
    int min_val = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] < min_val) {
            min_val = arr[i];
        }
    }
    return min_val;
}

// ==================== PARALLEL OPERATIONS ====================

long long parallel_sum(int* arr, int size, int num_threads) {
    long long sum = 0;

    // Using reduction(+:sum) - supported in OpenMP 2.0+
#pragma omp parallel for reduction(+:sum) num_threads(num_threads) schedule(guided)
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }

    return sum;
}

int parallel_min(int* arr, int size, int num_threads) {
    // MSVC supports only OpenMP 2.0, which does not have reduction(min:...)
    // So we use manual per-thread local minimums + critical section merge

    int global_min = arr[0];

#pragma omp parallel num_threads(num_threads)
    {
        int local_min = arr[0];

#pragma omp for schedule(guided) nowait
        for (int i = 0; i < size; i++) {
            if (arr[i] < local_min) {
                local_min = arr[i];
            }
        }

        // Merge local minimum into global minimum
#pragma omp critical
        {
            if (local_min < global_min) {
                global_min = local_min;
            }
        }
    }

    return global_min;
}