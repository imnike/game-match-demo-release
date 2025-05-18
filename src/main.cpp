// main.cpp
#include <iostream>
#include <sstream> // 用於字串解析
#include <limits>  // 用於 std::numeric_limits
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

std::atomic<bool> isRunning = true; // 控制命令處理執行緒的運行狀態

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

    // 1. 初始化所有核心管理器
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

	BattleManager::instance().startMatchmaking(); // 啟動匹配執行緒

    std::cout << "Game Server initialized. Main thread ready for commands.\n";
    std::cout << "Type 'exit' to shut down the demo.\n";

    std::thread cmdThread(commandThread);   // 啟動主控台命令處理執行緒

    // main 函式現在會等待 cmdThread 結束。
    // 這意味著只要 cmdThread 沒結束，main 函式就會保持活躍，整個程式也會運行。
    // 當你在 commandThread 中輸入 "exit" 時，isRunning 會變為 false，cmdThread 結束，
    // 之後 main 函式將繼續執行清理代碼。
    cmdThread.join();

    std::cout << "--- Game Match Demo Shutting Down ---\n";

    return 0;
}

void simulatePlayer(uint64_t playerId)
{
	// 指定id的玩家登入並匹配
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
        playerId = pPlayer->getId(); // 假設 playerLogin 會返回有效的玩家 ID
        // 將玩家加入匹配隊列
        BattleManager::instance().addPlayerToQueue(pPlayer);
        std::cout << " player " << playerId << " join matchQueue.\n";
    }
    else
    {
        std::cout << " player " << playerId << " is not in lobby.\n";
    }
    //std::cout << "Waiting 1 seconds for players to potentially match...\n";
    // 等待一段時間，讓玩家有機會匹配和戰鬥
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

