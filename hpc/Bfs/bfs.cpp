//to run code
//g++ -fopenmp bfs.cpp -o bfs
//./bfs input.txt

#include <omp.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <algorithm>
#include <array>
#include <chrono>

// Generic representation of a graph implemented with an adjacency matrix
struct Graph {
    using Node = int;
    int task_threshold = 60;
    int max_depth_rdfs = 10000;

    std::vector<std::vector<int>> adj_matrix;

    bool edge_exists(Node n1, Node n2) { return adj_matrix[n1][n2] > 0; }
    int n_nodes() { return adj_matrix.size(); }
    int size() { return n_nodes(); }

    void dfs(Node src, std::vector<int>& visited) {
        std::vector<Node> queue{src};

        while (!queue.empty()) {
            Node node = queue.back();
            queue.pop_back();

            if (!visited[node]) {
                visited[node] = true;
                for (int next_node = 0; next_node < n_nodes(); next_node++) {
                    if (edge_exists(node, next_node) && !visited[next_node]) {
                        queue.push_back(next_node);
                    }
                }
            }
        }
    }

    void pdfs(Node src, std::vector<int>& visited) {
        std::vector<Node> queue{src};

        while (!queue.empty()) {
            Node node = queue.back();
            queue.pop_back();

            if (!visited[node]) {
                visited[node] = true;

                #pragma omp parallel shared(queue, visited)
                {
                    std::vector<Node> private_queue;

                    #pragma omp for nowait schedule(static)
                    for (int next_node = 0; next_node < n_nodes(); next_node++) {
                        if (edge_exists(node, next_node) && !visited[next_node]) {
                            private_queue.push_back(next_node);
                        }
                    }

                    #pragma omp critical(queue_update)
                    queue.insert(queue.end(), private_queue.begin(), private_queue.end());
                }
            }
        }
    }

    void p_dfs_with_locks(Node src, std::vector<int>& visited, std::vector<omp_lock_t>& node_locks) {
        std::vector<Node> queue{src};

        while (!queue.empty()) {
            Node node = queue.back();
            queue.pop_back();

            if (!atomic_test_visited(node, visited, &node_locks[node])) {
                atomic_set_visited(node, visited, &node_locks[node]);

                #pragma omp parallel shared(queue, visited)
                {
                    std::vector<Node> private_queue;

                    #pragma omp for nowait
                    for (int next_node = 0; next_node < n_nodes(); next_node++) {
                        if (edge_exists(node, next_node)) {
                            if (!atomic_test_visited(next_node, visited, &node_locks[next_node])) {
                                private_queue.push_back(next_node);
                            }
                        }
                    }

                    #pragma omp critical(queue_update)
                    queue.insert(queue.end(), private_queue.begin(), private_queue.end());
                }
            }
        }
    }

    std::pair<std::vector<Node>, std::vector<Node>> dijkstra(Node src) {
        std::vector<Node> queue{src};

        std::vector<Node> came_from(size(), -1);
        std::vector<Node> cost_so_far(size(), -1);

        came_from[src] = src;
        cost_so_far[src] = 0;

        while (!queue.empty()) {
            Node current = queue.back();
            queue.pop_back();

            for (int next = 0; next < n_nodes(); next++) {
                if (edge_exists(current, next)) {
                    int new_cost = cost_so_far[current] + adj_matrix[current][next];
                    if (cost_so_far[next] == -1 || new_cost < cost_so_far[next]) {
                        cost_so_far[next] = new_cost;
                        queue.push_back(next);
                        came_from[next] = current;
                    }
                }
            }
        }

        return {came_from, cost_so_far};
    }

    std::vector<omp_lock_t> initialize_locks() {
        std::vector<omp_lock_t> node_locks(n_nodes());

        for (int node = 0; node < n_nodes(); node++) {
            omp_init_lock(&node_locks[node]);
        }

        return node_locks;
    }

