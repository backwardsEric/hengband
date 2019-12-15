﻿/*!
 * @file monster1.c
 * @brief モンスター情報の記述 / describe monsters (using monster memory)
 * @date 2013/12/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "cmd-pet.h"
#include "monsterrace-hook.h"
#include "objectkind-hook.h"
#include "projection.h"
#include "monster.h"
#include "monster-spell.h"
#include "spells-summon.h"
#include "patron.h"
#include "quest.h"
#include "artifact.h"
#include "avatar.h"
#include "wild.h"


/*
 * Pronoun arrays, by gender.
 */
static concptr wd_he[3] =
#ifdef JP
{ "それ", "彼", "彼女" };
#else
{ "it", "he", "she" };
#endif

static concptr wd_his[3] =
#ifdef JP
{ "それの", "彼の", "彼女の" };
#else
{ "its", "his", "her" };
#endif



/*!
 * 英語の複数系記述用マクロ / Pluralizer.  Args(count, singular, plural)
 */
#define plural(c,s,p) \
    (((c) == 1) ? (s) : (p))



/*!
 * @brief モンスターのAC情報を得ることができるかを返す / Determine if the "armor" is known
 * @param r_idx モンスターの種族ID
 * @return 敵のACを知る条件が満たされているならTRUEを返す
 * @details
 * The higher the level, the fewer kills needed.
 */
static bool know_armour(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	DEPTH level = r_ptr->level;
	MONSTER_NUMBER kills = r_ptr->r_tkills;

    bool known = (r_ptr->r_cast_spell == MAX_UCHAR)? TRUE: FALSE;

	if (cheat_know || known) return (TRUE);

	/* Normal monsters */
	if (kills > 304 / (4 + level)) return (TRUE);

	/* Skip non-uniques */
	if (!(r_ptr->flags1 & RF1_UNIQUE)) return (FALSE);

	/* Unique monsters */
	if (kills > 304 / (38 + (5 * level) / 4)) return (TRUE);

	/* Assume false */
	return (FALSE);
}


/*!
 * @brief モンスターの打撃威力を知ることができるかどうかを返す
 * Determine if the "damage" of the given attack is known
 * @param r_idx モンスターの種族ID
 * @param i 確認したい攻撃手番
 * @return 敵のダメージダイスを知る条件が満たされているならTRUEを返す
 * @details
 * <pre>
 * the higher the level of the monster, the fewer the attacks you need,
 * the more damage an attack does, the more attacks you need
 * </pre>
 */
static bool know_damage(MONRACE_IDX r_idx, int i)
{
	monster_race *r_ptr = &r_info[r_idx];

	DEPTH level = r_ptr->level;

	s32b a = r_ptr->r_blows[i];

	s32b d1 = r_ptr->blow[i].d_dice;
	s32b d2 = r_ptr->blow[i].d_side;

	s32b d = d1 * d2;

	if (d >= ((4+level)*MAX_UCHAR)/80) d = ((4+level)*MAX_UCHAR-1)/80;

	/* Normal monsters */
	if ((4 + level) * a > 80 * d) return (TRUE);

	/* Skip non-uniques */
	if (!(r_ptr->flags1 & RF1_UNIQUE)) return (FALSE);

	/* Unique monsters */
	if ((4 + level) * (2 * a) > 80 * d) return (TRUE);

	/* Assume false */
	return (FALSE);
}


/*
 * Prepare hook for c_roff(). It will be changed for spoiler generation in wizard1.c.
 */
void (*hook_c_roff)(TERM_COLOR attr, concptr str) = c_roff;

/*!
 * @brief モンスターの思い出メッセージをあらかじめ指定された関数ポインタに基づき出力する
 * @param str 出力文字列
 * @return なし
 */
static void hooked_roff(concptr str)
{
	/* Spawn */
	hook_c_roff(TERM_WHITE, str);
}

/*!
* @brief ダイス目を文字列に変換する
* @param base_damage 固定値
* @param dice_num ダイス数
* @param dice_side ダイス面
* @param dice_mult ダイス倍率
* @param dice_div ダイス除数
* @param msg 文字列を格納するポインタ
* @return なし
*/
void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char* msg)
{
	char base[80] = "", dice[80] = "", mult[80] = "";

	if (dice_num == 0)
	{
		sprintf(msg, "%d", base_damage);
	}
	else
	{
		if (base_damage != 0)
			sprintf(base, "%d+", base_damage);

		if (dice_num == 1)
			sprintf(dice, "d%d", dice_side);
		else
			sprintf(dice, "%dd%d", dice_num, dice_side);

		if (dice_mult != 1 || dice_div != 1)
		{
			if (dice_div == 1)
				sprintf(mult, "*%d", dice_mult);
			else
				sprintf(mult, "*(%d/%d)", dice_mult, dice_div);
		}
		sprintf(msg, "%s%s%s", base, dice, mult);
	}
}

/*!
* @brief 文字列にモンスターの攻撃力を加える
* @param r_idx モンスターの種族ID
* @param SPELL_NUM 呪文番号
* @param msg 表示する文字列
* @param tmp 返すメッセージを格納する配列
* @return なし
*/
void set_damage(MONRACE_IDX r_idx, int SPELL_NUM, char* msg, char* tmp)
{
	int base_damage = monspell_race_damage(SPELL_NUM, r_idx, BASE_DAM);
	int dice_num = monspell_race_damage(SPELL_NUM, r_idx, DICE_NUM);
	int dice_side = monspell_race_damage(SPELL_NUM, r_idx, DICE_SIDE);
	int dice_mult = monspell_race_damage(SPELL_NUM, r_idx, DICE_MULT);
	int dice_div = monspell_race_damage(SPELL_NUM, r_idx, DICE_DIV);
	char dmg_str[80], dice_str[80];
	dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div, dmg_str);
	sprintf(dice_str, "(%s)", dmg_str);

	if (know_armour(r_idx))
		sprintf(tmp, msg, dice_str);
	else
		sprintf(tmp, msg, "");
}

/*!
 * @brief モンスターの思い出情報を表示する
 * Hack -- display monster information using "hooked_roff()"
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 * @return なし
 * @details
 * This function should only be called with the cursor placed at the
 * left edge of the screen, on a cleared line, in which the recall is
 * to take place.  One extra blank line is left after the recall.
 */
