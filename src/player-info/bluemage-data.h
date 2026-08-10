#pragma once

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"
#include <array>
#include <string>
#include <vector>

enum class BlueMagicType {
    BOLT = 1,
    BALL = 2,
    BREATH = 3,
    SUMMON = 4,
    OTHER = 5,
};

inline constexpr std::array<BlueMagicType, 5> BLUE_MAGIC_TYPE_LIST = { {
    BlueMagicType::BOLT,
    BlueMagicType::BALL,
    BlueMagicType::BREATH,
    BlueMagicType::SUMMON,
    BlueMagicType::OTHER,
} };

class BluemageData {
public:
    EnumClassFlagGroup<MonsterAbilityType> learnt_blue_magics{};
    bool new_magic_learned = false;
    std::vector<std::string> build_learnt_magics_info() const;
};
