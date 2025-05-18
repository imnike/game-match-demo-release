// @file  : battleManager.cpp
// @brief : manager for simulating battles, matchmaking queues and battle rooms
// @author: August
// @date  : 2025-05-15
#include "battleManager.h"
#include "playerManager.h"
#include "../../include/globalDefine.h"
#include "../../utils/utils.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>

BattleRoom::BattleRoom(const std::vector<Player*>& refVecTeamRed, const std::vector<Player*>& refVecTeamBlue)
    : m_roomId(BattleManager::instance().getNextRoomId())
{
	// red team
    for (Player* pPlayer : refVecTeamRed)
    {
        if (pPlayer)
        {
            pPlayer->setStatus(common::PlayerStatus::battle);
            m_vecTeamRed.emplace_back(std::make_unique<Hero>(pPlayer->getId()));
        }
    }

	// blue team
    for (Player* pPlayer : refVecTeamBlue)
    {
        if (pPlayer)
        {
            pPlayer->setStatus(common::PlayerStatus::battle);
            m_vecTeamBlue.emplace_back(std::make_unique<Hero>(pPlayer->getId()));
        }
    }
    std::cout << "Battle Room " << m_roomId << " created." << std::endl;
}

BattleRoom::~BattleRoom()
{
    std::cout << "Battle Room " << m_roomId << " destroyed." << std::endl;
}

void BattleRoom::startBattle()
{
    std::cout << "red team members: ";
    for (const auto& pHero : m_vecTeamRed)
    {
        if (pHero)
        {
            std::cout << pHero->getPlayerId() << "(hero:" << pHero->getId() << ") ";
        }
    }
    std::cout << std::endl;

    std::cout << "blue team members: ";
    for (const auto& pHero : m_vecTeamBlue)
    {
        if (pHero)
        {
            std::cout << pHero->getPlayerId() << "(hero:" << pHero->getId() << ") ";
        }
    }
    std::cout << std::endl;

    std::cout << "\n----- BATTLE START (Room " << m_roomId << ") -----" << std::endl;

	// simulate battle for 3 seconds
    std::this_thread::sleep_for(std::chrono::seconds(3));

	// simulate battle result(50% chance for each team to win)
    uint8_t dice = 2;
    bool isRedWin = (random_utils::getRandom(dice) == 0);

    std::vector<std::unique_ptr<Hero>>& vecWinningTeam = isRedWin ? m_vecTeamRed : m_vecTeamBlue;
    std::vector<std::unique_ptr<Hero>>& vecLosingTeam = isRedWin ? m_vecTeamBlue : m_vecTeamRed;

    std::cout << "\n" << (isRedWin ? "Red" : "Blue") << " Team wins in Room " << m_roomId << "!!!" << std::endl;

    for (auto& pHero : vecWinningTeam)
    {
        if (!pHero) continue;
        const uint64_t playerId = pHero->getPlayerId();
        BattleManager::instance().handlePlayerWin(playerId);
    }

    for (auto& pHero : vecLosingTeam)
    {
        if (!pHero) continue;
        const uint64_t playerId = pHero->getPlayerId();
        BattleManager::instance().handlePlayerLose(playerId);
    }
    std::cout << "----- BATTLE STOP (Room " << m_roomId << ") -----\n" << std::endl;

    finishBattle();
}

void BattleRoom::finishBattle()
{
    const uint64_t orgRoomId = m_roomId;
    std::cout << "Battle finished for Room " << orgRoomId << "." << std::endl;
    m_vecTeamRed.clear();
    m_vecTeamBlue.clear();

    BattleManager::instance().removeBattleRoom(orgRoomId);
    std::cout << "----- BATTLE FINISHED (Room " << orgRoomId << ") -----\n" << std::endl;
}

TeamMatchQueue::TeamMatchQueue() {}
TeamMatchQueue::~TeamMatchQueue() {}

void TeamMatchQueue::addMember(Player* pPlayer)
{
    std::lock_guard<std::mutex> lock(mutex);
    uint32_t tier = pPlayer->getTier();
    m_mapTierQueues[tier].emplace_back(pPlayer);
    std::cout << "Player " << pPlayer->getId() << " added to TEAM match queue for tier " << tier << std::endl;
}

bool TeamMatchQueue::hasEnoughMemberForTeam(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = m_mapTierQueues.find(tier);
    return (it != m_mapTierQueues.end()) && (it->second.size() >= battle::TeamMembers::TeamMemberMax);
}

std::vector<Player*> TeamMatchQueue::getPlayersForTeam(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<Player*> teamPlayers;
    auto it = m_mapTierQueues.find(tier);

	const uint8_t maxTeamSize = battle::TeamMembers::TeamMemberMax;

    if (it != m_mapTierQueues.end() && it->second.size() >= maxTeamSize)
    {
        teamPlayers.assign(it->second.begin(), it->second.begin() + maxTeamSize);
        it->second.erase(it->second.begin(), it->second.begin() + maxTeamSize);
        if (it->second.empty())
        {
            m_mapTierQueues.erase(it);
        }
    }
    return teamPlayers;
}

