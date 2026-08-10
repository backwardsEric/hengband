#pragma once

class ItemEntity;
class PlayerType;
bool activate_dragon_breath(PlayerType *player_ptr, const ItemEntity &item);
bool activate_breath_fire(PlayerType *player_ptr, const ItemEntity &item);
bool activate_breath_cold(PlayerType *player_ptr, const ItemEntity &item);
