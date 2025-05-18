// @file  : utils.cpp
// @brief : tools
// @author: August
// @date  : 2025-05-15
#include "utils.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace time_utils
{
    uint64_t getTimestampMS()
    {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return static_cast<uint64_t>(duration.count());
    }

    uint64_t getTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
		return static_cast<uint64_t>(duration.count());
    }
    std::string formatTimestampMs(uint64_t timestamp_ms)
    {
		// convert milliseconds to seconds
        std::time_t timeT_in_seconds = static_cast<std::time_t>(timestamp_ms / 1000);

        std::tm local_tm_struct;

		// get the local time from the time_t value
        if (localtime_s(&local_tm_struct, &timeT_in_seconds) != 0)
        {
            return "Invalid Time (conversion failed)";
        }

        std::ostringstream oss;
		// format the time as "YYYY-MM-DD HH:MM:SS"
        oss << std::put_time(&local_tm_struct, "%Y-%m-%d %H:%M:%S");

		// add milliseconds
        int remaining_ms = timestamp_ms % 1000;
        oss << "." << std::setw(3) << std::setfill('0') << remaining_ms;

        return oss.str();
    }
}