const std::map<uint32_t, std::vector<Player*>> TeamMatchQueue::getTierQueue() const
{
    std::lock_guard<std::mutex> lock(mutex);
	return m_mapTierQueues;
}

void TeamMatchQueue::clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    _clearNoLock();
}

void TeamMatchQueue::_clearNoLock()
{
    m_mapTierQueues.clear();
}


BattleMatchQueue::BattleMatchQueue() {}
BattleMatchQueue::~BattleMatchQueue() {}

void BattleMatchQueue::addTeam(std::vector<Player*> team)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (team.empty()) { return; }
    uint32_t tier = team[0]->getTier();
    m_mapTierQueues[tier].emplace_back(team);

    std::cout << "Team (Tier " << tier << ") added to BATTLE match queue. Players: ";
    for (Player* p : team)
    {
        std::cout << p->getId() << " ";
    }
    std::cout << std::endl;
}

bool BattleMatchQueue::hasEnoughTeamsForBattle(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = m_mapTierQueues.find(tier);
    return (it != m_mapTierQueues.end()) && (it->second.size() >= battle::TeamColor::TeamColorMax);
}

std::vector<std::vector<Player*>> BattleMatchQueue::getTeamsForBattle(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::vector<Player*>> battleTeams;
    auto it = m_mapTierQueues.find(tier);

    if (it != m_mapTierQueues.end() && it->second.size() >= battle::TeamColor::TeamColorMax)
    {
        battleTeams.assign(it->second.begin(), it->second.begin() + 2);
        it->second.erase(it->second.begin(), it->second.begin() + 2);
        if (it->second.empty())
        {
            m_mapTierQueues.erase(it);
        }
    }
    return battleTeams;
}

const std::map<uint32_t, std::vector<std::vector<Player*>>> BattleMatchQueue::getTierQueue() const
{
    std::lock_guard<std::mutex> lock(mutex);
	return m_mapTierQueues;
}

void BattleMatchQueue::clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    _clearNoLock();
}

void BattleMatchQueue::_clearNoLock()
{
    m_mapTierQueues.clear();
}

BattleManager& BattleManager::instance()
{
    static BattleManager instance;
    return instance;
}

BattleManager::BattleManager()
{
}

BattleManager::~BattleManager()
{
}

bool BattleManager::initialize()
{
    m_isRunning = false;

    std::cout << "[BattleManager] : initialized! " << std::endl;
    return true;
}

void BattleManager::release()
{
    stopMatchmaking();

	// lock all mutexes in automatic mode to avoid deadlock
    std::unique_lock<std::mutex> lockBattleRooms(m_battleRoomsMutex, std::defer_lock);
    std::unique_lock<std::mutex> lockTeamQueue(m_teamMatchQueue.mutex, std::defer_lock);
    std::unique_lock<std::mutex> lockBattleQueue(m_battleMatchQueue.mutex, std::defer_lock);

	std::lock(lockBattleRooms, lockTeamQueue, lockBattleQueue); // lock all at once

	// clear all resources
    m_battleRooms.clear();
    m_teamMatchQueue._clearNoLock();
    m_battleMatchQueue._clearNoLock();

    m_nextRoomId.store(0);
    std::cout << "[BattleManager] : released! " << std::endl;
}

void BattleManager::startMatchmaking()
{
    if (!m_isRunning)
    {
        m_isRunning = true;
		// create a new thread for matchmaking
        m_matchmakingThreadHandle = std::thread(&BattleManager::matchmakingThread, this);
    }
}

void BattleManager::stopMatchmaking()
{
    if (m_isRunning)
    {
        m_isRunning = false;
        if (m_matchmakingThreadHandle.joinable())
        {
			// wait for the matchmaking thread to finish
            m_matchmakingThreadHandle.join();
        }
    }
}

void BattleManager::addPlayerToQueue(Player* pPlayer)
{
    if (!pPlayer)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_playerAddQueueMutex);
    {
        if (pPlayer->isInLobby() == false)
        {
            //std::cerr << "Error: Player " << pPlayer->getId() << " is not in lobby." << std::endl;
            return;
        }
        m_teamMatchQueue.addMember(pPlayer);
        pPlayer->setStatus(common::PlayerStatus::queue);
    }
}

void BattleManager::handlePlayerWin(uint64_t playerId)
{
    std::cout << "Player " << playerId << " WIN!!! (+ " << battle::WINNER_SCORE << " points)" << std::endl;
    PlayerManager::instance().handlePlayerBattleResult(playerId, battle::WINNER_SCORE, true);
}

void BattleManager::handlePlayerLose(uint64_t playerId)
{
    std::cout << "Player " << playerId << " LOSE... (" << battle::LOSER_SCORE << " points)" << std::endl;
    PlayerManager::instance().handlePlayerBattleResult(playerId, battle::LOSER_SCORE, false);
}

