// @file: Hero.h
#ifndef HERO_H
#define HERO_H

#include <cstdint>

class Hero
{
public:
    uint64_t m_playerId;    // playerId
    uint32_t m_id;          // heroId
	uint32_t m_hp;          // health points
	uint32_t m_maxHp;       // max health points
	uint32_t m_mp;          // mana points
	uint32_t m_maxMp;       // max mana points
	uint16_t m_atk;         // attack power
	uint16_t m_def;         // defense power
	uint16_t m_spd;         // speed
	uint8_t m_lv;           // level
	uint32_t m_exp;         // experience points

    Hero(const uint64_t playerId);
    ~Hero();

    //void heal(uint32_t hp);
    //void gainExp(uint32_t exp);
    //void levelUp();

    uint64_t getPlayerId() const { return m_playerId; }
    uint32_t getId() const { return m_id; }

private:

};

#endif // HERO_H