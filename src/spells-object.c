﻿
#include "angband.h"
#include "artifact.h"
#include "spells-object.h"
#include "object-boost.h"
#include "object-hook.h"
#include "player-status.h"
#include "avatar.h"


typedef struct
{
	OBJECT_TYPE_VALUE tval;
	OBJECT_SUBTYPE_VALUE sval;
	PERCENTAGE prob;
	byte flag;
} amuse_type;

/*
 * Scatter some "amusing" objects near the player
 */

#define AMS_NOTHING   0x00 /* No restriction */
#define AMS_NO_UNIQUE 0x01 /* Don't make the amusing object of uniques */
#define AMS_FIXED_ART 0x02 /* Make a fixed artifact based on the amusing object */
#define AMS_MULTIPLE  0x04 /* Drop 1-3 objects for one type */
#define AMS_PILE      0x08 /* Drop 1-99 pile objects for one type */

static amuse_type amuse_info[] =
{
	{ TV_BOTTLE, SV_ANY, 5, AMS_NOTHING },
	{ TV_JUNK, SV_ANY, 3, AMS_MULTIPLE },
	{ TV_SPIKE, SV_ANY, 10, AMS_PILE },
	{ TV_STATUE, SV_ANY, 15, AMS_NOTHING },
	{ TV_CORPSE, SV_ANY, 15, AMS_NO_UNIQUE },
	{ TV_SKELETON, SV_ANY, 10, AMS_NO_UNIQUE },
	{ TV_FIGURINE, SV_ANY, 10, AMS_NO_UNIQUE },
	{ TV_PARCHMENT, SV_ANY, 1, AMS_NOTHING },
	{ TV_POLEARM, SV_TSURIZAO, 3, AMS_NOTHING }, //Fishing Pole of Taikobo
	{ TV_SWORD, SV_BROKEN_DAGGER, 3, AMS_FIXED_ART }, //Broken Dagger of Magician
	{ TV_SWORD, SV_BROKEN_DAGGER, 10, AMS_NOTHING },
	{ TV_SWORD, SV_BROKEN_SWORD, 5, AMS_NOTHING },
	{ TV_SCROLL, SV_SCROLL_AMUSEMENT, 10, AMS_NOTHING },

	{ 0, 0, 0 }
};

/*!
 * @brief「弾/矢の製造」処理 / do_cmd_cast calls this function if the player's class is 'archer'.
 * Hook to determine if an object is contertible in an arrow/bolt
 * @return 製造を実際に行ったらTRUE、キャンセルしたらFALSEを返す
 */
