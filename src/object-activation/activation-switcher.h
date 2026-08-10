#pragma once

#include <memory>
#include <string_view>
#include <utility>

enum class RandomArtActType : short;
class ItemEntity;
class PlayerType;
std::pair<bool, std::shared_ptr<ItemEntity>> switch_activation(PlayerType *player_ptr, ItemEntity &item, const RandomArtActType index, std::string_view name);
