// @file  : playerManager.cpp
// @brief : manager for player data
// @author: August
// @date  : 2025-05-15
#include "playerManager.h"
#include "dbManager.h"
#include "battleManager.h"
#include "../../utils/utils.h"
#include "../../include/globalDefine.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <algorithm>

PlayerManager& PlayerManager::instance()
{
    static PlayerManager instance;
    return instance;
}

PlayerManager::PlayerManager()
{
}

PlayerManager::~PlayerManager()
{
}

bool PlayerManager::initialize()
{
    m_mapPlayers.clear();
    m_setOnlinePlayerIds.clear();
    m_setDirtyPlayerIds.clear();

    std::cout << "[PlayerManager] : initialized!" << std::endl;
    return true;
}

void PlayerManager::release()
{
    // lock all mutexes in automatic mode to avoid deadlock
    std::unique_lock<std::mutex> lockMapPlayers(m_mapPlayersMutex, std::defer_lock);
    std::unique_lock<std::mutex> lockSetdirtyPlayerIds(m_setdirtyPlayerIdsMutex, std::defer_lock);

	std::lock(lockMapPlayers, lockSetdirtyPlayerIds);   // lock all at once
    
	// clear all resources
    m_setOnlinePlayerIds.clear();
    m_mapPlayers.clear();
	m_setDirtyPlayerIds.clear();
    std::cout << "[PlayerManager] : released!" << std::endl;
}

Player* PlayerManager::playerLogin(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mapPlayersMutex);

    if (id == 0)
    {
		// insert new player
        id = DbManager::instance().insertPlayerBattles();
        if (id == 0)
        {
            std::cerr << "[ERROR] "
                << "[" << __FILE__ << ":" << __LINE__ << "] "
                << "[" << __func__ << "] "
				<< "Failed to create player in database." 
                << std::endl;
			return nullptr;
        }
        //std::cout << "New player created with ID: " << id << std::endl;
    }

    Player* pPlayer = _getPlayerNoLock(id);
    if (!pPlayer)
    {
        std::cerr << "[WARNING] "
            << "[" << __FILE__ << ":" << __LINE__ << "] "
            << "[" << __func__ << "] "
			<< "Player " << id << " not found." 
            << std::endl;
        return nullptr;
    }
    //std::cout << "Player " << id << " login." << std::endl;
    _setPlayerOnlineNoLock(id, true);
    if (pPlayer->getStatus() == common::PlayerStatus::offline)
    {
        pPlayer->setStatus(common::PlayerStatus::lobby);
    }
    return pPlayer;
}

bool PlayerManager::playerLogout(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mapPlayersMutex);

    Player* pPlayer = _getPlayerNoLock(id);
    if (!pPlayer)
    {
        // std::cout << "Player " << id << " is not logged in." << std::endl;
        return false;
    }

    //std::cout << "Player " << id << " logout." << std::endl;
    _setPlayerOnlineNoLock(id, false);
    pPlayer->setStatus(common::PlayerStatus::offline);
	// Save player data to database
	enqueuePlayerSave(id);
    return true;
}

bool PlayerManager::isPlayerOnline(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mapPlayersMutex);

    return (m_setOnlinePlayerIds.find(id) != m_setOnlinePlayerIds.end());
}

Player* PlayerManager::getPlayer(uint64_t id)
{
	std::lock_guard<std::mutex> lock(m_mapPlayersMutex);
	Player* pPlayer = _getPlayerNoLock(id);
    if (!pPlayer)
    {
		std::cout << "Player " << id << " not found." << std::endl;
		return nullptr;
    }
	return pPlayer;
}

std::vector<Player*> PlayerManager::getOnlinePlayers() 
{
    std::lock_guard<std::mutex> lock(m_mapPlayersMutex);

    std::vector<Player*> tmpVecPlayers;
    for (auto& id : m_setOnlinePlayerIds) 
    {
        Player* pPlayer = _getPlayerNoLock(id);
        if (!pPlayer)
        {
            continue;
        }
        tmpVecPlayers.emplace_back(pPlayer);
    }
    return tmpVecPlayers;
}

// *** only for dbManager to sync player data from db ***
void PlayerManager::syncPlayerFromDbNoLock(uint64_t id, uint32_t score, uint32_t wins, uint64_t updatedTime)
{
	_syncPlayerNoLock(id, score, wins, updatedTime);
}

Player* PlayerManager::_getPlayerNoLock(uint64_t id)
{
    if (m_mapPlayers.empty())
    {
        return nullptr;
    }
    auto it = m_mapPlayers.find(id);
    if (it != m_mapPlayers.end())
    {
        return it->second.get();
    }
    return nullptr;
}

void PlayerManager::_setPlayerOnlineNoLock(uint64_t id, bool isOnline)
{
    if (isOnline)
    {
        m_setOnlinePlayerIds.emplace(id);
    }
    else
    {
        m_setOnlinePlayerIds.erase(id);
    }
}

void PlayerManager::_syncPlayerNoLock(uint64_t id, uint32_t score, uint32_t wins, uint64_t updatedTime)
{
    if (m_mapPlayers.find(id) != m_mapPlayers.end())
    {
        //std::cerr << "Player " << id << " already exists." << std::endl;
        return;
    }
    std::unique_ptr<Player> uPlayer = std::make_unique<Player>(id, score, wins, updatedTime);
    m_mapPlayers[id] = std::move(uPlayer);
}

void PlayerManager::handlePlayerBattleResult(uint64_t playerId, uint32_t scoreDelta, bool isWin)
{
    {
        std::lock_guard<std::mutex> lock(m_mapPlayersMutex); // 保護對 m_mapPlayers 的訪問

        Player* pPlayer = _getPlayerNoLock(playerId);
        if (!pPlayer)
        {
            return;
        }
        if (isWin)
        {
            pPlayer->addWins();
            pPlayer->addScore(scoreDelta);
        }
        else
        {
            pPlayer->subScore(scoreDelta);
        }
        pPlayer->setStatus(common::PlayerStatus::lobby);
    }
    enqueuePlayerSave(playerId);
}

void PlayerManager::enqueuePlayerSave(uint64_t playerId)
{
	std::lock_guard<std::mutex> lock(m_setdirtyPlayerIdsMutex);
	m_setDirtyPlayerIds.emplace(playerId);
}

void PlayerManager::saveDirtyPlayers()
{
    if (m_setDirtyPlayerIds.empty())
    {
        return;
    }
    std::set<uint64_t> tmpSetSaveIds;
    {
		// lock m_setDirtyPlayerIds
        std::lock_guard<std::mutex> lock(m_setdirtyPlayerIdsMutex);
		tmpSetSaveIds.swap(m_setDirtyPlayerIds);    // save the ids to tmpSetSaveIds and clear m_setDirtyPlayerIds
    }
    for (auto& id : tmpSetSaveIds)
    {
		Player* pPlayer = _getPlayerNoLock(id);
        if (!pPlayer)
        {
            continue;
        }
        DbManager::instance().updatePlayerBattles(pPlayer->getId(), pPlayer->getScore(), pPlayer->getWins());
    }
}