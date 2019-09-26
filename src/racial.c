﻿/*!
 * @file racial.c
 * @brief レイシャルと突然変異の技能処理 / Racial powers (and mutations)
 * @date 2014/01/08
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen. \n
 */

#include "angband.h"
#include "cmd-magiceat.h"
#include "cmd-mane.h"
#include "cmd-zapwand.h"
#include "cmd-pet.h"
#include "melee.h"
#include "object-hook.h"

#include "mutation.h"
#include "player-status.h"
#include "spells-status.h"
#include "spells-object.h"
#include "spells-floor.h"
#include "cmd-spell.h"
#include "realm-hex.h"

/*!
 * @brief 修行僧の構え設定処理
 * @return 構えを変化させたらTRUE、構え不能かキャンセルしたらFALSEを返す。
 */
static bool choose_kamae(void)
{
	char choice;
	int new_kamae = 0;
	int i;
	char buf[80];

	if (cmd_limit_confused(p_ptr)) return FALSE;
	screen_save();
	prt(_(" a) 構えをとく", " a) No form"), 2, 20);

	for (i = 0; i < MAX_KAMAE; i++)
	{
		if (p_ptr->lev >= kamae_shurui[i].min_level)
		{
			sprintf(buf," %c) %-12s  %s",I2A(i+1), kamae_shurui[i].desc, kamae_shurui[i].info);
			prt(buf, 3+i, 20);
		}
	}

	prt("", 1, 0);
	prt(_("        どの構えをとりますか？", "        Choose Stance: "), 1, 14);

	while(1)
	{
		choice = inkey();

		if (choice == ESCAPE)
		{
			screen_load();
			return FALSE;
		}
		else if ((choice == 'a') || (choice == 'A'))
		{
			if (p_ptr->action == ACTION_KAMAE)
			{
				set_action(ACTION_NONE);
			}
			else
				msg_print(_("もともと構えていない。", "You are not in a special stance."));
			screen_load();
			return TRUE;
		}
		else if ((choice == 'b') || (choice == 'B'))
		{
			new_kamae = 0;
			break;
		}
		else if (((choice == 'c') || (choice == 'C')) && (p_ptr->lev > 29))
		{
			new_kamae = 1;
			break;
		}
		else if (((choice == 'd') || (choice == 'D')) && (p_ptr->lev > 34))
		{
			new_kamae = 2;
			break;
		}
		else if (((choice == 'e') || (choice == 'E')) && (p_ptr->lev > 39))
		{
			new_kamae = 3;
			break;
		}
	}
	set_action(ACTION_KAMAE);

	if (p_ptr->special_defense & (KAMAE_GENBU << new_kamae))
	{
		msg_print(_("構え直した。", "You reassume a stance."));
	}
	else
	{
		p_ptr->special_defense &= ~(KAMAE_MASK);
		p_ptr->update |= (PU_BONUS);
		p_ptr->redraw |= (PR_STATE);
		msg_format(_("%sの構えをとった。", "You assume the %s stance."),kamae_shurui[new_kamae].desc);
		p_ptr->special_defense |= (KAMAE_GENBU << new_kamae);
	}
	p_ptr->redraw |= PR_STATE;
	screen_load();
	return TRUE;
}

/*!
 * @brief 剣術家の型設定処理
 * @return 型を変化させたらTRUE、型の構え不能かキャンセルしたらFALSEを返す。
 */
static bool choose_kata(void)
{
	char choice;
	int new_kata = 0;
	int i;
	char buf[80];

	if (cmd_limit_confused(p_ptr)) return FALSE;

	if (p_ptr->stun)
	{
		msg_print(_("意識がはっきりとしない。", "You are not clear headed"));
		return FALSE;
	}

	if (p_ptr->afraid)
	{
		msg_print(_("体が震えて構えられない！", "You are trembling with fear!"));
		return FALSE;
	}
	screen_save();
	prt(_(" a) 型を崩す", " a) No Form"), 2, 20);

	for (i = 0; i < MAX_KATA; i++)
	{
		if (p_ptr->lev >= kata_shurui[i].min_level)
		{
			sprintf(buf,_(" %c) %sの型    %s", " %c) Stance of %-12s  %s"),I2A(i+1), kata_shurui[i].desc, kata_shurui[i].info);
			prt(buf, 3+i, 20);
		}
	}

	prt("", 1, 0);
	prt(_("        どの型で構えますか？", "        Choose Stance: "), 1, 14);

	while(1)
	{
		choice = inkey();

		if (choice == ESCAPE)
		{
			screen_load();
			return FALSE;
		}
		else if ((choice == 'a') || (choice == 'A'))
		{
			if (p_ptr->action == ACTION_KATA)
			{
				set_action(ACTION_NONE);
			}
			else
				msg_print(_("もともと構えていない。", "You are not in a special stance."));
			screen_load();
			return TRUE;
		}
		else if ((choice == 'b') || (choice == 'B'))
		{
			new_kata = 0;
			break;
		}
		else if (((choice == 'c') || (choice == 'C')) && (p_ptr->lev > 29))
		{
			new_kata = 1;
			break;
		}
		else if (((choice == 'd') || (choice == 'D')) && (p_ptr->lev > 34))
		{
			new_kata = 2;
			break;
		}
		else if (((choice == 'e') || (choice == 'E')) && (p_ptr->lev > 39))
		{
			new_kata = 3;
			break;
		}
	}
	set_action(ACTION_KATA);

	if (p_ptr->special_defense & (KATA_IAI << new_kata))
	{
		msg_print(_("構え直した。", "You reassume a stance."));
	}
	else
	{
		p_ptr->special_defense &= ~(KATA_MASK);
		p_ptr->update |= (PU_BONUS | PU_MONSTERS);
		msg_format(_("%sの型で構えた。", "You assume the %s stance."),kata_shurui[new_kata].desc);
		p_ptr->special_defense |= (KATA_IAI << new_kata);
	}
	p_ptr->redraw |= (PR_STATE | PR_STATUS);
	screen_load();
	return TRUE;
}


/*!
 * @brief レイシャル・パワー情報のtypedef
 */
typedef struct power_desc_type power_desc_type;

/*!
 * @brief レイシャル・パワー情報の構造体定義
 */
struct power_desc_type
{
	GAME_TEXT name[MAX_NLEN];      //!<レイシャル名
	PLAYER_LEVEL level;	//!<体得レベル
	int cost;
	int stat;
	PERCENTAGE fail;
	int number;
};


/*!
 * @brief レイシャル・パワーの発動成功率を計算する / Returns the chance to activate a racial power/mutation
 * @param pd_ptr 発動したいレイシャル・パワー情報の構造体参照ポインタ
 * @return 成功率(%)を返す
 */
