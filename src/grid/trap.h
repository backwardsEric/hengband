#pragma once

#include "system/enums/terrain/trap.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <vector>

extern const std::vector<EnumClassFlagGroup<ChestTrapType>> chest_traps;

class FloorType;
class PlayerType;
void disclose_grid(PlayerType *player_ptr, const Pos2D &pos);
void hit_trap(PlayerType *player_ptr, bool break_trap);