// 模擬玩家登入並匹配
void simulateBatch(uint32_t counts)
{
    std::cout << "--- Starting player simulation batch ---\n";

    std::vector<uint64_t> vecLoggedInIds;

    // 獲取所有玩家ob
    auto pMapAllPlayers = PlayerManager::instance().getAllPlayers();

    std::set<uint64_t> tmpSetGotIds{}; // 用於存儲已抓取的玩家ID，避免重複使用

    // 實作隨機取得一個不在線的玩家id
    uint64_t maxId = static_cast<uint64_t>(pMapAllPlayers->size());
    for (uint32_t i = 0; i < counts; i++)
    {
        // 取得一個隨機(1~maxID)
        Player* pPlayer = nullptr;
        uint64_t playerId = random_utils::getRandom(maxId);
		auto itGot = tmpSetGotIds.find(playerId); // 檢查是否已經抓取過這個ID
        if (itGot != tmpSetGotIds.end())
        {
			// 如果已經抓取過, 用0會創建一個新帳號
            playerId = 0;
        }
        pPlayer = PlayerManager::instance().playerLogin(playerId);
        if (pPlayer)
        {
            if (pPlayer->isInLobby())
            {
                playerId = pPlayer->getId(); // 假設 playerLogin 會返回有效的玩家 ID
                vecLoggedInIds.emplace_back(playerId);
                // 將玩家加入匹配隊列
                BattleManager::instance().addPlayerToQueue(pPlayer);
				tmpSetGotIds.emplace(playerId); // 將玩家ID加入已抓取的集合
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
    // 等待一段時間，讓玩家有機會匹配和戰鬥
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // // 如果需要，可以使這些玩家登出
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
// 命令處理執行緒的函式
void commandThread()
{
    std::cout << "\nCommand thread started. \nPlease type 'help' to see available commands.\n";

    std::string line_input; // 用於讀取整行輸入
    while (isRunning)
    {
        // ***** 關鍵修正：移除或註銷掉這些行 *****
        // std::cin.clear(); // 移除這行，因為 getline 不會設置錯誤狀態
        // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 移除這行，因為 getline 已經移除了換行符
        // ***************************************

        std::cout << "> "; // 提示用戶輸入
        std::getline(std::cin, line_input); // 阻塞式讀取一行輸入。它會讀取直到換行符，並把換行符移除。

        // 檢查是否讀取成功（避免在 EOF 或錯誤狀態下繼續處理）
        if (std::cin.fail() || std::cin.eof()) {
            std::cout << "Input stream error or end of file detected. Exiting command thread.\n";
            exitGame(); // 或者其他適當的錯誤處理
            break;
        }

        std::istringstream iss(line_input);
        std::string command_name;
        iss >> command_name;

        if (command_name.empty()) {
            continue; // 處理空行
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
            // 獲取 BattleMatchQueue 的內部 map 指針
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
                std::set<uint64_t> tmpSetPlayerIds{}; // 用於存儲玩家ID
                while (std::getline(ss_arg, segment, ','))
                {
                    if (segment.empty()) continue; // Skip empty segments in case of consecutive commas

                    try {
                        uint64_t playerId = std::stoull(segment);
						tmpSetPlayerIds.insert(playerId); // 將玩家ID加入集合
                    }
                    catch (const std::invalid_argument&) {
                        std::cout << "Invalid player ID format in list: '" << segment << "'. Please enter a valid number.\n";
                    }
                    catch (const std::out_of_range&) {
                        std::cout << "Player ID '" << segment << "' is out of range.\n";
                    }
                }
				listSpecPlayers(tmpSetPlayerIds); // 列出所有玩家資訊
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

            // ！！！修正：首先移除前後空白，然後檢查是否為空 ！！！
            // 移除前導和後續空白
            size_t first_char = arg_string.find_first_not_of(" \t");
            if (std::string::npos == first_char) { // 字串只包含空白或為空
                arg_string.clear(); // 設為空字串
            }
            else {
                size_t last_char = arg_string.find_last_not_of(" \t");
                arg_string = arg_string.substr(first_char, (last_char - first_char + 1));
            }

            // 現在檢查是否提供了參數（處理完空白後）
            if (arg_string.empty())
            {
                std::cout << "Usage: join <id1>[,<id2>,...]\n";
				continue;
            }

            // 將逗號替換為空格，以便 std::stringstream 可以解析每個 ID
            // 注意：這裡應該在確認 arg_string 不為空之後再執行
            std::replace(arg_string.begin(), arg_string.end(), ',', ' ');

            std::stringstream id_stream(arg_string);
            std::string single_id_str;

            // 迴圈讀取每個 ID
            while (id_stream >> single_id_str) {
                try {
                    uint64_t playerId = std::stoull(single_id_str);
                    // 這裡呼叫你的 simulatePlayer 函式來處理每個 ID
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
        else if (command_name == "top") // 新增 top 指令處理
        {
            int count = 10; // 預設顯示前 10 名
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
            showTopPlayers(count); // 呼叫新的函數
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
            showPlayer(pPlayer, true); // 使用 get() 取得原始指標
        }
    }

    std::cout << "---------------------------------------------------\n";
}

// 列出所有玩家資訊
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
        showPlayer(itPlayer.second.get(), true); // 使用 get() 取得原始指標
    }

    std::cout << "---------------------------------------------------\n";
}
// 新增：顯示前 N 名玩家的函數
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
		count = maxSize; // 限制 count 不超過玩家數量
    }

    // 將所有玩家的裸指針放入一個 vector
    std::vector<Player*> tmpVecPlayers;
    for (const auto& pair : *pMapPlayers)
    {
        tmpVecPlayers.push_back(pair.second.get());
    }

    // 進行排序
    // 排序規則：
    // 1. 按 Score 降序 (高的優先)
    // 2. 如果 Score 相同，按 Wins 降序 (高的優先)
    // 3. 如果 Score 和 Wins 都相同，按 ID 升序 (小的優先，確保穩定性)
    std::sort(tmpVecPlayers.begin(), tmpVecPlayers.end(), [](const Player* a, const Player* b) {
        if (a->getScore() != b->getScore()) //
        {
            return a->getScore() > b->getScore(); // 分數高的優先
        }
        if (a->getWins() != b->getWins()) //
        {
            return a->getWins() > b->getWins(); // 勝場高的優先
        }
        return a->getId() < b->getId(); // ID 小的優先 (升序)
        });

    // 顯示前 count 名玩家
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
        if (currentRank > count) // 只顯示前 count 名
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

// 退出遊戲，執行清理操作
void exitGame()
{
    isRunning = false; // 設定旗標，指示 commandThread 結束迴圈
    std::cout << "Exiting game...\n";

    // 確保所有管理器的 release() 函式被呼叫，並按照正確的順序釋放資源。
    // 通常依賴其他組件的先釋放，或者確保執行緒相關的組件先安全停止。
    // ScheduleManager.release() 會等待其內部執行緒結束。
    BattleManager::instance().release();   // 釋放戰鬥管理器
    PlayerManager::instance().release();   // 釋放玩家管理器
	ScheduleManager::instance().release(); // 釋放排程管理器
    DbManager::instance().release();       // 釋放資料庫管理器

    // 這裡可以添加任何其他需要的清理代碼
}