static PERCENTAGE racial_chance(power_desc_type *pd_ptr)
{
	PLAYER_LEVEL min_level  = pd_ptr->level;
	PERCENTAGE difficulty = pd_ptr->fail;

	int i;
	int val;
	int sum = 0;
	BASE_STATUS stat = p_ptr->stat_cur[pd_ptr->stat];

	/* No chance for success */
	if ((p_ptr->lev < min_level) || p_ptr->confused)
	{
		return (0);
	}

	if (difficulty == 0) return 100;

	/* Calculate difficulty */
	if (p_ptr->stun)
	{
		difficulty += (PERCENTAGE)p_ptr->stun;
	}
	else if (p_ptr->lev > min_level)
	{
		PERCENTAGE lev_adj = (PERCENTAGE)((p_ptr->lev - min_level) / 3);
		if (lev_adj > 10) lev_adj = 10;
		difficulty -= lev_adj;
	}

	if (difficulty < 5) difficulty = 5;

	/* We only need halfs of the difficulty */
	difficulty = difficulty / 2;

	for (i = 1; i <= stat; i++)
	{
		val = i - difficulty;
		if (val > 0)
			sum += (val <= difficulty) ? val : difficulty;
	}

	if (difficulty == 0)
		return (100);
	else
		return (((sum * 100) / difficulty) / stat);
}


static int racial_cost;

/*!
 * @brief レイシャル・パワーの発動の判定処理
 * @param pd_ptr 発動したいレイシャル・パワー情報の構造体参照ポインタ
 * @return
 * 発動成功ならば1、発動失敗ならば-1、キャンセルならば0を返す。
 * return value indicates that we have succesfully used the power 1: Succeeded, 0: Cancelled, -1: Failed
 */
static int racial_aux(power_desc_type *pd_ptr)
{
	PLAYER_LEVEL min_level  = pd_ptr->level;
	int use_stat   = pd_ptr->stat;
	int difficulty = pd_ptr->fail;
	int use_hp = 0;

	racial_cost = pd_ptr->cost;

	/* Not enough mana - use hp */
	if (p_ptr->csp < racial_cost) use_hp = racial_cost - p_ptr->csp;

	/* Power is not available yet */
	if (p_ptr->lev < min_level)
	{
		msg_format(_("この能力を使用するにはレベル %d に達していなければなりません。", 
					 "You need to attain level %d to use this power."), min_level);

		free_turn(p_ptr);
		return FALSE;
	}

	if (cmd_limit_confused(p_ptr))
	{
		free_turn(p_ptr);
		return FALSE;
	}

	/* Risk death? */
	else if (p_ptr->chp < use_hp)
	{
		if (!get_check(_("本当に今の衰弱した状態でこの能力を使いますか？", "Really use the power in your weakened state? ")))
		{
			free_turn(p_ptr);
			return FALSE;
		}
	}

	/* Else attempt to do it! */

	if (difficulty)
	{
		if (p_ptr->stun)
		{
			difficulty += p_ptr->stun;
		}
		else if (p_ptr->lev > min_level)
		{
			int lev_adj = ((p_ptr->lev - min_level) / 3);
			if (lev_adj > 10) lev_adj = 10;
			difficulty -= lev_adj;
		}

		if (difficulty < 5) difficulty = 5;
	}

	/* take time and pay the price */
	take_turn(p_ptr, 100);

	/* Success? */
	if (randint1(p_ptr->stat_cur[use_stat]) >= ((difficulty / 2) + randint1(difficulty / 2)))
	{
		return 1;
	}

	if (flush_failure) flush();
	msg_print(_("充分に集中できなかった。", "You've failed to concentrate hard enough."));

	return -1;
}


/*!
 * @brief レイシャル・パワー発動処理
 * @param command 発動するレイシャルのID
 * @return 処理を実際に実行した場合はTRUE、キャンセルした場合FALSEを返す。
 */
