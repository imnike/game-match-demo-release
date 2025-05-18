// main.cpp
#include <iostream>
#include <sstream> // �Ω�r��ѪR
#include <limits>  // �Ω� std::numeric_limits
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

std::atomic<bool> isRunning = true; // ����R�O�B�z��������B�檬�A

void commandThread();
void showPlayer(Player* pPlayer, bool isList);
void showTopPlayers(size_t count);
void listAllPlayers();
void listSpecPlayers(std::set<uint64_t> setPlayerIds);
void simulatePlayer(uint64_t playerId);
void simulateBatch(uint32_t counts);
void exitGame();

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "--- Game Match Demo Starting (Multithreaded Server) ---\n";

    // 1. ��l�ƩҦ��֤ߺ޲z��
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

	BattleManager::instance().startMatchmaking(); // �Ұʤǰt�����

    std::cout << "Game Server initialized. Main thread ready for commands.\n";
    std::cout << "Type 'exit' to shut down the demo.\n";

    std::thread cmdThread(commandThread);   // �ҰʥD���x�R�O�B�z�����

    // main �禡�{�b�|���� cmdThread �����C
    // �o�N���ۥu�n cmdThread �S�����Amain �禡�N�|�O�����D�A��ӵ{���]�|�B��C
    // ��A�b commandThread ����J "exit" �ɡAisRunning �|�ܬ� false�AcmdThread �����A
    // ���� main �禡�N�~�����M�z�N�X�C
    cmdThread.join();

    std::cout << "--- Game Match Demo Shutting Down ---\n";

    return 0;
}

void simulatePlayer(uint64_t playerId)
{
	// ���wid�����a�n�J�äǰt
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
        playerId = pPlayer->getId(); // ���] playerLogin �|��^���Ī����a ID
        // �N���a�[�J�ǰt���C
        BattleManager::instance().addPlayerToQueue(pPlayer);
        std::cout << " player " << playerId << " join matchQueue.\n";
    }
    else
    {
        std::cout << " player " << playerId << " is not in lobby.\n";
    }
    //std::cout << "Waiting 1 seconds for players to potentially match...\n";
    // ���ݤ@�q�ɶ��A�����a�����|�ǰt�M�԰�
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

