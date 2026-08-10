#pragma once

#include <map>
#include <string_view>

enum class MonsterAbilityType;

/* Imitator */
struct monster_power {
    short level;
    int smana;
    int fail;
    int manedam;
    int manefail;
    int use_stat;
    std::string_view name;
};

#define MAX_MONSPELLS 96

extern const std::map<MonsterAbilityType, const monster_power> monster_powers;