static bool cmd_racial_power_aux(s32b command)
{
	PLAYER_LEVEL plev = p_ptr->lev;
	DIRECTION dir = 0;

	if (command <= -3)
	{
		switch (p_ptr->pclass)
		{
		case CLASS_WARRIOR:
		{
			return sword_dancing(p_ptr);
			break;
		}
		case CLASS_HIGH_MAGE:
		if (p_ptr->realm1 == REALM_HEX)
		{
			bool retval = stop_hex_spell();
			if (retval) p_ptr->energy_use = 10;
			return (retval);
		}
		case CLASS_MAGE:
		/* case CLASS_HIGH_MAGE: */
		case CLASS_SORCERER:
		{
			if (!eat_magic(p_ptr->lev * 2)) return FALSE;
			break;
		}
		case CLASS_PRIEST:
		{
			if (is_good_realm(p_ptr->realm1))
			{
				if (!bless_weapon()) return FALSE;
			}
			else
			{
				(void)dispel_monsters(plev * 4);
				turn_monsters(plev * 4);
				banish_monsters(plev * 4);
			}
			break;
		}
		case CLASS_ROGUE:
		{
			if(!panic_hit()) return FALSE;
			break;
		}
		case CLASS_RANGER:
		case CLASS_SNIPER:
		{
			msg_print(_("敵を調査した...", "You examine your foes..."));
			probing();
			break;
		}
		case CLASS_PALADIN:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			fire_beam(is_good_realm(p_ptr->realm1) ? GF_HOLY_FIRE : GF_HELL_FIRE,
			          dir, plev * 3);
			break;
		}
		case CLASS_WARRIOR_MAGE:
		{
			if (command == -3)
			{
				return comvert_hp_to_mp(p_ptr);
			}
			else if (command == -4)
			{
				return comvert_mp_to_hp(p_ptr);
			}
			break;
		}
		case CLASS_CHAOS_WARRIOR:
		{
			return confusing_light(p_ptr);
			break;
		}
		case CLASS_MONK:
		{
			if (!(empty_hands(TRUE) & EMPTY_HAND_RARM))
			{
				msg_print(_("素手じゃないとできません。", "You need to be barehanded."));
				return FALSE;
			}
			if (p_ptr->riding)
			{
				msg_print(_("乗馬中はできません。", "You need to get off a pet."));
				return FALSE;
			}

			if (command == -3)
			{
				if (!choose_kamae()) return FALSE;
				p_ptr->update |= (PU_BONUS);
			}
			else if (command == -4)
			{
				return double_attack(p_ptr);
			}
			break;
		}
		case CLASS_MINDCRAFTER:
		case CLASS_FORCETRAINER:
		{
			return clear_mind(p_ptr);
		}
		case CLASS_TOURIST:
		{
			if (command == -3)
			{
				if (!get_aim_dir(&dir)) return FALSE;
				project_length = 1;
				fire_beam(GF_PHOTO, dir, 1);
			}
			else if (command == -4)
			{
				if (!identify_fully(FALSE)) return FALSE;
			}
			break;
		}
		case CLASS_IMITATOR:
		{
			handle_stuff();
			if (!do_cmd_mane(TRUE)) return FALSE;
			break;
		}
		case CLASS_BEASTMASTER:
		{
			if (command == -3)
			{
				if (!get_aim_dir(&dir)) return FALSE;
				(void)fire_ball_hide(GF_CHARM_LIVING, dir, p_ptr->lev, 0);
			}
			else if (command == -4)
			{
				project_all_los(GF_CHARM_LIVING, p_ptr->lev);
			}
			break;
		}
		case CLASS_ARCHER:
		{
			if (!create_ammo()) return FALSE;
			break;
		}
		case CLASS_MAGIC_EATER:
		{
			if (command == -3) {
				if (!import_magic_device()) return FALSE;
			} else if (command == -4) {
				if (cmd_limit_cast(p_ptr)) return FALSE;
				if (!do_cmd_magic_eater(FALSE, TRUE)) return FALSE;
			}
			break;
		}
		case CLASS_BARD:
		{
			/* Singing is already stopped */
			if (!SINGING_SONG_EFFECT(p_ptr) && !INTERUPTING_SONG_EFFECT(p_ptr)) return FALSE;

			stop_singing(p_ptr);
			p_ptr->energy_use = 10;
			break;
		}
		case CLASS_RED_MAGE:
		{
			if (cmd_limit_cast(p_ptr)) return FALSE;
			handle_stuff();
			do_cmd_cast();
			handle_stuff();
			if (!p_ptr->paralyzed && !cmd_limit_cast(p_ptr))
				do_cmd_cast();
			break;
		}
		case CLASS_SAMURAI:
		{
			if (command == -3)
			{
				concentration(p_ptr);
			}
			else if (command == -4)
			{
				if (!has_melee_weapon(INVEN_RARM) && !has_melee_weapon(INVEN_LARM))
				{
					msg_print(_("武器を持たないといけません。", "You need to wield a weapon."));
					return FALSE;
				}
				if (!choose_kata()) return FALSE;
				p_ptr->update |= (PU_BONUS);
			}
			break;
		}
		case CLASS_BLUE_MAGE:
		{
			if (p_ptr->action == ACTION_LEARN)
			{
				set_action(ACTION_NONE);
			}
			else
			{
				set_action(ACTION_LEARN);
			}
			free_turn(p_ptr);
			break;
		}
		case CLASS_CAVALRY:
		{
			return rodeo(p_ptr);
		}
		case CLASS_BERSERKER:
		{
			if (!recall_player(p_ptr, randint0(21) + 15)) return FALSE;
			break;
		}
		case CLASS_SMITH:
		{
			if (p_ptr->lev > 29)
			{
				if (!identify_fully(TRUE)) return FALSE;
			}
			else
			{
				if (!ident_spell(TRUE)) return FALSE;
			}
			break;
		}
		case CLASS_MIRROR_MASTER:
		{
			if (command == -3)
			{
				/* Explode all mirrors */
				remove_all_mirrors(TRUE);
			}
			else if (command == -4)
			{
				return mirror_concentration(p_ptr);
			}
			break;
		}
		case CLASS_NINJA:
			hayagake(p_ptr);
			break;
		}
	}
	else if (p_ptr->mimic_form)
	{
		switch (p_ptr->mimic_form)
		{
		case MIMIC_DEMON:
		case MIMIC_DEMON_LORD:
		{
			return demonic_breath(p_ptr);
		}
		case MIMIC_VAMPIRE:
			vampirism();
			break;
		}
	}

	else 
	{

	switch (p_ptr->prace)
	{
		case RACE_DWARF:
			msg_print(_("周囲を調べた。", "You examine your surroundings."));
			(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
			(void)detect_doors(DETECT_RAD_DEFAULT);
			(void)detect_stairs(DETECT_RAD_DEFAULT);
			break;

		case RACE_HOBBIT:
			return create_ration(p_ptr);
			break;

		case RACE_GNOME:
			msg_print(_("パッ！", "Blink!"));
			teleport_player(10, 0L);
			break;

		case RACE_HALF_ORC:
			msg_print(_("勇気を出した。", "You play tough."));
			(void)set_afraid(0);
			break;

		case RACE_HALF_TROLL:
			msg_print(_("うがぁぁ！", "RAAAGH!"));
			(void)berserk(10 + randint1(plev));
			break;

		case RACE_AMBERITE:
			if (command == -1)
			{
				msg_print(_("あなたは歩き周り始めた。", "You start walking around. "));
				alter_reality();
			}
			else if (command == -2)
			{
				msg_print(_("あなたは「パターン」を心に描いてその上を歩いた...", "You picture the Pattern in your mind and walk it..."));

				(void)true_healing(0);
				(void)restore_all_status();
				(void)restore_level();
			}
			break;

		case RACE_BARBARIAN:
			msg_print(_("うぉぉおお！", "Raaagh!"));
			(void)berserk(10 + randint1(plev));
			break;

		case RACE_HALF_OGRE:
			msg_print(_("爆発のルーンを慎重に仕掛けた...", "You carefully set an explosive rune..."));
			explosive_rune();
			break;

		case RACE_HALF_GIANT:
			if (!get_aim_dir(&dir)) return FALSE;
			(void)wall_to_mud(dir, 20 + randint1(30));
			break;

		case RACE_HALF_TITAN:
			msg_print(_("敵を調査した...", "You examine your foes..."));
			probing();
			break;

		case RACE_CYCLOPS:
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("巨大な岩を投げた。", "You throw a huge boulder."));
			fire_bolt(GF_MISSILE, dir, (3 * plev) / 2);
			break;

		case RACE_YEEK:
			if (!get_aim_dir(&dir)) return FALSE;
			stop_mouth();
			msg_print(_("身の毛もよだつ叫び声を上げた！", "You make a horrible scream!"));
			(void)fear_monster(dir, plev);
			break;

		case RACE_KLACKON:
			if (!get_aim_dir(&dir)) return FALSE;
			stop_mouth();
			msg_print(_("酸を吐いた。", "You spit acid."));
			if (plev < 25) fire_bolt(GF_ACID, dir, plev);
			else fire_ball(GF_ACID, dir, plev, 2);
			break;

		case RACE_KOBOLD:
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("毒のダーツを投げた。", "You throw a dart of poison."));
			fire_bolt(GF_POIS, dir, plev);
			break;

		case RACE_NIBELUNG:
			msg_print(_("周囲を調査した。", "You examine your surroundings."));
			(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
			(void)detect_doors(DETECT_RAD_DEFAULT);
			(void)detect_stairs(DETECT_RAD_DEFAULT);
			break;

		case RACE_DARK_ELF:
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("マジック・ミサイルを放った。", "You cast a magic missile."));
			fire_bolt_or_beam(10, GF_MISSILE, dir, damroll(3 + ((plev - 1) / 5), 4));
			break;

		case RACE_DRACONIAN:
			return draconian_breath(p_ptr);
			break;

		case RACE_MIND_FLAYER:
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("あなたは集中し、目が赤く輝いた...", "You concentrate and your eyes glow red..."));
			fire_bolt(GF_PSI, dir, plev);
			break;

		case RACE_IMP:
			if (!get_aim_dir(&dir)) return FALSE;
			if (plev >= 30)
			{
				msg_print(_("ファイア・ボールを放った。", "You cast a ball of fire."));
				fire_ball(GF_FIRE, dir, plev, 2);
			}
			else
			{
				msg_print(_("ファイア・ボルトを放った。", "You cast a bolt of fire."));
				fire_bolt(GF_FIRE, dir, plev);
			}
			break;

		case RACE_GOLEM:
			(void)set_shield(randint1(20) + 30, FALSE);
			break;

		case RACE_SKELETON:
		case RACE_ZOMBIE:
			msg_print(_("あなたは失ったエネルギーを取り戻そうと試みた。", "You attempt to restore your lost energies."));
			(void)restore_level();
			break;

		case RACE_VAMPIRE:
			vampirism();
			break;

		case RACE_SPECTRE:
			if (!get_aim_dir(&dir)) return FALSE;
			stop_mouth();
			msg_print(_("あなたはおどろおどろしい叫び声をあげた！", "You emit an eldritch howl!"));
			(void)fear_monster(dir, plev);
			break;

		case RACE_SPRITE:
			msg_print(_("あなたは魔法の粉を投げつけた...", "You throw some magic dust..."));
			if (plev < 25) sleep_monsters_touch();
			else (void)sleep_monsters(plev);
			break;

		case RACE_DEMON:
			return demonic_breath(p_ptr); 
			break;

		case RACE_KUTAR:
			(void)set_tsubureru(randint1(20) + 30, FALSE);
			break;

		case RACE_ANDROID:
			return android_inside_weapon(p_ptr);
			break;

		default:
			msg_print(_("この種族は特殊な能力を持っていません。", "This race has no bonus power."));
			free_turn(p_ptr);
	}
	}
	return TRUE;
}

