# GameMatchDemo

## Project Overview

**GameMatchDemo** is a simple game matching demo project developed in C++ with multi-threading support, running on Windows 11, and utilizing SQLite3 as its database.
It simulates the entire process of players batching into a matching queue, automatic team formation, entering battles, handling battle results, and saving data.
This project aims to demonstrate the core logic of implementing a matching system, scheduling, player data management, persistent storage, and multi-threaded concurrency.

## Project Goals

This project serves as a demo of my personal programming capabilities and understanding of server development.
* Demonstrates C++ multi-threading programming and synchronization mechanisms for concurrent operations.
* Core logic includes player creation, state management, a tiered matching system, and database read/write operations.
* Implements data persistence, using SQLite for asynchronous player data reads and writes.
* Features a scheduled task manager for executing background tasks (e.g., periodic database writes).

## Core Features

### Player Manager (PlayerManager)
* Player login/logout mechanisms.
* Manages player online status and in-game states (offline/matching/in-battle/lobby).
* Processes player battle results and updates scores and wins.
* **Asynchronous Write-back:** When player data changes, it's added to a schedule for periodic write-back to the database.

### Battle Match Manager (BattleManager)
* Tier-based matching according to player rank. Includes an automatic team formation mechanism (3v3).
* Creates independent battle rooms (BattleRoom) and battle units (Hero).
* Each room runs a simple simulated battle and result determination on its own thread.
* Battle outcomes (win/loss) are synchronized after the battle concludes.

### Database Manager (DbManager)
* Implements data persistent storage based on the lightweight SQLite database.
* Provides interfaces for Create, Read, Update, and Delete (CRUD) operations on player battle data.
* Ensures database table structures exist upon initialization and loads data into the game.

### Schedule Task Manager (ScheduleManager)
* A general-purpose, multi-thread safe task scheduler.
* Supports both periodic tasks (e.g., player data write-back) and one-time tasks.

## Project Structure
```
.
├── include/
│   └── globalDefine.h          # Global definitions
├── libs/
│   └── sqlite/                 # SQLite database library
│       ├── sqlite3.c
│       └── sqlite3.h
├── src/
│   ├── managers/
│   │   ├── battleManager.cpp   # Battle and matching core logic
│   │   ├── battleManager.h
│   │   ├── dbManager.cpp       # SQLite database operation interface
│   │   ├── dbManager.h
│   │   ├── playerManager.cpp   # Player data management
│   │   ├── playerManager.h
│   │   ├── scheduleManager.cpp # Timed task scheduler
│   │   └── scheduleManager.h
│   ├── objects/
│   │   ├── hero.cpp            # Hero class
│   │   ├── hero.h
│   │   ├── player.cpp          # Player class
│   │   └── player.h
│   └── main.cpp                # Application entry point, initializes managers, handles user commands
├── utils/
│   ├── utils.cpp               # Utility functions (time, string processing)
│   └── utils.h
├── README.md
└── README.zh-TW.md
```

## Areas for Optimization

This project serves as a conceptual demo, aiming to quickly set up a multi-threaded server application and showcase basic player management, battle matching, and data persistence.
During development, I constantly became aware of many potential areas for optimization or necessary improvements. However, due to time constraints in creating this demo, many advanced features could not be fully implemented.
Therefore, I've outlined a review of the currently known design and implementation shortcomings, along with concrete optimization directions and practical recommendations below:

---

### 1. Graceful Shutdown Mechanism

* **Current Status:** The current shutdown functionality merely terminates the main control via an `exit` command in `main.cpp` and calls each Manager's `release()` method to cease operations.
* **Problem:** Without a robust **graceful shutdown** mechanism, we can't ensure all functionalities exit cleanly (e.g., player matching or database write-backs). This poses risks of **data loss** or **deadlocks**.
* **Optimization:** Implement a **global, centralized server state management console**. Upon receiving a shutdown command, it will orchestrate the Managers to safely stop services by sequentially calling their dedicated `shutdown()` methods, ensuring data is fully persisted before final termination.

---

### 2. Matching Mechanism Security

* **Current Status:** The current matchmaking is a simplistic list-based implementation driven by functional requirements, without comprehensive consideration of all scenarios.
* **Problem:** There's a lack of robust mechanisms for **safe mid-match removal** of players, including active exits or timeout removals. This can lead to **state desynchronization** or **null pointer issues**.
* **Optimization:** Implement a secure player exit mechanism that ensures **state synchronization** and maintains **queue integrity** during both player addition and removal.

---

### 3. Matching Mechanism Efficiency

* **Current Status:** The current matchmaking mechanism relies on periodically checking a simple list, which is a functional implementation without full consideration for efficiency.
* **Problem:** Being a plain list, it lacks **score-based sorting**, limiting its scalability and flexibility. Furthermore, both single-player and team-based matching stages involve **traversal (O(n))** for each match, leading to inefficiency and significant memory consumption.
* **Optimization:** Consider an **event-driven** approach, incorporating a **score-based sorting algorithm** and enhanced **team-forming functionalities**. This should save memory and improve matching efficiency to **O(log N)** levels.

---

### 4. Database Selection

* **Current Status:** For rapid and easy setup, the project initially uses **SQLite** as its sole database, primarily responsible for storing player battle data.
* **Problem:** SQLite is a lightweight database inherently less mature in terms of **throughput, scalability, and security**. It presents **concurrency bottlenecks** and risks of **data loss**, making it unsuitable for a live game environment.
* **Optimization:** Migrate the data to more comprehensive database servers like **MySQL or PostgreSQL**. Concurrently, implement a **connection pool** and various **read/write optimization mechanisms** to ensure stability and performance under high concurrency.

---

### 5. Object Pool Mechanism

* **Current Status:** During matchmaking, the system frequently creates and destroys objects like `BattleRoom` and `Hero`. Although **smart pointers** are used to prevent memory leaks and null pointer issues, this only addresses the safety aspect.
* **Problem:** This **frequent dynamic memory allocation and deallocation** significantly **increases CPU overhead**, slowing down application performance. Moreover, it substantially **raises the risk of memory fragmentation**, impacting long-term system stability.
* **Optimization:** Introduce an **Object Pool mechanism** to manage these objects. This involves **pre-configuring** a set number of objects. When needed, objects are **borrowed** from the pool, and once their use is complete, they are **returned** to it. This approach avoids repetitive memory allocation and deallocation, thereby **reducing performance overhead**.

---

## Author

* August Jian (sppaugust@gmail.com)