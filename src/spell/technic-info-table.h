#pragma once

#include "realm/realm-types.h"
#include "system/angband.h"

constexpr auto NUM_TECHNIC = std::ssize(TECHNIC_REALM_RANGE);

/*
 * The "name" of spell 'N' is stored as spell_names[X][N],
 * where X is 0 for mage-spells and 1 for priest-spells.
 */
struct magic_type {
    PLAYER_LEVEL slevel; /* Required level (to learn) */
    MANA_POINT smana; /* Required mana (to cast) */
    PERCENTAGE sfail; /* Minimum chance of failure */
    EXP sexp; /* Encoded experience bonus */
};

extern const magic_type technic_info[NUM_TECHNIC][32];
