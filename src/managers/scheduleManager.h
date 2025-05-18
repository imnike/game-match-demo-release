// scheduleManager.h
#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include <vector>
#include <functional>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>

struct ScheduledTask
{
	std::function<void()> m_funcCallback;   // callback function to be executed
	std::chrono::seconds m_interval;        // interval (seconds), use std::chrono::seconds for better readability
	std::chrono::steady_clock::time_point m_lastExecutionTime;  // last execution time
	bool m_isRepeating;                     // is the task repeating

    ScheduledTask(std::function<void()> cb, std::chrono::seconds iv, bool repeat = true)
        : m_funcCallback(std::move(cb)), m_interval(iv), m_lastExecutionTime(std::chrono::steady_clock::now()), m_isRepeating(repeat)
    {
    }
};

class ScheduleManager
{
public:
    static ScheduleManager& instance()
    {
        static ScheduleManager instance;
        return instance;
    }

    bool initialize();

    void release();

    // register a new task
	// funcCallback: function to be called when the task is executed
	// intervalSeconds: execution interval in seconds
	// isRepeating: whether the task is repeating (default is true)
    void registerTask(std::function<void()> funcCallback, int intervalSeconds, bool m_isRepeating = true);

private:
    ScheduleManager();
    ~ScheduleManager();

    ScheduleManager(const ScheduleManager&) = delete;
    ScheduleManager& operator=(const ScheduleManager&) = delete;
    ScheduleManager(ScheduleManager&&) = delete;
    ScheduleManager& operator=(ScheduleManager&&) = delete;

    std::vector<ScheduledTask> m_vecTasks{};
	std::mutex m_mutex;             // lock for thread safety

	std::thread m_workerThread;     // thread for task scheduling
	std::atomic<bool> m_running;    // thread control flag

    void workerLoop();
};

#endif // SCHEDULE_MANAGER_H