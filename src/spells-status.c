﻿#include "angband.h"
#include "avatar.h"
#include "player-status.h"
#include "spells-status.h"
#include "projection.h"
#include "spells.h"
#include "monster.h"

/*!
 * @brief モンスター回復処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool heal_monster(DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_HEAL, dir, dam, flg));
}

/*!
 * @brief モンスター加速処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool speed_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_SPEED, dir, power, flg));
}

/*!
 * @brief モンスター減速処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool slow_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_SLOW, dir, power, flg));
}

/*!
 * @brief モンスター催眠処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool sleep_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_SLEEP, dir, power, flg));
}

/*!
 * @brief モンスター拘束(STASIS)処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_monster(DIRECTION dir)
{
	return (fire_ball_hide(GF_STASIS, dir, p_ptr->lev * 2, 0));
}

/*!
 * @brief 邪悪なモンスター拘束(STASIS)処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_evil(DIRECTION dir)
{
	return (fire_ball_hide(GF_STASIS_EVIL, dir, p_ptr->lev * 2, 0));
}

/*!
 * @brief モンスター混乱処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_CONF, dir, plev, flg));
}

/*!
 * @brief モンスター朦朧処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_STUN, dir, plev, flg));
}

/*!
 * @brief チェンジモンスター処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool poly_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	bool tester = (project_hook(GF_OLD_POLY, dir, power, flg));
	if (tester)
		chg_virtue(V_CHANCE, 1);
	return(tester);
}

/*!
 * @brief クローンモンスター処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool clone_monster(DIRECTION dir)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_CLONE, dir, 0, flg));
}

/*!
 * @brief モンスター恐慌処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool fear_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_TURN_ALL, dir, plev, flg));
}

/*!
* @brief 歌の停止を処理する / Stop singing if the player is a Bard
* @return なし
*/
void stop_singing(player_type *creature_ptr)
{
	if (creature_ptr->pclass != CLASS_BARD) return;

	/* Are there interupted song? */
	if (INTERUPTING_SONG_EFFECT(creature_ptr))
	{
		/* Forget interupted song */
		INTERUPTING_SONG_EFFECT(creature_ptr) = MUSIC_NONE;
		return;
	}

	/* The player is singing? */
	if (!SINGING_SONG_EFFECT(creature_ptr)) return;

	/* Hack -- if called from set_action(), avoid recursive loop */
	if (creature_ptr->action == ACTION_SING) set_action(ACTION_NONE);

	/* Message text of each song or etc. */
	do_spell(REALM_MUSIC, SINGING_SONG_ID(creature_ptr), SPELL_STOP);

	SINGING_SONG_EFFECT(creature_ptr) = MUSIC_NONE;
	SINGING_SONG_ID(creature_ptr) = 0;
	creature_ptr->update |= (PU_BONUS);
	creature_ptr->redraw |= (PR_STATUS);
}

bool time_walk(player_type *creature_ptr)
{
	if (creature_ptr->timewalk)
	{
		msg_print(_("既に時は止まっている。", "Time is already stopped."));
		return (FALSE);
	}
	creature_ptr->timewalk = TRUE;
	msg_print(_("「時よ！」", "You yell 'Time!'"));
//	msg_print(_("「『ザ・ワールド』！時は止まった！」", "You yell 'The World! Time has stopped!'"));
	msg_print(NULL);

	/* Hack */
	creature_ptr->energy_need -= 1000 + (100 + p_ptr->csp - 50)*TURNS_PER_TICK / 10;
	creature_ptr->redraw |= (PR_MAP);
	creature_ptr->update |= (PU_MONSTERS);
	creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	handle_stuff();
	return TRUE;
}

/*!
 * @brief プレイヤーのヒットダイスを振る / Role Hitpoints
 * @param options スペル共通オプション
 * @return なし
 */
