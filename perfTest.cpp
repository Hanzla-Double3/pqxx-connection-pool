#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>
#include "ConnectionPool.hpp"

//g++ -std=c++17 perfTest.cpp -lpqxx -lpq
#define con_str_ "dbname=test user=postgres host=localhost port=5432 password=password"

void test_performance() {
    constexpr size_t pool_size = 10;
    constexpr int threads_count = 20;
    constexpr int iterations = 1000;
    constexpr auto test_duration = std::chrono::seconds(10);

    constexpr std::array<std::pair<const char*, const char*>, 1> prep_statements = {{
        {"perf_stmt", "SELECT $1::int"}
    }};

    ConnectionPool<1> pool(
        con_str_,
        pool_size,
        std::chrono::seconds(5),
        prep_statements
    );
    
    pool.init();

    std::atomic<bool> running{false};
    std::atomic<bool> stop{false};
    std::atomic<long> total_operations{0};
    std::atomic<long> successful_operations{0};
    std::atomic<long> failed_operations{0};
    
    auto worker = [&]() {
        while(!running.load()); // Wait for start signal
        
        while(!stop.load()) {
            try {
                auto guard = pool.get_connection();
                pqxx::work txn(*guard);
                auto result = txn.exec_prepared("perf_stmt", 1);
                txn.commit();
                successful_operations++;
            } catch (const std::exception& e) {
                failed_operations++;
            }
            total_operations++;
        }
    };

    // Create worker threads
    std::vector<std::thread> workers;
    for(int i = 0; i < threads_count; ++i) {
        workers.emplace_back(worker);
    }

    // Start test
    auto start = std::chrono::high_resolution_clock::now();
    running.store(true);
    
    // Run test for specified duration
    std::this_thread::sleep_for(test_duration);
    stop.store(true);

    // Wait for workers
    for(auto& t : workers) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Calculate metrics
    double ops_per_sec = static_cast<double>(total_operations) * 1000 / duration.count();
    double avg_latency = duration.count() / static_cast<double>(total_operations);
    double success_rate = (static_cast<double>(successful_operations) / total_operations) * 100;

    std::cout << "\nPerformance Test Results:\n";
    std::cout << "=========================\n";
    std::cout << "Test duration:      " << duration.count() << "ms\n";
    std::cout << "Total operations:   " << total_operations << "\n";
    std::cout << "Successful ops:     " << successful_operations << "\n";
    std::cout << "Failed ops:         " << failed_operations << "\n";
    std::cout << "Throughput:         " << ops_per_sec << " ops/ms\n";
    std::cout << "Average latency:    " << avg_latency << "ms/op\n";
    std::cout << "Success rate:       " << success_rate << "%\n";
    std::cout << "Active connections: " << pool.active_count_ << "\n";
    std::cout << "Idle connections:   " << pool.idle_connections_.size() << "\n";
}

int main() {
    test_performance();
    return 0;
}