    std::pair<std::vector<Node>, std::vector<Node>> p_dijkstra(Node src) {
        std::vector<Node> queue{src};

        std::vector<Node> came_from(size(), -1);
        std::vector<Node> cost_so_far(size(), -1);

        came_from[src] = src;
        cost_so_far[src] = 0;

        auto node_locks = initialize_locks();

        while (!queue.empty()) {
            Node current = queue.back();
            queue.pop_back();

            #pragma omp parallel shared(queue, node_locks)
            #pragma omp for
            for (int next = 0; next < n_nodes(); next++) {
                if (edge_exists(current, next)) {
                    omp_set_lock(&node_locks[current]);
                    int cost_so_far_current = cost_so_far[current];
                    omp_unset_lock(&node_locks[current]);

                    int new_cost = cost_so_far_current + adj_matrix[current][next];

                    omp_set_lock(&node_locks[next]);
                    int cost_so_far_next = cost_so_far[next];
                    omp_unset_lock(&node_locks[next]);

                    if (cost_so_far_next == -1 || new_cost < cost_so_far_next) {
                        omp_set_lock(&node_locks[next]);
                        cost_so_far[next] = new_cost;
                        came_from[next] = current;
                        omp_unset_lock(&node_locks[next]);

                        #pragma omp critical(queue_update)
                        queue.push_back(next);
                    }
                }
            }
        }

        for (int node = 0; node < n_nodes(); node++) {
            omp_destroy_lock(&node_locks[node]);
        }

        return {came_from, cost_so_far};
    }

    std::vector<Node> reconstruct_path(Node src, Node dst, std::vector<Node> origins) {
        Node current_node = dst;
        std::vector<Node> path;

        while (current_node != src) {
            path.push_back(current_node);
            current_node = origins.at(current_node);
        }

        path.push_back(src);
        std::reverse(path.begin(), path.end());
        return path;
    }

private:
    bool atomic_test_visited(Node node, const std::vector<int>& visited, omp_lock_t* lock) {
        omp_set_lock(lock);
        bool already_visited = visited.at(node);
        omp_unset_lock(lock);
        return already_visited;
    }

    void atomic_set_visited(Node node, std::vector<int>& visited, omp_lock_t* lock) {
        omp_set_lock(lock);
        visited[node] = true;
        omp_unset_lock(lock);
    }
};

// Helper functions
static Graph import_from_file(const std::string& path) {
    Graph graph;
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::invalid_argument("Input file does not exist or is not readable.");
    }

    std::string line;
    while (getline(file, line)) {
        std::vector<int> lineData;
        std::stringstream lineStream(line);
        int value;
        while (lineStream >> value) {
            lineData.push_back(value);
        }
        graph.adj_matrix.push_back(std::move(lineData));
    }

    return graph;
}

std::string bench_traverse(std::function<void()> traverse_fn) {
    auto start = std::chrono::high_resolution_clock::now();
    traverse_fn();
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    return std::to_string(duration.count());
}

void full_bench(Graph& graph) {
    int num_test = 1;
    std::array<int, 6> num_threads{{1, 2, 4, 8, 16, 32}};

    std::vector<Graph::Node> visited(graph.size(), false);
    Graph::Node src = 0;

    omp_set_dynamic(0);

    std::cout << "Number of nodes: " << graph.size() << "\n\n";

    for (int i = 0; i < num_test; i++) {
        std::cout << "\tExecution " << i + 1 << std::endl;
        std::cout << "Sequential iterative DFS: " << bench_traverse([&] { graph.dfs(src, visited); }) << "ms\n";

        std::fill(visited.begin(), visited.end(), false);
        std::cout << "Sequential iterative BFS: " << bench_traverse([&] { graph.dijkstra(src); }) << "ms\n";

        for (const auto n : num_threads) {
            std::fill(visited.begin(), visited.end(), false);
            std::cout << "Using " << n << " threads...\n";
            omp_set_num_threads(n);

            std::cout << "Parallel iterative DFS: " << bench_traverse([&] { graph.pdfs(src, visited); }) << "ms\n";

            std::fill(visited.begin(), visited.end(), false);
            std::cout << "Parallel iterative BFS: " << bench_traverse([&] { graph.p_dijkstra(src); }) << "ms\n";
        }

        std::cout << std::endl;
    }
}

int main(int argc, const char** argv) {
	std::string filename = "input.txt";
     try {
        // Attempt to read the file into a Graph object
        Graph graph = import_from_file(filename);
        full_bench(graph);  // Assuming this function runs benchmarks on the graph
    } catch (const std::exception& ex) {
        // Catch any exceptions (e.g., file not found, incorrect format)
        std::cerr << "Error: " << ex.what() << "\n";
    }

    return 0;
}