bool create_ammo(void)
{
	int ext = 0;
	char ch;

	object_type	forge;
	object_type *q_ptr;

	char com[80];
	GAME_TEXT o_name[MAX_NLEN];

	q_ptr = &forge;

	if (p_ptr->lev >= 20)
		sprintf(com, _("[S]弾, [A]矢, [B]クロスボウの矢 :", "Create [S]hots, Create [A]rrow or Create [B]olt ?"));
	else if (p_ptr->lev >= 10)
		sprintf(com, _("[S]弾, [A]矢:", "Create [S]hots or Create [A]rrow ?"));
	else
		sprintf(com, _("[S]弾:", "Create [S]hots ?"));

	if (p_ptr->confused)
	{
		msg_print(_("混乱してる！", "You are too confused!"));
		return FALSE;
	}

	if (p_ptr->blind)
	{
		msg_print(_("目が見えない！", "You are blind!"));
		return FALSE;
	}

	while (TRUE)
	{
		if (!get_com(com, &ch, TRUE))
		{
			return FALSE;
		}
		if (ch == 'S' || ch == 's')
		{
			ext = 1;
			break;
		}
		if ((ch == 'A' || ch == 'a') && (p_ptr->lev >= 10))
		{
			ext = 2;
			break;
		}
		if ((ch == 'B' || ch == 'b') && (p_ptr->lev >= 20))
		{
			ext = 3;
			break;
		}
	}

	/**********Create shots*********/
	if (ext == 1)
	{
		POSITION x, y;
		DIRECTION dir;
		grid_type *g_ptr;

		if (!get_rep_dir(&dir, FALSE)) return FALSE;
		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];
		g_ptr = &current_floor_ptr->grid_array[y][x];

		if (!have_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_CAN_DIG))
		{
			msg_print(_("そこには岩石がない。", "You need a pile of rubble."));
			return FALSE;
		}
		else if (!cave_have_flag_grid(g_ptr, FF_CAN_DIG) || !cave_have_flag_grid(g_ptr, FF_HURT_ROCK))
		{
			msg_print(_("硬すぎて崩せなかった。", "You failed to make ammo."));
		}
		else
		{
			s16b slot;
			q_ptr = &forge;

			/* Hack -- Give the player some small firestones */
			object_prep(q_ptr, lookup_kind(TV_SHOT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, p_ptr->lev) + 1));
			q_ptr->number = (byte)rand_range(15, 30);
			object_aware(q_ptr);
			object_known(q_ptr);
			apply_magic(q_ptr, p_ptr->lev, AM_NO_FIXED_ART);
			q_ptr->discount = 99;

			slot = inven_carry(q_ptr);

			object_desc(o_name, q_ptr, 0);
			msg_format(_("%sを作った。", "You make some ammo."), o_name);

			/* Auto-inscription */
			if (slot >= 0) autopick_alter_item(slot, FALSE);

			/* Destroy the wall */
			cave_alter_feat(y, x, FF_HURT_ROCK);

			p_ptr->update |= (PU_FLOW);
		}
	}
	/**********Create arrows*********/
	else if (ext == 2)
	{
		OBJECT_IDX item;
		concptr q, s;
		s16b slot;

		item_tester_hook = item_tester_hook_convertible;

		q = _("どのアイテムから作りますか？ ", "Convert which item? ");
		s = _("材料を持っていない。", "You have no item to convert.");
		q_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR));
		if (!q_ptr) return FALSE;

		q_ptr = &forge;

		/* Hack -- Give the player some small firestones */
		object_prep(q_ptr, lookup_kind(TV_ARROW, (OBJECT_SUBTYPE_VALUE)m_bonus(1, p_ptr->lev) + 1));
		q_ptr->number = (byte)rand_range(5, 10);
		object_aware(q_ptr);
		object_known(q_ptr);
		apply_magic(q_ptr, p_ptr->lev, AM_NO_FIXED_ART);

		q_ptr->discount = 99;

		object_desc(o_name, q_ptr, 0);
		msg_format(_("%sを作った。", "You make some ammo."), o_name);

		if (item >= 0)
		{
			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);
		}
		else
		{
			floor_item_increase(0 - item, -1);
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}

		slot = inven_carry(q_ptr);

		/* Auto-inscription */
		if (slot >= 0) autopick_alter_item(slot, FALSE);
	}
	/**********Create bolts*********/
	else if (ext == 3)
	{
		OBJECT_IDX item;
		concptr q, s;
		s16b slot;

		item_tester_hook = item_tester_hook_convertible;

		q = _("どのアイテムから作りますか？ ", "Convert which item? ");
		s = _("材料を持っていない。", "You have no item to convert.");

		q_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR));
		if (!q_ptr) return FALSE;

		q_ptr = &forge;

		/* Hack -- Give the player some small firestones */
		object_prep(q_ptr, lookup_kind(TV_BOLT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, p_ptr->lev) + 1));
		q_ptr->number = (byte)rand_range(4, 8);
		object_aware(q_ptr);
		object_known(q_ptr);
		apply_magic(q_ptr, p_ptr->lev, AM_NO_FIXED_ART);

		q_ptr->discount = 99;

		object_desc(o_name, q_ptr, 0);
		msg_format(_("%sを作った。", "You make some ammo."), o_name);

		if (item >= 0)
		{
			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);
		}
		else
		{
			floor_item_increase(0 - item, -1);
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}

		slot = inven_carry(q_ptr);

		/* Auto-inscription */
		if (slot >= 0) autopick_alter_item(slot, FALSE);
	}
	return TRUE;
}

/*!
 * @brief 魔道具術師の魔力取り込み処理
 * @return 取り込みを実行したらTRUE、キャンセルしたらFALSEを返す
 */
