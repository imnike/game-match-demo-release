// dbManager.h
#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

struct sqlite3;

class DbManager
{
public:
    static DbManager& instance();

    bool initialize();
    bool connect();
    void release();
    void loadTableData();
    bool ensureTableSchema();
    bool isTableExists(const std::string tableName);
    bool createTable(const std::string tableName);

    void syncAllPlayerBattles();
    uint64_t insertPlayerBattles();
    bool updatePlayerBattles(uint64_t id, uint32_t score, uint32_t wins);
    bool queryPlayerBattles(uint64_t id, uint32_t& score, uint32_t& wins, uint64_t& updateTime);


private:
    DbManager();
    ~DbManager();

    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;
    DbManager(DbManager&&) = delete;
    DbManager& operator=(DbManager&&) = delete;

    sqlite3* m_dbHandler = nullptr;
    std::string m_dbName = "";
    std::unordered_map<std::string/* table name */, std::function<void()>> m_mapFuncSyncData{};

	std::mutex m_mutex; // lock for thread safety
};

#endif // DB_MANAGER_H