void roll_hitdice(player_type *creature_ptr, SPOP_FLAGS options)
{
	PERCENTAGE percent;

	/* Minimum hitpoints at highest level */
	HIT_POINT min_value = creature_ptr->hitdie + ((PY_MAX_LEVEL + 2) * (creature_ptr->hitdie + 1)) * 3 / 8;

	/* Maximum hitpoints at highest level */
	HIT_POINT max_value = creature_ptr->hitdie + ((PY_MAX_LEVEL + 2) * (creature_ptr->hitdie + 1)) * 5 / 8;

	int i;

	/* Rerate */
	while (1)
	{
		/* Pre-calculate level 1 hitdice */
		creature_ptr->player_hp[0] = (HIT_POINT)creature_ptr->hitdie;

		for (i = 1; i < 4; i++)
		{
			creature_ptr->player_hp[0] += randint1(creature_ptr->hitdie);
		}

		/* Roll the hitpoint values */
		for (i = 1; i < PY_MAX_LEVEL; i++)
		{
			creature_ptr->player_hp[i] = creature_ptr->player_hp[i - 1] + randint1(creature_ptr->hitdie);
		}

		/* Require "valid" hitpoints at highest level */
		if ((creature_ptr->player_hp[PY_MAX_LEVEL - 1] >= min_value) &&
			(creature_ptr->player_hp[PY_MAX_LEVEL - 1] <= max_value)) break;
	}

	percent = (int)(((long)creature_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) /
		(2 * creature_ptr->hitdie + ((PY_MAX_LEVEL - 1 + 3) * (creature_ptr->hitdie + 1))));

	/* Update and redraw hitpoints */
	creature_ptr->update |= (PU_HP);
	creature_ptr->redraw |= (PR_HP);
	creature_ptr->window |= (PW_PLAYER);

	if (!(options & SPOP_NO_UPDATE)) handle_stuff();

	if (options & SPOP_DISPLAY_MES)
	{
		if (options & SPOP_DEBUG)
		{
			msg_format(_("現在の体力ランクは %d/100 です。", "Your life rate is %d/100 now."), percent);
			creature_ptr->knowledge |= KNOW_HPRATE;
		}
		else
		{
			msg_print(_("体力ランクが変わった。", "Life rate is changed."));
			creature_ptr->knowledge &= ~(KNOW_HPRATE);
		}
	}
}

bool_hack life_stream(bool_hack message, bool_hack virtue_change)
{
	if (virtue_change)
	{
		chg_virtue(V_VITALITY, 1);
		chg_virtue(V_UNLIFE, -5);
	}
	if (message)
	{
		msg_print(_("体中に生命力が満ちあふれてきた！", "You feel life flow through your body!"));
	}
	restore_level();
	(void)set_poisoned(0);
	(void)set_blind(0);
	(void)set_confused(0);
	(void)set_image(0);
	(void)set_stun(0);
	(void)set_cut(0);
	(void)restore_all_status();
	(void)set_shero(0, TRUE);
	handle_stuff();
	hp_player(5000);

	return TRUE;
}

bool_hack heroism(int base)
{
	bool_hack ident = FALSE;
	if (set_afraid(0)) ident = TRUE;
	if (set_hero(p_ptr->hero + randint1(base) + base, FALSE)) ident = TRUE;
	if (hp_player(10)) ident = TRUE;
	return ident;
}

bool_hack berserk(int base)
{
	bool_hack ident = FALSE;
	if (set_afraid(0)) ident = TRUE;
	if (set_shero(p_ptr->shero + randint1(base) + base, FALSE)) ident = TRUE;
	if (hp_player(30)) ident = TRUE;
	return ident;
}

bool_hack cure_light_wounds(DICE_NUMBER dice, DICE_SID sides)
{
	bool_hack ident = FALSE;
	if (hp_player(damroll(dice, sides))) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_cut(p_ptr->cut - 10)) ident = TRUE;
	if (set_shero(0, TRUE)) ident = TRUE;
	return ident;
}

bool_hack cure_serious_wounds(DICE_NUMBER dice, DICE_SID sides)
{
	bool_hack ident = FALSE;
	if (hp_player(damroll(dice, sides))) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_cut((p_ptr->cut / 2) - 50)) ident = TRUE;
	if (set_shero(0, TRUE)) ident = TRUE;
	return ident;
}

