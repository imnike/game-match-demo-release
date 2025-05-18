// @file  : scheduleManager.cpp
// @brief : handles the scheduling of tasks in the game server
// @author: August
// @date  : 2025-05-16
#include "ScheduleManager.h"
#include "PlayerManager.h"

bool ScheduleManager::initialize()
{
	// register a task to save player data every 5 seconds
    registerTask(
        []()
        {
            PlayerManager::instance().saveDirtyPlayers();
            //std::cout << "[ScheduleManager] Player data save triggered.\n";
        },
        5
    );

    //scheduleTask(
    //    []()
    //    {
    //        std::cout << "[ScheduleManager] Game heartbeat triggered.\n";
    //    },
    //    10
    //);

    //scheduleTask(
    //    []()
    //    {
    //        std::cout << "[ScheduleManager] One-time startup check completed!\n";
    //    },
    //    3,
    //    false   // once
    //);

	// start the worker thread
    m_running = true;
    m_workerThread = std::thread(&ScheduleManager::workerLoop, this);

    std::cout << "[ScheduleManager] : initialized!" << std::endl;
    return true;
}

void ScheduleManager::release()
{
    if (m_running)
    {
        m_running = false;
        if (m_workerThread.joinable())
        {
			// wait for the worker thread to finish its work and exit
            m_workerThread.join();
            //std::cout << "[ScheduleManager] Worker thread joined.\n";
        }
    }
	// lock after thread finished
    std::lock_guard<std::mutex> lock(m_mutex);
    m_vecTasks.clear();
    std::cout << "[ScheduleManager] : released!" << std::endl;
}

// register a new task with a callback function and interval
void ScheduleManager::registerTask(std::function<void()> funcCallback, int intervalSeconds, bool m_isRepeating)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Converts the integer 'intervalSeconds' into a std::chrono::seconds duration object.
    // This provides type safety and clarity for time units.
    m_vecTasks.emplace_back(funcCallback, std::chrono::seconds(intervalSeconds), m_isRepeating);
}

ScheduleManager::ScheduleManager()
{
}

ScheduleManager::~ScheduleManager()
{
}

// handler for the worker thread
void ScheduleManager::workerLoop()
{
    //std::cout << "[ScheduleManager] Worker loop started.\n";
    while (m_running)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

		auto now = std::chrono::steady_clock::now();    // get current time point

		// loop through the tasks and check if they need
        for (auto it = m_vecTasks.begin(); it != m_vecTasks.end(); )
        {
			// check if the task is due (current time - last execution time >= task interval)
            if ((now - it->m_lastExecutionTime) >= it->m_interval)
            {
				it->m_funcCallback();   // execute the task callback function

				it->m_lastExecutionTime = now;  // update the last execution time

                if (!it->m_isRepeating)
                {
					// if it's a one-time task, remove it from the list
                    it = m_vecTasks.erase(it);
                }
                else
                {
					// if it's a repeating task, just move to the next task
                    ++it;
                }
            }
            else
            {
				// if the task is not due, just move to the next task
                ++it;
            }
        }
		std::this_thread::sleep_for(std::chrono::milliseconds(1));  // wait 1ms before checking again
    }
    //std::cout << "[ScheduleManager Thread] Worker loop stopped.\n";
}