bool import_magic_device(void)
{
	OBJECT_IDX item;
	PARAMETER_VALUE pval;
	int ext = 0;
	concptr q, s;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];

	/* Only accept legal items */
	item_tester_hook = item_tester_hook_recharge;

	q = _("どのアイテムの魔力を取り込みますか? ", "Gain power of which item? ");
	s = _("魔力を取り込めるアイテムがない。", "You have nothing to gain power.");

	o_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR));
	if (!o_ptr) return (FALSE);

	if (o_ptr->tval == TV_STAFF && o_ptr->sval == SV_STAFF_NOTHING)
	{
		msg_print(_("この杖には発動の為の能力は何も備わっていないようだ。", "This staff doesn't have any magical ability."));
		return FALSE;
	}

	if (!object_is_known(o_ptr))
	{
		msg_print(_("鑑定されていないと取り込めない。", "You need to identify before absorbing."));
		return FALSE;
	}

	if (o_ptr->timeout)
	{
		msg_print(_("充填中のアイテムは取り込めない。", "This item is still charging."));
		return FALSE;
	}

	pval = o_ptr->pval;
	if (o_ptr->tval == TV_ROD)
		ext = 72;
	else if (o_ptr->tval == TV_WAND)
		ext = 36;

	if (o_ptr->tval == TV_ROD)
	{
		p_ptr->magic_num2[o_ptr->sval + ext] += (MAGIC_NUM2)o_ptr->number;
		if (p_ptr->magic_num2[o_ptr->sval + ext] > 99) p_ptr->magic_num2[o_ptr->sval + ext] = 99;
	}
	else
	{
		int num;
		for (num = o_ptr->number; num; num--)
		{
			int gain_num = pval;
			if (o_ptr->tval == TV_WAND) gain_num = (pval + num - 1) / num;
			if (p_ptr->magic_num2[o_ptr->sval + ext])
			{
				gain_num *= 256;
				gain_num = (gain_num / 3 + randint0(gain_num / 3)) / 256;
				if (gain_num < 1) gain_num = 1;
			}
			p_ptr->magic_num2[o_ptr->sval + ext] += (MAGIC_NUM2)gain_num;
			if (p_ptr->magic_num2[o_ptr->sval + ext] > 99) p_ptr->magic_num2[o_ptr->sval + ext] = 99;
			p_ptr->magic_num1[o_ptr->sval + ext] += pval * 0x10000;
			if (p_ptr->magic_num1[o_ptr->sval + ext] > 99 * 0x10000) p_ptr->magic_num1[o_ptr->sval + ext] = 99 * 0x10000;
			if (p_ptr->magic_num1[o_ptr->sval + ext] > p_ptr->magic_num2[o_ptr->sval + ext] * 0x10000) p_ptr->magic_num1[o_ptr->sval + ext] = p_ptr->magic_num2[o_ptr->sval + ext] * 0x10000;
			if (o_ptr->tval == TV_WAND) pval -= (pval + num - 1) / num;
		}
	}

	object_desc(o_name, o_ptr, 0);
	msg_format(_("%sの魔力を取り込んだ。", "You absorb magic of %s."), o_name);

	/* Eliminate the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -999);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Eliminate the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -999);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
	take_turn(p_ptr, 100);
	return TRUE;
}

/*!
 * @brief 誰得ドロップを行う。
 * @param y1 配置したいフロアのY座標
 * @param x1 配置したいフロアのX座標
 * @param num 誰得の処理回数
 * @param known TRUEならばオブジェクトが必ず＊鑑定＊済になる
 * @return なし
 */