bool_hack cure_critical_wounds(HIT_POINT pow)
{
	bool_hack ident = FALSE;
	if (hp_player(pow)) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_poisoned(0)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	if (set_shero(0, TRUE)) ident = TRUE;
	return ident;
}

bool_hack true_healing(HIT_POINT pow)
{
	bool_hack ident = FALSE;
	if (hp_player(pow)) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_poisoned(0)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	if (set_image(0)) ident = TRUE;
	return ident;
}

bool_hack restore_mana(bool_hack magic_eater)
{
	bool_hack ident = FALSE;

	if (p_ptr->pclass == CLASS_MAGIC_EATER && magic_eater)
	{
		int i;
		for (i = 0; i < EATER_EXT * 2; i++)
		{
			p_ptr->magic_num1[i] += (p_ptr->magic_num2[i] < 10) ? EATER_CHARGE * 3 : p_ptr->magic_num2[i] * EATER_CHARGE / 3;
			if (p_ptr->magic_num1[i] > p_ptr->magic_num2[i] * EATER_CHARGE) p_ptr->magic_num1[i] = p_ptr->magic_num2[i] * EATER_CHARGE;
		}
		for (; i < EATER_EXT * 3; i++)
		{
			KIND_OBJECT_IDX k_idx = lookup_kind(TV_ROD, i - EATER_EXT * 2);
			p_ptr->magic_num1[i] -= ((p_ptr->magic_num2[i] < 10) ? EATER_ROD_CHARGE * 3 : p_ptr->magic_num2[i] * EATER_ROD_CHARGE / 3)*k_info[k_idx].pval;
			if (p_ptr->magic_num1[i] < 0) p_ptr->magic_num1[i] = 0;
		}
		msg_print(_("頭がハッキリとした。", "You feel your head clear."));
		p_ptr->window |= (PW_PLAYER);
		ident = TRUE;
	}
	else if (p_ptr->csp < p_ptr->msp)
	{
		p_ptr->csp = p_ptr->msp;
		p_ptr->csp_frac = 0;
		msg_print(_("頭がハッキリとした。", "You feel your head clear."));
		p_ptr->redraw |= (PR_MANA);
		p_ptr->window |= (PW_PLAYER);
		p_ptr->window |= (PW_SPELL);
		ident = TRUE;
	}

	return ident;
}

bool restore_all_status(void)
{
	bool ident = FALSE;
	if (do_res_stat(A_STR)) ident = TRUE;
	if (do_res_stat(A_INT)) ident = TRUE;
	if (do_res_stat(A_WIS)) ident = TRUE;
	if (do_res_stat(A_DEX)) ident = TRUE;
	if (do_res_stat(A_CON)) ident = TRUE;
	if (do_res_stat(A_CHR)) ident = TRUE;
	return ident;
}

bool fishing(player_type *creature_ptr)
{
	DIRECTION dir;
	POSITION x, y;

	if (!get_direction(&dir, FALSE, FALSE)) return FALSE;
	y = creature_ptr->y + ddy[dir];
	x = creature_ptr->x + ddx[dir];
	creature_ptr->fishing_dir = dir;
	if (!cave_have_flag_bold(y, x, FF_WATER))
	{
		msg_print(_("そこは水辺ではない。", "There is no fishing place."));
		return FALSE;
	}
	else if (current_floor_ptr->grid_array[y][x].m_idx)
	{
		GAME_TEXT m_name[MAX_NLEN];
		monster_desc(m_name, &current_floor_ptr->m_list[current_floor_ptr->grid_array[y][x].m_idx], 0);
		msg_format(_("%sが邪魔だ！", "%^s is standing in your way."), m_name);
		free_turn(creature_ptr);
		return FALSE;
	}
	set_action(ACTION_FISH);
	creature_ptr->redraw |= (PR_STATE);
	return TRUE;
}


