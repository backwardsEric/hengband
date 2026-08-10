#pragma once

#include "util/point-2d.h"
#include <memory>

class ItemEntity;
class PlayerType;
std::shared_ptr<ItemEntity> choose_warning_item(PlayerType *player_ptr);
bool process_warning(PlayerType *player_ptr, const Pos2D &pos);
