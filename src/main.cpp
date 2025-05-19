// main.cpp
#include <iostream>
#include <sstream>
#include <limits>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <string>
#include <atomic>

#include "./managers/battleManager.h"
#include "./managers/playerManager.h"
#include "./managers/scheduleManager.h"
#include "./managers/dbManager.h"
#include "../utils/utils.h"

std::atomic<bool> isRunning = true;

void commandThread();
// display player status
void showPlayer(Player* pPlayer, bool isList);
// display top players
void showTopPlayers(size_t counts);
// display player list
void listAllPlayers();
// display specific player(s) by ID(s)
void listSpecPlayers(std::set<uint64_t> setPlayerIds);
// simulate player matchmaking by ID
void simulatePlayer(uint64_t playerId);
// simulate a batch of players
void simulateBatch(uint32_t counts);
void exitGame();

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "--- Game Match Demo Starting (Multithreaded Server) ---\n";

	// initialize managers
    if (!DbManager::instance().initialize())
    {
        std::cerr << "Error: Failed to initialize DbManager!\n";
        return 1;
    }

    if (!PlayerManager::instance().initialize())
    {
        std::cerr << "Error: Failed to initialize PlayerManager!\n";
        return 1;
    }

    if (!BattleManager::instance().initialize())
    {
        std::cerr << "Error: Failed to initialize BattleManager!\n";
        return 1;
    }

    if (!ScheduleManager::instance().initialize())
    {
        std::cerr << "Error: Failed to initialize ScheduleManager!\n";
        return 1;
    }

	// connect to database
    if (DbManager::instance().connect() == false)
    {
        std::cerr << "Error: Failed to connect to database!\n";
		return 1;
    }
	// make sure the database schema is correct
    if (DbManager::instance().ensureTableSchema() == false)
    {
        std::cerr << "Error: Failed to ensure database schema!\n";
		return 1;
    }
	
    DbManager::instance().loadTableData();

	BattleManager::instance().startMatchmaking();   // startup matchmaking thread

    std::cout << "Game Server initialized. Main thread ready for commands.\n";
    std::cout << "Type 'exit' to shut down the demo.\n";

	std::thread cmdThread(commandThread);   // startup command thread

    cmdThread.join();

    std::cout << "--- Game Match Demo Shutting Down ---\n";

    return 0;
}

