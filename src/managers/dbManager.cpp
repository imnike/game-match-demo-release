// @file  : dbManager.cpp
// @brief : manager for database(sqlite3)
// @author: August
// @date  : 2025-05-15
#include "dbManager.h"
#include "playerManager.h"
#include "../../libs/sqlite/sqlite3.h"
#include "../../utils/utils.h"
#include <iostream>
#include <chrono>

// create table sql statements
std::unordered_map<std::string, std::string> MAP_CREATE_TABLE_SQL = {
    {"player_battles", "CREATE TABLE IF NOT EXISTS player_battles (id INTEGER PRIMARY KEY, score INTEGER, wins INTEGER, updated_time INTEGER)"},
};

DbManager& DbManager::instance()
{
    static DbManager instance;
    return instance;
}

DbManager::DbManager()
    : m_dbHandler(nullptr), m_dbName("gameMatch.db")
{
}

DbManager::~DbManager()
{
}

bool DbManager::initialize()
{
	m_mapFuncSyncData.clear();
    m_mapFuncSyncData["player_battles"] = [this]() { this->syncAllPlayerBattles(); };
    if (m_dbHandler)
    {
        sqlite3_close(m_dbHandler);
        m_dbHandler = nullptr;
    }
	std::cout << "[DbManager] : initialized!" << std::endl;
    return true;
}

bool DbManager::connect()
{
    int rc = sqlite3_open(m_dbName.c_str(), &m_dbHandler);
    if (rc != SQLITE_OK)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "SQL error: " << sqlite3_errmsg(m_dbHandler)
            << std::endl;
        m_dbHandler = nullptr;
		return false;
    }
	std::cout << "[DbManager] : connect database opened successfully." << std::endl;
    return true;
}

void DbManager::release()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_dbHandler)
    {
		// close database connection
        sqlite3_close(m_dbHandler);
        m_dbHandler = nullptr;
	}
	m_mapFuncSyncData.clear();
    std::cout << "[DbManager] : released!" << std::endl;
}

bool DbManager::ensureTableSchema()
{
    for (auto& itTable : MAP_CREATE_TABLE_SQL)
    {
        const std::string tableName = itTable.first;
        if (!isTableExists(tableName))
        {
            if (!createTable(tableName))
            {
                std::cerr << "[ERROR] "
                    << "[" << __FILE__ << ":" << __LINE__ << "] "
                    << "[" << __func__ << "] "
                    << "Failed to create table '" << tableName << "'."
                    << std::endl;
                return false;
            }
        }
    }
    return true;
}

void DbManager::loadTableData()
{
    for (auto& itFunc : m_mapFuncSyncData)
    {
		const std::string tableName = itFunc.first;
		// execute the function to load data from the table
        itFunc.second();
    }
}

// sync all player battles data from database to PlayerManager
void DbManager::syncAllPlayerBattles()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_dbHandler)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "Database not open."
            << std::endl;
        return;
    }

    const char* sql = "SELECT id, score, wins, updated_time FROM player_battles;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_dbHandler, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "SQL error: " << sqlite3_errmsg(m_dbHandler)
            << std::endl;
        return;
    }
    uint64_t id = 0;
    uint32_t score = 0;
    uint32_t wins = 0;
    uint64_t updatedTime = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        id = sqlite3_column_int64(stmt, 0);
        score = sqlite3_column_int(stmt, 1);
        wins = sqlite3_column_int(stmt, 2);
        updatedTime = sqlite3_column_int64(stmt, 3);
        if (id == 0)
        {
            continue;
        }
        PlayerManager::instance().syncPlayerFromDbNoLock(id, score, wins, updatedTime);
    }
    sqlite3_finalize(stmt);
}

bool DbManager::isTableExists(const std::string tableName)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_dbHandler) 
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "Database not open."
            << std::endl;
        return false;
    }

    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt* stmt = nullptr;
    bool exists = false;

    int rc = sqlite3_prepare_v2(m_dbHandler, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) 
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "SQL error: " << sqlite3_errmsg(m_dbHandler)
            << std::endl;

		if (stmt) sqlite3_finalize(stmt);   // clean up the statement if it was prepared
        return false;
    }

    sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        exists = true;
    }

	sqlite3_finalize(stmt); // clean up the statement
    return exists;
}

bool DbManager::createTable(const std::string tableName)
{
    if (!m_dbHandler)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "Database not open."
            << std::endl;
        return false;
    }

    auto itSql = MAP_CREATE_TABLE_SQL.find(tableName);
    if (itSql == MAP_CREATE_TABLE_SQL.end())
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "not found in map Table name : " << tableName
            << std::endl;
		return false;
    }
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_dbHandler, itSql->second.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "SQL error: " << errMsg
            << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

uint64_t DbManager::insertPlayerBattles()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_dbHandler)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "Database not open."
            << std::endl;
        return false;
    }

	// no id in the insert statement, it will be auto-incremented
    const char* sql =
        "INSERT INTO player_battles (score, wins, updated_time) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_dbHandler, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "SQL error: " << sqlite3_errmsg(m_dbHandler)
            << std::endl;
        return 0;
    }
    const uint32_t score = 0;
	const uint32_t wins = 0;
	const uint64_t updatedTime = time_utils::getTimestampMS();

    sqlite3_bind_int(stmt, 1, score);
    sqlite3_bind_int(stmt, 2, wins);
    sqlite3_bind_int64(stmt, 3, updatedTime);

    rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt); // clean up the statement

    if (rc != SQLITE_DONE)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "SQL error: " << sqlite3_errmsg(m_dbHandler)
            << std::endl;
        return 0;
    }

	// get the last inserted row id after the insert operation
    uint64_t id = sqlite3_last_insert_rowid(m_dbHandler);
    if (id == 0)
    {
        return 0;
    }
	// sync the player data to PlayerManager
	PlayerManager::instance().syncPlayerFromDbNoLock(id, score, wins, updatedTime);
    return id;
}

bool DbManager::updatePlayerBattles(uint64_t id, uint32_t score, uint32_t wins)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_dbHandler)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "Database not open."
            << std::endl;
        return false;
    }

    const char* sql = "UPDATE player_battles SET score = ?, wins = ?, updated_time = ? WHERE id = ?;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_dbHandler, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "SQL error: " << sqlite3_errmsg(m_dbHandler)
            << std::endl;
        return false;
    }

    const uint64_t updatedTime = time_utils::getTimestampMS();

    sqlite3_bind_int(stmt, 1, score);
    sqlite3_bind_int(stmt, 2, wins);
    sqlite3_bind_int64(stmt, 3, updatedTime);
    sqlite3_bind_int64(stmt, 4, id);

    rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt); // clean up the statement

    if (rc != SQLITE_DONE)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "SQL error: " << sqlite3_errmsg(m_dbHandler)
            << std::endl;
        return false;
    }

    return true;
}
bool DbManager::queryPlayerBattles(uint64_t id, uint32_t& score, uint32_t& wins, uint64_t& updateTime)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_dbHandler)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "Database not open."
            << std::endl;
        return false;
    }

    const char* sql = "SELECT score, wins, updated_time FROM player_battles WHERE id = ?;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_dbHandler, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "[ERROR] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
            << "SQL error: " << sqlite3_errmsg(m_dbHandler)
            << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        score = sqlite3_column_int(stmt, 0);
        wins = sqlite3_column_int(stmt, 1);
        updateTime = sqlite3_column_int64(stmt, 2);
		sqlite3_finalize(stmt); // clean up the statement
        return true;
    }
    return false;
}