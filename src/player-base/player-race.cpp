﻿/*!
 * @brief プレイヤーの種族に基づく耐性・能力の判定処理等を行うクラス
 * @date 2021/09/08
 * @author Hourier
 * @details 本クラス作成時点で責務に対する余裕はかなりあるので、適宜ここへ移してくること.
 */
#include "player-base/player-race.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
#include "player-info/mimic-info-table.h"
#include "player/race-info-table.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief Construct a new Player Race:: Player Race object
 *
 * @param base_race true の場合、仮に変身している場合でも元の種族について扱う。 false の場合変身している種族について扱う。
 * 引数を省略した場合は false
 */
PlayerRace::PlayerRace(player_type *player_ptr, bool base_race)
    : player_ptr(player_ptr)
    , base_race(base_race)
{
}

/*!
 * @brief 種族固有の特性フラグを取得する
 * @return
 */
TrFlags PlayerRace::tr_flags() const
{
    TrFlags flags;

    auto race_ptr = this->get_info();
    if (race_ptr->infra > 0)
        flags.set(TR_INFRA);

    for (auto &cond : race_ptr->extra_flags) {
        if (player_ptr->lev < cond.level)
            continue;
        if (cond.pclass != std::nullopt) {
            if (cond.not_class && player_ptr->pclass == cond.pclass)
                continue;
            if (!cond.not_class && player_ptr->pclass != cond.pclass)
                continue;
        }

        flags.set(cond.type);
    }

    return flags;
}

/*!
 * @brief プレイヤーの種族情報テーブルへのポインタを取得する
 *
 * @return プレイヤーの種族情報テーブルへのポインタ
 */
const player_race_info *PlayerRace::get_info() const
{
    if (this->base_race) {
        return &race_info[enum2i(this->player_ptr->prace)];
    }

    switch (this->player_ptr->mimic_form) {
    case MIMIC_DEMON:
    case MIMIC_DEMON_LORD:
    case MIMIC_VAMPIRE:
        return &mimic_info[this->player_ptr->mimic_form];
    default: // MIMIC_NONE or undefined
        return &race_info[enum2i(this->player_ptr->prace)];
    }
}

/*!
 * @brief 種族の生命形態を返す
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @return 生命形態
 */
PlayerRaceLife PlayerRace::life() const
{
    return this->get_info()->life;
}

/*!
 * @brief 種族の食料形態を返す
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param base_race ミミック中も元種族の情報を返すならtrue
 * @return 食料形態
 */
PlayerRaceFood PlayerRace::food() const
{
    return this->get_info()->food;
}

bool PlayerRace::is_mimic_nonliving() const
{
    constexpr int nonliving_flag = 1;
    return any_bits(mimic_info[this->player_ptr->mimic_form].choice, nonliving_flag);
}

bool PlayerRace::can_resist_cut() const
{
    auto can_resist_cut = this->player_ptr->prace == PlayerRaceType::GOLEM;
    can_resist_cut |= this->player_ptr->prace == PlayerRaceType::SKELETON;
    can_resist_cut |= this->player_ptr->prace == PlayerRaceType::SPECTRE;
    can_resist_cut |= (this->player_ptr->prace == PlayerRaceType::ZOMBIE) && (this->player_ptr->lev > 11);
    return can_resist_cut;
}

bool PlayerRace::equals(PlayerRaceType prace) const
{
    return (this->player_ptr->mimic_form == MIMIC_NONE) && (this->player_ptr->prace == prace);
}

/*!
 * @brief 速度計算 - 種族
 * @return 速度値の増減分
 * @details
 * ** クラッコンと妖精に加算(+レベル/10)
 * ** 悪魔変化/吸血鬼変化で加算(+3)
 * ** 魔王変化で加算(+5)
 * ** マーフォークがFF_WATER地形にいれば加算(+2+レベル/10)
 * ** そうでなく浮遊を持っていないなら減算(-2)
 */
int16_t PlayerRace::speed() const
{
    int16_t result = 0;

    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::KLACKON) || PlayerRace(this->player_ptr).equals(PlayerRaceType::SPRITE))
        result += (this->player_ptr->lev) / 10;

    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::MERFOLK)) {
        floor_type *floor_ptr = this->player_ptr->current_floor_ptr;
        feature_type *f_ptr = &f_info[floor_ptr->grid_array[this->player_ptr->y][this->player_ptr->x].feat];
        if (f_ptr->flags.has(FF::WATER)) {
            result += (2 + this->player_ptr->lev / 10);
        } else if (!this->player_ptr->levitation) {
            result -= 2;
        }
    }

    if (this->player_ptr->mimic_form) {
        switch (this->player_ptr->mimic_form) {
        case MIMIC_DEMON:
            result += 3;
            break;
        case MIMIC_DEMON_LORD:
            result += 5;
            break;
        case MIMIC_VAMPIRE:
            result += 3;
            break;
        }
    }
    return result;
}

/*!
 * @brief 腕力補正計算 - 種族
 * @return 腕力補正値
 * @details
 * * 種族による腕力修正値。
 * * エントはレベル26,41,46到達ごとに加算(+1)
 */
int16_t PlayerRace::additional_strength() const
{
    int16_t result = 0;

    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::ENT)) {
        if (this->player_ptr->lev > 25)
            result++;
        if (this->player_ptr->lev > 40)
            result++;
        if (this->player_ptr->lev > 45)
            result++;
    }

    return result;
}

/*!
 * @brief 器用さ補正計算 - 種族
 * @return 器用さ補正値
 * @details
 * * 種族による器用さ修正値。
 * * エントはレベル26,41,46到達ごとに減算(-1)
 */
int16_t PlayerRace::additional_dexterity() const
{
    int16_t result = 0;

    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::ENT)) {
        if (this->player_ptr->lev > 25)
            result--;
        if (this->player_ptr->lev > 40)
            result--;
        if (this->player_ptr->lev > 45)
            result--;
    }

    return result;
}

/*!
 * @brief 耐久力補正計算 - 種族
 * @return 耐久力補正値
 * @details
 * * 種族による耐久力修正値。
 * * エントはレベル26,41,46到達ごとに加算(+1)
 */
int16_t PlayerRace::additional_constitution() const
{
    int16_t result = 0;

    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::ENT)) {
        if (this->player_ptr->lev > 25)
            result++;
        if (this->player_ptr->lev > 40)
            result++;
        if (this->player_ptr->lev > 45)
            result++;
    }

    return result;
}
