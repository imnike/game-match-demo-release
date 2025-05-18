// playerManager.h
#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H
#include "../objects/player.h"
#include <unordered_map>
#include <set>
#include <mutex>
#include <cstdint>

class PlayerManager
{
public:

    static PlayerManager& instance();

    bool initialize();
    void release();
    Player* playerLogin(uint64_t id);
    bool playerLogout(uint64_t id);
    bool isPlayerOnline(uint64_t id);
    Player* getPlayer(uint64_t id);
    std::vector<Player*> getOnlinePlayers();
	std::unordered_map<uint64_t, std::unique_ptr<Player>>* getAllPlayers() { return &m_mapPlayers; }
    std::set<uint64_t>* getOnlinePlayerIds() { return &m_setOnlinePlayerIds; }
    void syncPlayerFromDbNoLock(uint64_t id, uint32_t score, uint32_t wins, uint64_t updatedTime);

    void handlePlayerBattleResult(uint64_t playerId, uint32_t scoreDelta, bool isWin);

    void enqueuePlayerSave(uint64_t playerId);
    void saveDirtyPlayers();

private:

    PlayerManager();
    ~PlayerManager();

    PlayerManager(const PlayerManager&) = delete;
    PlayerManager& operator=(const PlayerManager&) = delete;
    PlayerManager(PlayerManager&&) = delete;
    PlayerManager& operator=(PlayerManager&&) = delete;

	// Private methods without lock
    Player* _getPlayerNoLock(uint64_t id);
    void _setPlayerOnlineNoLock(uint64_t id, bool isOnline);
    void _syncPlayerNoLock(uint64_t id, uint32_t score, uint32_t wins, uint64_t updatedTime);

    std::unordered_map<uint64_t/* playerId */, std::unique_ptr<Player>> m_mapPlayers{};
	std::set<uint64_t/* playerId */> m_setOnlinePlayerIds{};
	std::mutex m_mapPlayersMutex;   // lock for m_mapPlayers

    std::set<uint64_t/* playerId */> m_setDirtyPlayerIds{};
	std::mutex m_setdirtyPlayerIdsMutex;    // lock for m_setDirtyPlayerIds
};

#endif // !PLAYER_MANAGER_H