bool cosmic_cast_off(player_type *creature_ptr, object_type *o_ptr)
{
	INVENTORY_IDX inv;
	int t;
	OBJECT_IDX o_idx;
	GAME_TEXT o_name[MAX_NLEN];
	object_type forge;

	/* Cast off activated item */
	for (inv = INVEN_RARM; inv <= INVEN_FEET; inv++)
	{
		if (o_ptr == &inventory[inv]) break;
	}
	if (inv > INVEN_FEET) return FALSE;

	object_copy(&forge, o_ptr);
	inven_item_increase(inv, (0 - o_ptr->number));
	inven_item_optimize(inv);
	o_idx = drop_near(&forge, 0, creature_ptr->y, creature_ptr->x);
	o_ptr = &current_floor_ptr->o_list[o_idx];

	object_desc(o_name, o_ptr, OD_NAME_ONLY);
	msg_format(_("%sを脱ぎ捨てた。", "You cast off %s."), o_name);

	/* Get effects */
	msg_print(_("「燃え上がれ俺の小宇宙！」", "You say, 'Burn up my cosmo!"));
	t = 20 + randint1(20);
	(void)set_blind(creature_ptr->blind + t);
	(void)set_afraid(0);
	(void)set_tim_esp(creature_ptr->tim_esp + t, FALSE);
	(void)set_tim_regen(creature_ptr->tim_regen + t, FALSE);
	(void)set_hero(creature_ptr->hero + t, FALSE);
	(void)set_blessed(creature_ptr->blessed + t, FALSE);
	(void)set_fast(creature_ptr->fast + t, FALSE);
	(void)set_shero(creature_ptr->shero + t, FALSE);
	if (creature_ptr->pclass == CLASS_FORCETRAINER)
	{
		P_PTR_KI = creature_ptr->lev * 5 + 190;
		msg_print(_("気が爆発寸前になった。", "Your force absorbs the explosion."));
	}

	return TRUE;
}


/*!
 * @brief プレイヤーの因果混乱処理 / Apply Nexus
 * @param m_ptr 因果混乱をプレイヤーに与えたモンスターの情報参照ポインタ
 * @return なし
 */
void apply_nexus(monster_type *m_ptr)
{
	switch (randint1(7))
	{
	case 1: case 2: case 3:
	{
		teleport_player(200, TELEPORT_PASSIVE);
		break;
	}

	case 4: case 5:
	{
		teleport_player_to(m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
		break;
	}

	case 6:
	{
		if (randint0(100) < p_ptr->skill_sav)
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			break;
		}
		teleport_level(0);
		break;
	}

	case 7:
	{
		if (randint0(100) < p_ptr->skill_sav)
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			break;
		}

		msg_print(_("体がねじれ始めた...", "Your body starts to scramble..."));
		status_shuffle();
		break;
	}
	}
}

/*!
 * @brief プレイヤーのステータスシャッフル処理
 * @return なし
 */
void status_shuffle(void)
{
	BASE_STATUS max1, cur1, max2, cur2;
	int ii, jj, i;

	/* Pick a pair of stats */
	ii = randint0(A_MAX);
	for (jj = ii; jj == ii; jj = randint0(A_MAX)) /* loop */;

	max1 = p_ptr->stat_max[ii];
	cur1 = p_ptr->stat_cur[ii];
	max2 = p_ptr->stat_max[jj];
	cur2 = p_ptr->stat_cur[jj];

	p_ptr->stat_max[ii] = max2;
	p_ptr->stat_cur[ii] = cur2;
	p_ptr->stat_max[jj] = max1;
	p_ptr->stat_cur[jj] = cur1;

	for (i = 0; i < A_MAX; i++)
	{
		if (p_ptr->stat_max[i] > p_ptr->stat_max_max[i]) p_ptr->stat_max[i] = p_ptr->stat_max_max[i];
		if (p_ptr->stat_cur[i] > p_ptr->stat_max_max[i]) p_ptr->stat_cur[i] = p_ptr->stat_max_max[i];
	}

	p_ptr->update |= (PU_BONUS);
}
