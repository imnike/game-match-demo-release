// battleManager.h
#ifndef BATTLE_MANAGER_H
#define BATTLE_MANAGER_H
#include "../objects/player.h"
#include "../objects/hero.h"
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

class BattleRoom
{
public:
    BattleRoom(const std::vector<Player*>& refVecTeamRed, const std::vector<Player*>& refVecTeamBlue);
    ~BattleRoom();
    void startBattle();
    void finishBattle();

    uint64_t getRoomId() const { return m_roomId; }

private:
    uint64_t m_roomId;
    std::vector<std::unique_ptr<Hero>> m_vecTeamRed;
    std::vector<std::unique_ptr<Hero>> m_vecTeamBlue;
};

// team : 3 players
class TeamMatchQueue
{
    friend class BattleManager;
public:
    TeamMatchQueue();
    ~TeamMatchQueue();

    void addMember(Player* pPlayer);
    bool hasEnoughMemberForTeam(uint32_t tier);
    std::vector<Player*> getPlayersForTeam(uint32_t tier);
    const std::map<uint32_t/* tier */, std::vector<Player*>> getTierQueue() const;
    void clear();

    mutable std::mutex mutex;

private:
	// private method without lock
    void _clearNoLock();

    std::map<uint32_t, std::vector<Player*>> m_mapTierQueues;
};

// battle : 2 teams
class BattleMatchQueue
{
    friend class BattleManager;
public:
    BattleMatchQueue();
    ~BattleMatchQueue();

    void addTeam(std::vector<Player*> team);
    bool hasEnoughTeamsForBattle(uint32_t tier);
    std::vector<std::vector<Player*>> getTeamsForBattle(uint32_t tier);
    const std::map<uint32_t/* tier */, std::vector<std::vector<Player*>>> getTierQueue() const;
    void clear();

    mutable std::mutex mutex;

private:
    // private method without lock
    void _clearNoLock();

    std::map<uint32_t, std::vector<std::vector<Player*>>> m_mapTierQueues;
};

class BattleManager
{
public:
    static BattleManager& instance();

    bool initialize();
    void release();

    void startMatchmaking();
    void stopMatchmaking();

    void addPlayerToQueue(Player* pPlayer);

    void handlePlayerWin(uint64_t playerId);
    void handlePlayerLose(uint64_t playerId);

	uint64_t getNextRoomId();   // get auto increment roomID

    void removeBattleRoom(uint64_t roomId);

    const TeamMatchQueue* getTeamMatchQueue() const { return &m_teamMatchQueue; }
    const BattleMatchQueue* getBattleMatchQueue() const { return &m_battleMatchQueue; }

private:
    BattleManager();
    ~BattleManager();

    BattleManager(const BattleManager&) = delete;
    BattleManager& operator=(const BattleManager&) = delete;
    BattleManager(BattleManager&&) = delete;
    BattleManager& operator=(BattleManager&&) = delete;

    void matchmakingThread();

	std::atomic<bool> m_isRunning = false;  // matchmaking thread running flag
	std::thread m_matchmakingThreadHandle;  // thread for matchmaking

	TeamMatchQueue m_teamMatchQueue{};      // queue for player match to team
	BattleMatchQueue m_battleMatchQueue{};  // queue for team mathch to battle

    std::map<uint64_t/* roomId */, std::unique_ptr<BattleRoom>> m_battleRooms{};
    
	std::atomic<uint64_t> m_nextRoomId = 1; // auto increment room ID
	std::mutex m_playerAddQueueMutex;       // lock for add player to queue
	std::mutex m_battleRoomsMutex;          // lock for battle rooms
};

#endif // BATTLE_MANAGER_H