/*!
 * @brief レイシャル・パワーコマンドのメインルーチン / Allow user to choose a power (racial / mutation) to activate
 * @return なし
 */
void do_cmd_racial_power(void)
{
	power_desc_type power_desc[36];
	int num;
	COMMAND_CODE i = 0;
	int ask = TRUE;
	PLAYER_LEVEL lvl = p_ptr->lev;
	bool            flag, redraw, cast = FALSE;
	bool            warrior = ((p_ptr->pclass == CLASS_WARRIOR || p_ptr->pclass == CLASS_BERSERKER) ? TRUE : FALSE);
	char            choice;
	char            out_val[160];
	int menu_line = (use_menu ? 1 : 0);

	if (p_ptr->wild_mode) return;

	for (num = 0; num < 36; num++)
	{
		strcpy(power_desc[num].name, "");
		power_desc[num].number = 0;
	}

	num = 0;

	if (cmd_limit_confused(p_ptr))
	{
		free_turn(p_ptr);
		return;
	}

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	switch (p_ptr->pclass)
	{
	case CLASS_WARRIOR:
	{
		strcpy(power_desc[num].name, _("剣の舞い", "Sword Dancing"));
		power_desc[num].level = 40;
		power_desc[num].cost = 75;
		power_desc[num].stat = A_DEX;
		power_desc[num].fail = 35;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_HIGH_MAGE:
	if (p_ptr->realm1 == REALM_HEX)
	{
		strcpy(power_desc[num].name, _("詠唱をやめる", "Stop spelling"));
		power_desc[num].level = 1;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_MAGE:
	/* case CLASS_HIGH_MAGE: */
	case CLASS_SORCERER:
	{
		strcpy(power_desc[num].name, _("魔力食い", "Eat Magic"));
		power_desc[num].level = 25;
		power_desc[num].cost = 1;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 25;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_PRIEST:
	{
		if (is_good_realm(p_ptr->realm1))
		{
			strcpy(power_desc[num].name, _("武器祝福", "Bless Weapon"));
			power_desc[num].level = 35;
			power_desc[num].cost = 70;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 50;
			power_desc[num++].number = -3;
		}
		else
		{
			strcpy(power_desc[num].name, _("召魂", "Evocation"));
			power_desc[num].level = 42;
			power_desc[num].cost = 40;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 35;
			power_desc[num++].number = -3;
		}
		break;
	}
	case CLASS_ROGUE:
	{
		strcpy(power_desc[num].name, _("ヒット＆アウェイ", "Hit and Away"));
		power_desc[num].level = 8;
		power_desc[num].cost = 12;
		power_desc[num].stat = A_DEX;
		power_desc[num].fail = 14;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_RANGER:
	case CLASS_SNIPER:
	{
		strcpy(power_desc[num].name, _("モンスター調査", "Probe Monster"));
		power_desc[num].level = 15;
		power_desc[num].cost = 20;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 12;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_PALADIN:
	{
		if (is_good_realm(p_ptr->realm1))
		{
			strcpy(power_desc[num].name, _("ホーリー・ランス", "Holy Lance"));
			power_desc[num].level = 30;
			power_desc[num].cost = 30;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 30;
			power_desc[num++].number = -3;
		}
		else
		{
			strcpy(power_desc[num].name, _("ヘル・ランス", "Hell Lance"));
			power_desc[num].level = 30;
			power_desc[num].cost = 30;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 30;
			power_desc[num++].number = -3;
		}
		break;
	}
	case CLASS_WARRIOR_MAGE:
	{
		strcpy(power_desc[num].name, _("変換: ＨＰ→ＭＰ", "Convert HP to SP"));
		power_desc[num].level = 25;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 10;
		power_desc[num++].number = -3;
			
		strcpy(power_desc[num].name, _("変換: ＭＰ→ＨＰ", "Convert SP to HP"));
		power_desc[num].level = 25;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 10;
		power_desc[num++].number = -4;
		break;
	}
	case CLASS_CHAOS_WARRIOR:
	{
		strcpy(power_desc[num].name, _("幻惑の光", "Confusing Light"));
		power_desc[num].level = 40;
		power_desc[num].cost = 50;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 25;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_MONK:
	{
		strcpy(power_desc[num].name, _("構える", "Assume a Stance"));
		power_desc[num].level = 25;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_DEX;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
			
		strcpy(power_desc[num].name, _("百裂拳", "Double Attack"));
		power_desc[num].level = 30;
		power_desc[num].cost = 30;
		power_desc[num].stat = A_STR;
		power_desc[num].fail = 20;
		power_desc[num++].number = -4;
		break;
	}
	case CLASS_MINDCRAFTER:
	case CLASS_FORCETRAINER:
	{
		strcpy(power_desc[num].name, _("明鏡止水", "Clear Mind"));
		power_desc[num].level = 15;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_WIS;
		power_desc[num].fail = 10;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_TOURIST:
	{
		strcpy(power_desc[num].name, _("写真撮影", "Take a Photograph"));
		power_desc[num].level = 1;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_DEX;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
		
		strcpy(power_desc[num].name, _("真・鑑定", "Identify True"));
		power_desc[num].level = 25;
		power_desc[num].cost = 20;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 20;
		power_desc[num++].number = -4;
		break;
	}
	case CLASS_IMITATOR:
	{
		strcpy(power_desc[num].name, _("倍返し", "Double Revenge"));
		power_desc[num].level = 30;
		power_desc[num].cost = 100;
		power_desc[num].stat = A_DEX;
		power_desc[num].fail = 30;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_BEASTMASTER:
	{
		strcpy(power_desc[num].name, _("生物支配", "Dominate a Living Thing"));
		power_desc[num].level = 1;
		power_desc[num].cost = (p_ptr->lev+3)/4;
		power_desc[num].stat = A_CHR;
		power_desc[num].fail = 10;
		power_desc[num++].number = -3;
		
		strcpy(power_desc[num].name, _("真・生物支配", "Dominate Living Things"));
		power_desc[num].level = 30;
		power_desc[num].cost = (p_ptr->lev+20)/2;
		power_desc[num].stat = A_CHR;
		power_desc[num].fail = 10;
		power_desc[num++].number = -4;
		break;
	}
	case CLASS_ARCHER:
	{
		strcpy(power_desc[num].name, _("弾/矢の製造", "Create Ammo"));
		power_desc[num].level = 1;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_DEX;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_MAGIC_EATER:
	{
		strcpy(power_desc[num].name, _("魔力の取り込み", "Absorb Magic"));
		power_desc[num].level = 1;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;

		strcpy(power_desc[num].name, _("強力発動", "Powerful Activation"));
		power_desc[num].level = 10;
		power_desc[num].cost = 10 + (lvl - 10) / 2;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 0;
		power_desc[num++].number = -4;
		break;
	}
	case CLASS_BARD:
	{
		strcpy(power_desc[num].name, _("歌を止める", "Stop Singing"));
		power_desc[num].level = 1;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_CHR;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_RED_MAGE:
	{
		strcpy(power_desc[num].name, _("連続魔", "Double Magic"));
		power_desc[num].level = 48;
		power_desc[num].cost = 20;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_SAMURAI:
	{
		strcpy(power_desc[num].name, _("気合いため", "Concentration"));
		power_desc[num].level = 1;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_WIS;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
		
		strcpy(power_desc[num].name, _("型", "Assume a Stance"));
		power_desc[num].level = 25;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_DEX;
		power_desc[num].fail = 0;
		power_desc[num++].number = -4;
		break;
	}
	case CLASS_BLUE_MAGE:
	{
		strcpy(power_desc[num].name, _("ラーニング", "Learning"));
		power_desc[num].level = 1;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_CAVALRY:
	{
		strcpy(power_desc[num].name, _("荒馬ならし", "Rodeo"));
		power_desc[num].level = 10;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_STR;
		power_desc[num].fail = 10;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_BERSERKER:
	{
		strcpy(power_desc[num].name, _("帰還", "Recall"));
		power_desc[num].level = 10;
		power_desc[num].cost = 10;
		power_desc[num].stat = A_DEX;
		power_desc[num].fail = 20;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_MIRROR_MASTER:
	{
		strcpy(power_desc[num].name, _("鏡割り", "Break Mirrors"));
		power_desc[num].level = 1;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
		
		strcpy(power_desc[num].name, _("静水", "Mirror Concentration"));
		power_desc[num].level = 30;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 20;
		power_desc[num++].number = -4;
		break;
	}
	case CLASS_SMITH:
	{
		strcpy(power_desc[num].name, _("目利き", "Judgment"));
		power_desc[num].level = 5;
		power_desc[num].cost = 15;
		power_desc[num].stat = A_INT;
		power_desc[num].fail = 20;
		power_desc[num++].number = -3;
		break;
	}
	case CLASS_NINJA:
	{
		strcpy(power_desc[num].name, _("速駆け", "Quick Walk"));
		power_desc[num].level = 20;
		power_desc[num].cost = 0;
		power_desc[num].stat = A_DEX;
		power_desc[num].fail = 0;
		power_desc[num++].number = -3;
		break;
	}
	default:
		strcpy(power_desc[0].name, _("(なし)", "(none)"));
	}

	if (p_ptr->mimic_form)
	{
		switch (p_ptr->mimic_form)
		{
		case MIMIC_DEMON:
		case MIMIC_DEMON_LORD:
			sprintf(power_desc[num].name, _("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), lvl * 3);
			power_desc[num].level = 15;
			power_desc[num].cost = 10+lvl/3;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 20;
			power_desc[num++].number = -1;
			break;
		case MIMIC_VAMPIRE:
			strcpy(power_desc[num].name, _("吸血", "Vampiric Drain"));
			power_desc[num].level = 2;
			power_desc[num].cost = 1 + (lvl / 3);
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 9;
			power_desc[num++].number = -1;
			break;
		}
	}
	else
	{
	switch (p_ptr->prace)
	{
		case RACE_DWARF:
			strcpy(power_desc[num].name, _("ドアと罠 感知", "Detect Doors+Traps"));
			power_desc[num].level = 5;
			power_desc[num].cost = 5;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 12;
			power_desc[num++].number = -1;
			break;
		case RACE_NIBELUNG:
			strcpy(power_desc[num].name, _("ドアと罠 感知", "Detect Doors+Traps"));
			power_desc[num].level = 10;
			power_desc[num].cost = 5;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 10;
			power_desc[num++].number = -1;
			break;
		case RACE_HOBBIT:
			strcpy(power_desc[num].name, _("食糧生成", "Create Food"));
			power_desc[num].level = 15;
			power_desc[num].cost = 10;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 10;
			power_desc[num++].number = -1;
			break;
		case RACE_GNOME:
			sprintf(power_desc[num].name, _("ショート・テレポート", "Blink"));
			power_desc[num].level = 5;
			power_desc[num].cost = 5;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 12;
			power_desc[num++].number = -1;
			break;
		case RACE_HALF_ORC:
			strcpy(power_desc[num].name, _("恐怖除去", "Remove Fear"));
			power_desc[num].level = 3;
			power_desc[num].cost = 5;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = warrior ? 5 : 10;
			power_desc[num++].number = -1;
			break;
		case RACE_HALF_TROLL:
			strcpy(power_desc[num].name, _("狂戦士化", "Berserk"));
			power_desc[num].level = 10;
			power_desc[num].cost = 12;
			power_desc[num].stat = A_STR;
			power_desc[num].fail = warrior ? 6 : 12;
			power_desc[num++].number = -1;
			break;
		case RACE_BARBARIAN:
			strcpy(power_desc[num].name, _("狂戦士化", "Berserk"));
			power_desc[num].level = 8;
			power_desc[num].cost = 10;
			power_desc[num].stat = A_STR;
			power_desc[num].fail = warrior ? 6 : 12;
			power_desc[num++].number = -1;
			break;
		case RACE_AMBERITE:
			strcpy(power_desc[num].name, _("シャドウ・シフト", "Shadow Shifting"));
			power_desc[num].level = 30;
			power_desc[num].cost = 50;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 50;
			power_desc[num++].number = -1;
			
			strcpy(power_desc[num].name, _("パターン・ウォーク", "Pattern Mindwalking"));
			power_desc[num].level = 40;
			power_desc[num].cost = 75;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 50;
			power_desc[num++].number = -2;
			break;
		case RACE_HALF_OGRE:
			strcpy(power_desc[num].name, _("爆発のルーン", "Explosive Rune"));
			power_desc[num].level = 25;
			power_desc[num].cost = 35;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 15;
			power_desc[num++].number = -1;
			break;
		case RACE_HALF_GIANT:
			strcpy(power_desc[num].name, _("岩石溶解", "Stone to Mud"));
			power_desc[num].level = 20;
			power_desc[num].cost = 10;
			power_desc[num].stat = A_STR;
			power_desc[num].fail = 12;
			power_desc[num++].number = -1;
			break;
		case RACE_HALF_TITAN:
			strcpy(power_desc[num].name, _("スキャン・モンスター", "Probing"));
			power_desc[num].level = 15;
			power_desc[num].cost = 10;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 12;
			power_desc[num++].number = -1;
			break;
		case RACE_CYCLOPS:
			sprintf(power_desc[num].name, _("岩石投げ（ダメージ %d）", "Throw Boulder (dam %d)"), (3 * lvl) / 2);
			power_desc[num].level = 20;
			power_desc[num].cost = 15;
			power_desc[num].stat = A_STR;
			power_desc[num].fail = 12;
			power_desc[num++].number = -1;
			break;
		case RACE_YEEK:
			strcpy(power_desc[num].name, _("モンスター恐慌", "Scare Monster"));
			power_desc[num].level = 15;
			power_desc[num].cost = 15;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 10;
			power_desc[num++].number = -1;
			break;
		case RACE_SPECTRE:
			strcpy(power_desc[num].name, _("モンスター恐慌", "Scare Monster"));
			power_desc[num].level = 4;
			power_desc[num].cost = 6;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 3;
			power_desc[num++].number = -1;
			break;
		case RACE_KLACKON:
			sprintf(power_desc[num].name, _("酸の唾 (ダメージ %d)", "Spit Acid (dam %d)"), lvl);
			power_desc[num].level = 9;
			power_desc[num].cost = 9;
			power_desc[num].stat = A_DEX;
			power_desc[num].fail = 14;
			power_desc[num++].number = -1;
			break;
		case RACE_KOBOLD:
			sprintf(power_desc[num].name, _("毒のダーツ (ダメージ %d)", "Poison Dart (dam %d)"), lvl);
			power_desc[num].level = 12;
			power_desc[num].cost = 8;
			power_desc[num].stat = A_DEX;
			power_desc[num].fail = 14;
			power_desc[num++].number = -1;
			break;
		case RACE_DARK_ELF:
			sprintf(power_desc[num].name, _("マジック・ミサイル (ダメージ %dd%d)", "Magic Missile (dm %dd%d)"), 3 + ((lvl - 1) / 5), 4);
			power_desc[num].level = 2;
			power_desc[num].cost = 2;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 9;
			power_desc[num++].number = -1;
			break;
		case RACE_DRACONIAN:
			sprintf(power_desc[num].name, _("ブレス (ダメージ %d)", "Breath Weapon (dam %d)"), lvl * 2);
			power_desc[num].level = 1;
			power_desc[num].cost = lvl;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 12;
			power_desc[num++].number = -1;
			break;
		case RACE_MIND_FLAYER:
			sprintf(power_desc[num].name, _("精神攻撃 (ダメージ %d)", "Mind Blast (dam %d)"), lvl);
			power_desc[num].level = 15;
			power_desc[num].cost = 12;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 14;
			power_desc[num++].number = -1;
			break;
		case RACE_IMP:
			sprintf(power_desc[num].name, _("ファイア・ボルト/ボール (ダメージ %d)", "Fire Bolt/Ball (dam %d)"), lvl);
			power_desc[num].level = 9;
			power_desc[num].cost = 15;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 15;
			power_desc[num++].number = -1;
			break;
		case RACE_GOLEM:
			strcpy(power_desc[num].name, _("肌石化 (期間 1d20+30)", "Stone Skin (dur 1d20+30)"));
			power_desc[num].level = 20;
			power_desc[num].cost = 15;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 8;
			power_desc[num++].number = -1;
			break;
		case RACE_SKELETON:
		case RACE_ZOMBIE:
			strcpy(power_desc[num].name, _("経験値復活", "Restore Experience"));
			power_desc[num].level = 30;
			power_desc[num].cost = 30;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 18;
			power_desc[num++].number = -1;
			break;
		case RACE_VAMPIRE:
			strcpy(power_desc[num].name, _("吸血", "Vampiric Drain"));
			power_desc[num].level = 2;
			power_desc[num].cost = 1 + (lvl / 3);
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 9;
			power_desc[num++].number = -1;
			break;
		case RACE_SPRITE:
			strcpy(power_desc[num].name, _("眠り粉", "Sleeping Dust"));
			power_desc[num].level = 12;
			power_desc[num].cost = 12;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 15;
			power_desc[num++].number = -1;
			break;
		case RACE_DEMON:
			sprintf(power_desc[num].name, _("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), lvl * 3);
			power_desc[num].level = 15;
			power_desc[num].cost = 10+lvl/3;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 20;
			power_desc[num++].number = -1;
			break;
		case RACE_KUTAR:
			strcpy(power_desc[num].name, _("横に伸びる", "Expand Horizontally (dur 30+1d20)"));
			power_desc[num].level = 20;
			power_desc[num].cost = 15;
			power_desc[num].stat = A_CHR;
			power_desc[num].fail = 8;
			power_desc[num++].number = -1;
			break;
		case RACE_ANDROID:
			if (p_ptr->lev < 10)
			{
				strcpy(power_desc[num].name, _("レイガン", "Ray Gun"));
				power_desc[num].level = 1;
				power_desc[num].cost = 7;
				power_desc[num].fail = 8;
			}
			else if (p_ptr->lev < 25)
			{
				strcpy(power_desc[num].name, _("ブラスター", "Blaster"));
				power_desc[num].level = 10;
				power_desc[num].cost = 13;
				power_desc[num].fail = 10;
			}
			else if (p_ptr->lev < 35)
			{
				strcpy(power_desc[num].name, _("バズーカ", "Bazooka"));
				power_desc[num].level = 25;
				power_desc[num].cost = 26;
				power_desc[num].fail = 12;
			}
			else if (p_ptr->lev < 45)
			{
				strcpy(power_desc[num].name, _("ビームキャノン", "Beam Cannon"));
				power_desc[num].level = 35;
				power_desc[num].cost = 40;
				power_desc[num].fail = 15;
			}
			else
			{
				strcpy(power_desc[num].name, _("ロケット", "Rocket"));
				power_desc[num].level = 45;
				power_desc[num].cost = 60;
				power_desc[num].fail = 18;
			}
			power_desc[num].stat = A_STR;
			power_desc[num++].number = -1;
			break;
		default:
		{
			break;
		}
	}
	}

	if (p_ptr->muta1)
	{
		if (p_ptr->muta1 & MUT1_SPIT_ACID)
		{
			strcpy(power_desc[num].name, _("酸の唾", "Spit Acid"));
			power_desc[num].level = 9;
			power_desc[num].cost = 9;
			power_desc[num].stat = A_DEX;
			power_desc[num].fail = 15;
			power_desc[num++].number = MUT1_SPIT_ACID;
		}

		if (p_ptr->muta1 & MUT1_BR_FIRE)
		{
			strcpy(power_desc[num].name, _("炎のブレス", "Fire Breath"));
			power_desc[num].level = 20;
			power_desc[num].cost = lvl;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 18;
			power_desc[num++].number = MUT1_BR_FIRE;
		}

		if (p_ptr->muta1 & MUT1_HYPN_GAZE)
		{
			strcpy(power_desc[num].name, _("催眠睨み", "Hypnotic Gaze"));
			power_desc[num].level = 12;
			power_desc[num].cost = 12;
			power_desc[num].stat = A_CHR;
			power_desc[num].fail = 18;
			power_desc[num++].number = MUT1_HYPN_GAZE;
		}

		if (p_ptr->muta1 & MUT1_TELEKINES)
		{
			strcpy(power_desc[num].name, _("念動力", "Telekinesis"));
			power_desc[num].level = 9;
			power_desc[num].cost = 9;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 14;
			power_desc[num++].number = MUT1_TELEKINES;
		}

		if (p_ptr->muta1 & MUT1_VTELEPORT)
		{
			strcpy(power_desc[num].name, _("テレポート", "Teleport"));
			power_desc[num].level = 7;
			power_desc[num].cost = 7;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 15;
			power_desc[num++].number = MUT1_VTELEPORT;
		}

		if (p_ptr->muta1 & MUT1_MIND_BLST)
		{
			strcpy(power_desc[num].name, _("精神攻撃", "Mind Blast"));
			power_desc[num].level = 5;
			power_desc[num].cost = 3;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 15;
			power_desc[num++].number = MUT1_MIND_BLST;
		}

		if (p_ptr->muta1 & MUT1_RADIATION)
		{
			strcpy(power_desc[num].name, _("放射能", "Emit Radiation"));
			power_desc[num].level = 15;
			power_desc[num].cost = 15;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 14;
			power_desc[num++].number = MUT1_RADIATION;
		}

		if (p_ptr->muta1 & MUT1_VAMPIRISM)
		{
			strcpy(power_desc[num].name, _("吸血", "Vampiric Drain"));
			power_desc[num].level = 2;
			power_desc[num].cost = (1 + (lvl / 3));
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 9;
			power_desc[num++].number = MUT1_VAMPIRISM;
		}

		if (p_ptr->muta1 & MUT1_SMELL_MET)
		{
			strcpy(power_desc[num].name, _("金属嗅覚", "Smell Metal"));
			power_desc[num].level = 3;
			power_desc[num].cost = 2;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 12;
			power_desc[num++].number = MUT1_SMELL_MET;
		}

		if (p_ptr->muta1 & MUT1_SMELL_MON)
		{
			strcpy(power_desc[num].name, _("敵臭嗅覚", "Smell Monsters"));
			power_desc[num].level = 5;
			power_desc[num].cost = 4;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 15;
			power_desc[num++].number = MUT1_SMELL_MON;
		}

		if (p_ptr->muta1 & MUT1_BLINK)
		{
			strcpy(power_desc[num].name, _("ショート・テレポート", "Blink"));
			power_desc[num].level = 3;
			power_desc[num].cost = 3;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 12;
			power_desc[num++].number = MUT1_BLINK;
		}

		if (p_ptr->muta1 & MUT1_EAT_ROCK)
		{
			strcpy(power_desc[num].name, _("岩食い", "Eat Rock"));
			power_desc[num].level = 8;
			power_desc[num].cost = 12;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 18;
			power_desc[num++].number = MUT1_EAT_ROCK;
		}

		if (p_ptr->muta1 & MUT1_SWAP_POS)
		{
			strcpy(power_desc[num].name, _("位置交換", "Swap Position"));
			power_desc[num].level = 15;
			power_desc[num].cost = 12;
			power_desc[num].stat = A_DEX;
			power_desc[num].fail = 16;
			power_desc[num++].number = MUT1_SWAP_POS;
		}

		if (p_ptr->muta1 & MUT1_SHRIEK)
		{
			strcpy(power_desc[num].name, _("叫び", "Shriek"));
			power_desc[num].level = 20;
			power_desc[num].cost = 14;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 16;
			power_desc[num++].number = MUT1_SHRIEK;
		}

		if (p_ptr->muta1 & MUT1_ILLUMINE)
		{
			strcpy(power_desc[num].name, _("照明", "Illuminate"));
			power_desc[num].level = 3;
			power_desc[num].cost = 2;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 10;
			power_desc[num++].number = MUT1_ILLUMINE;
		}

		if (p_ptr->muta1 & MUT1_DET_CURSE)
		{
			strcpy(power_desc[num].name, _("呪い感知", "Detect Curses"));
			power_desc[num].level = 7;
			power_desc[num].cost = 14;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 14;
			power_desc[num++].number = MUT1_DET_CURSE;
		}

		if (p_ptr->muta1 & MUT1_BERSERK)
		{
			strcpy(power_desc[num].name, _("狂戦士化", "Berserk"));
			power_desc[num].level = 8;
			power_desc[num].cost = 8;
			power_desc[num].stat = A_STR;
			power_desc[num].fail = 14;
			power_desc[num++].number = MUT1_BERSERK;
		}

		if (p_ptr->muta1 & MUT1_POLYMORPH)
		{
			strcpy(power_desc[num].name, _("変身", "Polymorph"));
			power_desc[num].level = 18;
			power_desc[num].cost = 20;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 18;
			power_desc[num++].number = MUT1_POLYMORPH;
		}

		if (p_ptr->muta1 & MUT1_MIDAS_TCH)
		{
			strcpy(power_desc[num].name, _("ミダスの手", "Midas Touch"));
			power_desc[num].level = 10;
			power_desc[num].cost = 5;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 12;
			power_desc[num++].number = MUT1_MIDAS_TCH;
		}

		if (p_ptr->muta1 & MUT1_GROW_MOLD)
		{
			strcpy(power_desc[num].name, _("カビ発生", "Grow Mold"));
			power_desc[num].level = 1;
			power_desc[num].cost = 6;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 14;
			power_desc[num++].number = MUT1_GROW_MOLD;
		}

		if (p_ptr->muta1 & MUT1_RESIST)
		{
			strcpy(power_desc[num].name, _("エレメント耐性", "Resist Elements"));
			power_desc[num].level = 10;
			power_desc[num].cost = 12;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 12;
			power_desc[num++].number = MUT1_RESIST;
		}

		if (p_ptr->muta1 & MUT1_EARTHQUAKE)
		{
			strcpy(power_desc[num].name, _("地震", "Earthquake"));
			power_desc[num].level = 12;
			power_desc[num].cost = 12;
			power_desc[num].stat = A_STR;
			power_desc[num].fail = 16;
			power_desc[num++].number = MUT1_EARTHQUAKE;
		}

		if (p_ptr->muta1 & MUT1_EAT_MAGIC)
		{
			strcpy(power_desc[num].name, _("魔力食い", "Eat Magic"));
			power_desc[num].level = 17;
			power_desc[num].cost = 1;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 15;
			power_desc[num++].number = MUT1_EAT_MAGIC;
		}

		if (p_ptr->muta1 & MUT1_WEIGH_MAG)
		{
			strcpy(power_desc[num].name, _("魔力感知", "Weigh Magic"));
			power_desc[num].level = 6;
			power_desc[num].cost = 6;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 10;
			power_desc[num++].number = MUT1_WEIGH_MAG;
		}

		if (p_ptr->muta1 & MUT1_STERILITY)
		{
			strcpy(power_desc[num].name, _("増殖阻止", "Sterilize"));
			power_desc[num].level = 12;
			power_desc[num].cost = 23;
			power_desc[num].stat = A_CHR;
			power_desc[num].fail = 15;
			power_desc[num++].number = MUT1_STERILITY;
		}

		if (p_ptr->muta1 & MUT1_PANIC_HIT)
		{
			strcpy(power_desc[num].name, _("ヒット＆アウェイ", "Panic Hit"));
			power_desc[num].level = 10;
			power_desc[num].cost = 12;
			power_desc[num].stat = A_DEX;
			power_desc[num].fail = 14;
			power_desc[num++].number = MUT1_PANIC_HIT;
		}

		if (p_ptr->muta1 & MUT1_DAZZLE)
		{
			strcpy(power_desc[num].name, _("眩惑", "Dazzle"));
			power_desc[num].level = 7;
			power_desc[num].cost = 15;
			power_desc[num].stat = A_CHR;
			power_desc[num].fail = 8;
			power_desc[num++].number = MUT1_DAZZLE;
		}

		if (p_ptr->muta1 & MUT1_LASER_EYE)
		{
			strcpy(power_desc[num].name, _("レーザー・アイ", "Laser Eye"));
			power_desc[num].level = 7;
			power_desc[num].cost = 10;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 9;
			power_desc[num++].number = MUT1_LASER_EYE;
		}

		if (p_ptr->muta1 & MUT1_RECALL)
		{
			strcpy(power_desc[num].name, _("帰還", "Recall"));
			power_desc[num].level = 17;
			power_desc[num].cost = 50;
			power_desc[num].stat = A_INT;
			power_desc[num].fail = 16;
			power_desc[num++].number = MUT1_RECALL;
		}

		if (p_ptr->muta1 & MUT1_BANISH)
		{
			strcpy(power_desc[num].name, _("邪悪消滅", "Banish Evil"));
			power_desc[num].level = 25;
			power_desc[num].cost = 25;
			power_desc[num].stat = A_WIS;
			power_desc[num].fail = 18;
			power_desc[num++].number = MUT1_BANISH;
		}

		if (p_ptr->muta1 & MUT1_COLD_TOUCH)
		{
			strcpy(power_desc[num].name, _("凍結の手", "Cold Touch"));
			power_desc[num].level = 2;
			power_desc[num].cost = 2;
			power_desc[num].stat = A_CON;
			power_desc[num].fail = 11;
			power_desc[num++].number = MUT1_COLD_TOUCH;
		}

		if (p_ptr->muta1 & MUT1_LAUNCHER)
		{
			strcpy(power_desc[num].name, _("アイテム投げ", "Throw Object"));
			power_desc[num].level = 1;
			power_desc[num].cost = lvl;
			power_desc[num].stat = A_STR;
			power_desc[num].fail = 6;
			/* XXX_XXX_XXX Hack! MUT1_LAUNCHER counts as negative... */
			power_desc[num++].number = 3;
		}
	}

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Build a prompt */
	(void) strnfmt(out_val, 78, 
				_("(特殊能力 %c-%c, *'で一覧, ESCで中断) どの特殊能力を使いますか？", "(Powers %c-%c, *=List, ESC=exit) Use which power? "),
				I2A(0), (num <= 26) ? I2A(num - 1) : '0' + num - 27);

if (!repeat_pull(&i) || i<0 || i>=num) {
	if (use_menu) screen_save();

	choice = (always_show_list || use_menu) ? ESCAPE:1;
	while (!flag)
	{
		if( choice==ESCAPE ) choice = ' '; 
		else if( !get_com(out_val, &choice, FALSE) )break; 

		if (use_menu && choice != ' ')
		{
			switch(choice)
			{
				case '0':
				{
					screen_load();
					free_turn(p_ptr);
					return;
				}

				case '8':
				case 'k':
				case 'K':
				{
					menu_line += (num - 1);
					break;
				}

				case '2':
				case 'j':
				case 'J':
				{
					menu_line++;
					break;
				}

				case '6':
				case 'l':
				case 'L':
				case '4':
				case 'h':
				case 'H':
				{
					if (menu_line > 18)
						menu_line -= 18;
					else if (menu_line+18 <= num)
						menu_line += 18;
					break;
				}

				case 'x':
				case 'X':
				case '\r':
				{
					i = menu_line - 1;
					ask = FALSE;
					break;
				}
			}
			if (menu_line > num) menu_line -= num;
		}
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask))
		{
			/* Show the list */
			if (!redraw || use_menu)
			{
				byte y = 1, x = 0;
				int ctr = 0;
				char dummy[80];
				char letter;
				TERM_LEN x1, y1;

				strcpy(dummy, "");
				redraw = TRUE;
				if (!use_menu) screen_save();

				/* Print header(s) */
				if (num < 18)
					prt(_("                            Lv   MP 失率", "                            Lv Cost Fail"), y++, x);
				else
				prt(_("                            Lv   MP 失率                            Lv   MP 失率", 
					  "                            Lv Cost Fail                            Lv Cost Fail"), y++, x);


				/* Print list */
				while (ctr < num)
				{
					x1 = ((ctr < 18) ? x : x + 40);
					y1 = ((ctr < 18) ? y + ctr : y + ctr - 18);

					if (use_menu)
					{
						if (ctr == (menu_line-1)) strcpy(dummy, _(" 》 ", " >  "));
						else strcpy(dummy, "    ");
					}
					else
					{
						/* letter/number for power selection */
						if (ctr < 26)
							letter = I2A(ctr);
						else
							letter = '0' + ctr - 26;
						sprintf(dummy, " %c) ",letter);
					}
					strcat(dummy, format("%-23.23s %2d %4d %3d%%",
						power_desc[ctr].name, power_desc[ctr].level, power_desc[ctr].cost,
						100 - racial_chance(&power_desc[ctr])));
					prt(dummy, y1, x1);
					ctr++;
				}
			}

			/* Hide the list */
			else
			{
				/* Hide list */
				redraw = FALSE;
				screen_load();
			}

			/* Redo asking */
			continue;
		}

		if (!use_menu)
		{
			if (choice == '\r' && num == 1)
			{
				choice = 'a';
			}

			if (isalpha(choice))
			{
				/* Note verify */
				ask = (isupper(choice));

				/* Lowercase */
				if (ask) choice = (char)tolower(choice);

				/* Extract request */
				i = (islower(choice) ? A2I(choice) : -1);
			}
			else
			{
				ask = FALSE; /* Can't uppercase digits */

				i = choice - '0' + 26;
			}
		}

		/* Totally Illegal */
		if ((i < 0) || (i >= num))
		{
			bell();
			continue;
		}

		/* Verify it */
		if (ask)
		{
			char tmp_val[160];

			/* Prompt */
			(void) strnfmt(tmp_val, 78, _("%sを使いますか？ ", "Use %s? "), power_desc[i].name);

			/* Belay that order */
			if (!get_check(tmp_val)) continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}
	if (redraw) screen_load();

	/* Abort if needed */
	if (!flag)
	{
		free_turn(p_ptr);
		return;
	}
	repeat_push(i);
	} /*if (!repeat_pull(&i) || ...)*/
	switch (racial_aux(&power_desc[i]))
	{
	case 1:
		if (power_desc[i].number < 0)
			cast = cmd_racial_power_aux(power_desc[i].number);
		else
			cast = mutation_power_aux(power_desc[i].number);
		break;
	case 0:
		cast = FALSE;
		break;
	case -1:
		cast = TRUE;
		break;
	}

	if (cast)
	{
		if (racial_cost)
		{
			int actual_racial_cost = racial_cost / 2 + randint1(racial_cost / 2);

			/* If mana is not enough, player consumes hit point! */
			if (p_ptr->csp < actual_racial_cost)
			{
				actual_racial_cost -= p_ptr->csp;
				p_ptr->csp = 0;
				take_hit(DAMAGE_USELIFE, actual_racial_cost, _("過度の集中", "concentrating too hard"), -1);
			}
			else p_ptr->csp -= actual_racial_cost;

			p_ptr->redraw |= (PR_HP | PR_MANA);
			p_ptr->window |= (PW_PLAYER | PW_SPELL);
		}
	}
	else free_turn(p_ptr);

	/* Success */
	return;
}