void amusement(POSITION y1, POSITION x1, int num, bool known)
{
	object_type *i_ptr;
	object_type object_type_body;
	int n, t = 0;

	for (n = 0; amuse_info[n].tval != 0; n++)
	{
		t += amuse_info[n].prob;
	}

	/* Acquirement */
	while (num)
	{
		int i;
		KIND_OBJECT_IDX k_idx;
		ARTIFACT_IDX a_idx = 0;
		int r = randint0(t);
		bool insta_art, fixed_art;

		for (i = 0; ; i++)
		{
			r -= amuse_info[i].prob;
			if (r <= 0) break;
		}
		i_ptr = &object_type_body;
		object_wipe(i_ptr);
		k_idx = lookup_kind(amuse_info[i].tval, amuse_info[i].sval);

		/* Paranoia - reroll if nothing */
		if (!k_idx) continue;

		/* Search an artifact index if need */
		insta_art = (k_info[k_idx].gen_flags & TRG_INSTA_ART);
		fixed_art = (amuse_info[i].flag & AMS_FIXED_ART);

		if (insta_art || fixed_art)
		{
			for (a_idx = 1; a_idx < max_a_idx; a_idx++)
			{
				if (insta_art && !(a_info[a_idx].gen_flags & TRG_INSTA_ART)) continue;
				if (a_info[a_idx].tval != k_info[k_idx].tval) continue;
				if (a_info[a_idx].sval != k_info[k_idx].sval) continue;
				if (a_info[a_idx].cur_num > 0) continue;
				break;
			}

			if (a_idx >= max_a_idx) continue;
		}

		/* Make an object (if possible) */
		object_prep(i_ptr, k_idx);
		if (a_idx) i_ptr->name1 = a_idx;
		apply_magic(i_ptr, 1, AM_NO_FIXED_ART);

		if (amuse_info[i].flag & AMS_NO_UNIQUE)
		{
			if (r_info[i_ptr->pval].flags1 & RF1_UNIQUE) continue;
		}

		if (amuse_info[i].flag & AMS_MULTIPLE) i_ptr->number = randint1(3);
		if (amuse_info[i].flag & AMS_PILE) i_ptr->number = randint1(99);

		if (known)
		{
			object_aware(i_ptr);
			object_known(i_ptr);
		}

		/* Paranoia - reroll if nothing */
		if (!(i_ptr->k_idx)) continue;

		(void)drop_near(i_ptr, -1, y1, x1);

		num--;
	}
}



/*!
 * @brief 獲得ドロップを行う。
 * Scatter some "great" objects near the player
 * @param y1 配置したいフロアのY座標
 * @param x1 配置したいフロアのX座標
 * @param num 獲得の処理回数
 * @param great TRUEならば必ず高級品以上を落とす
 * @param special TRUEならば必ず特別品を落とす
 * @param known TRUEならばオブジェクトが必ず＊鑑定＊済になる
 * @return なし
 */
void acquirement(POSITION y1, POSITION x1, int num, bool great, bool special, bool known)
{
	object_type *i_ptr;
	object_type object_type_body;
	BIT_FLAGS mode = AM_GOOD | (great || special ? AM_GREAT : 0L) | (special ? AM_SPECIAL : 0L);

	/* Acquirement */
	while (num--)
	{
		i_ptr = &object_type_body;
		object_wipe(i_ptr);

		/* Make a good (or great) object (if possible) */
		if (!make_object(i_ptr, mode)) continue;

		if (known)
		{
			object_aware(i_ptr);
			object_known(i_ptr);
		}

		(void)drop_near(i_ptr, -1, y1, x1);
	}
}

void acquire_chaos_weapon(player_type *creature_ptr)
{
	object_type forge;
	object_type *q_ptr = &forge;
	OBJECT_TYPE_VALUE dummy = TV_SWORD;
	OBJECT_SUBTYPE_VALUE dummy2;
	switch (randint1(creature_ptr->lev))
	{
	case 0: case 1:
		dummy2 = SV_DAGGER;
		break;
	case 2: case 3:
		dummy2 = SV_MAIN_GAUCHE;
		break;
	case 4:
		dummy2 = SV_TANTO;
		break;
	case 5: case 6:
		dummy2 = SV_RAPIER;
		break;
	case 7: case 8:
		dummy2 = SV_SMALL_SWORD;
		break;
	case 9: case 10:
		dummy2 = SV_BASILLARD;
		break;
	case 11: case 12: case 13:
		dummy2 = SV_SHORT_SWORD;
		break;
	case 14: case 15:
		dummy2 = SV_SABRE;
		break;
	case 16: case 17:
		dummy2 = SV_CUTLASS;
		break;
	case 18:
		dummy2 = SV_WAKIZASHI;
		break;
	case 19:
		dummy2 = SV_KHOPESH;
		break;
	case 20:
		dummy2 = SV_TULWAR;
		break;
	case 21:
		dummy2 = SV_BROAD_SWORD;
		break;
	case 22: case 23:
		dummy2 = SV_LONG_SWORD;
		break;
	case 24: case 25:
		dummy2 = SV_SCIMITAR;
		break;
	case 26:
		dummy2 = SV_NINJATO;
		break;
	case 27:
		dummy2 = SV_KATANA;
		break;
	case 28: case 29:
		dummy2 = SV_BASTARD_SWORD;
		break;
	case 30:
		dummy2 = SV_GREAT_SCIMITAR;
		break;
	case 31:
		dummy2 = SV_CLAYMORE;
		break;
	case 32:
		dummy2 = SV_ESPADON;
		break;
	case 33:
		dummy2 = SV_TWO_HANDED_SWORD;
		break;
	case 34:
		dummy2 = SV_FLAMBERGE;
		break;
	case 35:
		dummy2 = SV_NO_DACHI;
		break;
	case 36:
		dummy2 = SV_EXECUTIONERS_SWORD;
		break;
	case 37:
		dummy2 = SV_ZWEIHANDER;
		break;
	case 38:
		dummy2 = SV_HAYABUSA;
		break;
	default:
		dummy2 = SV_BLADE_OF_CHAOS;
	}

	object_prep(q_ptr, lookup_kind(dummy, dummy2));
	q_ptr->to_h = 3 + randint1(current_floor_ptr->dun_level) % 10;
	q_ptr->to_d = 3 + randint1(current_floor_ptr->dun_level) % 10;
	one_resistance(q_ptr);
	q_ptr->name2 = EGO_CHAOTIC;
	(void)drop_near(q_ptr, -1, creature_ptr->y, creature_ptr->x);
}