// get next room id in atomic way
uint64_t BattleManager::getNextRoomId()
{
	return m_nextRoomId.fetch_add(1);   // automatically increment and return the current value
}

void BattleManager::removeBattleRoom(uint64_t roomId)
{
    std::lock_guard<std::mutex> lock(m_battleRoomsMutex);
    auto it = m_battleRooms.find(roomId);
    if (it != m_battleRooms.end())
    {
        m_battleRooms.erase(it);
        std::cout << "Removed Battle Room " << roomId << "." << std::endl;
    }
}

void BattleManager::matchmakingThread()
{
    std::cout << "[BattleManager] : Matchmaking thread started" << std::endl;

    while (m_isRunning)
    {
		// player to team
        std::vector<uint32_t> tiersToFormTeams;
        {
            std::lock_guard<std::mutex> lock(m_teamMatchQueue.mutex);
            for (const auto& queuePair : m_teamMatchQueue.m_mapTierQueues)
            {
                tiersToFormTeams.emplace_back(queuePair.first);
            }
        }

        for (uint32_t tier : tiersToFormTeams)
        {
            if (m_teamMatchQueue.hasEnoughMemberForTeam(tier))
            {
                std::vector<Player*> vecNewTeam = m_teamMatchQueue.getPlayersForTeam(tier);

                if (vecNewTeam.size() == battle::TeamMembers::TeamMemberMax)
                {
                    //std::cout << "Formed a 3-player team for tier " << tier << ". Players: ";
                    for (Player* pPlayer : vecNewTeam) 
                    {
                        std::cout << pPlayer->getId() << " ";
                    }
                    std::cout << std::endl;

                    m_battleMatchQueue.addTeam(vecNewTeam);
                }
                else 
                {
                    std::cerr << "[ERROR] "
                        << "[" << __FILE__ << ":" << __LINE__ << "] "
                        << "[" << __func__ << "] "
                        << "incorrect number of players for tier : " << tier
                        << std::endl;
                }
            }
        }

        // team to battle
        std::vector<uint32_t> tiersToStartBattles;
        {
            std::lock_guard<std::mutex> lock(m_battleMatchQueue.mutex);
            for (const auto& queuePair : m_battleMatchQueue.m_mapTierQueues)
            {
                tiersToStartBattles.emplace_back(queuePair.first);
            }
        }

        for (uint32_t tier : tiersToStartBattles)
        {
            if (m_battleMatchQueue.hasEnoughTeamsForBattle(tier))
            {
                std::vector<std::vector<Player*>> vecBattleTeams = m_battleMatchQueue.getTeamsForBattle(tier);

                if (vecBattleTeams.size() == battle::TeamColor::TeamColorMax)
                {
                    std::cout << "\nMatched 2 teams for tier " << tier << ". Initiating battle!\n";

                    std::unique_ptr<BattleRoom> uRoom;
                    uint64_t roomIdForThread = 0;

					// create a new BattleRoom and add it to the map
                    {
                        std::lock_guard<std::mutex> lock(m_battleRoomsMutex);
                        uRoom = std::make_unique<BattleRoom>(vecBattleTeams[battle::TeamColor::TeamColorRed], vecBattleTeams[battle::TeamColor::TeamColorBlue]);
						roomIdForThread = uRoom->getRoomId();   // get the room id in automatic way
                        m_battleRooms[roomIdForThread] = std::move(uRoom);
                    }
                    // release lock m_battleRoomsMutex

					// new thread for battle
                    std::thread battle_thread([roomIdForThread]() 
                        {
                        BattleRoom* pBattleRoom = nullptr;
                        {
                            std::lock_guard<std::mutex> lock(BattleManager::instance().m_battleRoomsMutex);
                            auto it = BattleManager::instance().m_battleRooms.find(roomIdForThread);
                            if (it != BattleManager::instance().m_battleRooms.end()) 
                            {
                                pBattleRoom = it->second.get();
                            }
                        }
						// release lock m_battleRoomsMutex

                        if (pBattleRoom) 
                        {
                            pBattleRoom->startBattle();
                        }
                        else 
                        {
                            std::cerr << "[ERROR] "
                                << "[" << __FILE__ << ":" << __LINE__ << "] "
                                << "[" << __func__ << "] "
								<< "BattleRoom not found for roomId : " << roomIdForThread
                                << std::endl;
                        }
                        });
					battle_thread.detach(); // detach the thread to run independently
                }
                else 
                {
                    std::cerr << "[ERROR] "
                        << "[" << __FILE__ << ":" << __LINE__ << "] "
                        << "[" << __func__ << "] "
						<< "getTeamsForBattle returned incorrect number of teams for tier : " << tier
						<< std::endl;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[BattleManager] : Matchmaking thread stopped" << std::endl;
}