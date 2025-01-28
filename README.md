# C++ PostgreSQL Connection Pool

![Screenshot 2025-01-27 221128](https://github.com/user-attachments/assets/bbfc2873-293a-469d-8e39-f2c59b6797c9)

*Performance metrics showing throughput vs. pool size*

*Performance on my shitty laptop never cared to check its specs, probably 10 gen i5, tested on Windows subsystem for linux*

A Maybe high-performance connection pool implementation for PostgreSQL in C++ using libpqxx, designed for efficient database resource management in concurrent applications.

## Features

- **Connection Pooling** - Reuse database connections to reduce overhead
- **Prepared Statements** - Built-in SQL statement preparation
- **RAII Management** - Automatic connection release via guard objects
- **Thread Safety** - Full mutex protection for concurrent access
- **Timeout Handling** - Configurable connection acquisition timeout
- **Dynamic Sizing** - Connection pool expansion/contraction (WIP)

## Installation

### Dependencies
- libpqxx 7.7+
- PostgreSQL 12+
- C++17 compiler

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install libpqxx-dev postgresql-server-dev-12
```

## Usage
```cpp
#include "ConnectionPool.hpp"

constexpr std::array prep_statements = {{
    {"get_user", "SELECT * FROM users WHERE id = $1::int"},
    {"insert_log", "INSERT INTO logs (message) VALUES ($1::text)"}
}};

int main() {
    ConnectionPool<2> pool(
        "host=localhost dbname=mydb user=postgres password=secret",
        10,  // Pool size
        std::chrono::seconds(5),
        prep_statements
    );
    
    pool.init();

    try {
        auto guard = pool.get_connection();
        pqxx::work txn(*guard);
        auto result = txn.exec_prepared("get_user", 42);
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
    }
    
    return 0;
}
```

## Note
- Replace connection string with your database credentials
- Prepare all SQL statements during pool initialization
- Connection guards automatically return connections to the pool

## Lincense

WTFPL (Do What The Fuck You Want To Public License)