/*!
 * @brief 防具呪縛処理 /
 * Curse the players armor
 * @return 実際に呪縛されたらTRUEを返す
 */
bool curse_armor(void)
{
	int i;
	object_type *o_ptr;

	GAME_TEXT o_name[MAX_NLEN];

	/* Curse the body armor */
	o_ptr = &inventory[INVEN_BODY];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);

	object_desc(o_name, o_ptr, OD_OMIT_PREFIX);

	/* Attempt a saving throw for artifacts */
	if (object_is_artifact(o_ptr) && (randint0(100) < 50))
	{
		/* Cool */
#ifdef JP
		msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！",
			"恐怖の暗黒オーラ", "防具", o_name);
#else
		msg_format("A %s tries to %s, but your %s resists the effects!",
			"terrible black aura", "surround your armor", o_name);
#endif

	}

	/* not artifact or failed save... */
	else
	{
		msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
		chg_virtue(V_ENCHANT, -5);

		/* Blast the armor */
		o_ptr->name1 = 0;
		o_ptr->name2 = EGO_BLASTED;
		o_ptr->to_a = 0 - randint1(5) - randint1(5);
		o_ptr->to_h = 0;
		o_ptr->to_d = 0;
		o_ptr->ac = 0;
		o_ptr->dd = 0;
		o_ptr->ds = 0;

		for (i = 0; i < TR_FLAG_SIZE; i++)
			o_ptr->art_flags[i] = 0;

		/* Curse it */
		o_ptr->curse_flags = TRC_CURSED;

		/* Break it */
		o_ptr->ident |= (IDENT_BROKEN);
		p_ptr->update |= (PU_BONUS | PU_MANA);
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	}

	return (TRUE);
}

/*!
 * @brief 武器呪縛処理 /
 * Curse the players weapon
 * @param force 無条件に呪縛を行うならばTRUE
 * @param o_ptr 呪縛する武器のアイテム情報参照ポインタ
 * @return 実際に呪縛されたらTRUEを返す
 */
bool curse_weapon_object(bool force, object_type *o_ptr)
{
	int i;
	GAME_TEXT o_name[MAX_NLEN];

	/* Nothing to curse */
	if (!o_ptr->k_idx) return (FALSE);
	object_desc(o_name, o_ptr, OD_OMIT_PREFIX);

	/* Attempt a saving throw */
	if (object_is_artifact(o_ptr) && (randint0(100) < 50) && !force)
	{
		/* Cool */
#ifdef JP
		msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！",
			"恐怖の暗黒オーラ", "武器", o_name);
#else
		msg_format("A %s tries to %s, but your %s resists the effects!",
			"terrible black aura", "surround your weapon", o_name);
#endif
	}

	/* not artifact or failed save... */
	else
	{
		if (!force) msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
		chg_virtue(V_ENCHANT, -5);

		/* Shatter the weapon */
		o_ptr->name1 = 0;
		o_ptr->name2 = EGO_SHATTERED;
		o_ptr->to_h = 0 - randint1(5) - randint1(5);
		o_ptr->to_d = 0 - randint1(5) - randint1(5);
		o_ptr->to_a = 0;
		o_ptr->ac = 0;
		o_ptr->dd = 0;
		o_ptr->ds = 0;

		for (i = 0; i < TR_FLAG_SIZE; i++)
			o_ptr->art_flags[i] = 0;

		/* Curse it */
		o_ptr->curse_flags = TRC_CURSED;

		/* Break it */
		o_ptr->ident |= (IDENT_BROKEN);
		p_ptr->update |= (PU_BONUS | PU_MANA);
		p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
	}

	return (TRUE);
}

