// utils.h
#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <cstdint>
#include <random>
#include <limits>

namespace time_utils
{
    uint64_t getTimestampMS();
    uint64_t getTimestamp();
    std::string formatTimestampMs(uint64_t timestamp);
}
namespace random_utils
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

	// get random number in range [min, max]
    template <typename T>
    T getRandomRange(T min, T max)
    {
        if (min > max)
        {
            throw std::invalid_argument("random_utils::getRandomRange: min cannot be greater than max.");
        }

		// fixed for the issue of using uint8_t as range
        std::uniform_int_distribution<int> distrib(static_cast<int>(min), static_cast<int>(max));

        return static_cast<T>(distrib(gen));
    }

	// get random number in range [0, max - 1)
    template <typename T>
    T getRandom(T max)
    {
        if (max <= 0)
        {
            return 0;
        }
        return getRandomRange(static_cast<T>(0), static_cast<T>(max - 1));
    }
}
#endif // TIME_UTILS_H