// �������a�n�J�äǰt
void simulateBatch(uint32_t counts)
{
    std::cout << "--- Starting player simulation batch ---\n";

    std::vector<uint64_t> vecLoggedInIds;

    // ����Ҧ����aob
    auto pMapAllPlayers = PlayerManager::instance().getAllPlayers();

    std::set<uint64_t> tmpSetGotIds{}; // �Ω�s�x�w��������aID�A�קK���ƨϥ�

    // ��@�H�����o�@�Ӥ��b�u�����aid
    uint64_t maxId = static_cast<uint64_t>(pMapAllPlayers->size());
    for (uint32_t i = 0; i < counts; i++)
    {
        // ���o�@���H��(1~maxID)
        Player* pPlayer = nullptr;
        uint64_t playerId = random_utils::getRandom(maxId);
		auto itGot = tmpSetGotIds.find(playerId); // �ˬd�O�_�w�g����L�o��ID
        if (itGot != tmpSetGotIds.end())
        {
			// �p�G�w�g����L, ��0�|�Ыؤ@�ӷs�b��
            playerId = 0;
        }
        pPlayer = PlayerManager::instance().playerLogin(playerId);
        if (pPlayer)
        {
            if (pPlayer->isInLobby())
            {
                playerId = pPlayer->getId(); // ���] playerLogin �|��^���Ī����a ID
                vecLoggedInIds.emplace_back(playerId);
                // �N���a�[�J�ǰt���C
                BattleManager::instance().addPlayerToQueue(pPlayer);
				tmpSetGotIds.emplace(playerId); // �N���aID�[�J�w��������X
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
    // ���ݤ@�q�ɶ��A�����a�����|�ǰt�M�԰�
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // // �p�G�ݭn�A�i�H�ϳo�Ǫ��a�n�X
    // for (uint64_t id : vecLoggedInIds)
    // {
    //     if (PlayerManager::instance().playerLogout(id))
    //     {
    //         std::cout << "Player " << id << " logged out.\n";
    //     }
    //     else
    //     {
    //         std::cerr << "Error: Failed to logout player " << id << "\n";
    //     }
    // }

    //std::cout << "--- Player simulation batch finished ---\n";
}
// �R�O�B�z��������禡
void commandThread()
{
    std::cout << "\nCommand thread started. \nPlease type 'help' to see available commands.\n";

    std::string line_input; // �Ω�Ū������J
    while (isRunning)
    {
        // ***** ����ץ��G�����ε��P���o�Ǧ� *****
        // std::cin.clear(); // �����o��A�]�� getline ���|�]�m���~���A
        // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // �����o��A�]�� getline �w�g�����F�����
        // ***************************************

        std::cout << "> "; // ���ܥΤ��J
        std::getline(std::cin, line_input); // ���릡Ū���@���J�C���|Ū�����촫��šA�ç⴫��Ų����C

        // �ˬd�O�_Ū�����\�]�קK�b EOF �ο��~���A�U�~��B�z�^
        if (std::cin.fail() || std::cin.eof()) {
            std::cout << "Input stream error or end of file detected. Exiting command thread.\n";
            exitGame(); // �Ϊ̨�L�A�����~�B�z
            break;
        }

        std::istringstream iss(line_input);
        std::string command_name;
        iss >> command_name;

        if (command_name.empty()) {
            continue; // �B�z�Ŧ�
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
            // ��� TeamMatchQueue ������ map ���w
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
            // ��� BattleMatchQueue ������ map ���w
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
                try {
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
                catch (const std::invalid_argument&) {
                    std::cout << "Invalid player ID format: '" << arg << "'. Please enter a valid number.\n";
                }
                catch (const std::out_of_range&) {
                    std::cout << "Player ID '" << arg << "' is out of range.\n";
                }
            }
            else // Multiple IDs provided (e.g., "1,2,3,6")
            {
                std::stringstream ss_arg(arg);
                std::string segment;
                std::set<uint64_t> tmpSetPlayerIds{}; // �Ω�s�x���aID
                while (std::getline(ss_arg, segment, ','))
                {
                    if (segment.empty()) continue; // Skip empty segments in case of consecutive commas

                    try {
                        uint64_t playerId = std::stoull(segment);
						tmpSetPlayerIds.insert(playerId); // �N���aID�[�J���X
                    }
                    catch (const std::invalid_argument&) {
                        std::cout << "Invalid player ID format in list: '" << segment << "'. Please enter a valid number.\n";
                    }
                    catch (const std::out_of_range&) {
                        std::cout << "Player ID '" << segment << "' is out of range.\n";
                    }
                }
				listSpecPlayers(tmpSetPlayerIds); // �C�X�Ҧ����a��T
            }
        }
        /*
        else if (command_name == "query")
        {
            std::string arg;
            if (!(iss >> arg))
            {
                std::cout << "Usage: query <player_id>\n";
                continue;
            }

            try {
                uint64_t playerId = std::stoull(arg);
                uint32_t score = 0;
                uint32_t wins = 0;
                uint64_t updatedTime = 0;

                if (DbManager::instance().queryPlayerBattles(playerId, score, wins, updatedTime))
                {
                    std::cout << "\n----- Player Data (ID: " << playerId << ") -----\n";
                    std::cout << std::left << std::setw(10) << "Score"
                        << std::setw(10) << "Wins"
                        << std::setw(25) << "Updated Time" << "\n";
                    std::cout << "---------------------------------------------------\n";
                    std::cout << std::left << std::setw(10) << score
                        << std::setw(10) << wins
                        << std::setw(25) << time_utils::formatTimestampMs(updatedTime) << "\n";
                    std::cout << "---------------------------------------------------\n";
                }
                else
                {
                    std::cout << "Player ID " << arg << " not found or no data.\n";
                }
            }
            catch (const std::invalid_argument&) {
                std::cout << "Invalid player ID format: '" << arg << "'. Please enter a valid number.\n";
            }
            catch (const std::out_of_range&) {
                std::cout << "Player ID '" << arg << "' is out of range.\n";
            }
        }*/
        else if (command_name == "join")
        {
            std::string arg_string;
            if (!(iss >> arg_string))
            {
                std::cout << "Usage: join <id1>[,<id2>,...]\n";
                continue;
            }
            std::getline(iss, arg_string);

            // �I�I�I�ץ��G���������e��ťաA�M���ˬd�O�_���� �I�I�I
            // �����e�ɩM����ť�
            size_t first_char = arg_string.find_first_not_of(" \t");
            if (std::string::npos == first_char) { // �r��u�]�t�ťթά���
                arg_string.clear(); // �]���Ŧr��
            }
            else {
                size_t last_char = arg_string.find_last_not_of(" \t");
                arg_string = arg_string.substr(first_char, (last_char - first_char + 1));
            }

            // �{�b�ˬd�O�_���ѤF�Ѽơ]�B�z���ťի�^
            if (arg_string.empty())
            {
                std::cout << "Usage: join <id1>[,<id2>,...]\n";
				continue;
            }

            // �N�r���������Ů�A�H�K std::stringstream �i�H�ѪR�C�� ID
            // �`�N�G�o�����Ӧb�T�{ arg_string �����Ť���A����
            std::replace(arg_string.begin(), arg_string.end(), ',', ' ');

            std::stringstream id_stream(arg_string);
            std::string single_id_str;

            // �j��Ū���C�� ID
            while (id_stream >> single_id_str) {
                try {
                    uint64_t playerId = std::stoull(single_id_str);
                    // �o�̩I�s�A�� simulatePlayer �禡�ӳB�z�C�� ID
                    simulatePlayer(playerId);
                }
                catch (const std::invalid_argument&) {
                    std::cout << "Invalid player ID format: '" << single_id_str << "'. Please enter a valid number.\n";
                }
                catch (const std::out_of_range&) {
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
                try {
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
                catch (const std::invalid_argument&) {
                    std::cout << "Invalid count format: '" << arg << "'. Using default (1).\n";
                    count = 1;
                }
                catch (const std::out_of_range&) {
                    std::cout << "Count '" << arg << "' is out of range. Using default (1).\n";
                    count = 1;
                }
            }
            simulateBatch(count);
        }
        else if (command_name == "top") // �s�W top ���O�B�z
        {
            int count = 10; // �w�]��ܫe 10 �W
            std::string arg;
            if (iss >> arg)
            {
                try {
                    count = std::stoi(arg);
                    if (count <= 0) {
                        std::cout << "Count must be a positive number.\n";
                        continue;
                    }
                }
                catch (const std::invalid_argument&) {
                    std::cout << "Invalid count format: '" << arg << "'. Using default (10).\n";
                    count = 10;
                }
                catch (const std::out_of_range&) {
                    std::cout << "Count '" << arg << "' is out of range. Using default (10).\n";
                    count = 10;
                }
            }
            showTopPlayers(count); // �I�s�s�����
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
//            << std::setw(25) << "Updated Time" << "\n";
        std::cout << "---------------------------------------------------\n";
    }
    std::cout << std::left << std::setw(10) << pPlayer->getId()
        << std::setw(10) << pPlayer->getScore()
        << std::setw(10) << pPlayer->getTier()
        << std::setw(10) << pPlayer->getWins()
        << std::setw(15) << getStatusToString(pPlayer->getStatus()) << "\n";
//        << std::setw(25) << time_utils::formatTimestampMs(pPlayer->getUpdatedTime()) << "\n";
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
 //       << std::setw(25) << "Updated Time" << "\n";
    std::cout << "---------------------------------------------------\n";

    for (const auto& itPlayerId : setPlayerIds)
    {
        Player* pPlayer = PlayerManager::instance().getPlayer(itPlayerId);
        if (pPlayer)
        {
            showPlayer(pPlayer, true); // �ϥ� get() ���o��l����
        }
    }

    std::cout << "---------------------------------------------------\n";
}

// �C�X�Ҧ����a��T
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
//        << std::setw(25) << "Updated Time" << "\n";
    std::cout << "---------------------------------------------------\n";

    for (const auto& itPlayer : *pMapPlayers)
    {
        showPlayer(itPlayer.second.get(), true); // �ϥ� get() ���o��l����
    }

    std::cout << "---------------------------------------------------\n";
}
// �s�W�G��ܫe N �W���a�����
void showTopPlayers(size_t count)
{
    auto pMapPlayers = PlayerManager::instance().getAllPlayers();
    if (pMapPlayers->empty())
    {
        std::cout << "No players currently.\n";
        return;
    }
	const size_t maxSize = pMapPlayers->size();
    if (count > maxSize)
    {
		std::cout << "Count " << count << " > " << maxSize << " Showing all players.\n";
		count = maxSize; // ���� count ���W�L���a�ƶq
    }

    // �N�Ҧ����a���r���w��J�@�� vector
    std::vector<Player*> tmpVecPlayers;
    for (const auto& pair : *pMapPlayers)
    {
        tmpVecPlayers.push_back(pair.second.get());
    }

    // �i��Ƨ�
    // �ƧǳW�h�G
    // 1. �� Score ���� (�����u��)
    // 2. �p�G Score �ۦP�A�� Wins ���� (�����u��)
    // 3. �p�G Score �M Wins ���ۦP�A�� ID �ɧ� (�p���u���A�T�Oí�w��)
    std::sort(tmpVecPlayers.begin(), tmpVecPlayers.end(), [](const Player* a, const Player* b) {
        if (a->getScore() != b->getScore()) //
        {
            return a->getScore() > b->getScore(); // ���ư����u��
        }
        if (a->getWins() != b->getWins()) //
        {
            return a->getWins() > b->getWins(); // �ӳ������u��
        }
        return a->getId() < b->getId(); // ID �p���u�� (�ɧ�)
        });

    // ��ܫe count �W���a
    std::cout << "\n----- TOP " << count << " PLAYERS -----\n";
    std::cout << std::left << std::setw(5) << "Rank"
        << std::setw(10) << "ID"
        << std::setw(10) << "Score"
        << std::setw(10) << "Tier"
        << std::setw(10) << "Wins"
        << std::setw(15) << "Status" << "\n";
        //        << std::setw(25) << "Updated Time" << "\n";
    std::cout << "---------------------------------------------------\n";

    uint32_t currentRank = 1;
    for (const auto& pPlayer : tmpVecPlayers)
    {
        if (currentRank > count) // �u��ܫe count �W
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
//                << std::setw(25) << time_utils::formatTimestampMs(pPlayer->getUpdatedTime()) << "\n";
        }
    }

    std::cout << "---------------------------------------------------\n";
}

// �h�X�C���A����M�z�ާ@
void exitGame()
{
    isRunning = false; // �]�w�X�СA���� commandThread �����j��
    std::cout << "Exiting game...\n";

    // �T�O�Ҧ��޲z���� release() �禡�Q�I�s�A�ë��ӥ��T����������귽�C
    // �q�`�̿��L�ե󪺥�����A�Ϊ̽T�O������������ե���w������C
    // ScheduleManager.release() �|���ݨ䤺������������C
    BattleManager::instance().release();   // ����԰��޲z��
    PlayerManager::instance().release();   // ���񪱮a�޲z��
	ScheduleManager::instance().release(); // ����Ƶ{�޲z��
    DbManager::instance().release();       // �����Ʈw�޲z��

    // �o�̥i�H�K�[�����L�ݭn���M�z�N�X
}