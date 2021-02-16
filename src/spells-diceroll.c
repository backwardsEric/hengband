﻿#include "angband.h"

#include "spells-diceroll.h"

#include "monster.h"
#include "monsterrace-hook.h"
#include "mutation.h"
#include "projection.h"
#include "rooms.h"


/*!
 * @brief モンスター魅了用セービングスロー共通部(汎用系)
 * @param pow 魅了パワー
 * @param m_ptr 対象モンスター
 * @return 魅了に抵抗したらTRUE
 */
bool_hack common_saving_throw_charm(player_type *player_ptr, HIT_POINT pow, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (p_ptr->inside_arena) return TRUE;

	/* Memorize a flag */
	if (r_ptr->flagsr & RFR_RES_ALL)
	{
		if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_ALL);
		return TRUE;
	}

	if (r_ptr->flags3 & RF3_NO_CONF)
	{
		if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
		return TRUE;
	}

	if (r_ptr->flags1 & RF1_QUESTOR || m_ptr->mflag2 & MFLAG2_NOPET) return TRUE;

	pow += (adj_chr_chm[player_ptr->stat_ind[A_CHR]] - 1);
	if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)) pow = pow * 2 / 3;
	return (r_ptr->level > randint1((pow - 10) < 1 ? 1 : (pow - 10)) + 5);
}

/*!
 * @brief モンスター服従用セービングスロー共通部(部族依存系)
 * @param pow 服従パワー
 * @param m_ptr 対象モンスター
 * @return 服従に抵抗したらTRUE
 */
bool_hack common_saving_throw_control(player_type *player_ptr, HIT_POINT pow, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (p_ptr->inside_arena) return TRUE;

	/* Memorize a flag */
	if (r_ptr->flagsr & RFR_RES_ALL)
	{
		if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_ALL);
		return TRUE;
	}

	if (r_ptr->flags1 & RF1_QUESTOR || m_ptr->mflag2 & MFLAG2_NOPET) return TRUE;

	pow += adj_chr_chm[player_ptr->stat_ind[A_CHR]] - 1;
	if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)) pow = pow * 2 / 3;
	return (r_ptr->level > randint1((pow - 10) < 1 ? 1 : (pow - 10)) + 5);
}

/*!
* @brief 一部ボルト魔法のビーム化確率を算出する / Prepare standard probability to become beam for fire_bolt_or_beam()
* @return ビーム化確率(%)
* @details
* ハードコーティングによる実装が行われている。
* メイジは(レベル)%、ハイメイジ、スペルマスターは(レベル)%、それ以外の職業は(レベル/2)%
*/
PERCENTAGE beam_chance(void)
{
	if (p_ptr->pclass == CLASS_MAGE)
		return (PERCENTAGE)(p_ptr->lev);
	if (p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER)
		return (PERCENTAGE)(p_ptr->lev + 10);

	return (PERCENTAGE)(p_ptr->lev / 2);
}