/*!
 * @brief 武器呪縛処理のメインルーチン /
 * Curse the players weapon
 * @param force 無条件に呪縛を行うならばTRUE
 * @param slot 呪縛する武器の装備スロット
 * @return 実際に呪縛されたらTRUEを返す
 */
bool curse_weapon(bool force, int slot)
{
	return curse_weapon_object(force, &inventory[slot]);
}


/*!
 * @brief 防具の錆止め防止処理
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool rustproof(void)
{
	OBJECT_IDX item;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	concptr q, s;

	/* Select a piece of armour */
	item_tester_hook = object_is_armour;

	q = _("どの防具に錆止めをしますか？", "Rustproof which piece of armour? ");
	s = _("錆止めできるものがありません。", "You have nothing to rustproof.");

	o_ptr = choose_object(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
	if (!o_ptr) return FALSE;

	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

	add_flag(o_ptr->art_flags, TR_IGNORE_ACID);

	if ((o_ptr->to_a < 0) && !object_is_cursed(o_ptr))
	{
#ifdef JP
		msg_format("%sは新品同様になった！", o_name);
#else
		msg_format("%s %s look%s as good as new!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif

		o_ptr->to_a = 0;
	}

#ifdef JP
	msg_format("%sは腐食しなくなった。", o_name);
#else
	msg_format("%s %s %s now protected against corrosion.", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "are" : "is"));
#endif

	calc_android_exp();
	return TRUE;
}

/*!
 * @brief ボルトのエゴ化処理(火炎エゴのみ) /
 * Enchant some bolts
 * @return 常にTRUEを返す
 */
bool brand_bolts(void)
{
	int i;

	/* Use the first acceptable bolts */
	for (i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-bolts */
		if (o_ptr->tval != TV_BOLT) continue;

		/* Skip artifacts and ego-items */
		if (object_is_artifact(o_ptr) || object_is_ego(o_ptr))
			continue;

		/* Skip cursed/broken items */
		if (object_is_cursed(o_ptr) || object_is_broken(o_ptr)) continue;

		/* Randomize */
		if (randint0(100) < 75) continue;

		msg_print(_("クロスボウの矢が炎のオーラに包まれた！", "Your bolts are covered in a fiery aura!"));

		/* Ego-item */
		o_ptr->name2 = EGO_FLAME;
		enchant(o_ptr, randint0(3) + 4, ENCH_TOHIT | ENCH_TODAM);
		return (TRUE);
	}

	if (flush_failure) flush();

	/* Fail */
	msg_print(_("炎で強化するのに失敗した。", "The fiery enchantment failed."));

	return (TRUE);
}


bool perilous_secrets(player_type *creature_ptr)
{
	if (!ident_spell(FALSE)) return FALSE;

	if (mp_ptr->spell_book)
	{
		/* Sufficient mana */
		if (20 <= creature_ptr->csp)
		{
			/* Use some mana */
			creature_ptr->csp -= 20;
		}

		/* Over-exert the player */
		else
		{
			int oops = 20 - creature_ptr->csp;

			/* No mana left */
			creature_ptr->csp = 0;
			creature_ptr->csp_frac = 0;

			msg_print(_("石を制御できない！", "You are too weak to control the stone!"));
			/* Hack -- Bypass free action */
			(void)set_paralyzed(creature_ptr->paralyzed + randint1(5 * oops + 1));

			/* Confusing. */
			(void)set_confused(creature_ptr->confused + randint1(5 * oops + 1));
		}
		creature_ptr->redraw |= (PR_MANA);
	}
	take_hit(DAMAGE_LOSELIFE, damroll(1, 12), _("危険な秘密", "perilous secrets"), -1);
	/* Confusing. */
	if (one_in_(5)) (void)set_confused(creature_ptr->confused + randint1(10));

	/* Exercise a little care... */
	if (one_in_(20)) take_hit(DAMAGE_LOSELIFE, damroll(4, 10), _("危険な秘密", "perilous secrets"), -1);
	return TRUE;

}