// simulate player matchmaking by ID
void simulatePlayer(uint64_t playerId)
{
    Player* pPlayer = nullptr;
    pPlayer = PlayerManager::instance().playerLogin(playerId);
    if (pPlayer == nullptr)
    {
        std::cout << " player " << playerId << " not exists.\n";
        return;
    }
    std::cout << "--- Starting player " << playerId << " to simulation match --- \n";
    if (pPlayer->isInLobby())
    {
        playerId = pPlayer->getId();
        BattleManager::instance().addPlayerToQueue(pPlayer);
        std::cout << " player " << playerId << " join matchQueue.\n";
    }
    else
    {
        std::cout << " player " << playerId << " is not in lobby.\n";
    }
    //std::cout << "Waiting 1 seconds for players to potentially match...\n";
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

// simulate a batch of players
void simulateBatch(uint32_t counts)
{
    std::cout << "--- Starting player simulation batch ---\n";

    std::vector<uint64_t> vecLoggedInIds;

    auto pMapAllPlayers = PlayerManager::instance().getAllPlayers();

	std::set<uint64_t> tmpSetGotIds{};  // avoid duplicate player IDs

    uint64_t maxId = static_cast<uint64_t>(pMapAllPlayers->size());
    for (uint32_t i = 0; i < counts; i++)
    {
        Player* pPlayer = nullptr;
		// get a random player ID
        uint64_t playerId = random_utils::getRandom(maxId);
		auto itGot = tmpSetGotIds.find(playerId);
        if (itGot != tmpSetGotIds.end())
        {
			// if the player ID already exists, get a new one by id = 0
            playerId = 0;
        }
        pPlayer = PlayerManager::instance().playerLogin(playerId);
        if (pPlayer)
        {
            if (pPlayer->isInLobby())
            {
                playerId = pPlayer->getId();
                vecLoggedInIds.emplace_back(playerId);
                BattleManager::instance().addPlayerToQueue(pPlayer);
				tmpSetGotIds.emplace(playerId);
                std::cout << " player " << playerId << " join matchQueue.\n";
            }
            else
            {
                std::cout << " player " << playerId << " is not in lobby.\n";
            }
        }
        else
        {
            std::cerr << "Error: Failed to get player " << playerId << "\n";
        }
    }

    //std::cout << "Waiting 1 seconds for players to potentially match...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void commandThread()
{
    std::cout << "\nCommand thread started. \nPlease type 'help' to see available commands.\n";

    std::string line_input;
    while (isRunning)
    {
        std::cout << "> ";
        std::getline(std::cin, line_input);

        if (std::cin.fail() || std::cin.eof())
        {
            std::cout << "Input stream error or end of file detected. Exiting command thread.\n";
            exitGame();
            break;
        }

        std::istringstream iss(line_input);
        std::string command_name;
        iss >> command_name;

        if (command_name.empty())
        {
            continue;
        }

        std::transform(command_name.begin(), command_name.end(), command_name.begin(), ::tolower);

        if (command_name == "help")
        {
            std::cout << "--- Available Commands ---\n";
            std::cout << "  help           : Display this help message.\n";
            std::cout << "  batch <count>  : Simulate player logins and add them to the matchmaking queue. 'count' is optional (default: 1).\n";
			std::cout << "  join <id1>[,<id2>,...] : Simulate specific player(s) by ID(s) joining the matchmaking queue. Use '0' for a new player.\n";
            std::cout << "  queue          : Display the current status of the team matchmaking queue and battle matchmaking queue.\n";
            std::cout << "  top <count>    : Display top players sorted list. 'count' is optional (default: 10).\n";
            std::cout << "  list           : Display all players.\n";
            std::cout << "  show <id1>[,<id2>,...] : Display specific player(s) by their ID(s).\n";
            std::cout << "  exit           : Shut down the game demo.\n";
            std::cout << "--------------------------\n";
        }
        else if (command_name == "list")
        {
            listAllPlayers();
        }
        else if (command_name == "queue")
        {
            auto pTeamTierQueues = BattleManager::instance().getTeamMatchQueue();
            std::cout << "\n--- Team Match Queue  ---" << std::endl;
            // 獲取 TeamMatchQueue 的內部 map 指針
            if (pTeamTierQueues)
            {
				const auto tmpMapQueue = pTeamTierQueues->getTierQueue();
                if (tmpMapQueue.empty())
                {
                    std::cout << "  (Team Match Queue no players)\n";
                }
                else 
                {
                    for (const auto& pair : tmpMapQueue)
                    {
                        uint32_t tier = pair.first;
                        const std::vector<Player*>& playersInTier = pair.second;
                        std::cout << "  Tier " << tier << " (players: " << playersInTier.size() << "): ";
                        for (const auto& pPlayer : playersInTier) 
                        {
                            if (pPlayer)
                            {
                                std::cout << pPlayer->getId() << " ";
                            }
                        }
                        std::cout << std::endl;
                    }
                }
            }
            else 
            {
                std::cout << "  (Team Match Queue no players)\n";
            }

            std::cout << "\n--- Battle Match Queue  ---" << std::endl;

            auto pBattleTierQueues = BattleManager::instance().getBattleMatchQueue();
            if (pBattleTierQueues)
            {
                const auto tmpMapQueue = pBattleTierQueues->getTierQueue();
                if (tmpMapQueue.empty())
                {
                    std::cout << "  (Battle Match Queue no teams)\n";
                }
                else 
                {
                    for (const auto& pair : tmpMapQueue) 
                    {
                        uint32_t tier = pair.first;
                        const std::vector<std::vector<Player*>>& teamsInTier = pair.second;
                        std::cout << "  Tier " << tier << " (teams: " << teamsInTier.size() << "): ";
                        for (const auto& team : teamsInTier) 
                        {
                            std::cout << "[";
                            for (const auto& pPlayer : team) 
                            {
                                if (pPlayer) 
                                {
                                    std::cout << pPlayer->getId() << " ";
                                }
                            }
                            std::cout << "] ";
                        }
                        std::cout << std::endl;
                    }
                }
            }
            else 
            {
                std::cout << "  (Battle Match Queue no teams)\n";
            }
        }
        else if (command_name == "show")
        {
            std::string arg;
            if (!(iss >> arg))
            {
                std::cout << "Usage: show <id> [,<id>,...]\n"; // Updated usage message
                continue;
            }

            // Check if the argument contains commas, indicating multiple IDs
            size_t first_comma = arg.find(',');
            if (first_comma == std::string::npos) // Single ID provided
            {
                try
                {
                    uint64_t playerId = std::stoull(arg);
                    Player* pPlayer = PlayerManager::instance().getPlayer(playerId);
                    if (pPlayer)
                    {
                        showPlayer(pPlayer, false); // Display player information
                    }
                    else
                    {
                        std::cout << "Player ID " << arg << " not found.\n";
                    }
                }
                catch (const std::invalid_argument&) 
                {
                    std::cout << "Invalid player ID format: '" << arg << "'. Please enter a valid number.\n";
                }
                catch (const std::out_of_range&) 
                {
                    std::cout << "Player ID '" << arg << "' is out of range.\n";
                }
            }
            else // Multiple IDs provided (e.g., "1,2,3,6")
            {
                std::stringstream ss_arg(arg);
                std::string segment;
                std::set<uint64_t> tmpSetPlayerIds{};
                while (std::getline(ss_arg, segment, ','))
                {
                    if (segment.empty()) continue; // Skip empty segments in case of consecutive commas

                    try 
                    {
                        uint64_t playerId = std::stoull(segment);
						tmpSetPlayerIds.insert(playerId);
                    }
                    catch (const std::invalid_argument&)
                    {
                        std::cout << "Invalid player ID format in list: '" << segment << "'. Please enter a valid number.\n";
                    }
                    catch (const std::out_of_range&)
                    {
                        std::cout << "Player ID '" << segment << "' is out of range.\n";
                    }
                }
				listSpecPlayers(tmpSetPlayerIds);
            }
        }
        else if (command_name == "join")
        {
            std::string arg_string;
            if (!(iss >> arg_string))
            {
                std::cout << "Usage: join <id1>[,<id2>,...]\n";
                continue;
            }
            std::getline(iss, arg_string);

            size_t first_char = arg_string.find_first_not_of(" \t");
            if (std::string::npos == first_char)
            {
                arg_string.clear();
            }
            else {
                size_t last_char = arg_string.find_last_not_of(" \t");
                arg_string = arg_string.substr(first_char, (last_char - first_char + 1));
            }

            if (arg_string.empty())
            {
                std::cout << "Usage: join <id1>[,<id2>,...]\n";
				continue;
            }

            std::replace(arg_string.begin(), arg_string.end(), ',', ' ');

            std::stringstream id_stream(arg_string);
            std::string single_id_str;

            while (id_stream >> single_id_str)
            {
                try 
                {
                    uint64_t playerId = std::stoull(single_id_str);
                    simulatePlayer(playerId);
                }
                catch (const std::invalid_argument&)
                {
                    std::cout << "Invalid player ID format: '" << single_id_str << "'. Please enter a valid number.\n";
                }
                catch (const std::out_of_range&)
                {
                    std::cout << "Player ID '" << single_id_str << "' is out of range.\n";
                }
            }
        }
        else if (command_name == "batch")
        {
            int count = 1;
            std::string arg;
            if (iss >> arg)
            {
                try
                {
                    count = std::stoi(arg);
                    if (count <= 0)
                    {
                        std::cout << "Count must be a positive number.\n";
                        continue;
                    }
                    else if (count > 100)
                    {
						std::cout << "Please enter a number between 1 and 100.\n";
                        continue;
					}
                }
                catch (const std::invalid_argument&)
                {
                    std::cout << "Invalid count format: '" << arg << "'. Using default (1).\n";
                    count = 1;
                }
                catch (const std::out_of_range&)
                {
                    std::cout << "Count '" << arg << "' is out of range. Using default (1).\n";
                    count = 1;
                }
            }
            simulateBatch(count);
        }
        else if (command_name == "top")
        {
			int count = 10; // default
            std::string arg;
            if (iss >> arg)
            {
                try
                {
                    count = std::stoi(arg);
                    if (count <= 0)
                    {
                        std::cout << "Count must be a positive number.\n";
                        continue;
                    }
                }
                catch (const std::invalid_argument&)
                {
                    std::cout << "Invalid count format: '" << arg << "'. Using default (10).\n";
                    count = 10;
                }
                catch (const std::out_of_range&)
                {
                    std::cout << "Count '" << arg << "' is out of range. Using default (10).\n";
                    count = 10;
                }
            }
            showTopPlayers(count);
        }
        else if (command_name == "exit")
        {
            exitGame();
        }
        else
        {
            std::cout << "Unknown command '" << line_input << "'. Please type 'help' to see available commands.\n";
        }
    }

    std::cout << "Command thread ended.\n";
}

static const std::string getStatusToString(common::PlayerStatus status)
 {
    switch (status) 
    {
    case common::PlayerStatus::offline:
        return "offline";
        break;
    case common::PlayerStatus::lobby:
        return "Lobby";
        break;
    case common::PlayerStatus::queue:
        return "Matching";
        break;
    case common::PlayerStatus::battle:
        return "Fighting";
        break;
    default:
        break;
    }
    return "unknown";
}

void showPlayer(Player* pPlayer, bool isList)
{
    if (!pPlayer)
    {
        std::cout << "Usage: show <id>\n";
		return;
    }

    if (isList == false)
    {
        std::cout << "\n----- PLAYERS LIST -----\n";
        std::cout << std::left << std::setw(10) << "ID"
            << std::setw(10) << "Score"
            << std::setw(10) << "Tier"
            << std::setw(10) << "Wins"
            << std::setw(15) << "Status" << "\n";
        std::cout << "---------------------------------------------------\n";
    }
    std::cout << std::left << std::setw(10) << pPlayer->getId()
        << std::setw(10) << pPlayer->getScore()
        << std::setw(10) << pPlayer->getTier()
        << std::setw(10) << pPlayer->getWins()
        << std::setw(15) << getStatusToString(pPlayer->getStatus()) << "\n";
    if (isList == false)
    {
        std::cout << "---------------------------------------------------\n";
    }
}

void listSpecPlayers(std::set<uint64_t> setPlayerIds)
{
    if (setPlayerIds.empty())
    {
        std::cout << "No players currently.\n";
		return;
    }
    std::cout << "\n----- PLAYERS LIST -----\n";
    std::cout << std::left << std::setw(10) << "ID"
        << std::setw(10) << "Score"
        << std::setw(10) << "Tier"
        << std::setw(10) << "Wins"
        << std::setw(15) << "Status" << "\n";
    std::cout << "---------------------------------------------------\n";

    for (const auto& itPlayerId : setPlayerIds)
    {
        Player* pPlayer = PlayerManager::instance().getPlayer(itPlayerId);
        if (pPlayer)
        {
            showPlayer(pPlayer, true);
        }
    }

    std::cout << "---------------------------------------------------\n";
}

void listAllPlayers()
{
    auto pMapPlayers = PlayerManager::instance().getAllPlayers();
    if (pMapPlayers->empty())
    {
        std::cout << "No players currently.\n";
        return;
    }
    std::cout << "\n----- PLAYERS LIST -----\n";
    std::cout << std::left << std::setw(10) << "ID"
        << std::setw(10) << "Score"
        << std::setw(10) << "Tier"
        << std::setw(10) << "Wins"
        << std::setw(15) << "Status" << "\n";
    std::cout << "---------------------------------------------------\n";

    for (const auto& itPlayer : *pMapPlayers)
    {
        showPlayer(itPlayer.second.get(), true);
    }

    std::cout << "---------------------------------------------------\n";
}

// show top players count
void showTopPlayers(size_t counts)
{
    auto pMapPlayers = PlayerManager::instance().getAllPlayers();
    if (pMapPlayers->empty())
    {
        std::cout << "No players currently.\n";
        return;
    }
	const size_t maxSize = pMapPlayers->size();
    if (counts > maxSize)
    {
		// if count is greater than the number of players, set it to maxSize
		std::cout << "Count " << counts << " > " << maxSize << " Showing all players.\n";
		counts = maxSize;
    }

    std::vector<Player*> tmpVecPlayers;
    for (const auto& pair : *pMapPlayers)
    {
        tmpVecPlayers.push_back(pair.second.get());
    }

	// sorting rules:
	// 1. score (descending)
	// 2. wins (descending)
	// 3. id (ascending)
    std::sort(tmpVecPlayers.begin(), tmpVecPlayers.end(), [](const Player* a, const Player* b) {
        if (a->getScore() != b->getScore())
        {
            return a->getScore() > b->getScore();
        }
        if (a->getWins() != b->getWins())
        {
            return a->getWins() > b->getWins();
        }
        return a->getId() < b->getId();
        });

    std::cout << "\n----- TOP " << counts << " PLAYERS -----\n";
    std::cout << std::left << std::setw(5) << "Rank"
        << std::setw(10) << "ID"
        << std::setw(10) << "Score"
        << std::setw(10) << "Tier"
        << std::setw(10) << "Wins"
        << std::setw(15) << "Status" << "\n";
    std::cout << "---------------------------------------------------\n";

    uint32_t currentRank = 1;
    for (const auto& pPlayer : tmpVecPlayers)
    {
        if (currentRank > counts)
        {
            break;
        }
        if (pPlayer)
        {
            std::cout << std::left << std::setw(5) << currentRank++
                << std::setw(10) << pPlayer->getId()
                << std::setw(10) << pPlayer->getScore()
                << std::setw(10) << pPlayer->getTier()
                << std::setw(10) << pPlayer->getWins()
                << std::setw(15) << getStatusToString(pPlayer->getStatus()) << "\n";
        }
    }

    std::cout << "---------------------------------------------------\n";
}

// exit game and clean up resources
void exitGame()
{
    isRunning = false;
    std::cout << "Exiting game...\n";

	// release managers
    BattleManager::instance().release();
    PlayerManager::instance().release();
	ScheduleManager::instance().release();
    DbManager::instance().release();

	// --- add any other necessary cleanup code here ---

	std::exit(0); // Exit the program with success status

}