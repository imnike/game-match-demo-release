// globalDefine.h
#ifndef GLOBAL_DEFINE_H
#define GLOBAL_DEFINE_H

#include <cstdint> 

namespace common
{
    enum PlayerStatus : uint8_t
    {
        offline,
        lobby,
        queue,
        battle
    };
}

namespace battle
{
    const uint32_t WINNER_SCORE = 50;
    const uint32_t LOSER_SCORE = 50;

    enum TeamMembers : uint8_t
    {
        TeamLeader = 0,
        TeamMember1 = 1,
        TeamMember2 = 2,
        TeamMemberMax
	};

    enum TeamColor : uint8_t
    {
        TeamColorRed = 0,    // team_0
        TeamColorBlue = 1,   // team_1
        TeamColorMax
    };
}

#endif // GLOBAL_DEFINE_H