/*!
 * @brief 固定アーティファクト『ブラッディムーン』の特性を変更する。
 * @details スレイ2d2種、及びone_resistance()による耐性1d2種、pval2種を得る。
 * @param o_ptr 対象のオブジェクト構造体（ブラッディムーン）のポインタ
 * @return なし
 */
void get_bloody_moon_flags(object_type *o_ptr)
{
	int dummy, i;

	for (i = 0; i < TR_FLAG_SIZE; i++)
		o_ptr->art_flags[i] = a_info[ART_BLOOD].flags[i];

	dummy = randint1(2) + randint1(2);
	for (i = 0; i < dummy; i++)
	{
		int flag = randint0(26);
		if (flag >= 20) add_flag(o_ptr->art_flags, TR_KILL_UNDEAD + flag - 20);
		else if (flag == 19) add_flag(o_ptr->art_flags, TR_KILL_ANIMAL);
		else if (flag == 18) add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
		else add_flag(o_ptr->art_flags, TR_CHAOTIC + flag);
	}

	dummy = randint1(2);
	for (i = 0; i < dummy; i++) one_resistance(o_ptr);

	for (i = 0; i < 2; i++)
	{
		int tmp = randint0(11);
		if (tmp < A_MAX) add_flag(o_ptr->art_flags, TR_STR + tmp);
		else add_flag(o_ptr->art_flags, TR_STEALTH + tmp - 6);
	}
}

/*!
 * @brief 寿命つき光源の燃素追加処理 /
 * Charge a lite (torch or latern)
 * @return なし
 */
void phlogiston(void)
{
	GAME_TURN max_flog = 0;
	object_type * o_ptr = &inventory[INVEN_LITE];

	/* It's a lamp */
	if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_LANTERN))
	{
		max_flog = FUEL_LAMP;
	}

	/* It's a torch */
	else if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH))
	{
		max_flog = FUEL_TORCH;
	}

	/* No torch to refill */
	else
	{
		msg_print(_("燃素を消費するアイテムを装備していません。", "You are not wielding anything which uses phlogiston."));
		return;
	}

	if (o_ptr->xtra4 >= max_flog)
	{
		msg_print(_("このアイテムにはこれ以上燃素を補充できません。", "No more phlogiston can be put in this item."));
		return;
	}

	/* Refuel */
	o_ptr->xtra4 += (XTRA16)(max_flog / 2);
	msg_print(_("照明用アイテムに燃素を補充した。", "You add phlogiston to your light item."));

	if (o_ptr->xtra4 >= max_flog)
	{
		o_ptr->xtra4 = (XTRA16)max_flog;
		msg_print(_("照明用アイテムは満タンになった。", "Your light item is full."));
	}

	p_ptr->update |= (PU_TORCH);
}

