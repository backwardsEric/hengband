﻿#pragma once

#include "object-enchant/abstract-protector-enchanter.h"
#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

struct object_type;
struct player_type;
class GlovesEnchanter : AbstractProtectorEnchanter {
public:
    GlovesEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
    GlovesEnchanter() = delete;
    virtual ~GlovesEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    player_type *owner_ptr;
};