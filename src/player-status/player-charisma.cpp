﻿#include "player-status/player-charisma.h"
#include "mutation/mutation-flag-types.h"
#include "object/object-flags.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "util/bit-flags-calculator.h"

void PlayerCharisma::set_locals()
{
    this->max_value = +99;
    this->min_value = -99;
    this->status_type = A_CHR;
    this->tr_flag = TR_CHR;
    this->tr_bad_flag = TR_CHR;
}

/*!
 * @brief 魅力補正計算 - 型
 * @return 魅力補正値
 * @details
 * * 型による魅力修正値
 * * 降鬼陣で加算(+5)
 */
s16b PlayerCharisma::battleform_value()
{
    s16b result = 0;

    if (any_bits(this->owner_ptr->special_defense, KATA_KOUKIJIN)) {
        result += 5;
    }

    return result;
}

/*!
 * @brief 魅力補正計算 - 変異
 * @return 魅力補正値
 * @details
 * * 変異による魅力修正値
 * * 変異MUT3_FLESH_ROTで減算(-1)
 * * 変異MUT3_SILLY_VOIで減算(-4)
 * * 変異MUT3_BLANK_FACで減算(-1)
 * * 変異MUT3_WART_SKINで減算(-2)
 * * 変異MUT3_SCALESで減算(-1)
 */
s16b PlayerCharisma::mutation_value()
{
    s16b result = 0;

    if (this->owner_ptr->muta3) {
        if (any_bits(this->owner_ptr->muta3, MUT3_FLESH_ROT)) {
            result -= 1;
        }
        if (any_bits(this->owner_ptr->muta3, MUT3_SILLY_VOI)) {
            result -= 4;
        }
        if (any_bits(this->owner_ptr->muta3, MUT3_BLANK_FAC)) {
            result -= 1;
        }
        if (any_bits(this->owner_ptr->muta3, MUT3_WART_SKIN)) {
            result -= 2;
        }
        if (any_bits(this->owner_ptr->muta3, MUT3_SCALES)) {
            result -= 1;
        }
    }

    return result;
}

s16b PlayerCharisma::set_exception_value(s16b value)
{
    s16b result = value;

    if (any_bits(this->owner_ptr->muta3, MUT3_ILL_NORM)) {
        result = 0;
    }

    return result;
}

BIT_FLAGS PlayerCharisma::getAllFlags()
{
    BIT_FLAGS flags = PlayerStatusBase::getAllFlags();

    if (any_bits(this->owner_ptr->muta3, MUT3_ILL_NORM)) {
        set_bits(flags, FLAG_CAUSE_MUTATION);
    }

    return flags;
}

BIT_FLAGS PlayerCharisma::getBadFlags()
{
    BIT_FLAGS flags = PlayerStatusBase::getBadFlags();

    if (any_bits(this->owner_ptr->muta3, MUT3_ILL_NORM)) {
        set_bits(flags, FLAG_CAUSE_MUTATION);
    }

    return flags;
}

/*!
 * @brief ステータス現在値更新の例外処理
 * @params 通常処理されたステータスの値
 * @returns 例外処理されたステータスの値
 * @details
 * * MUT3_ILL_NORMを保持しているときの例外処理。
 * * 魅力現在値をレベル依存の値に修正する。
 */
s16b PlayerCharisma::set_exception_use_status(s16b value)
{
    if (any_bits(this->owner_ptr->muta3, MUT3_ILL_NORM)) {
        /* 10 to 18/90 charisma, guaranteed, based on level */
        if (value < 8 + 2 * this->owner_ptr->lev) {
            value = 8 + 2 * this->owner_ptr->lev;
        }
    }
    return value;
}