static void roff_aux(MONRACE_IDX r_idx, BIT_FLAGS mode)
{
	monster_race    *r_ptr = &r_info[r_idx];
	bool            old = FALSE;
	int             m, n, r;
	concptr            p, q;

#ifdef JP
	char            jverb_buf[64];
#else
	bool            sin = FALSE;
#endif
	int             msex = 0;

	bool nightmare = ironman_nightmare && !(mode & 0x02);
	SPEED speed = nightmare ? r_ptr->speed + 5 : r_ptr->speed;

	bool            breath = FALSE;
	bool            magic = FALSE;
	bool            reinforce = FALSE;

	BIT_FLAGS flags1;
	BIT_FLAGS flags2;
	BIT_FLAGS flags3;
	BIT_FLAGS flags4;
	BIT_FLAGS a_ability_flags1;
	BIT_FLAGS a_ability_flags2;
	BIT_FLAGS flags7;
	BIT_FLAGS flagsr;

	ITEM_NUMBER drop_gold, drop_item;

	int		vn = 0;
	byte		color[96];
	concptr		vp[96];
	char tmp_msg[96][96];

	bool know_everything = FALSE;

	/* Obtain a copy of the "known" number of drops */
	drop_gold = r_ptr->r_drop_gold;
	drop_item = r_ptr->r_drop_item;

	/* Obtain a copy of the "known" flags */
	flags1 = (r_ptr->flags1 & r_ptr->r_flags1);
	flags2 = (r_ptr->flags2 & r_ptr->r_flags2);
	flags3 = (r_ptr->flags3 & r_ptr->r_flags3);
	flags4 = (r_ptr->flags4 & r_ptr->r_flags4);
	a_ability_flags1 = (r_ptr->a_ability_flags1 & r_ptr->r_flags5);
	a_ability_flags2 = (r_ptr->a_ability_flags2 & r_ptr->r_flags6);
	flags7 = (r_ptr->flags7 & r_ptr->flags7);
	flagsr = (r_ptr->flagsr & r_ptr->r_flagsr);

	for(n = 0; n < A_MAX; n++)
	{
		if(r_ptr->reinforce_id[n] > 0) reinforce = TRUE;
	}

	/* cheat_know or research_mon() */
	if (cheat_know || (mode & 0x01))
		know_everything = TRUE;

	/* Cheat -- Know everything */
	if (know_everything)
	{
		/* Hack -- maximal drops */
		drop_gold = drop_item =
		(((r_ptr->flags1 & RF1_DROP_4D2) ? 8 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_3D2) ? 6 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_2D2) ? 4 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_1D2) ? 2 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_90)  ? 1 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_60)  ? 1 : 0));

		/* Hack -- but only "valid" drops */
		if (r_ptr->flags1 & RF1_ONLY_GOLD) drop_item = 0;
		if (r_ptr->flags1 & RF1_ONLY_ITEM) drop_gold = 0;

		/* Hack -- know all the flags */
		flags1 = r_ptr->flags1;
		flags2 = r_ptr->flags2;
		flags3 = r_ptr->flags3;
		flags4 = r_ptr->flags4;
		a_ability_flags1 = r_ptr->a_ability_flags1;
		a_ability_flags2 = r_ptr->a_ability_flags2;
		flagsr = r_ptr->flagsr;
	}


	/* Extract a gender (if applicable) */
	if (r_ptr->flags1 & RF1_FEMALE) msex = 2;
	else if (r_ptr->flags1 & RF1_MALE) msex = 1;

	/* Assume some "obvious" flags */
	if (r_ptr->flags1 & RF1_UNIQUE)  flags1 |= (RF1_UNIQUE);
	if (r_ptr->flags1 & RF1_QUESTOR) flags1 |= (RF1_QUESTOR);
	if (r_ptr->flags1 & RF1_MALE)    flags1 |= (RF1_MALE);
	if (r_ptr->flags1 & RF1_FEMALE)  flags1 |= (RF1_FEMALE);

	/* Assume some "creation" flags */
	if (r_ptr->flags1 & RF1_FRIENDS) flags1 |= (RF1_FRIENDS);
	if (r_ptr->flags1 & RF1_ESCORT)  flags1 |= (RF1_ESCORT);
	if (r_ptr->flags1 & RF1_ESCORTS) flags1 |= (RF1_ESCORTS);

	/* Killing a monster reveals some properties */
	if (r_ptr->r_tkills || know_everything)
	{
		/* Know "race" flags */
		if (r_ptr->flags3 & RF3_ORC)      flags3 |= (RF3_ORC);
		if (r_ptr->flags3 & RF3_TROLL)    flags3 |= (RF3_TROLL);
		if (r_ptr->flags3 & RF3_GIANT)    flags3 |= (RF3_GIANT);
		if (r_ptr->flags3 & RF3_DRAGON)   flags3 |= (RF3_DRAGON);
		if (r_ptr->flags3 & RF3_DEMON)    flags3 |= (RF3_DEMON);
		if (r_ptr->flags3 & RF3_UNDEAD)   flags3 |= (RF3_UNDEAD);
		if (r_ptr->flags3 & RF3_EVIL)     flags3 |= (RF3_EVIL);
		if (r_ptr->flags3 & RF3_GOOD)     flags3 |= (RF3_GOOD);
		if (r_ptr->flags3 & RF3_ANIMAL)   flags3 |= (RF3_ANIMAL);
		if (r_ptr->flags3 & RF3_AMBERITE) flags3 |= (RF3_AMBERITE);
		if (r_ptr->flags2 & RF2_HUMAN)    flags2 |= (RF2_HUMAN);

		/* Know 'quantum' flag */
		if (r_ptr->flags2 & RF2_QUANTUM)  flags2 |= (RF2_QUANTUM);

		/* Know "forced" flags */
		if (r_ptr->flags1 & RF1_FORCE_DEPTH) flags1 |= (RF1_FORCE_DEPTH);
		if (r_ptr->flags1 & RF1_FORCE_MAXHP) flags1 |= (RF1_FORCE_MAXHP);
	}

	/* For output_monster_spoiler() */
	if (mode & 0x02)
	{
		/* Nothing to do */
	}
	else

	/* Treat uniques differently */
	if (flags1 & RF1_UNIQUE)
	{
		/* Hack -- Determine if the unique is "dead" */
		bool dead = (r_ptr->max_num == 0) ? TRUE : FALSE;

		/* We've been killed... */
		if (r_ptr->r_deaths)
		{
			/* Killed ancestors */
			hooked_roff(format(_("%^sはあなたの先祖を %d 人葬っている", "%^s has slain %d of your ancestors"),
					   wd_he[msex], r_ptr->r_deaths));

			/* But we've also killed it */
			if (dead)
			{
				hooked_roff(
				        _(format("が、すでに仇討ちは果たしている！"), 
				          format(", but you have avenged %s!  ", plural(r_ptr->r_deaths, "him", "them"))));
			}

			/* Unavenged (ever) */
			else
			{
				hooked_roff(
				        _(format("のに、まだ仇討ちを果たしていない。"), 
					  format(", who %s unavenged.  ", plural(r_ptr->r_deaths, "remains", "remain"))));
			}

			/* Start a new line */
			hooked_roff("\n");
		}

		/* Dead unique who never hurt us */
		else if (dead)
		{
			hooked_roff(_("あなたはこの仇敵をすでに葬り去っている。", "You have slain this foe.  "));

			/* Start a new line */
			hooked_roff("\n");
		}
	}

	/* Not unique, but killed us */
	else if (r_ptr->r_deaths)
	{
		/* Dead ancestors */
		hooked_roff(
			_(format("このモンスターはあなたの先祖を %d 人葬っている", r_ptr->r_deaths),
			  format("%d of your ancestors %s been killed by this creature, ", r_ptr->r_deaths, plural(r_ptr->r_deaths, "has", "have"))));

		/* Some kills this life */
		if (r_ptr->r_pkills)
		{
			hooked_roff(format(
				_("が、あなたはこのモンスターを少なくとも %d 体は倒している。", 
				 "and you have exterminated at least %d of the creatures.  "), r_ptr->r_pkills));
		}

		/* Some kills past lives */
		else if (r_ptr->r_tkills)
		{
			hooked_roff(format(
				_("が、あなたの先祖はこのモンスターを少なくとも %d 体は倒している。", 
				  "and your ancestors have exterminated at least %d of the creatures.  "), r_ptr->r_tkills));
		}

		/* No kills */
		else
		{
			hooked_roff(format(
				_("が、まだ%sを倒したことはない。", 
				  "and %s is not ever known to have been defeated.  "), wd_he[msex]));
		}

		/* Start a new line */
		hooked_roff("\n");
	}

	/* Normal monsters */
	else
	{
		/* Killed some this life */
		if (r_ptr->r_pkills)
		{
			hooked_roff(format(
				_("あなたはこのモンスターを少なくとも %d 体は殺している。",
				  "You have killed at least %d of these creatures.  "), r_ptr->r_pkills));
		}

		/* Killed some last life */
		else if (r_ptr->r_tkills)
		{
			hooked_roff(format(
				_("あなたの先祖はこのモンスターを少なくとも %d 体は殺している。", 
				  "Your ancestors have killed at least %d of these creatures.  "), r_ptr->r_tkills));
		}

		/* Killed none */
		else
		{
			hooked_roff(_("このモンスターを倒したことはない。", "No battles to the death are recalled.  "));
		}

		/* Start a new line */
		hooked_roff("\n");
	}

	/* Descriptions */
	{
		concptr tmp = r_text + r_ptr->text;

		if (tmp[0])
		{
			/* Dump it */
			hooked_roff(tmp);

			/* Start a new line */
			hooked_roff("\n");
		}
	}

	if (r_idx == MON_KAGE)
	{
		/* All done */
		hooked_roff("\n");

		return;
	}

	/* Nothing yet */
	old = FALSE;

	/* Describe location */
	if (r_ptr->level == 0)
	{
		hooked_roff(format(_("%^sは町に住み", "%^s lives in the town"), wd_he[msex]));
		old = TRUE;
	}
	else if (r_ptr->r_tkills || know_everything)
	{
		if (depth_in_feet)
		{
			hooked_roff(format(
				_("%^sは通常地下 %d フィートで出現し", "%^s is normally found at depths of %d feet"),
				  wd_he[msex], r_ptr->level * 50));
		}
		else
		{
			hooked_roff(format(
				_("%^sは通常地下 %d 階で出現し", "%^s is normally found on dungeon level %d"),
				  wd_he[msex], r_ptr->level));
		}
		old = TRUE;
	}


	/* Describe movement */
	if (r_idx == MON_CHAMELEON)
	{
		hooked_roff(_("、他のモンスターに化ける。", "and can take the shape of other monster."));
		return;
	}
	else
	{
		/* Introduction */
		if (old)
		{
			hooked_roff(_("、", ", and "));
		}
		else
		{
			hooked_roff(format(_("%^sは", "%^s "), wd_he[msex]));
			old = TRUE;
		}
#ifndef JP
		hooked_roff("moves");
#endif

		/* Random-ness */
		if ((flags1 & RF1_RAND_50) || (flags1 & RF1_RAND_25))
		{
			/* Adverb */
			if ((flags1 & RF1_RAND_50) && (flags1 & RF1_RAND_25))
			{
				hooked_roff(_("かなり", " extremely"));
			}
			else if (flags1 & RF1_RAND_50)
			{
				hooked_roff(_("幾分", " somewhat"));
			}
			else if (flags1 & RF1_RAND_25)
			{
				hooked_roff(_("少々", " a bit"));
			}

			/* Adjective */
			hooked_roff(_("不規則に", " erratically"));

			/* Hack -- Occasional conjunction */
			if (speed != 110) hooked_roff(_("、かつ", ", and"));
		}

		if (speed > 110)
		{
			if (speed > 139) hook_c_roff(TERM_RED, _("信じ難いほど", " incredibly"));
			else if (speed > 134) hook_c_roff(TERM_ORANGE, _("猛烈に", " extremely"));
			else if (speed > 129) hook_c_roff(TERM_ORANGE, _("非常に", " very"));
			else if (speed > 124) hook_c_roff(TERM_UMBER, _("かなり", " fairly"));
			else if (speed < 120) hook_c_roff(TERM_L_UMBER, _("やや", " somewhat"));
			hook_c_roff(TERM_L_RED, _("素早く", " quickly"));
		}
		else if (speed < 110)
		{
			if (speed < 90) hook_c_roff(TERM_L_GREEN, _("信じ難いほど", " incredibly"));
			else if (speed < 95) hook_c_roff(TERM_BLUE, _("非常に", " very"));
			else if (speed < 100) hook_c_roff(TERM_BLUE, _("かなり", " fairly"));
			else if (speed > 104) hook_c_roff(TERM_GREEN, _("やや", " somewhat"));
			hook_c_roff(TERM_L_BLUE, _("ゆっくりと", " slowly"));
		}
		else
		{
			hooked_roff(_("普通の速さで", " at normal speed"));
		}
#ifdef JP
		hooked_roff("動いている");
#endif
	}

	/* The code above includes "attack speed" */
	if (flags1 & RF1_NEVER_MOVE)
	{
		/* Introduce */
		if (old)
		{
			hooked_roff(_("、しかし", ", but "));
		}
		else
		{
			hooked_roff(format(_("%^sは", "%^s "), wd_he[msex]));
			old = TRUE;
		}
		hooked_roff(_("侵入者を追跡しない", "does not deign to chase intruders"));
	}

	/* End this sentence */
	if (old)
	{
		hooked_roff(_("。", ".  "));
		old = FALSE;
	}


	/* Describe experience if known */
	if (r_ptr->r_tkills || know_everything)
	{
		/* Introduction */
#ifdef JP
		hooked_roff("この");
#else
		if (flags1 & RF1_UNIQUE)
		{
			hooked_roff("Killing this");
		}
		else
		{
			hooked_roff("A kill of this");
		}
#endif


		/* Describe the "quality" */
		if (flags2 & RF2_ELDRITCH_HORROR) hook_c_roff(TERM_VIOLET, _("狂気を誘う", " sanity-blasting"));/*nuke me*/
		if (flags3 & RF3_ANIMAL)          hook_c_roff(TERM_L_GREEN, _("自然界の", " natural"));
		if (flags3 & RF3_EVIL)            hook_c_roff(TERM_L_DARK, _("邪悪なる", " evil"));
		if (flags3 & RF3_GOOD)            hook_c_roff(TERM_YELLOW, _("善良な", " good"));
		if (flags3 & RF3_UNDEAD)          hook_c_roff(TERM_VIOLET, _("アンデッドの", " undead"));
		if (flags3 & RF3_AMBERITE)        hook_c_roff(TERM_VIOLET, _("アンバーの王族の", " Amberite"));

		if ((flags3 & (RF3_DRAGON | RF3_DEMON | RF3_GIANT | RF3_TROLL | RF3_ORC | RF3_ANGEL)) || (flags2 & (RF2_QUANTUM | RF2_HUMAN)))
		{
		/* Describe the "race" */
			if (flags3 & RF3_DRAGON)   hook_c_roff(TERM_ORANGE, _("ドラゴン", " dragon"));
			if (flags3 & RF3_DEMON)    hook_c_roff(TERM_VIOLET, _("デーモン", " demon"));
			if (flags3 & RF3_GIANT)    hook_c_roff(TERM_L_UMBER, _("ジャイアント", " giant"));
			if (flags3 & RF3_TROLL)    hook_c_roff(TERM_BLUE, _("トロル", " troll"));
			if (flags3 & RF3_ORC)      hook_c_roff(TERM_UMBER, _("オーク", " orc"));
			if (flags2 & RF2_HUMAN)    hook_c_roff(TERM_L_WHITE, _("人間", " human"));
			if (flags2 & RF2_QUANTUM)  hook_c_roff(TERM_VIOLET, _("量子生物", " quantum creature"));
			if (flags3 & RF3_ANGEL)    hook_c_roff(TERM_YELLOW, _("天使", " angel"));
		}
		else
		{
			hooked_roff(_("モンスター", " creature"));
		}

#ifdef JP
		hooked_roff("を倒すことは");
#endif
		/* Group some variables */
		{
			long i, j;

			/* calculate the integer exp part */
			i = (long)r_ptr->mexp * r_ptr->level / (p_ptr->max_plv + 2) * 3 / 2;

			/* calculate the fractional exp part scaled by 100, */
			/* must use long arithmetic to avoid overflow  */
			j = ((((long)r_ptr->mexp * r_ptr->level % (p_ptr->max_plv + 2) * 3 / 2) *
				(long)1000 / (p_ptr->max_plv + 2) + 5) / 10);

#ifdef JP
			hooked_roff(format(" %d レベルのキャラクタにとって 約%ld.%02ld ポイントの経験となる。",
				p_ptr->lev, (long)i, (long)j ));
#else

			/* Mention the experience */
			hooked_roff(format(" is worth about %ld.%02ld point%s",
				(long)i, (long)j,
				(((i == 1) && (j == 0)) ? "" : "s")));

			/* Take account of annoying English */
			p = "th";
			i = p_ptr->lev % 10;
			if ((p_ptr->lev / 10) == 1) /* nothing */;
			else if (i == 1) p = "st";
			else if (i == 2) p = "nd";
			else if (i == 3) p = "rd";

			/* Take account of "leading vowels" in numbers */
			q = "";
			i = p_ptr->lev;
			if ((i == 8) || (i == 11) || (i == 18)) q = "n";

			/* Mention the dependance on the player's level */
			hooked_roff(format(" for a%s %lu%s level character.  ",
				    q, (long)i, p));
#endif

		}
	}

	if ((flags2 & RF2_AURA_FIRE) && (flags2 & RF2_AURA_ELEC) && (flags3 & RF3_AURA_COLD))
	{
		hook_c_roff(TERM_VIOLET, format(
			_("%^sは炎と氷とスパークに包まれている。", "%^s is surrounded by flames, ice and electricity.  "), wd_he[msex]));
	}
	else if ((flags2 & RF2_AURA_FIRE) && (flags2 & RF2_AURA_ELEC))
	{
		hook_c_roff(TERM_L_RED, format(
			_("%^sは炎とスパークに包まれている。", "%^s is surrounded by flames and electricity.  "), wd_he[msex]));
	}
	else if ((flags2 & RF2_AURA_FIRE) && (flags3 & RF3_AURA_COLD))
	{
		hook_c_roff(TERM_BLUE, format(
			_("%^sは炎と氷に包まれている。", "%^s is surrounded by flames and ice.  "), wd_he[msex]));
	}
	else if ((flags3 & RF3_AURA_COLD) && (flags2 & RF2_AURA_ELEC))
	{
		hook_c_roff(TERM_L_GREEN, format(
			_("%^sは氷とスパークに包まれている。", "%^s is surrounded by ice and electricity.  "), wd_he[msex]));
	}
	else if (flags2 & RF2_AURA_FIRE)
	{
		hook_c_roff(TERM_RED, format(
			_("%^sは炎に包まれている。", "%^s is surrounded by flames.  "), wd_he[msex]));
	}
	else if (flags3 & RF3_AURA_COLD)
	{
		hook_c_roff(TERM_BLUE, format(
			_("%^sは氷に包まれている。", "%^s is surrounded by ice.  "), wd_he[msex]));
	}
	else if (flags2 & RF2_AURA_ELEC)
	{
		hook_c_roff(TERM_L_BLUE, format(
			_("%^sはスパークに包まれている。", "%^s is surrounded by electricity.  "), wd_he[msex]));
	}

	if (flags2 & RF2_REFLECTING)
		hooked_roff(format(_("%^sは矢の呪文を跳ね返す。", "%^s reflects bolt spells.  "), wd_he[msex]));

	/* Describe escorts */
	if ((flags1 & RF1_ESCORT) || (flags1 & RF1_ESCORTS) || reinforce)
	{
		hooked_roff(format(
			_("%^sは通常護衛を伴って現れる。", "%^s usually appears with escorts.  "), wd_he[msex]));

		if(reinforce)
		{
			hooked_roff(_("護衛の構成は", "These escorts"));
			if((flags1 & RF1_ESCORT) || (flags1 & RF1_ESCORTS))
			{
				hooked_roff(_("少なくとも", " at the least"));
			}
#ifndef JP
			hooked_roff(" contain ");
#endif			
			for(n = 0; n < A_MAX; n++)
			{
				if(r_ptr->reinforce_id[n] && r_ptr->reinforce_dd[n] && r_ptr->reinforce_ds[n])
				{
					monster_race *rf_ptr = &r_info[r_ptr->reinforce_id[n]];
					if(rf_ptr->flags1 & RF1_UNIQUE)
					{
						hooked_roff(format(_("、%s", ", %s"), r_name + rf_ptr->name));
					}
					else
					{
#ifdef JP
						hooked_roff(format("、 %dd%d 体の%s", r_ptr->reinforce_dd[n], r_ptr->reinforce_ds[n], r_name + rf_ptr->name));
#else
						bool plural = (r_ptr->reinforce_dd[n] * r_ptr->reinforce_ds[n] > 1);
						GAME_TEXT name[MAX_NLEN];
						strcpy(name, r_name + rf_ptr->name);
						if(plural) plural_aux(name);
						hooked_roff(format(",%dd%d %s", r_ptr->reinforce_dd[n], r_ptr->reinforce_ds[n], name));
#endif
					}
				}
			}
			hooked_roff(_("で成り立っている。", "."));
		}
	}

	/* Describe friends */
	else if (flags1 & RF1_FRIENDS)
	{
		hooked_roff(format(_("%^sは通常集団で現れる。", "%^s usually appears in groups.  "), wd_he[msex]));
	}


	/* Collect inate attacks */
	vn = 0;
	if (flags4 & RF4_SHRIEK)  { vp[vn] = _("悲鳴で助けを求める", "shriek for help"); color[vn++] = TERM_L_WHITE; }
	if (flags4 & RF4_ROCKET)  
    {
		set_damage(r_idx, (MS_ROCKET), _("ロケット%sを発射する", "shoot a rocket%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_UMBER; 
    }
    
	if (flags4 & RF4_SHOOT)
	{
		for (r = 0, m = 0; m < 4; m++)
		{
			if (r_ptr->blow[m].method == RBM_SHOOT)
			{
				if (know_armour(r_idx))
					sprintf(tmp_msg[vn], _("威力 %dd%d の射撃をする", "fire an arrow (Power:%dd%d)"), r_ptr->blow[m].d_side, r_ptr->blow[m].d_dice);
				else
					sprintf(tmp_msg[vn], _("射撃をする", "fire an arrow"));
				vp[vn] = tmp_msg[vn]; color[vn++] = TERM_UMBER;
				break;
			}
		}		
	}
	if (a_ability_flags2 & (RF6_SPECIAL)) { vp[vn] = _("特別な行動をする", "do something"); color[vn++] = TERM_VIOLET; }

	/* Describe inate attacks */
	if (vn)
	{
		/* Intro */
		hooked_roff(format(_("%^sは", "%^s"), wd_he[msex]));


		/* Scan */
		for (n = 0; n < vn; n++)
		{
#ifdef JP
			if (n != vn - 1)
			{
				jverb(vp[n], jverb_buf, JVERB_OR);
				hook_c_roff(color[n], jverb_buf);
				hook_c_roff(color[n], "り");
				hooked_roff("、");
			}
			else hook_c_roff(color[n], vp[n]);
#else
			/* Intro */
			if (n == 0) hooked_roff(" may ");
			else if (n < vn - 1) hooked_roff(", ");
			else hooked_roff(" or ");

			/* Dump */
			hook_c_roff(color[n], vp[n]);
#endif

		}

		/* End */
		hooked_roff(_("ことがある。", ".  "));
	}


	/* Collect breaths */
	vn = 0;
	if (flags4 & (RF4_BR_ACID))
	{
		set_damage(r_idx, (MS_BR_ACID), _("酸%s", "acid%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_GREEN;
	}
	if (flags4 & (RF4_BR_ELEC))
	{
		set_damage(r_idx, (MS_BR_ELEC), _("稲妻%s", "lightning%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_BLUE;
	}
	if (flags4 & (RF4_BR_FIRE))
	{
		set_damage(r_idx, (MS_BR_FIRE), _("火炎%s", "fire%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_RED;
	}
	if (flags4 & (RF4_BR_COLD))
	{
		set_damage(r_idx, (MS_BR_COLD), _("冷気%s", "frost%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_WHITE;
	}
	if (flags4 & (RF4_BR_POIS))
	{
		set_damage(r_idx, (MS_BR_POIS), _("毒%s", "poison%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_GREEN;
	}
	if (flags4 & (RF4_BR_NETH))
	{
		set_damage(r_idx, (MS_BR_NETHER), _("地獄%s", "nether%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_DARK;
	}
	if (flags4 & (RF4_BR_LITE))
	{
		set_damage(r_idx, (MS_BR_LITE), _("閃光%s", "light%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_YELLOW;
	}
	if (flags4 & (RF4_BR_DARK))
	{
		set_damage(r_idx, (MS_BR_DARK), _("暗黒%s", "darkness%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_DARK;
	}
	if (flags4 & (RF4_BR_CONF))
	{
		set_damage(r_idx, (MS_BR_CONF), _("混乱%s", "confusion%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_UMBER;
	}
	if (flags4 & (RF4_BR_SOUN))
	{
		set_damage(r_idx, (MS_BR_SOUND), _("轟音%s", "sound%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_ORANGE;
	}
	if (flags4 & (RF4_BR_CHAO))
	{
		set_damage(r_idx, (MS_BR_CHAOS), _("カオス%s", "chaos%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_VIOLET;
	}
	if (flags4 & (RF4_BR_DISE))
	{
		set_damage(r_idx, (MS_BR_DISEN), _("劣化%s", "disenchantment%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_VIOLET;
	}
	if (flags4 & (RF4_BR_NEXU))
	{
		set_damage(r_idx, (MS_BR_NEXUS), _("因果混乱%s", "nexus%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_VIOLET;
	}
	if (flags4 & (RF4_BR_TIME))
	{
		set_damage(r_idx, (MS_BR_TIME), _("時間逆転%s", "time%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_BLUE;
	}
	if (flags4 & (RF4_BR_INER))
	{
		set_damage(r_idx, (MS_BR_INERTIA), _("遅鈍%s", "inertia%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_SLATE;
	}
	if (flags4 & (RF4_BR_GRAV))
	{
		set_damage(r_idx, (MS_BR_GRAVITY), _("重力%s", "gravity%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_SLATE;
	}
	if (flags4 & (RF4_BR_SHAR))
	{
		set_damage(r_idx, (MS_BR_SHARDS), _("破片%s", "shards%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_UMBER;
	}
	if (flags4 & (RF4_BR_PLAS))
	{
		set_damage(r_idx, (MS_BR_PLASMA), _("プラズマ%s", "plasma%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_RED;
	}
	if (flags4 & (RF4_BR_WALL))
	{
		set_damage(r_idx, (MS_BR_FORCE), _("フォース%s", "force%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_UMBER;
	}
	if (flags4 & (RF4_BR_MANA))
	{
		set_damage(r_idx, (MS_BR_MANA), _("魔力%s", "mana%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_BLUE;
	}
	if (flags4 & (RF4_BR_NUKE))
	{
		set_damage(r_idx, (MS_BR_NUKE), _("放射性廃棄物%s", "toxic waste%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_GREEN;
	}
	if (flags4 & (RF4_BR_DISI))
	{
		set_damage(r_idx, (MS_BR_DISI), _("分解%s", "disintegration%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_SLATE;
	}

	/* Describe breaths */
	if (vn)
	{
		/* Note breath */
		breath = TRUE;

		/* Intro */
		hooked_roff(format(_("%^sは", "%^s"), wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) hooked_roff("や");
#else
			if (n == 0) hooked_roff(" may breathe ");
			else if (n < vn-1) hooked_roff(", ");
			else hooked_roff(" or ");
#endif


			/* Dump */
			hook_c_roff(color[n], vp[n]);
		}
#ifdef JP
		hooked_roff("のブレスを吐くことがある");
#endif
	}


	/* Collect spells */
	vn = 0;
	if (a_ability_flags1 & (RF5_BA_ACID))
	{
		set_damage(r_idx, (MS_BALL_ACID), _("アシッド・ボール%s", "produce acid balls%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_GREEN;
	}
	if (a_ability_flags1 & (RF5_BA_ELEC))
	{
		set_damage(r_idx, (MS_BALL_ELEC), _("サンダー・ボール%s", "produce lightning balls%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_BLUE;
	}
	if (a_ability_flags1 & (RF5_BA_FIRE))
	{
		set_damage(r_idx, (MS_BALL_FIRE), _("ファイア・ボール%s", "produce fire balls%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_RED;
	}
	if (a_ability_flags1 & (RF5_BA_COLD))
	{
		set_damage(r_idx, (MS_BALL_COLD), _("アイス・ボール%s", "produce frost balls%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_WHITE;
	}
	if (a_ability_flags1 & (RF5_BA_POIS))
	{
		set_damage(r_idx, (MS_BALL_POIS), _("悪臭雲%s", "produce poison balls%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_GREEN;
	}
	if (a_ability_flags1 & (RF5_BA_NETH))
	{
		set_damage(r_idx, (MS_BALL_NETHER), _("地獄球%s", "produce nether balls%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_DARK;
	}
	if (a_ability_flags1 & (RF5_BA_WATE))
	{
		set_damage(r_idx, (MS_BALL_WATER), _("ウォーター・ボール%s", "produce water balls%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_BLUE;
	}
	if (flags4 & (RF4_BA_NUKE))
	{
		set_damage(r_idx, (MS_BALL_NUKE), _("放射能球%s", "produce balls of radiation%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_GREEN;
	}
	if (a_ability_flags1 & (RF5_BA_MANA))
	{
		set_damage(r_idx, (MS_BALL_MANA), _("魔力の嵐%s", "invoke mana storms%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_BLUE;
	}
	if (a_ability_flags1 & (RF5_BA_DARK))
	{
		set_damage(r_idx, (MS_BALL_DARK), _("暗黒の嵐%s", "invoke darkness storms%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_DARK;
	}
	if (a_ability_flags1 & (RF5_BA_LITE))
	{
		set_damage(r_idx, (MS_STARBURST), _("スターバースト%s", "invoke starburst%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_YELLOW;
	}
	if (flags4 & (RF4_BA_CHAO))
	{
		set_damage(r_idx, (MS_BALL_CHAOS), _("純ログルス%s", "invoke raw Logrus%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_VIOLET;
	}
	if (a_ability_flags2 & (RF6_HAND_DOOM)) { vp[vn] = _("破滅の手(40%-60%)", "invoke the Hand of Doom(40%-60%)"); color[vn++] = TERM_VIOLET; }
	if (a_ability_flags2 & (RF6_PSY_SPEAR))
	{
		set_damage(r_idx, (MS_PSY_SPEAR), _("光の剣%s", "psycho-spear%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_YELLOW;
	}
	if (a_ability_flags1 & (RF5_DRAIN_MANA))
	{
		set_damage(r_idx, (MS_DRAIN_MANA), _("魔力吸収%s", "drain mana%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_SLATE;
	}
	if (a_ability_flags1 & (RF5_MIND_BLAST))
	{
		set_damage(r_idx, (MS_MIND_BLAST), _("精神攻撃%s", "cause mind blasting%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_RED;
	}
	if (a_ability_flags1 & (RF5_BRAIN_SMASH))
	{
		set_damage(r_idx, (MS_BRAIN_SMASH), _("脳攻撃%s", "cause brain smashing%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_RED;
	}
	if (a_ability_flags1 & (RF5_CAUSE_1))
	{
		set_damage(r_idx, (MS_CAUSE_1),
			_("軽傷＋呪い%s", "cause light wounds and cursing%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_WHITE;
	}
	if (a_ability_flags1 & (RF5_CAUSE_2))
	{
		set_damage(r_idx, (MS_CAUSE_2),
			_("重傷＋呪い%s", "cause serious wounds and cursing%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_WHITE;
	}
	if (a_ability_flags1 & (RF5_CAUSE_3))
	{
		set_damage(r_idx, (MS_CAUSE_3),
			_("致命傷＋呪い%s", "cause critical wounds and cursing%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_WHITE;
	}
	if (a_ability_flags1 & (RF5_CAUSE_4))
	{
		set_damage(r_idx, (MS_CAUSE_4),
			_("秘孔を突く%s", "cause mortal wounds%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_WHITE;
	}
	if (a_ability_flags1 & (RF5_BO_ACID))
	{
		set_damage(r_idx, (MS_BOLT_ACID), _("アシッド・ボルト%s", "produce acid bolts%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_GREEN;
	}
	if (a_ability_flags1 & (RF5_BO_ELEC))
	{
		set_damage(r_idx, (MS_BOLT_ELEC), _("サンダー・ボルト%s", "produce lightning bolts%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_BLUE;
	}
	if (a_ability_flags1 & (RF5_BO_FIRE))
	{
		set_damage(r_idx, (MS_BOLT_FIRE), _("ファイア・ボルト%s", "produce fire bolts%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_RED;
	}
	if (a_ability_flags1 & (RF5_BO_COLD))
	{
		set_damage(r_idx, (MS_BOLT_COLD), _("アイス・ボルト%s", "produce frost bolts%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_WHITE;
	}
	if (a_ability_flags1 & (RF5_BO_NETH))
	{
		set_damage(r_idx, (MS_BOLT_NETHER), _("地獄の矢%s", "produce nether bolts%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_DARK;
	}
	if (a_ability_flags1 & (RF5_BO_WATE))
	{
		set_damage(r_idx, (MS_BOLT_WATER), _("ウォーター・ボルト%s", "produce water bolts%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_BLUE;
	}
	if (a_ability_flags1 & (RF5_BO_MANA))
	{
		set_damage(r_idx, (MS_BOLT_MANA), _("魔力の矢%s", "produce mana bolts%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_BLUE;
	}
	if (a_ability_flags1 & (RF5_BO_PLAS))
	{
		set_damage(r_idx, (MS_BOLT_PLASMA), _("プラズマ・ボルト%s", "produce plasma bolts%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_L_RED;
	}
	if (a_ability_flags1 & (RF5_BO_ICEE))
	{
		set_damage(r_idx, (MS_BOLT_ICE), _("極寒の矢%s", "produce ice bolts%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_WHITE;
	}
	if (a_ability_flags1 & (RF5_MISSILE))
	{
		set_damage(r_idx, (MS_MAGIC_MISSILE), _("マジックミサイル%s", "produce magic missiles%s"), tmp_msg[vn]);
		vp[vn] = tmp_msg[vn];
		color[vn++] = TERM_SLATE;
	}
	if (a_ability_flags1 & (RF5_SCARE))           { vp[vn] = _("恐怖", "terrify"); color[vn++] = TERM_SLATE; }
	if (a_ability_flags1 & (RF5_BLIND))           { vp[vn] = _("目くらまし", "blind"); color[vn++] = TERM_L_DARK; }
	if (a_ability_flags1 & (RF5_CONF))            { vp[vn] = _("混乱", "confuse"); color[vn++] = TERM_L_UMBER; }
	if (a_ability_flags1 & (RF5_SLOW))            { vp[vn] = _("減速", "slow"); color[vn++] = TERM_UMBER; }
	if (a_ability_flags1 & (RF5_HOLD))            { vp[vn] = _("麻痺", "paralyze"); color[vn++] = TERM_RED; }
	if (a_ability_flags2 & (RF6_HASTE))           { vp[vn] = _("加速", "haste-self"); color[vn++] = TERM_L_GREEN; }
	if (a_ability_flags2 & (RF6_HEAL))            { vp[vn] = _("治癒", "heal-self"); color[vn++] = TERM_WHITE; }
	if (a_ability_flags2 & (RF6_INVULNER))        { vp[vn] = _("無敵化", "make invulnerable"); color[vn++] = TERM_WHITE; }
	if (flags4 & RF4_DISPEL)            { vp[vn] = _("魔力消去", "dispel-magic"); color[vn++] = TERM_L_WHITE; }
	if (a_ability_flags2 & (RF6_BLINK))           { vp[vn] = _("ショートテレポート", "blink-self"); color[vn++] = TERM_UMBER; }
	if (a_ability_flags2 & (RF6_TPORT))           { vp[vn] = _("テレポート", "teleport-self"); color[vn++] = TERM_ORANGE; }
	if (a_ability_flags2 & (RF6_WORLD))           { vp[vn] = _("時を止める", "stop the time"); color[vn++] = TERM_L_BLUE; }
	if (a_ability_flags2 & (RF6_TELE_TO))         { vp[vn] = _("テレポートバック", "teleport to"); color[vn++] = TERM_L_UMBER; }
	if (a_ability_flags2 & (RF6_TELE_AWAY))       { vp[vn] = _("テレポートアウェイ", "teleport away"); color[vn++] = TERM_UMBER; }
	if (a_ability_flags2 & (RF6_TELE_LEVEL))      { vp[vn] = _("テレポート・レベル", "teleport level"); color[vn++] = TERM_ORANGE; }

	if (a_ability_flags2 & (RF6_DARKNESS))
	{
		if ((p_ptr->pclass != CLASS_NINJA) || (r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) || (r_ptr->flags7 & RF7_DARK_MASK))
		{
			vp[vn] = _("暗闇", "create darkness"); color[vn++] = TERM_L_DARK;
		}
		else
		{
			vp[vn] = _("閃光", "create light"); color[vn++] = TERM_YELLOW;
		}
	}

	if (a_ability_flags2 & (RF6_TRAPS))           { vp[vn] = _("トラップ", "create traps"); color[vn++] = TERM_BLUE; }
	if (a_ability_flags2 & (RF6_FORGET))          { vp[vn] = _("記憶消去", "cause amnesia"); color[vn++] = TERM_BLUE; }
	if (a_ability_flags2 & (RF6_RAISE_DEAD))      { vp[vn] = _("死者復活", "raise dead"); color[vn++] = TERM_RED; }
	if (a_ability_flags2 & (RF6_S_MONSTER))       { vp[vn] = _("モンスター一体召喚", "summon a monster"); color[vn++] = TERM_SLATE; }
	if (a_ability_flags2 & (RF6_S_MONSTERS))      { vp[vn] = _("モンスター複数召喚", "summon monsters"); color[vn++] = TERM_L_WHITE; }
	if (a_ability_flags2 & (RF6_S_KIN))           { vp[vn] = _("救援召喚", "summon aid"); color[vn++] = TERM_ORANGE; }
	if (a_ability_flags2 & (RF6_S_ANT))           { vp[vn] = _("アリ召喚", "summon ants"); color[vn++] = TERM_RED; }
	if (a_ability_flags2 & (RF6_S_SPIDER))        { vp[vn] = _("クモ召喚", "summon spiders"); color[vn++] = TERM_L_DARK; }
	if (a_ability_flags2 & (RF6_S_HOUND))         { vp[vn] = _("ハウンド召喚", "summon hounds"); color[vn++] = TERM_L_UMBER; }
	if (a_ability_flags2 & (RF6_S_HYDRA))         { vp[vn] = _("ヒドラ召喚", "summon hydras"); color[vn++] = TERM_L_GREEN; }
	if (a_ability_flags2 & (RF6_S_ANGEL))         { vp[vn] = _("天使一体召喚", "summon an angel"); color[vn++] = TERM_YELLOW; }
	if (a_ability_flags2 & (RF6_S_DEMON))         { vp[vn] = _("デーモン一体召喚", "summon a demon"); color[vn++] = TERM_L_RED; }
	if (a_ability_flags2 & (RF6_S_UNDEAD))        { vp[vn] = _("アンデッド一体召喚", "summon an undead"); color[vn++] = TERM_L_DARK; }
	if (a_ability_flags2 & (RF6_S_DRAGON))        { vp[vn] = _("ドラゴン一体召喚", "summon a dragon"); color[vn++] = TERM_ORANGE; }
	if (a_ability_flags2 & (RF6_S_HI_UNDEAD))     { vp[vn] = _("強力なアンデッド召喚", "summon Greater Undead"); color[vn++] = TERM_L_DARK; }
	if (a_ability_flags2 & (RF6_S_HI_DRAGON))     { vp[vn] = _("古代ドラゴン召喚", "summon Ancient Dragons"); color[vn++] = TERM_ORANGE; }	
	if (a_ability_flags2 & (RF6_S_CYBER))         { vp[vn] = _("サイバーデーモン召喚", "summon Cyberdemons"); color[vn++] = TERM_UMBER; }
	if (a_ability_flags2 & (RF6_S_AMBERITES))     { vp[vn] = _("アンバーの王族召喚", "summon Lords of Amber"); color[vn++] = TERM_VIOLET; }
	if (a_ability_flags2 & (RF6_S_UNIQUE))        { vp[vn] = _("ユニーク・モンスター召喚", "summon Unique Monsters"); color[vn++] = TERM_VIOLET; }


	/* Describe spells */
	if (vn)
	{
		/* Note magic */
		magic = TRUE;

		/* Intro */
		if (breath)
		{
			hooked_roff(_("、なおかつ", ", and is also"));
		}
		else
		{
			hooked_roff(format(_("%^sは", "%^s is"), wd_he[msex]));
		}

#ifdef JP
		/* Adverb */
		if (flags2 & (RF2_SMART)) hook_c_roff(TERM_YELLOW, "的確に");

		/* Verb Phrase */
		hooked_roff("魔法を使うことができ、");
#else
		/* Verb Phrase */
		hooked_roff(" magical, casting spells");

		/* Adverb */
		if (flags2 & RF2_SMART) hook_c_roff(TERM_YELLOW, " intelligently");
#endif


		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) hooked_roff("、");
#else
			if (n == 0) hooked_roff(" which ");
			else if (n < vn-1) hooked_roff(", ");
			else hooked_roff(" or ");
#endif


			/* Dump */
			hook_c_roff(color[n], vp[n]);
		}
#ifdef JP
		hooked_roff("の呪文を唱えることがある");
#endif
	}


	/* End the sentence about inate/other spells */
	if (breath || magic)
	{
		/* Total casting */
		m = r_ptr->r_cast_spell;

		/* Average frequency */
		n = r_ptr->freq_spell;

		/* Describe the spell frequency */
		if (m > 100 || know_everything)
		{
			hooked_roff(format(
				_("(確率:1/%d)", "; 1 time in %d"), 100 / n));
		}

		/* Guess at the frequency */
		else if (m)
		{
			n = ((n + 9) / 10) * 10;
			hooked_roff(format(
				_("(確率:約1/%d)", "; about 1 time in %d"), 100 / n));
		}

		/* End this sentence */
		hooked_roff(_("。", ".  "));
	}

	/* Describe monster "toughness" */
    if (know_everything || know_armour(r_idx))
	{
		/* Armor */
		hooked_roff(format(
			_("%^sは AC%d の防御力と", "%^s has an armor rating of %d"),
			    wd_he[msex], r_ptr->ac));

		/* Maximized hitpoints */
		if ((flags1 & RF1_FORCE_MAXHP) || (r_ptr->hside == 1))
		{
			u32b hp = r_ptr->hdice * (nightmare ? 2 : 1) * r_ptr->hside;
			hooked_roff(format(
				_(" %d の体力がある。", " and a life rating of %d.  "),
				    (s16b)MIN(30000, hp)));
		}

		/* Variable hitpoints */
		else
		{
			hooked_roff(format(
				_(" %dd%d の体力がある。", " and a life rating of %dd%d.  "),
				    r_ptr->hdice * (nightmare ? 2 : 1), r_ptr->hside));
		}
	}



	/* Collect special abilities. */
	vn = 0;
	if (flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) { vp[vn] = _("ダンジョンを照らす", "illuminate the dungeon");     color[vn++] = TERM_WHITE; }
	if (flags7 & (RF7_HAS_DARK_1 | RF7_HAS_DARK_2)) { vp[vn] = _("ダンジョンを暗くする", "darken the dungeon");   color[vn++] = TERM_L_DARK; }
	if (flags2 & RF2_OPEN_DOOR) { vp[vn] = _("ドアを開ける", "open doors"); color[vn++] = TERM_WHITE; }
	if (flags2 & RF2_BASH_DOOR) { vp[vn] = _("ドアを打ち破る", "bash down doors"); color[vn++] = TERM_WHITE; }
	if (flags7 & RF7_CAN_FLY)  { vp[vn] = _("空を飛ぶ", "fly"); color[vn++] = TERM_WHITE; }
	if (flags7 & RF7_CAN_SWIM)   { vp[vn] = _("水を渡る", "swim"); color[vn++] = TERM_WHITE; }
	if (flags2 & RF2_PASS_WALL) { vp[vn] = _("壁をすり抜ける", "pass through walls"); color[vn++] = TERM_WHITE; }
	if (flags2 & RF2_KILL_WALL) { vp[vn] = _("壁を掘り進む", "bore through walls"); color[vn++] = TERM_WHITE; }
	if (flags2 & RF2_MOVE_BODY) { vp[vn] = _("弱いモンスターを押しのける", "push past weaker monsters"); color[vn++] = TERM_WHITE; }
	if (flags2 & RF2_KILL_BODY) { vp[vn] = _("弱いモンスターを倒す", "destroy weaker monsters"); color[vn++] = TERM_WHITE; }
	if (flags2 & RF2_TAKE_ITEM) { vp[vn] = _("アイテムを拾う", "pick up objects"); color[vn++] = TERM_WHITE; }
	if (flags2 & RF2_KILL_ITEM) { vp[vn] = _("アイテムを壊す", "destroy objects"); color[vn++] = TERM_WHITE; }


	/* Describe special abilities. */
	if (vn)
	{
		/* Intro */
		hooked_roff(format(_("%^sは", "%^s"), wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if (n != vn - 1)
			{
				jverb(vp[n], jverb_buf, JVERB_AND);
				hook_c_roff(color[n], jverb_buf);
				hooked_roff("、");
			}
			else hook_c_roff(color[n], vp[n]);
#else
			if (n == 0) hooked_roff(" can ");
			else if (n < vn - 1) hooked_roff(", ");
			else hooked_roff(" and ");

			/* Dump */
			hook_c_roff(color[n], vp[n]);
#endif

		}

		/* End */
		hooked_roff(_("ことができる。", ".  "));

	}
	
	/* Aquatic */
	if (flags7 & RF7_AQUATIC)
	{
		hooked_roff(format(_("%^sは水中に棲んでいる。", "%^s lives in water.  "), wd_he[msex]));
	}

	/* Describe special abilities. */
	if (flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2))
	{
		hooked_roff(format(_("%^sは光っている。", "%^s is shining.  "), wd_he[msex]));
	}
	if (flags7 & (RF7_SELF_DARK_1 | RF7_SELF_DARK_2))
	{
		hook_c_roff(TERM_L_DARK, format(_("%^sは暗黒に包まれている。", "%^s is surrounded by darkness.  "), wd_he[msex]));
	}
	if (flags2 & RF2_INVISIBLE)
	{
		hooked_roff(format(_("%^sは透明で目に見えない。", "%^s is invisible.  "), wd_he[msex]));
	}
	if (flags2 & RF2_COLD_BLOOD)
	{
		hooked_roff(format(_("%^sは冷血動物である。", "%^s is cold blooded.  "), wd_he[msex]));
	}
	if (flags2 & RF2_EMPTY_MIND)
	{
		hooked_roff(format(_("%^sはテレパシーでは感知できない。", "%^s is not detected by telepathy.  "), wd_he[msex]));
	}
	else if (flags2 & RF2_WEIRD_MIND)
	{
		hooked_roff(format(_("%^sはまれにテレパシーで感知できる。", "%^s is rarely detected by telepathy.  "), wd_he[msex]));
	}
	if (flags2 & RF2_MULTIPLY)
	{
		hook_c_roff(TERM_L_UMBER, format(_("%^sは爆発的に増殖する。", "%^s breeds explosively.  "), wd_he[msex]));
	}
	if (flags2 & RF2_REGENERATE)
	{
		hook_c_roff(TERM_L_WHITE, format(_("%^sは素早く体力を回復する。", "%^s regenerates quickly.  "), wd_he[msex]));
	}
	if (flags7 & RF7_RIDING)
	{
		hook_c_roff(TERM_SLATE, format(_("%^sに乗ることができる。", "%^s is suitable for riding.  "), wd_he[msex]));
	}


	/* Collect susceptibilities */
	vn = 0;
	if (flags3 & RF3_HURT_ROCK) { vp[vn] = _("岩を除去するもの", "rock remover"); color[vn++] = TERM_UMBER; }
	if (flags3 & RF3_HURT_LITE) { vp[vn] = _("明るい光", "bright light"); color[vn++] = TERM_YELLOW; }
	if (flags3 & RF3_HURT_FIRE) { vp[vn] = _("炎", "fire"); color[vn++] = TERM_RED; }
	if (flags3 & RF3_HURT_COLD) { vp[vn] = _("冷気", "cold"); color[vn++] = TERM_L_WHITE; }


	/* Describe susceptibilities */
	if (vn)
	{
		/* Intro */
		hooked_roff(format(_("%^sには", "%^s"), wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) hooked_roff("や");
#else
			if (n == 0) hooked_roff(" is hurt by ");
			else if (n < vn-1) hooked_roff(", ");
			else hooked_roff(" and ");
#endif


			/* Dump */
			hook_c_roff(color[n], vp[n]);
		}

		/* End */
		hooked_roff(_("でダメージを与えられる。", ".  "));
	}


	/* Collect immunities */
	vn = 0;
	if (flagsr & RFR_IM_ACID) { vp[vn] = _("酸", "acid"); color[vn++] = TERM_GREEN; }
	if (flagsr & RFR_IM_ELEC) { vp[vn] = _("稲妻", "lightning"); color[vn++] = TERM_BLUE; }
	if (flagsr & RFR_IM_FIRE) { vp[vn] = _("炎", "fire"); color[vn++] = TERM_RED; }
	if (flagsr & RFR_IM_COLD) { vp[vn] = _("冷気", "cold"); color[vn++] = TERM_L_WHITE; }
	if (flagsr & RFR_IM_POIS) { vp[vn] = _("毒", "poison"); color[vn++] = TERM_L_GREEN; }


	/* Collect resistances */
	if (flagsr & RFR_RES_LITE) { vp[vn] = _("閃光", "light"); color[vn++] = TERM_YELLOW; }
	if (flagsr & RFR_RES_DARK) { vp[vn] = _("暗黒", "dark"); color[vn++] = TERM_L_DARK; }
	if (flagsr & RFR_RES_NETH) { vp[vn] = _("地獄", "nether"); color[vn++] = TERM_L_DARK; }
	if (flagsr & RFR_RES_WATE) { vp[vn] = _("水", "water"); color[vn++] = TERM_BLUE; }
	if (flagsr & RFR_RES_PLAS) { vp[vn] = _("プラズマ", "plasma"); color[vn++] = TERM_L_RED; }
	if (flagsr & RFR_RES_SHAR) { vp[vn] = _("破片", "shards"); color[vn++] = TERM_L_UMBER; }
	if (flagsr & RFR_RES_SOUN) { vp[vn] = _("轟音", "sound"); color[vn++] = TERM_ORANGE; }
	if (flagsr & RFR_RES_CHAO) { vp[vn] = _("カオス", "chaos"); color[vn++] = TERM_VIOLET; }
	if (flagsr & RFR_RES_NEXU) { vp[vn] = _("因果混乱", "nexus"); color[vn++] = TERM_VIOLET; }
	if (flagsr & RFR_RES_DISE) { vp[vn] = _("劣化", "disenchantment"); color[vn++] = TERM_VIOLET; }
	if (flagsr & RFR_RES_WALL) { vp[vn] = _("フォース", "force"); color[vn++] = TERM_UMBER; }
	if (flagsr & RFR_RES_INER) { vp[vn] = _("遅鈍", "inertia"); color[vn++] = TERM_SLATE; }
	if (flagsr & RFR_RES_TIME) { vp[vn] = _("時間逆転", "time"); color[vn++] = TERM_L_BLUE; }
	if (flagsr & RFR_RES_GRAV) { vp[vn] = _("重力", "gravity"); color[vn++] = TERM_SLATE; }
	if (flagsr & RFR_RES_ALL) { vp[vn] = _("あらゆる攻撃", "all"); color[vn++] = TERM_YELLOW; }
	if ((flagsr & RFR_RES_TELE) && !(r_ptr->flags1 & RF1_UNIQUE)) { vp[vn] = _("テレポート", "teleportation"); color[vn++] = TERM_ORANGE; }

	/* Describe immunities and resistances */
	if (vn)
	{
		/* Intro */
		hooked_roff(format(_("%^sは", "%^s"), wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) hooked_roff("と");
#else
			if (n == 0) hooked_roff(" resists ");
			else if (n < vn-1) hooked_roff(", ");
			else hooked_roff(" and ");
#endif


			/* Dump */
			hook_c_roff(color[n], vp[n]);
		}

		/* End */
		hooked_roff(_("の耐性を持っている。", ".  "));
	}


	if ((r_ptr->r_xtra1 & MR1_SINKA) || know_everything)
	{
		if (r_ptr->next_r_idx)
		{
			hooked_roff(format(_("%^sは経験を積むと、", "%^s will evolve into "), wd_he[msex]));
			hook_c_roff(TERM_YELLOW, format("%s", r_name+r_info[r_ptr->next_r_idx].name));
			hooked_roff(
				_(format("に進化する。"), 
				  format(" when %s gets enough experience.  ", wd_he[msex])));
		}
		else if (!(r_ptr->flags1 & RF1_UNIQUE))
		{
			hooked_roff(format(_("%sは進化しない。", "%s won't evolve.  "), wd_he[msex]));
		}
	}

	/* Collect non-effects */
	vn = 0;
	if (flags3 & RF3_NO_STUN)  { vp[vn] = _("朦朧としない", "stunned"); color[vn++] = TERM_ORANGE; }
	if (flags3 & RF3_NO_FEAR)  { vp[vn] = _("恐怖を感じない", "frightened"); color[vn++] = TERM_SLATE; }
	if (flags3 & RF3_NO_CONF)  { vp[vn] = _("混乱しない", "confused"); color[vn++] = TERM_L_UMBER; }
	if (flags3 & RF3_NO_SLEEP) { vp[vn] = _("眠らされない", "slept"); color[vn++] = TERM_BLUE; }
	if ((flagsr & RFR_RES_TELE) && (r_ptr->flags1 & RF1_UNIQUE)) { vp[vn] = _("テレポートされない", "teleported"); color[vn++] = TERM_ORANGE; }

	/* Describe non-effects */
	if (vn)
	{
		/* Intro */
		hooked_roff(format(
			_("%^sは", "%^s"), wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) hooked_roff("し、");
#else
			if (n == 0) hooked_roff(" cannot be ");
			else if (n < vn - 1) hooked_roff(", ");
			else hooked_roff(" or ");
#endif


			/* Dump */
			hook_c_roff(color[n], vp[n]);
		}

		/* End */
		hooked_roff(_("。", ".  "));
	}


	/* Do we know how aware it is? */
	if ((((int)r_ptr->r_wake * (int)r_ptr->r_wake) > r_ptr->sleep) ||
		  (r_ptr->r_ignore == MAX_UCHAR) ||
	    (r_ptr->sleep == 0 && r_ptr->r_tkills >= 10) || know_everything)
	{
		concptr act;

		if (r_ptr->sleep > 200)
		{
			act = _("を無視しがちであるが", "prefers to ignore");
		}
		else if (r_ptr->sleep > 95)
		{
			act = _("に対してほとんど注意を払わないが", "pays very little attention to");
		}
		else if (r_ptr->sleep > 75)
		{
			act = _("に対してあまり注意を払わないが", "pays little attention to");
		}
		else if (r_ptr->sleep > 45)
		{
			act = _("を見過ごしがちであるが", "tends to overlook");
		}
		else if (r_ptr->sleep > 25)
		{
			act = _("をほんの少しは見ており", "takes quite a while to see");
		}
		else if (r_ptr->sleep > 10)
		{
			act = _("をしばらくは見ており", "takes a while to see");
		}
		else if (r_ptr->sleep > 5)
		{
			act = _("を幾分注意深く見ており", "is fairly observant of");
		}
		else if (r_ptr->sleep > 3)
		{
			act = _("を注意深く見ており", "is observant of");
		}
		else if (r_ptr->sleep > 1)
		{
			act = _("をかなり注意深く見ており", "is very observant of");
		}
		else if (r_ptr->sleep > 0)
		{
			act = _("を警戒しており", "is vigilant for");
		}
		else
		{
			act = _("をかなり警戒しており", "is ever vigilant for");
		}

		hooked_roff(
			_(format("%^sは侵入者%s、 %d フィート先から侵入者に気付くことがある。", wd_he[msex], act, 10 * r_ptr->aaf),
			  format("%^s %s intruders, which %s may notice from %d feet.  ", wd_he[msex], act, wd_he[msex], 10 * r_ptr->aaf)));
	}


	/* Drops gold and/or items */
	if (drop_gold || drop_item)
	{
		/* Intro */
		hooked_roff(format(
			_("%^sは", "%^s may carry"), wd_he[msex]));
#ifndef JP
		/* No "n" needed */
		sin = FALSE;
#endif


		/* Count maximum drop */
		n = MAX(drop_gold, drop_item);

		/* One drop (may need an "n") */
		if (n == 1)
		{
			hooked_roff(_("一つの", " a"));
#ifndef JP
			sin = TRUE;
#endif
		}

		/* Two drops */
		else if (n == 2)
		{
			hooked_roff(
				_("一つか二つの", " one or two"));
		}

		/* Many drops */
		else
		{
			hooked_roff(format(
				_(" %d 個までの", " up to %d"), n));
		}


		/* Great */
		if (flags1 & RF1_DROP_GREAT)
		{
			p = _("特別な", " exceptional");
		}

		/* Good (no "n" needed) */
		else if (flags1 & RF1_DROP_GOOD)
		{
			p = _("上質な", " good");
#ifndef JP
			sin = FALSE;
#endif
		}

		else
		{
			p = NULL;
		}


		/* Objects */
		if (drop_item)
		{
			/* Handle singular "an" */
#ifndef JP
			if (sin) hooked_roff("n");
			sin = FALSE;
#endif

			/* Dump "object(s)" */
			if (p) hooked_roff(p);
			hooked_roff(
				_("アイテム", " object"));

#ifndef JP
			if (n != 1) hooked_roff("s");
#endif

			/* Conjunction replaces variety, if needed for "gold" below */
			p = _("や", " or");
		}

		/* Treasures */
		if (drop_gold)
		{
#ifndef JP
			/* Cancel prefix */
			if (!p) sin = FALSE;

			/* Handle singular "an" */
			if (sin) hooked_roff("n");
			sin = FALSE;
#endif

			/* Dump "treasure(s)" */
			if (p) hooked_roff(p);
			hooked_roff(_("財宝", " treasure"));
#ifndef JP
			if (n != 1) hooked_roff("s");
#endif

		}

		/* End this sentence */
		hooked_roff(_("を持っていることがある。", ".  "));
	}


	/* Count the number of "known" attacks */
	for (n = 0, m = 0; m < 4; m++)
	{
		/* Skip non-attacks */
		if (!r_ptr->blow[m].method) continue;
		if (r_ptr->blow[m].method == RBM_SHOOT) continue;

		/* Count known attacks */
		if (r_ptr->r_blows[m] || know_everything) n++;
	}

	/* Examine (and count) the actual attacks */
	for (r = 0, m = 0; m < 4; m++)
	{
		int method, effect, d1, d2;

		/* Skip non-attacks */
		if (!r_ptr->blow[m].method) continue;
		if (r_ptr->blow[m].method == RBM_SHOOT) continue;

		/* Skip unknown attacks */
		if (!r_ptr->r_blows[m] && !know_everything) continue;

		/* Extract the attack info */
		method = r_ptr->blow[m].method;
		effect = r_ptr->blow[m].effect;
		d1 = r_ptr->blow[m].d_dice;
		d2 = r_ptr->blow[m].d_side;

		/* No method yet */
		p = NULL;

		/* Acquire the method */
		switch (method)
		{
			case RBM_HIT:		p = _("殴る", "hit"); break;
			case RBM_TOUCH:		p = _("触る", "touch"); break;
			case RBM_PUNCH:		p = _("パンチする", "punch"); break;
			case RBM_KICK:		p = _("蹴る", "kick"); break;
			case RBM_CLAW:		p = _("ひっかく", "claw"); break;
			case RBM_BITE:		p = _("噛む", "bite"); break;
			case RBM_STING:		p = _("刺す", "sting"); break;
			case RBM_SLASH:		p = _("斬る", "slash"); break;
			case RBM_BUTT:		p = _("角で突く", "butt"); break;
			case RBM_CRUSH:		p = _("体当たりする", "crush"); break;
			case RBM_ENGULF:	p = _("飲み込む", "engulf"); break;
			case RBM_CHARGE: 	p = _("請求書をよこす", "charge"); break;
			case RBM_CRAWL:		p = _("体の上を這い回る", "crawl on you"); break;
			case RBM_DROOL:		p = _("よだれをたらす", "drool on you"); break;
			case RBM_SPIT:		p = _("つばを吐く", "spit"); break;
			case RBM_EXPLODE:	p = _("爆発する", "explode"); break;
			case RBM_GAZE:		p = _("にらむ", "gaze"); break;
			case RBM_WAIL:		p = _("泣き叫ぶ", "wail"); break;
			case RBM_SPORE:		p = _("胞子を飛ばす", "release spores"); break;
			case RBM_XXX4:		break;
			case RBM_BEG:		p = _("金をせがむ", "beg"); break;
			case RBM_INSULT:	p = _("侮辱する", "insult"); break;
			case RBM_MOAN:		p = _("うめく", "moan"); break;
			case RBM_SHOW:  	p = _("歌う", "sing"); break;
		}


		/* Default effect */
		q = NULL;

		/* Acquire the effect */
		switch (effect)
		{
			case RBE_SUPERHURT: q = _("強力に攻撃する", "slaughter"); break;
			case RBE_HURT:    	q = _("攻撃する", "attack"); break;
			case RBE_POISON:  	q = _("毒をくらわす", "poison"); break;
			case RBE_UN_BONUS:	q = _("劣化させる", "disenchant"); break;
			case RBE_UN_POWER:	q = _("充填魔力を吸収する", "drain charges"); break;
			case RBE_EAT_GOLD:	q = _("金を盗む", "steal gold"); break;
			case RBE_EAT_ITEM:	q = _("アイテムを盗む", "steal items"); break;
			case RBE_EAT_FOOD:	q = _("あなたの食料を食べる", "eat your food"); break;
			case RBE_EAT_LITE:	q = _("明かりを吸収する", "absorb light"); break;
			case RBE_ACID:    	q = _("酸を飛ばす", "shoot acid"); break;
			case RBE_ELEC:    	q = _("感電させる", "electrocute"); break;
			case RBE_FIRE:    	q = _("燃やす", "burn"); break;
			case RBE_COLD:    	q = _("凍らせる", "freeze"); break;
			case RBE_BLIND:   	q = _("盲目にする", "blind"); break;
			case RBE_CONFUSE: 	q = _("混乱させる", "confuse"); break;
			case RBE_TERRIFY: 	q = _("恐怖させる", "terrify"); break;
			case RBE_PARALYZE:	q = _("麻痺させる", "paralyze"); break;
			case RBE_LOSE_STR:	q = _("腕力を減少させる", "reduce strength"); break;
			case RBE_LOSE_INT:	q = _("知能を減少させる", "reduce intelligence"); break;
			case RBE_LOSE_WIS:	q = _("賢さを減少させる", "reduce wisdom"); break;
			case RBE_LOSE_DEX:	q = _("器用さを減少させる", "reduce dexterity"); break;
			case RBE_LOSE_CON:	q = _("耐久力を減少させる", "reduce constitution"); break;
			case RBE_LOSE_CHR:	q = _("魅力を減少させる", "reduce charisma"); break;
			case RBE_LOSE_ALL:	q = _("全ステータスを減少させる", "reduce all stats"); break;
			case RBE_SHATTER:	q = _("粉砕する", "shatter"); break;
			case RBE_EXP_10:	q = _("経験値を減少(10d6+)させる", "lower experience (by 10d6+)"); break;
			case RBE_EXP_20:	q = _("経験値を減少(20d6+)させる", "lower experience (by 20d6+)"); break;
			case RBE_EXP_40:	q = _("経験値を減少(40d6+)させる", "lower experience (by 40d6+)"); break;
			case RBE_EXP_80:	q = _("経験値を減少(80d6+)させる", "lower experience (by 80d6+)"); break;
			case RBE_DISEASE:	q = _("病気にする", "disease"); break;
			case RBE_TIME:      q = _("時間を逆戻りさせる", "time"); break;
			case RBE_DR_LIFE:   q = _("生命力を吸収する", "drain life"); break;
			case RBE_DR_MANA:   q = _("魔力を奪う", "drain mana force"); break;
			case RBE_INERTIA:   q = _("減速させる", "slow"); break;
			case RBE_STUN:      q = _("朦朧とさせる", "stun"); break;
		}


#ifdef JP
		if ( r == 0 ) hooked_roff( format("%^sは", wd_he[msex]) );

		/***若干表現を変更 ita ***/

			/* Describe damage (if known) */
		if (d1 && d2 && (know_everything || know_damage(r_idx, m)))
		  {
		    
		    /* Display the damage */
		    hooked_roff(format(" %dd%d ", d1, d2));
		    hooked_roff("のダメージで");
		  }
		/* Hack -- force a method */
		if (!p) p = "何か奇妙なことをする";

		/* Describe the method */
		/* XXしてYYし/XXしてYYする/XXし/XXする */
		if(q) jverb( p ,jverb_buf, JVERB_TO);
		else if(r!=n-1) jverb( p ,jverb_buf, JVERB_AND);
		else strcpy(jverb_buf, p);

		hooked_roff(jverb_buf);

		/* Describe the effect (if any) */
		if (q)
		{
		  if(r!=n-1) jverb( q,jverb_buf, JVERB_AND);
		  else strcpy(jverb_buf,q); 
		  hooked_roff(jverb_buf);
		}
		if(r!=n-1) hooked_roff("、");
#else
		/* Introduce the attack description */
		if (!r)
		{
			hooked_roff(format("%^s can ", wd_he[msex]));
		}
		else if (r < n-1)
		{
			hooked_roff(", ");
		}
		else
		{
			hooked_roff(", and ");
		}


		/* Hack -- force a method */
		if (!p) p = "do something weird";

		/* Describe the method */
		hooked_roff(p);


		/* Describe the effect (if any) */
		if (q)
		{
			/* Describe the attack type */
			hooked_roff(" to ");
			hooked_roff(q);

			/* Describe damage (if known) */
			if (d1 && d2 && (know_everything || know_damage(r_idx, m)))
			{
				/* Display the damage */
				hooked_roff(" with damage");
				hooked_roff(format(" %dd%d", d1, d2));
			}
		}
#endif



		/* Count the attacks as printed */
		r++;
	}

	/* Finish sentence above */
	if (r)
	{
		hooked_roff(_("。", ".  "));
	}

	/* Notice lack of attacks */
	else if (flags1 & RF1_NEVER_BLOW)
	{
		hooked_roff(format(
			_("%^sは物理的な攻撃方法を持たない。",
			  "%^s has no physical attacks.  "), wd_he[msex]));
	}

	/* Or describe the lack of knowledge */
	else
	{
		hooked_roff(format(
			_("%s攻撃については何も知らない。",
			  "Nothing is known about %s attack.  "), wd_his[msex]));
	}


	/*
	 * Notice "Quest" monsters, but only if you
	 * already encountered the monster.
	 */
	if ((flags1 & RF1_QUESTOR) && ((r_ptr->r_sights) && (r_ptr->max_num) && ((r_idx == MON_OBERON) || (r_idx == MON_SERPENT))))
	{
		hook_c_roff(TERM_VIOLET, 
			_("あなたはこのモンスターを殺したいという強い欲望を感じている...",
			  "You feel an intense desire to kill this monster...  "));
	}

	else if (flags7 & RF7_GUARDIAN)
	{
		hook_c_roff(TERM_L_RED, 
			_("このモンスターはダンジョンの主である。",
			  "This monster is the master of a dungeon."));
	}


	/* All done */
	hooked_roff("\n");

}


/*!
 * @brief モンスター情報のヘッダを記述する
 * Hack -- Display the "name" and "attr/chars" of a monster race
 * @param r_idx モンスターの種族ID
 * @return なし
 */
void roff_top(MONRACE_IDX r_idx)
{
	monster_race	*r_ptr = &r_info[r_idx];

	TERM_COLOR	a1, a2;
	char		c1, c2;


	/* Access the chars */
	c1 = r_ptr->d_char;
	c2 = r_ptr->x_char;

	/* Access the attrs */
	a1 = r_ptr->d_attr;
	a2 = r_ptr->x_attr;


	/* Clear the top line */
	Term_erase(0, 0, 255);

	/* Reset the cursor */
	Term_gotoxy(0, 0);

#ifndef JP
	/* A title (use "The" for non-uniques) */
	if (!(r_ptr->flags1 & RF1_UNIQUE))
	{
		Term_addstr(-1, TERM_WHITE, "The ");
	}
#endif

	/* Dump the name */
	Term_addstr(-1, TERM_WHITE, (r_name + r_ptr->name));

	/* Append the "standard" attr/char info */
	Term_addstr(-1, TERM_WHITE, " ('");
	Term_add_bigch(a1, c1);
	Term_addstr(-1, TERM_WHITE, "')");

	/* Append the "optional" attr/char info */
	Term_addstr(-1, TERM_WHITE, "/('");
	Term_add_bigch(a2, c2);
	Term_addstr(-1, TERM_WHITE, "'):");

	/* Wizards get extra info */
	if (p_ptr->wizard)
	{
		char buf[16];

		sprintf(buf, "%d", r_idx);

		Term_addstr(-1, TERM_WHITE, " (");
		Term_addstr(-1, TERM_L_BLUE, buf);
		Term_addch(TERM_WHITE, ')');
	}
}



/*!
 * @brief  モンスター情報の表示と共に画面を一時消去するサブルーチン /
 * Hack -- describe the given monster race at the top of the screen
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 * @return なし
 */
void screen_roff(MONRACE_IDX r_idx, BIT_FLAGS mode)
{
	msg_erase();

	/* Begin recall */
	Term_erase(0, 1, 255);

	hook_c_roff = c_roff;

	/* Recall monster */
	roff_aux(r_idx, mode);

	/* Describe monster */
	roff_top(r_idx);
}




/*!
 * @brief モンスター情報の現在のウィンドウに表示する /
 * Hack -- describe the given monster race in the current "term" window
 * @param r_idx モンスターの種族ID
 * @return なし
 */
void display_roff(MONRACE_IDX r_idx)
{
	int y;

	/* Erase the window */
	for (y = 0; y < Term->hgt; y++)
	{
		/* Erase the line */
		Term_erase(0, y, 255);
	}

	/* Begin recall */
	Term_gotoxy(0, 1);

	hook_c_roff = c_roff;

	/* Recall monster */
	roff_aux(r_idx, 0);

	/* Describe monster */
	roff_top(r_idx);
}


/*!
 * @brief モンスター詳細情報を自動スポイラー向けに出力する /
 * Hack -- output description of the given monster race
 * @param r_idx モンスターの種族ID
 * @param roff_func 出力処理を行う関数ポインタ
 * @return なし
 */
void output_monster_spoiler(MONRACE_IDX r_idx, void (*roff_func)(TERM_COLOR attr, concptr str))
{
	hook_c_roff = roff_func;

	/* Recall monster */
	roff_aux(r_idx, 0x03);
}



/*!
 * @brief プレイヤーの現在の広域マップ座標から得た地勢を元にモンスターの生成条件関数を返す
 * @return 地勢にあったモンスターの生成条件関数
 */
monsterrace_hook_type get_monster_hook(void)
{
	if (!current_floor_ptr->dun_level && !p_ptr->inside_quest)
	{
		switch (wilderness[p_ptr->wilderness_y][p_ptr->wilderness_x].terrain)
		{
		case TERRAIN_TOWN:
			return (monsterrace_hook_type)mon_hook_town;
		case TERRAIN_DEEP_WATER:
			return (monsterrace_hook_type)mon_hook_ocean;
		case TERRAIN_SHALLOW_WATER:
		case TERRAIN_SWAMP:
			return (monsterrace_hook_type)mon_hook_shore;
		case TERRAIN_DIRT:
		case TERRAIN_DESERT:
			return (monsterrace_hook_type)mon_hook_waste;
		case TERRAIN_GRASS:
			return (monsterrace_hook_type)mon_hook_grass;
		case TERRAIN_TREES:
			return (monsterrace_hook_type)mon_hook_wood;
		case TERRAIN_SHALLOW_LAVA:
		case TERRAIN_DEEP_LAVA:
			return (monsterrace_hook_type)mon_hook_volcano;
		case TERRAIN_MOUNTAIN:
			return (monsterrace_hook_type)mon_hook_mountain;
		default:
			return (monsterrace_hook_type)mon_hook_dungeon;
		}
	}
	else
	{
		return (monsterrace_hook_type)mon_hook_dungeon;
	}
}

/*!
 * @brief 指定された広域マップ座標の地勢を元にモンスターの生成条件関数を返す
 * @return 地勢にあったモンスターの生成条件関数
 */
monsterrace_hook_type get_monster_hook2(POSITION y, POSITION x)
{
	feature_type *f_ptr = &f_info[current_floor_ptr->grid_array[y][x].feat];

	/* Set the monster list */

	/* Water */
	if (have_flag(f_ptr->flags, FF_WATER))
	{
		/* Deep water */
		if (have_flag(f_ptr->flags, FF_DEEP))
		{
			return (monsterrace_hook_type)mon_hook_deep_water;
		}

		/* Shallow water */
		else
		{
			return (monsterrace_hook_type)mon_hook_shallow_water;
		}
	}

	/* Lava */
	else if (have_flag(f_ptr->flags, FF_LAVA))
	{
		return (monsterrace_hook_type)mon_hook_lava;
	}

	else return (monsterrace_hook_type)mon_hook_floor;
}

/*!
 * @brief モンスターを友好的にする
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_friendly(monster_type *m_ptr)
{
	m_ptr->smart |= SM_FRIENDLY;
}

/*!
 * @brief モンスターをペットにする
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_pet(monster_type *m_ptr)
{
	check_quest_completion(m_ptr);

	m_ptr->smart |= SM_PET;
	if (!(r_info[m_ptr->r_idx].flags3 & (RF3_EVIL | RF3_GOOD)))
		m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
}

/*!
 * @brief モンスターを敵に回す
 * Makes the monster hostile towards the player
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_hostile(monster_type *m_ptr)
{
	if (p_ptr->inside_battle) return;
	m_ptr->smart &= ~SM_PET;
	m_ptr->smart &= ~SM_FRIENDLY;
}


/*!
 * @brief モンスターを怒らせる
 * Anger the monster
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void anger_monster(monster_type *m_ptr)
{
	if (p_ptr->inside_battle) return;
	if (is_friendly(m_ptr))
	{
		GAME_TEXT m_name[MAX_NLEN];

		monster_desc(m_name, m_ptr, 0);
		msg_format(_("%^sは怒った！", "%^s gets angry!"), m_name);

		set_hostile(m_ptr);

		chg_virtue(V_INDIVIDUALISM, 1);
		chg_virtue(V_HONOUR, -1);
		chg_virtue(V_JUSTICE, -1);
		chg_virtue(V_COMPASSION, -1);
	}
}


/*!
 * @brief モンスターが地形を踏破できるかどうかを返す
 * Check if monster can cross terrain
 * @param feat 地形ID
 * @param r_ptr モンスター種族構造体の参照ポインタ
 * @param mode オプション
 * @return 踏破可能ならばTRUEを返す
 */
bool monster_can_cross_terrain(FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode)
{
	feature_type *f_ptr = &f_info[feat];

	if (have_flag(f_ptr->flags, FF_PATTERN))
	{
		if (!(mode & CEM_RIDING))
		{
			if (!(r_ptr->flags7 & RF7_CAN_FLY)) return FALSE;
		}
		else
		{
			if (!(mode & CEM_P_CAN_ENTER_PATTERN)) return FALSE;
		}
	}

	/* "CAN" flags */
	if (have_flag(f_ptr->flags, FF_CAN_FLY) && (r_ptr->flags7 & RF7_CAN_FLY)) return TRUE;
	if (have_flag(f_ptr->flags, FF_CAN_SWIM) && (r_ptr->flags7 & RF7_CAN_SWIM)) return TRUE;
	if (have_flag(f_ptr->flags, FF_CAN_PASS))
	{
		if ((r_ptr->flags2 & RF2_PASS_WALL) && (!(mode & CEM_RIDING) || p_ptr->pass_wall)) return TRUE;
	}

	if (!have_flag(f_ptr->flags, FF_MOVE)) return FALSE;

	/* Some monsters can walk on mountains */
	if (have_flag(f_ptr->flags, FF_MOUNTAIN) && (r_ptr->flags8 & RF8_WILD_MOUNTAIN)) return TRUE;

	/* Water */
	if (have_flag(f_ptr->flags, FF_WATER))
	{
		if (!(r_ptr->flags7 & RF7_AQUATIC))
		{
			/* Deep water */
			if (have_flag(f_ptr->flags, FF_DEEP)) return FALSE;

			/* Shallow water */
			else if (r_ptr->flags2 & RF2_AURA_FIRE) return FALSE;
		}
	}

	/* Aquatic monster into non-water? */
	else if (r_ptr->flags7 & RF7_AQUATIC) return FALSE;

	/* Lava */
	if (have_flag(f_ptr->flags, FF_LAVA))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK)) return FALSE;
	}

	/* Cold */
	if (have_flag(f_ptr->flags, FF_COLD_PUDDLE))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK)) return FALSE;
	}

	/* Elec */
	if (have_flag(f_ptr->flags, FF_ELEC_PUDDLE))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK)) return FALSE;
	}

	/* Acid */
	if (have_flag(f_ptr->flags, FF_ACID_PUDDLE))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_ACID_MASK)) return FALSE;
	}

	/* Poison */
	if (have_flag(f_ptr->flags, FF_POISON_PUDDLE))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_POIS_MASK)) return FALSE;
	}

	return TRUE;
}


/*!
 * @brief 指定された座標の地形をモンスターが踏破できるかどうかを返す
 * Strictly check if monster can enter the grid
 * @param y 地形のY座標
 * @param x 地形のX座標
 * @param r_ptr モンスター種族構造体の参照ポインタ
 * @param mode オプション
 * @return 踏破可能ならばTRUEを返す
 */
bool monster_can_enter(POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Player or other monster */
	if (player_bold(y, x)) return FALSE;
	if (g_ptr->m_idx) return FALSE;

	return monster_can_cross_terrain(g_ptr->feat, r_ptr, mode);
}


/*!
 * @brief モンスターの属性の基づいた敵対関係の有無を返す（サブルーチン）
 * Check if this monster has "hostile" alignment (aux)
 * @param sub_align1 モンスター1のサブフラグ
 * @param sub_align2 モンスター2のサブフラグ
 * @return 敵対関係にあるならばTRUEを返す
 */
static bool check_hostile_align(byte sub_align1, byte sub_align2)
{
	if (sub_align1 != sub_align2)
	{
		if (((sub_align1 & SUB_ALIGN_EVIL) && (sub_align2 & SUB_ALIGN_GOOD)) ||
			((sub_align1 & SUB_ALIGN_GOOD) && (sub_align2 & SUB_ALIGN_EVIL)))
			return TRUE;
	}

	/* Non-hostile alignment */
	return FALSE;
}


/*!
 * @brief モンスターの属性の基づいた敵対関係の有無を返す
 * Check if two monsters are enemies
 * @param m_ptr モンスター1の構造体参照ポインタ
 * @param n_ptr モンスター2の構造体参照ポインタ
 * @return 敵対関係にあるならばTRUEを返す
 */
bool are_enemies(monster_type *m_ptr, monster_type *n_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race *s_ptr = &r_info[n_ptr->r_idx];

	if (p_ptr->inside_battle)
	{
		if (is_pet(m_ptr) || is_pet(n_ptr)) return FALSE;
		return TRUE;
	}

	if ((r_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL))
	    && (s_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL)))
	{
		if (!is_pet(m_ptr) && !is_pet(n_ptr)) return FALSE;
	}

	/* Friendly vs. opposite aligned normal or pet */
	if (check_hostile_align(m_ptr->sub_align, n_ptr->sub_align))
	{
		if (!(m_ptr->mflag2 & MFLAG2_CHAMELEON) || !(n_ptr->mflag2 & MFLAG2_CHAMELEON)) return TRUE;
	}

	/* Hostile vs. non-hostile */
	if (is_hostile(m_ptr) != is_hostile(n_ptr))
	{
		return TRUE;
	}

	/* Default */
	return FALSE;
}


/*!
 * @brief モンスターがプレイヤーに対して敵意を抱くかどうかを返す
 * Check if this monster race has "hostile" alignment
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @param pa_good プレイヤーの善傾向値
 * @param pa_evil プレイヤーの悪傾向値
 * @param r_ptr モンスター種族情報の構造体参照ポインタ
 * @return プレイヤーに敵意を持つならばTRUEを返す
 * @details
 * If user is player, m_ptr == NULL.
 */
bool monster_has_hostile_align(monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr)
{
	byte sub_align1 = SUB_ALIGN_NEUTRAL;
	byte sub_align2 = SUB_ALIGN_NEUTRAL;

	if (m_ptr) /* For a monster */
	{
		sub_align1 = m_ptr->sub_align;
	}
	else /* For player */
	{
		if (p_ptr->align >= pa_good) sub_align1 |= SUB_ALIGN_GOOD;
		if (p_ptr->align <= pa_evil) sub_align1 |= SUB_ALIGN_EVIL;
	}

	/* Racial alignment flags */
	if (r_ptr->flags3 & RF3_EVIL) sub_align2 |= SUB_ALIGN_EVIL;
	if (r_ptr->flags3 & RF3_GOOD) sub_align2 |= SUB_ALIGN_GOOD;

	if (check_hostile_align(sub_align1, sub_align2)) return TRUE;

	/* Non-hostile alignment */
	return FALSE;
}

/*!
 * @brief モンスターを倒した際の財宝svalを返す
 * @param r_idx 倒したモンスターの種族ID
 * @return 財宝のsval
 * @details
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 * Note the use of actual "monster names"
 */
static OBJECT_SUBTYPE_VALUE get_coin_type(MONRACE_IDX r_idx)
{
	/* Analyze monsters */
	switch (r_idx)
	{
	case MON_COPPER_COINS: return 2;
	case MON_SILVER_COINS: return 5;
	case MON_GOLD_COINS: return 10;
	case MON_MITHRIL_COINS:
	case MON_MITHRIL_GOLEM: return 16;
	case MON_ADAMANT_COINS: return 17;
	}

	/* Assume nothing */
	return 0;
}


/*!
 * @brief モンスターが死亡した時の処理 /
 * Handle the "death" of a monster.
 * @param m_idx 死亡したモンスターのID
 * @param drop_item TRUEならばモンスターのドロップ処理を行う
 * @return 撃破されたモンスターの述語
 * @details
 * <pre>
 * Disperse treasures centered at the monster location based on the
 * various flags contained in the monster flags fields.
 * Check for "Quest" completion when a quest monster is killed.
 * Note that only the player can induce "monster_death()" on Uniques.
 * Thus (for now) all Quest monsters should be Uniques.
 * Note that monsters can now carry objects, and when a monster dies,
 * it drops all of its objects, which may disappear in crowded rooms.
 * </pre>
 */
void monster_death(MONSTER_IDX m_idx, bool drop_item)
{
	int i, j;
	POSITION y, x;

	int dump_item = 0;
	int dump_gold = 0;
	int number = 0;

	monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	bool visible = ((m_ptr->ml && !p_ptr->image) || (r_ptr->flags1 & RF1_UNIQUE));

	u32b mo_mode = 0L;

	bool do_gold = (!(r_ptr->flags1 & RF1_ONLY_ITEM));
	bool do_item = (!(r_ptr->flags1 & RF1_ONLY_GOLD));
	bool cloned = (m_ptr->smart & SM_CLONED) ? TRUE : FALSE;
	int force_coin = get_coin_type(m_ptr->r_idx);

	object_type forge;
	object_type *q_ptr;

	bool drop_chosen_item = drop_item && !cloned && !p_ptr->inside_arena
		&& !p_ptr->inside_battle && !is_pet(m_ptr);

	/* The caster is dead? */
	if (current_world_ptr->timewalk_m_idx && current_world_ptr->timewalk_m_idx == m_idx) current_world_ptr->timewalk_m_idx = 0;

	/* Notice changes in view */
	if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
	{
		p_ptr->update |= (PU_MON_LITE);
	}

	y = m_ptr->fy;
	x = m_ptr->fx;

	if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
	{
		GAME_TEXT m_name[MAX_NLEN];

		monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
		do_cmd_write_nikki(NIKKI_NAMED_PET, 3, m_name);
	}

	/* Let monsters explode! */
	for (i = 0; i < 4; i++)
	{
		if (r_ptr->blow[i].method == RBM_EXPLODE)
		{
			BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
			EFFECT_ID typ = mbe_info[r_ptr->blow[i].effect].explode_type;
			DICE_NUMBER d_dice = r_ptr->blow[i].d_dice;
			DICE_SID d_side = r_ptr->blow[i].d_side;
			HIT_POINT damage = damroll(d_dice, d_side);

			project(m_idx, 3, y, x, damage, typ, flg, -1);
			break;
		}
	}

	if (m_ptr->mflag2 & MFLAG2_CHAMELEON)
	{
		choose_new_monster(m_idx, TRUE, MON_CHAMELEON);
		r_ptr = &r_info[m_ptr->r_idx];
	}

	check_quest_completion(m_ptr);

	/* Handle the possibility of player vanquishing arena combatant -KMW- */
	if (p_ptr->inside_arena && !is_pet(m_ptr))
	{
		p_ptr->exit_bldg = TRUE;

		if (p_ptr->arena_number > MAX_ARENA_MONS)
		{
			msg_print(_("素晴らしい！君こそ真の勝利者だ。", "You are a Genuine Champion!"));
		}
		else
		{
			msg_print(_("勝利！チャンピオンへの道を進んでいる。", "Victorious! You're on your way to becoming Champion."));
		}

		if (arena_info[p_ptr->arena_number].tval)
		{
			q_ptr = &forge;

			/* Prepare to make a prize */
			object_prep(q_ptr, lookup_kind(arena_info[p_ptr->arena_number].tval, arena_info[p_ptr->arena_number].sval));
			apply_magic(q_ptr, current_floor_ptr->object_level, AM_NO_FIXED_ART);
			(void)drop_near(q_ptr, -1, y, x);
		}

		if (p_ptr->arena_number > MAX_ARENA_MONS) p_ptr->arena_number++;
		p_ptr->arena_number++;
		if (record_arena)
		{
			GAME_TEXT m_name[MAX_NLEN];

			monster_desc(m_name, m_ptr, MD_WRONGDOER_NAME);

			do_cmd_write_nikki(NIKKI_ARENA, p_ptr->arena_number, m_name);
		}
	}

	if (m_idx == p_ptr->riding)
	{
		if (rakuba(-1, FALSE))
		{
			msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));
		}
	}

	/* Drop a dead corpse? */
	if (one_in_(r_ptr->flags1 & RF1_UNIQUE ? 1 : 4) &&
		(r_ptr->flags9 & (RF9_DROP_CORPSE | RF9_DROP_SKELETON)) &&
		!(p_ptr->inside_arena || p_ptr->inside_battle || cloned || ((m_ptr->r_idx == today_mon) && is_pet(m_ptr))))
	{
		/* Assume skeleton */
		bool corpse = FALSE;

		/*
		 * We cannot drop a skeleton? Note, if we are in this check,
		 * we *know* we can drop at least a corpse or a skeleton
		 */
		if (!(r_ptr->flags9 & RF9_DROP_SKELETON))
			corpse = TRUE;
		else if ((r_ptr->flags9 & RF9_DROP_CORPSE) && (r_ptr->flags1 & RF1_UNIQUE))
			corpse = TRUE;

		/* Else, a corpse is more likely unless we did a "lot" of damage */
		else if (r_ptr->flags9 & RF9_DROP_CORPSE)
		{
			/* Lots of damage in one blow */
			if ((0 - ((m_ptr->maxhp) / 4)) > m_ptr->hp)
			{
				if (one_in_(5)) corpse = TRUE;
			}
			else
			{
				if (!one_in_(5)) corpse = TRUE;
			}
		}
		q_ptr = &forge;

		/* Prepare to make an object */
		object_prep(q_ptr, lookup_kind(TV_CORPSE, (corpse ? SV_CORPSE : SV_SKELETON)));

		apply_magic(q_ptr, current_floor_ptr->object_level, AM_NO_FIXED_ART);

		q_ptr->pval = m_ptr->r_idx;
		(void)drop_near(q_ptr, -1, y, x);
	}

	/* Drop objects being carried */
	monster_drop_carried_objects(m_ptr);

	if (r_ptr->flags1 & RF1_DROP_GOOD) mo_mode |= AM_GOOD;
	if (r_ptr->flags1 & RF1_DROP_GREAT) mo_mode |= AM_GREAT;

	switch (m_ptr->r_idx)
	{
	case MON_PINK_HORROR:
		/* Pink horrors are replaced with 2 Blue horrors */
		if (!(p_ptr->inside_arena || p_ptr->inside_battle))
		{
			bool notice = FALSE;

			for (i = 0; i < 2; i++)
			{
				POSITION wy = y, wx = x;
				bool pet = is_pet(m_ptr);
				BIT_FLAGS mode = 0L;

				if (pet) mode |= PM_FORCE_PET;

				if (summon_specific((pet ? -1 : m_idx), wy, wx, 100, SUMMON_BLUE_HORROR, mode, '\0'))
				{
					if (player_can_see_bold(wy, wx)) notice = TRUE;
				}
			}

			if (notice) msg_print(_("ピンク・ホラーは分裂した！", "The Pink horror divides!"));
		}
		break;

	case MON_BLOODLETTER:
		/* Bloodletters of Khorne may drop a blade of chaos */
		if (drop_chosen_item && (randint1(100) < 15))
		{
			q_ptr = &forge;

			/* Prepare to make a Blade of Chaos */
			object_prep(q_ptr, lookup_kind(TV_SWORD, SV_BLADE_OF_CHAOS));

			apply_magic(q_ptr, current_floor_ptr->object_level, AM_NO_FIXED_ART | mo_mode);
			(void)drop_near(q_ptr, -1, y, x);
		}
		break;

	case MON_RAAL:
		if (drop_chosen_item && (current_floor_ptr->dun_level > 9))
		{
			q_ptr = &forge;
			object_wipe(q_ptr);

			/* Activate restriction */
			if ((current_floor_ptr->dun_level > 49) && one_in_(5))
				get_obj_num_hook = kind_is_good_book;
			else
				get_obj_num_hook = kind_is_book;

			/* Make a book */
			make_object(q_ptr, mo_mode);
			(void)drop_near(q_ptr, -1, y, x);
		}
		break;

	case MON_DAWN:
		/*
		 * Mega^3-hack: killing a 'Warrior of the Dawn' is likely to
		 * spawn another in the fallen one's place!
		 */
		if (!p_ptr->inside_arena && !p_ptr->inside_battle)
		{
			if (!one_in_(7))
			{
				POSITION wy = y, wx = x;
				int attempts = 100;
				bool pet = is_pet(m_ptr);

				do
				{
					scatter(&wy, &wx, y, x, 20, 0);
				} while (!(in_bounds(wy, wx) && cave_empty_bold2(wy, wx)) && --attempts);

				if (attempts > 0)
				{
					BIT_FLAGS mode = 0L;
					if (pet) mode |= PM_FORCE_PET;

					if (summon_specific((pet ? -1 : m_idx), wy, wx, 100, SUMMON_DAWN, mode, '\0'))
					{
						if (player_can_see_bold(wy, wx))
							msg_print(_("新たな戦士が現れた！", "A new warrior steps forth!"));
					}
				}
			}
		}
		break;

	case MON_UNMAKER:
		/* One more ultra-hack: An Unmaker goes out with a big bang! */
	{
		BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
		(void)project(m_idx, 6, y, x, 100, GF_CHAOS, flg, -1);
	}
	break;

	case MON_UNICORN_ORD:
	case MON_MORGOTH:
	case MON_ONE_RING:
		/* Reward for "lazy" player */
		if (p_ptr->pseikaku == SEIKAKU_NAMAKE)
		{
			ARTIFACT_IDX a_idx = 0;
			artifact_type *a_ptr = NULL;

			if (!drop_chosen_item) break;

			do
			{
				switch (randint0(3))
				{
				case 0:
					a_idx = ART_NAMAKE_HAMMER;
					break;
				case 1:
					a_idx = ART_NAMAKE_BOW;
					break;
				case 2:
					a_idx = ART_NAMAKE_ARMOR;
					break;
				}

				a_ptr = &a_info[a_idx];
			} while (a_ptr->cur_num);

			/* Create the artifact */
			if (create_named_art(a_idx, y, x))
			{
				a_ptr->cur_num = 1;

				/* Hack -- Memorize location of artifact in saved floors */
				if (character_dungeon) a_ptr->floor_id = p_ptr->floor_id;
			}
			else if (!preserve_mode) a_ptr->cur_num = 1;
		}
		break;

	case MON_SERPENT:
		if (!drop_chosen_item) break;
		q_ptr = &forge;

		/* Mega-Hack -- Prepare to make "Grond" */
		object_prep(q_ptr, lookup_kind(TV_HAFTED, SV_GROND));

		/* Mega-Hack -- Mark this item as "Grond" */
		q_ptr->name1 = ART_GROND;

		/* Mega-Hack -- Actually create "Grond" */
		apply_magic(q_ptr, -1, AM_GOOD | AM_GREAT);
		(void)drop_near(q_ptr, -1, y, x);
		q_ptr = &forge;

		/* Mega-Hack -- Prepare to make "Chaos" */
		object_prep(q_ptr, lookup_kind(TV_CROWN, SV_CHAOS));

		/* Mega-Hack -- Mark this item as "Chaos" */
		q_ptr->name1 = ART_CHAOS;

		/* Mega-Hack -- Actually create "Chaos" */
		apply_magic(q_ptr, -1, AM_GOOD | AM_GREAT);
		(void)drop_near(q_ptr, -1, y, x);
		break;

	case MON_B_DEATH_SWORD:
		if (drop_chosen_item)
		{
			q_ptr = &forge;

			/* Prepare to make a broken sword */
			object_prep(q_ptr, lookup_kind(TV_SWORD, randint1(2)));
			(void)drop_near(q_ptr, -1, y, x);
		}
		break;

	case MON_A_GOLD:
	case MON_A_SILVER:
		if (drop_chosen_item && ((m_ptr->r_idx == MON_A_GOLD) ||
			((m_ptr->r_idx == MON_A_SILVER) && (r_ptr->r_akills % 5 == 0))))
		{
			q_ptr = &forge;

			/* Prepare to make a Can of Toys */
			object_prep(q_ptr, lookup_kind(TV_CHEST, SV_CHEST_KANDUME));

			apply_magic(q_ptr, current_floor_ptr->object_level, AM_NO_FIXED_ART);
			(void)drop_near(q_ptr, -1, y, x);
		}
		break;

	case MON_ROLENTO:
	{
		BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
		(void)project(m_idx, 3, y, x, damroll(20, 10), GF_FIRE, flg, -1);
	}
	break;

	default:
		if (!drop_chosen_item) break;

		switch (r_ptr->d_char)
		{
		case '(':
			if (current_floor_ptr->dun_level > 0)
			{
				q_ptr = &forge;
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_cloak;

				/* Make a cloak */
				make_object(q_ptr, mo_mode);
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;

		case '/':
			if (current_floor_ptr->dun_level > 4)
			{
				q_ptr = &forge;
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_polearm;

				/* Make a poleweapon */
				make_object(q_ptr, mo_mode);
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;

		case '[':
			if (current_floor_ptr->dun_level > 19)
			{
				q_ptr = &forge;
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_armor;

				/* Make a hard armor */
				make_object(q_ptr, mo_mode);
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;

		case '\\':
			if (current_floor_ptr->dun_level > 4)
			{
				q_ptr = &forge;
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_hafted;

				/* Make a hafted weapon */
				make_object(q_ptr, mo_mode);
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;

		case '|':
			if (m_ptr->r_idx != MON_STORMBRINGER)
			{
				q_ptr = &forge;
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_sword;

				/* Make a sword */
				make_object(q_ptr, mo_mode);
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;
		}
		break;
	}

	/* Mega-Hack -- drop fixed items */
	if (drop_chosen_item)
	{
		ARTIFACT_IDX a_idx = 0;
		PERCENTAGE chance = 0;

		for (i = 0; i < 4; i++)
		{
			if (!r_ptr->artifact_id[i]) break;
			a_idx = r_ptr->artifact_id[i];
			chance = r_ptr->artifact_percent[i];
		}

		if ((a_idx > 0) && ((randint0(100) < chance) || p_ptr->wizard))
		{
			artifact_type *a_ptr = &a_info[a_idx];

			if (!a_ptr->cur_num)
			{
				/* Create the artifact */
				if (create_named_art(a_idx, y, x))
				{
					a_ptr->cur_num = 1;

					/* Hack -- Memorize location of artifact in saved floors */
					if (character_dungeon) a_ptr->floor_id = p_ptr->floor_id;
				}
				else if (!preserve_mode) a_ptr->cur_num = 1;
			}
		}

		if ((r_ptr->flags7 & RF7_GUARDIAN) && (d_info[p_ptr->dungeon_idx].final_guardian == m_ptr->r_idx))
		{
			KIND_OBJECT_IDX k_idx = d_info[p_ptr->dungeon_idx].final_object ? d_info[p_ptr->dungeon_idx].final_object
				: lookup_kind(TV_SCROLL, SV_SCROLL_ACQUIREMENT);

			if (d_info[p_ptr->dungeon_idx].final_artifact)
			{
				a_idx = d_info[p_ptr->dungeon_idx].final_artifact;
				artifact_type *a_ptr = &a_info[a_idx];

				if (!a_ptr->cur_num)
				{
					/* Create the artifact */
					if (create_named_art(a_idx, y, x))
					{
						a_ptr->cur_num = 1;

						/* Hack -- Memorize location of artifact in saved floors */
						if (character_dungeon) a_ptr->floor_id = p_ptr->floor_id;
					}
					else if (!preserve_mode) a_ptr->cur_num = 1;

					/* Prevent rewarding both artifact and "default" object */
					if (!d_info[p_ptr->dungeon_idx].final_object) k_idx = 0;
				}
			}

			if (k_idx)
			{
				q_ptr = &forge;

				/* Prepare to make a reward */
				object_prep(q_ptr, k_idx);

				apply_magic(q_ptr, current_floor_ptr->object_level, AM_NO_FIXED_ART | AM_GOOD);
				(void)drop_near(q_ptr, -1, y, x);
			}
			msg_format(_("あなたは%sを制覇した！", "You have conquered %s!"), d_name + d_info[p_ptr->dungeon_idx].name);
		}
	}

	/* Determine how much we can drop */
	if ((r_ptr->flags1 & RF1_DROP_60) && (randint0(100) < 60)) number++;
	if ((r_ptr->flags1 & RF1_DROP_90) && (randint0(100) < 90)) number++;
	if (r_ptr->flags1 & RF1_DROP_1D2) number += damroll(1, 2);
	if (r_ptr->flags1 & RF1_DROP_2D2) number += damroll(2, 2);
	if (r_ptr->flags1 & RF1_DROP_3D2) number += damroll(3, 2);
	if (r_ptr->flags1 & RF1_DROP_4D2) number += damroll(4, 2);

	if (cloned && !(r_ptr->flags1 & RF1_UNIQUE))
		number = 0; /* Clones drop no stuff unless Cloning Pits */

	if (is_pet(m_ptr) || p_ptr->inside_battle || p_ptr->inside_arena)
		number = 0; /* Pets drop no stuff */
	if (!drop_item && (r_ptr->d_char != '$')) number = 0;

	if ((r_ptr->flags2 & (RF2_MULTIPLY)) && (r_ptr->r_akills > 1024))
		number = 0; /* Limit of Multiply monster drop */

	/* Hack -- handle creeping coins */
	coin_type = force_coin;

	/* Average dungeon and monster levels */
	current_floor_ptr->object_level = (current_floor_ptr->dun_level + r_ptr->level) / 2;

	/* Drop some objects */
	for (j = 0; j < number; j++)
	{
		q_ptr = &forge;
		object_wipe(q_ptr);

		/* Make Gold */
		if (do_gold && (!do_item || (randint0(100) < 50)))
		{
			/* Make some gold */
			if (!make_gold(q_ptr)) continue;

			dump_gold++;
		}

		/* Make Object */
		else
		{
			/* Make an object */
			if (!make_object(q_ptr, mo_mode)) continue;

			dump_item++;
		}
		(void)drop_near(q_ptr, -1, y, x);
	}

	/* Reset the object level */
	current_floor_ptr->object_level = current_floor_ptr->base_level;

	/* Reset "coin" type */
	coin_type = 0;


	/* Take note of any dropped treasure */
	if (visible && (dump_item || dump_gold))
	{
		/* Take notes on treasure */
		lore_treasure(m_idx, dump_item, dump_gold);
	}

	/* Only process "Quest Monsters" */
	if (!(r_ptr->flags1 & RF1_QUESTOR)) return;
	if (p_ptr->inside_battle) return;

	/* Winner? */
	if ((m_ptr->r_idx == MON_SERPENT) && !cloned)
	{
		/* Total winner */
		p_ptr->total_winner = TRUE;

		/* Redraw the "title" */
		p_ptr->redraw |= (PR_TITLE);

		play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FINAL_QUEST_CLEAR);

		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, _("見事に変愚蛮怒の勝利者となった！", "finally become *WINNER* of Hengband!"));

		admire_from_patron(p_ptr);

		/* Congratulations */
		msg_print(_("*** おめでとう ***", "*** CONGRATULATIONS ***"));
		msg_print(_("あなたはゲームをコンプリートしました。", "You have won the game!"));
		msg_print(_("準備が整ったら引退(自殺コマンド)しても結構です。", "You may retire (commit suicide) when you are ready."));
	}
}

/*!
 * @brief モンスターを撃破した際の述語メッセージを返す /
 * Return monster death string
 * @param r_ptr 撃破されたモンスターの種族情報を持つ構造体の参照ポインタ
 * @return 撃破されたモンスターの述語
 */
concptr extract_note_dies(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	/* Some monsters get "destroyed" */
	if (!monster_living(r_idx))
	{
		int i;

		for (i = 0; i < 4; i++)
		{
			if (r_ptr->blow[i].method == RBM_EXPLODE)
			{
				return _("は爆発して粉々になった。", " explodes into tiny shreds.");
			}
		}
		return _("を倒した。", " is destroyed.");
	}

	return _("は死んだ。", " dies.");
}

/*
 * Monster health description
 */
concptr look_mon_desc(monster_type *m_ptr, BIT_FLAGS mode)
{
	monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
	bool living;
	int perc;
	concptr desc;
	concptr attitude;
	concptr clone;

	/* Determine if the monster is "living" */
	living = monster_living(m_ptr->ap_r_idx);

	/* Calculate a health "percentage" */
	perc = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->maxhp : 0;

	/* Healthy monsters */
	if (m_ptr->hp >= m_ptr->maxhp)
	{
		desc = living ? _("無傷", "unhurt") : _("無ダメージ", "undamaged");
	}

	else if (perc >= 60)
	{
		desc = living ? _("軽傷", "somewhat wounded") : _("小ダメージ", "somewhat damaged");
	}

	else if (perc >= 25)
	{
		desc = living ? _("負傷", "wounded") : _("中ダメージ", "damaged");
	}

	else if (perc >= 10)
	{
		desc = living ? _("重傷", "badly wounded") : _("大ダメージ", "badly damaged");
	}

	else
	{
		desc = living ? _("半死半生", "almost dead") : _("倒れかけ", "almost destroyed");
	}

	/* Need attitude information? */
	if (!(mode & 0x01))
	{
		/* Full information is not needed */
		attitude = "";
	}
	else if (is_pet(m_ptr))
	{
		attitude = _(", ペット", ", pet");
	}
	else if (is_friendly(m_ptr))
	{
		attitude = _(", 友好的", ", friendly");
	}
	else
	{
		attitude = _("", "");
	}

	/* Clone monster? */
	if (m_ptr->smart & SM_CLONED)
	{
		clone = ", clone";
	}
	else
	{
		clone = "";
	}

	/* Display monster's level --- idea borrowed from ToME */
	if (ap_r_ptr->r_tkills && !(m_ptr->mflag2 & MFLAG2_KAGE))
	{
		return format(_("レベル%d, %s%s%s", "Level %d, %s%s%s"), ap_r_ptr->level, desc, attitude, clone);
	}
	else
	{
		return format(_("レベル???, %s%s%s", "Level ???, %s%s%s"), desc, attitude, clone);
	}

}

