#pragma once

#include <memory>

class BaseitemKey;
class ItemEntity;
class PlayerType;
char index_to_label(int i);
short wield_slot(PlayerType *player_ptr, const ItemEntity &item);
bool check_book_realm(PlayerType *player_ptr, const BaseitemKey &bi_key);
std::shared_ptr<ItemEntity> ref_item(PlayerType *player_ptr, short i_idx);
