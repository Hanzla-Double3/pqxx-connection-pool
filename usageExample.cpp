#include "ConnectionPool.hpp"
#include <iostream>
#include <cassert>
#include <thread>

//g++ -std=c++17 usageExample.cpp -lpqxx -lpq

#define con_str_ "dbname=test user=postgres host=localhost port=5432 password=password"

// Usage Example
void example_usage() {
    // Prepared statements (name, SQL)
    constexpr std::array<std::pair<const char*, const char*>, 2> prep_statements = {{
        {"find_user", "SELECT * FROM users WHERE id = $1"},
        {"insert_user", "INSERT INTO users (name, email) VALUES ($1, $2)"}
    }};

    // Create connection pool
    ConnectionPool<2> pool(
        con_str_, 
        5, 
        std::chrono::seconds(5),
        prep_statements
    );

    // Initialize connections
    pool.init();

    try {
        // Get a connection
        auto guard = pool.get_connection();
        pqxx::work txn(*guard);
        
        // Execute prepared statement
        auto result = txn.exec_prepared("find_user", 1);
        txn.commit();
        
        std::cout << "Found " << result.size() << " users\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

// Test Cases
void test_basic_functionality() {
    constexpr std::array<std::pair<const char*, const char*>, 1> prep_statements = {{
        {"test_query", "SELECT 1"}
    }};

    ConnectionPool<1> pool(
        con_str_,
        3,
        std::chrono::seconds(2),
        prep_statements
    );
    
    pool.init();

    // Test initial connection count
    assert(pool.idle_connections_.size() == 3);
    assert(pool.active_count_ == 0);

    // Test connection acquisition
    {
        auto guard = pool.get_connection();
        assert(pool.active_count_ == 1);
        assert(pool.idle_connections_.size() == 2);
        
        pqxx::work txn(*guard);
        txn.exec_prepared("test_query");
        txn.commit();
    }

    // Test connection release
    assert(pool.active_count_ == 0);
    assert(pool.idle_connections_.size() == 3);
}

void test_concurrent_access() {
    constexpr std::array<std::pair<const char*, const char*>, 0> no_statements{};
    
    ConnectionPool<0> pool(
        con_str_,
        2,
        std::chrono::milliseconds(100),
        no_statements
    );
    
    pool.init();

    // Acquire all connections
    auto guard1 = pool.get_connection();
    auto guard2 = pool.get_connection();

     try {
        // Should timeout
        auto guard3 = pool.get_connection();
        assert(false && "Should have thrown timeout exception");
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()) == "Connection pool timeout");
    }
}

void test_drain_functionality() {
    constexpr std::array<std::pair<const char*, const char*>, 0> no_statements{};
    
    ConnectionPool<0> pool(
        con_str_,
        3,
        std::chrono::seconds(1),
        no_statements
    );
    
    pool.init();

    {
        auto guard = pool.get_connection();
        assert(pool.active_count_ == 1);
    }
    
    pool.drain();
    assert(pool.idle_connections_.empty());
    assert(pool.active_count_ == 0);
}

void test_prepared_statements() {
    constexpr std::array<std::pair<const char*, const char*>, 1> prep_statements = {{
        // Add explicit type casts to the SQL parameters
        {"test_stmt", "SELECT $1::int + $2::int"}
    }};

    ConnectionPool<1> pool(
        con_str_,
        1,
        std::chrono::seconds(1),
        prep_statements
    );
    
    pool.init();

    try {
        auto guard = pool.get_connection();
        pqxx::work txn(*guard);
        auto result = txn.exec_prepared("test_stmt", 2, 3);
        txn.commit();
        assert(result[0][0].as<int>() == 5);
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        throw;
    }
}

void test_increase_connections(){
    constexpr std::array<std::pair<const char*, const char*>, 1> prep_statements = {{
        // Add explicit type casts to the SQL parameters
        {"test_stmt", "SELECT $1::int + $2::int"}
    }};

    ConnectionPool<1> pool(
        con_str_,
        1,
        std::chrono::seconds(1),
        prep_statements
    );
    
    pool.init();
    auto conn1_ = pool.get_connection();
    pool.increase_connection();
    try {
        auto guard = pool.get_connection();
        pqxx::work txn(*guard);
        auto result = txn.exec_prepared("test_stmt", 2, 3);
        txn.commit();
        assert(result[0][0].as<int>() == 5);
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        throw;
    }
}

void test_decrease_connections(){
    constexpr std::array<std::pair<const char*, const char*>, 1> prep_statements = {{
        // Add explicit type casts to the SQL parameters
        {"test_stmt", "SELECT $1::int + $2::int"}
    }};

    ConnectionPool<1> pool(
        con_str_,
        2,
        std::chrono::seconds(1),
        prep_statements
    );
    
    pool.init();
    auto conn1_ = pool.get_connection();
    pool.decrease_connection();
    try {
        auto guard = pool.get_connection();
    } catch (const std::exception& e) {
        std::cout << "Test passed dec conn\n" << e.what() << std::endl;
    }
}

void test_number_of_connections(){
    constexpr std::array<std::pair<const char*, const char*>, 1> prep_statements = {{
        // Add explicit type casts to the SQL parameters
        {"test_stmt", "SELECT $1::int + $2::int"}
    }};

    ConnectionPool<1> pool(
        con_str_,
        2,
        std::chrono::seconds(1),
        prep_statements
    );
    pool.init();
    assert(pool.get_current_connections() == 2);
    pool.decrease_connection();
    assert(pool.get_current_connections() == 1);
    pool.increase_connection();
    assert(pool.get_current_connections() == 2);
}


int main() {
    example_usage();
    
    test_basic_functionality();
    test_concurrent_access();
    test_drain_functionality();
    test_prepared_statements();
    test_increase_connections();
    test_decrease_connections();
    test_number_of_connections();
    std::cout << "All tests passed!\n";
    return 0;
}