// @file  : hero.cpp
// @brief : hero object in battle
// @author: August
// @date  : 2025-05-17
#include "Hero.h"
#include "Player.h"
#include <iostream>
#include <algorithm>

Hero::Hero(const uint64_t playerId)
    : m_playerId(playerId),
    m_id(1),
    m_hp(100), m_maxHp(100),
    m_mp(50), m_maxMp(50),
    m_atk(10),
    m_def(5),
    m_spd(5),
    m_lv(1),
    m_exp(0)
{
    m_hp = m_maxHp;
    m_mp = m_maxMp;
}

Hero::~Hero() 
{
}