/*!
 * @brief 武器の祝福処理 /
 * Bless a weapon
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool bless_weapon(void)
{
	OBJECT_IDX item;
	object_type *o_ptr;
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	GAME_TEXT o_name[MAX_NLEN];
	concptr q, s;

	/* Bless only weapons */
	item_tester_hook = object_is_weapon;

	q = _("どのアイテムを祝福しますか？", "Bless which weapon? ");
	s = _("祝福できる武器がありません。", "You have weapon to bless.");

	o_ptr = choose_object(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
	if (!o_ptr) return FALSE;

	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
	object_flags(o_ptr, flgs);

	if (object_is_cursed(o_ptr))
	{
		if (((o_ptr->curse_flags & TRC_HEAVY_CURSE) && (randint1(100) < 33)) ||
			have_flag(flgs, TR_ADD_L_CURSE) ||
			have_flag(flgs, TR_ADD_H_CURSE) ||
			(o_ptr->curse_flags & TRC_PERMA_CURSE))
		{
#ifdef JP
			msg_format("%sを覆う黒いオーラは祝福を跳ね返した！", o_name);
#else
			msg_format("The black aura on %s %s disrupts the blessing!", ((item >= 0) ? "your" : "the"), o_name);
#endif

			return TRUE;
		}

#ifdef JP
		msg_format("%s から邪悪なオーラが消えた。", o_name);
#else
		msg_format("A malignant aura leaves %s %s.", ((item >= 0) ? "your" : "the"), o_name);
#endif


		o_ptr->curse_flags = 0L;

		o_ptr->ident |= (IDENT_SENSE);
		o_ptr->feeling = FEEL_NONE;

		/* Recalculate the bonuses */
		p_ptr->update |= (PU_BONUS);
		p_ptr->window |= (PW_EQUIP);
	}

	/*
	 * Next, we try to bless it. Artifacts have a 1/3 chance of
	 * being blessed, otherwise, the operation simply disenchants
	 * them, godly power negating the magic. Ok, the explanation
	 * is silly, but otherwise priests would always bless every
	 * artifact weapon they find. Ego weapons and normal weapons
	 * can be blessed automatically.
	 */
	if (have_flag(flgs, TR_BLESSED))
	{
#ifdef JP
		msg_format("%s は既に祝福されている。", o_name);
#else
		msg_format("%s %s %s blessed already.",
			((item >= 0) ? "Your" : "The"), o_name,
			((o_ptr->number > 1) ? "were" : "was"));
#endif

		return TRUE;
	}

	if (!(object_is_artifact(o_ptr) || object_is_ego(o_ptr)) || one_in_(3))
	{
#ifdef JP
		msg_format("%sは輝いた！", o_name);
#else
		msg_format("%s %s shine%s!",
			((item >= 0) ? "Your" : "The"), o_name,
			((o_ptr->number > 1) ? "" : "s"));
#endif

		add_flag(o_ptr->art_flags, TR_BLESSED);
		o_ptr->discount = 99;
	}
	else
	{
		bool dis_happened = FALSE;
		msg_print(_("その武器は祝福を嫌っている！", "The weapon resists your blessing!"));

		/* Disenchant tohit */
		if (o_ptr->to_h > 0)
		{
			o_ptr->to_h--;
			dis_happened = TRUE;
		}

		if ((o_ptr->to_h > 5) && (randint0(100) < 33)) o_ptr->to_h--;

		/* Disenchant todam */
		if (o_ptr->to_d > 0)
		{
			o_ptr->to_d--;
			dis_happened = TRUE;
		}

		if ((o_ptr->to_d > 5) && (randint0(100) < 33)) o_ptr->to_d--;

		/* Disenchant toac */
		if (o_ptr->to_a > 0)
		{
			o_ptr->to_a--;
			dis_happened = TRUE;
		}

		if ((o_ptr->to_a > 5) && (randint0(100) < 33)) o_ptr->to_a--;

		if (dis_happened)
		{
			msg_print(_("周囲が凡庸な雰囲気で満ちた...", "There is a static feeling in the air..."));

#ifdef JP
			msg_format("%s は劣化した！", o_name);
#else
			msg_format("%s %s %s disenchanted!", ((item >= 0) ? "Your" : "The"), o_name,
				((o_ptr->number > 1) ? "were" : "was"));
#endif

		}
	}

	p_ptr->update |= (PU_BONUS);
	p_ptr->window |= (PW_EQUIP | PW_PLAYER);
	calc_android_exp();

	return TRUE;
}


/*!
 * @brief 盾磨き処理 /
 * pulish shield
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool pulish_shield(void)
{
	OBJECT_IDX item;
	object_type *o_ptr;
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	GAME_TEXT o_name[MAX_NLEN];
	concptr            q, s;

	/* Assume enchant weapon */
	item_tester_tval = TV_SHIELD;

	q = _("どの盾を磨きますか？", "Pulish which weapon? ");
	s = _("磨く盾がありません。", "You have weapon to pulish.");

	o_ptr = choose_object(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
	if (!o_ptr) return FALSE;

	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
	object_flags(o_ptr, flgs);

	if (o_ptr->k_idx && !object_is_artifact(o_ptr) && !object_is_ego(o_ptr) &&
		!object_is_cursed(o_ptr) && (o_ptr->sval != SV_MIRROR_SHIELD))
	{
#ifdef JP
		msg_format("%sは輝いた！", o_name);
#else
		msg_format("%s %s shine%s!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif
		o_ptr->name2 = EGO_REFLECTION;
		enchant(o_ptr, randint0(3) + 4, ENCH_TOAC);

		o_ptr->discount = 99;
		chg_virtue(V_ENCHANT, 2);

		return TRUE;
	}
	else
	{
		if (flush_failure) flush();

		msg_print(_("失敗した。", "Failed."));
		chg_virtue(V_ENCHANT, -2);
	}
	calc_android_exp();

	return FALSE;
}
