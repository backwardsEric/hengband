﻿/*!
 * @file hex.c
 * @brief 呪術の処理実装 / Hex code
 * @date 2014/01/14
 * @author
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 * p_ptr-magic_num1\n
 * 0: Flag bits of spelling spells\n
 * 1: Flag bits of despelled spells\n
 * 2: Revange damage\n
 * p_ptr->magic_num2\n
 * 0: Number of spelling spells\n
 * 1: Type of revenge\n
 * 2: Turn count for revenge\n
 */

#include "angband.h"
#include "cmd-spell.h"
#include "cmd-quaff.h"
#include "object-hook.h"
#include "object-curse.h"
#include "projection.h"
#include "spells-status.h"
#include "player-status.h"

#define MAX_KEEP 4 /*!<呪術の最大詠唱数 */

/*!
 * @brief プレイヤーが詠唱中の全呪術を停止する
 * @return なし
 */
bool stop_hex_spell_all(void)
{
	SPELL_IDX i;

	for (i = 0; i < 32; i++)
	{
		if (hex_spelling(i)) do_spell(REALM_HEX, i, SPELL_STOP);
	}

	CASTING_HEX_FLAGS(p_ptr) = 0;
	CASTING_HEX_NUM(p_ptr) = 0;

	if (p_ptr->action == ACTION_SPELL) set_action(ACTION_NONE);

	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	p_ptr->redraw |= (PR_EXTRA | PR_HP | PR_MANA);

	return TRUE;
}

/*!
 * @brief プレイヤーが詠唱中の呪術から一つを選んで停止する
 * @return なし
 */
bool stop_hex_spell(void)
{
	int spell;
	char choice = 0;
	char out_val[160];
	bool flag = FALSE;
	TERM_LEN y = 1;
	TERM_LEN x = 20;
	int sp[MAX_KEEP];

	if (!hex_spelling_any())
	{
		msg_print(_("呪文を詠唱していません。", "You are not casting a spell."));
		return FALSE;
	}

	/* Stop all spells */
	else if ((CASTING_HEX_NUM(p_ptr) == 1) || (p_ptr->lev < 35))
	{
		return stop_hex_spell_all();
	}
	else
	{
		strnfmt(out_val, 78, _("どの呪文の詠唱を中断しますか？(呪文 %c-%c, 'l'全て, ESC)", "Which spell do you stop casting? (Spell %c-%c, 'l' to all, ESC)"),
			I2A(0), I2A(CASTING_HEX_NUM(p_ptr) - 1));

		screen_save();

		while (!flag)
		{
			int n = 0;
			Term_erase(x, y, 255);
			prt(_("     名前", "     Name"), y, x + 5);
			for (spell = 0; spell < 32; spell++)
			{
				if (hex_spelling(spell))
				{
					Term_erase(x, y + n + 1, 255);
					put_str(format("%c)  %s", I2A(n), do_spell(REALM_HEX, spell, SPELL_NAME)), y + n + 1, x + 2);
					sp[n++] = spell;
				}
			}

			if (!get_com(out_val, &choice, TRUE)) break;
			if (isupper(choice)) choice = (char)tolower(choice);

			if (choice == 'l')	/* All */
			{
				screen_load();
				return stop_hex_spell_all();
			}
			if ((choice < I2A(0)) || (choice > I2A(CASTING_HEX_NUM(p_ptr) - 1))) continue;
			flag = TRUE;
		}
	}

	screen_load();

	if (flag)
	{
		int n = sp[A2I(choice)];

		do_spell(REALM_HEX, n, SPELL_STOP);
		CASTING_HEX_FLAGS(p_ptr) &= ~(1L << n);
		CASTING_HEX_NUM(p_ptr)--;
	}

	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	p_ptr->redraw |= (PR_EXTRA | PR_HP | PR_MANA);

	return flag;
}


/*!
 * @brief 一定時間毎に呪術で消費するMPを処理する /
 * Upkeeping hex spells Called from dungeon.c
 * @return なし
 */
void check_hex(void)
{
	int spell;
	MANA_POINT need_mana;
	u32b need_mana_frac;
	bool res = FALSE;

	/* Spells spelled by player */
	if (p_ptr->realm1 != REALM_HEX) return;
	if (!CASTING_HEX_FLAGS(p_ptr) && !p_ptr->magic_num1[1]) return;

	if (p_ptr->magic_num1[1])
	{
		p_ptr->magic_num1[0] = p_ptr->magic_num1[1];
		p_ptr->magic_num1[1] = 0;
		res = TRUE;
	}

	/* Stop all spells when anti-magic ability is given */
	if (p_ptr->anti_magic)
	{
		stop_hex_spell_all();
		return;
	}

	need_mana = 0;
	for (spell = 0; spell < 32; spell++)
	{
		if (hex_spelling(spell))
		{
			const magic_type *s_ptr;
			s_ptr = &technic_info[REALM_HEX - MIN_TECHNIC][spell];
			need_mana += mod_need_mana(s_ptr->smana, spell, REALM_HEX);
		}
	}

	/* Culcurates final mana cost */
	need_mana_frac = 0;
	s64b_div(&need_mana, &need_mana_frac, 0, 3); /* Divide by 3 */
	need_mana += (CASTING_HEX_NUM(p_ptr) - 1);

	/* Not enough mana */
	if (s64b_cmp(p_ptr->csp, p_ptr->csp_frac, need_mana, need_mana_frac) < 0)
	{
		stop_hex_spell_all();
		return;
	}

	/* Enough mana */
	else
	{
		s64b_sub(&(p_ptr->csp), &(p_ptr->csp_frac), need_mana, need_mana_frac);

		p_ptr->redraw |= PR_MANA;
		if (res)
		{
			msg_print(_("詠唱を再開した。", "You restart casting."));

			p_ptr->action = ACTION_SPELL;

			p_ptr->update |= (PU_BONUS | PU_HP);
			p_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);
			p_ptr->update |= (PU_MONSTERS);
			p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		}
	}

	/* Gain experiences of spelling spells */
	for (spell = 0; spell < 32; spell++)
	{
		const magic_type *s_ptr;

		if (!hex_spelling(spell)) continue;

		s_ptr = &technic_info[REALM_HEX - MIN_TECHNIC][spell];

		if (p_ptr->spell_exp[spell] < SPELL_EXP_BEGINNER)
			p_ptr->spell_exp[spell] += 5;
		else if(p_ptr->spell_exp[spell] < SPELL_EXP_SKILLED)
		{ if (one_in_(2) && (current_floor_ptr->dun_level > 4) && ((current_floor_ptr->dun_level + 10) > p_ptr->lev)) p_ptr->spell_exp[spell] += 1; }
		else if(p_ptr->spell_exp[spell] < SPELL_EXP_EXPERT)
		{ if (one_in_(5) && ((current_floor_ptr->dun_level + 5) > p_ptr->lev) && ((current_floor_ptr->dun_level + 5) > s_ptr->slevel)) p_ptr->spell_exp[spell] += 1; }
		else if(p_ptr->spell_exp[spell] < SPELL_EXP_MASTER)
		{ if (one_in_(5) && ((current_floor_ptr->dun_level + 5) > p_ptr->lev) && (current_floor_ptr->dun_level > s_ptr->slevel)) p_ptr->spell_exp[spell] += 1; }
	}

	/* Do any effects of continual spells */
	for (spell = 0; spell < 32; spell++)
	{
		if (hex_spelling(spell))
		{
			do_spell(REALM_HEX, spell, SPELL_CONT);
		}
	}
}

/*!
 * @brief プレイヤーの呪術詠唱枠がすでに最大かどうかを返す
 * @return すでに全枠を利用しているならTRUEを返す
 */
bool hex_spell_fully(void)
{
	int k_max = 0;
	k_max = (p_ptr->lev / 15) + 1;
	k_max = MIN(k_max, MAX_KEEP);
	if (CASTING_HEX_NUM(p_ptr) < k_max) return FALSE;
	return TRUE;
}

/*!
 * @brief 一定ゲームターン毎に復讐処理の残り期間の判定を行う
 * @return なし
 */
void revenge_spell(void)
{
	if (p_ptr->realm1 != REALM_HEX) return;
	if (HEX_REVENGE_TURN(p_ptr) <= 0) return;

	switch(HEX_REVENGE_TYPE(p_ptr))
	{
		case 1: do_spell(REALM_HEX, HEX_PATIENCE, SPELL_CONT); break;
		case 2: do_spell(REALM_HEX, HEX_REVENGE, SPELL_CONT); break;
	}
}

/*!
 * @brief 復讐ダメージの追加を行う
 * @param dam 蓄積されるダメージ量
 * @return なし
 */
void revenge_store(HIT_POINT dam)
{
	if (p_ptr->realm1 != REALM_HEX) return;
	if (HEX_REVENGE_TURN(p_ptr) <= 0) return;

	HEX_REVENGE_POWER(p_ptr) += dam;
}

/*!
 * @brief 反テレポート結界の判定
 * @param m_idx 判定の対象となるモンスターID
 * @return 反テレポートの効果が適用されるならTRUEを返す
 */
bool teleport_barrier(MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!hex_spelling(HEX_ANTI_TELE)) return FALSE;
	if ((p_ptr->lev * 3 / 2) < randint1(r_ptr->level)) return FALSE;

	return TRUE;
}

/*!
 * @brief 反魔法結界の判定
 * @param m_idx 判定の対象となるモンスターID
 * @return 反魔法の効果が適用されるならTRUEを返す
 */
bool magic_barrier(MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!hex_spelling(HEX_ANTI_MAGIC)) return FALSE;
	if ((p_ptr->lev * 3 / 2) < randint1(r_ptr->level)) return FALSE;

	return TRUE;
}

/*!
 * @brief 反増殖結界の判定
 * @param m_idx 判定の対象となるモンスターID
 * @return 反増殖の効果が適用されるならTRUEを返す
 */
bool multiply_barrier(MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!hex_spelling(HEX_ANTI_MULTI)) return FALSE;
	if ((p_ptr->lev * 3 / 2) < randint1(r_ptr->level)) return FALSE;

	return TRUE;
}

/*!
* @brief 呪術領域魔法の各処理を行う
* @param spell 魔法ID
* @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST / SPELL_CONT / SPELL_STOP)
* @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST / SPELL_CONT / SPELL_STOP 時はNULL文字列を返す。
*/
concptr do_hex_spell(SPELL_IDX spell, BIT_FLAGS mode)
{
	bool name = (mode == SPELL_NAME) ? TRUE : FALSE;
	bool desc = (mode == SPELL_DESC) ? TRUE : FALSE;
	bool info = (mode == SPELL_INFO) ? TRUE : FALSE;
	bool cast = (mode == SPELL_CAST) ? TRUE : FALSE;
	bool cont = (mode == SPELL_CONT) ? TRUE : FALSE;
	bool stop = (mode == SPELL_STOP) ? TRUE : FALSE;

	bool add = TRUE;

	PLAYER_LEVEL plev = p_ptr->lev;
	HIT_POINT power;

	switch (spell)
	{
		/*** 1st book (0-7) ***/
	case 0:
		if (name) return _("邪なる祝福", "Evily blessing");
		if (desc) return _("祝福により攻撃精度と防御力が上がる。", "Attempts to increase +to_hit of a weapon and AC");
		if (cast)
		{
			if (!p_ptr->blessed)
			{
				msg_print(_("高潔な気分になった！", "You feel righteous!"));
			}
		}
		if (stop)
		{
			if (!p_ptr->blessed)
			{
				msg_print(_("高潔な気分が消え失せた。", "The prayer has expired."));
			}
		}
		break;

	case 1:
		if (name) return _("軽傷の治癒", "Cure light wounds");
		if (desc) return _("HPや傷を少し回復させる。", "Heals cuts and HP a little.");
		if (info) return info_heal(1, 10, 0);
		if (cast)
		{
			msg_print(_("気分が良くなってくる。", "You feel a little better."));
		}
		if (cast || cont) (void)cure_light_wounds(1, 10);
		break;

	case 2:
		if (name) return _("悪魔のオーラ", "Demonic aura");
		if (desc) return _("炎のオーラを身にまとい、回復速度が速くなる。", "Gives fire aura and regeneration.");
		if (cast)
		{
			msg_print(_("体が炎のオーラで覆われた。", "You are enveloped by a fiery aura!"));
		}
		if (stop)
		{
			msg_print(_("炎のオーラが消え去った。", "The fiery aura disappeared."));
		}
		break;

	case 3:
		if (name) return _("悪臭霧", "Stinking mist");
		if (desc) return _("視界内のモンスターに微弱量の毒のダメージを与える。", "Deals a little poison damage to all monsters in your sight.");
		power = plev / 2 + 5;
		if (info) return info_damage(1, power, 0);
		if (cast || cont)
		{
			project_all_los(GF_POIS, randint1(power));
		}
		break;

	case 4:
		if (name) return _("腕力強化", "Extra might");
		if (desc) return _("術者の腕力を上昇させる。", "Attempts to increase your strength.");
		if (cast)
		{
			msg_print(_("何だか力が湧いて来る。", "You feel stronger."));
		}
		break;

	case 5:
		if (name) return _("武器呪縛", "Curse weapon");
		if (desc) return _("装備している武器を呪う。", "Curses your weapon.");
		if (cast)
		{
			OBJECT_IDX item;
			concptr q, s;
			GAME_TEXT o_name[MAX_NLEN];
			object_type *o_ptr;
			u32b f[TR_FLAG_SIZE];

			item_tester_hook = item_tester_hook_weapon_except_bow;
			q = _("どれを呪いますか？", "Which weapon do you curse?");
			s = _("武器を装備していない。", "You're not wielding a weapon.");

			o_ptr = choose_object(&item, q, s, (USE_EQUIP));
			if (!o_ptr) return FALSE;

			object_desc(o_name, o_ptr, OD_NAME_ONLY);
			object_flags(o_ptr, f);

			if (!get_check(format(_("本当に %s を呪いますか？", "Do you curse %s, really？"), o_name))) return FALSE;

			if (!one_in_(3) &&
				(object_is_artifact(o_ptr) || have_flag(f, TR_BLESSED)))
			{
				msg_format(_("%s は呪いを跳ね返した。", "%s resists the effect."), o_name);
				if (one_in_(3))
				{
					if (o_ptr->to_d > 0)
					{
						o_ptr->to_d -= randint1(3) % 2;
						if (o_ptr->to_d < 0) o_ptr->to_d = 0;
					}
					if (o_ptr->to_h > 0)
					{
						o_ptr->to_h -= randint1(3) % 2;
						if (o_ptr->to_h < 0) o_ptr->to_h = 0;
					}
					if (o_ptr->to_a > 0)
					{
						o_ptr->to_a -= randint1(3) % 2;
						if (o_ptr->to_a < 0) o_ptr->to_a = 0;
					}
					msg_format(_("%s は劣化してしまった。", "Your %s was disenchanted!"), o_name);
				}
			}
			else
			{
				int curse_rank = 0;
				msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
				o_ptr->curse_flags |= (TRC_CURSED);

				if (object_is_artifact(o_ptr) || object_is_ego(o_ptr))
				{

					if (one_in_(3)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
					if (one_in_(666))
					{
						o_ptr->curse_flags |= (TRC_TY_CURSE);
						if (one_in_(666)) o_ptr->curse_flags |= (TRC_PERMA_CURSE);

						add_flag(o_ptr->art_flags, TR_AGGRAVATE);
						add_flag(o_ptr->art_flags, TR_VORPAL);
						add_flag(o_ptr->art_flags, TR_VAMPIRIC);
						msg_print(_("血だ！血だ！血だ！", "Blood, Blood, Blood!"));
						curse_rank = 2;
					}
				}

				o_ptr->curse_flags |= get_curse(curse_rank, o_ptr);
			}

			p_ptr->update |= (PU_BONUS);
			add = FALSE;
		}
		break;

	case 6:
		if (name) return _("邪悪感知", "Evil detection");
		if (desc) return _("周囲の邪悪なモンスターを感知する。", "Detects evil monsters.");
		if (info) return info_range(MAX_SIGHT);
		if (cast)
		{
			msg_print(_("邪悪な生物の存在を感じ取ろうとした。", "You sense the presence of evil creatures."));
		}
		break;

	case 7:
		if (name) return _("我慢", "Patience");
		if (desc) return _("数ターン攻撃を耐えた後、受けたダメージを地獄の業火として周囲に放出する。",
			"Bursts hell fire strongly after enduring damage for a few turns.");
		power = MIN(200, (HEX_REVENGE_POWER(p_ptr) * 2));
		if (info) return info_damage(0, 0, power);
		if (cast)
		{
			int a = 3 - (p_ptr->pspeed - 100) / 10;
			MAGIC_NUM2 r = 3 + randint1(3) + MAX(0, MIN(3, a));

			if (HEX_REVENGE_TURN(p_ptr) > 0)
			{
				msg_print(_("すでに我慢をしている。", "You are already biding your time for vengeance."));
				return NULL;
			}

			HEX_REVENGE_TYPE(p_ptr) = 1;
			HEX_REVENGE_TURN(p_ptr) = r;
			HEX_REVENGE_POWER(p_ptr) = 0;
			msg_print(_("じっと耐えることにした。", "You decide to endure damage for future retribution."));
			add = FALSE;
		}
		if (cont)
		{
			POSITION rad = 2 + (power / 50);

			HEX_REVENGE_TURN(p_ptr)--;

			if ((HEX_REVENGE_TURN(p_ptr) <= 0) || (power >= 200))
			{
				msg_print(_("我慢が解かれた！", "My patience is at an end!"));
				if (power)
				{
					project(0, rad, p_ptr->y, p_ptr->x, power, GF_HELL_FIRE,
						(PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
				}
				if (p_ptr->wizard)
				{
					msg_format(_("%d点のダメージを返した。", "You return %d damage."), power);
				}

				/* Reset */
				HEX_REVENGE_TYPE(p_ptr) = 0;
				HEX_REVENGE_TURN(p_ptr) = 0;
				HEX_REVENGE_POWER(p_ptr) = 0;
			}
		}
		break;

		/*** 2nd book (8-15) ***/
	case 8:
		if (name) return _("氷の鎧", "Armor of ice");
		if (desc) return _("氷のオーラを身にまとい、防御力が上昇する。", "Surrounds you with an icy aura and gives a bonus to AC.");
		if (cast)
		{
			msg_print(_("体が氷の鎧で覆われた。", "You are enveloped by icy armor!"));
		}
		if (stop)
		{
			msg_print(_("氷の鎧が消え去った。", "The icy armor disappeared."));
		}
		break;

	case 9:
		if (name) return _("重傷の治癒", "Cure serious wounds");
		if (desc) return _("体力や傷を多少回復させる。", "Heals cuts and HP.");
		if (info) return info_heal(2, 10, 0);
		if (cast)
		{
			msg_print(_("気分が良くなってくる。", "You feel better."));
		}
		if (cast || cont) (void)cure_serious_wounds(2, 10);
		break;

	case 10:
		if (name) return _("薬品吸入", "Inhale potion");
		if (desc) return _("呪文詠唱を中止することなく、薬の効果を得ることができる。", "Quaffs a potion without canceling spell casting.");
		if (cast)
		{
			CASTING_HEX_FLAGS(p_ptr) |= (1L << HEX_INHAIL);
			do_cmd_quaff_potion();
			CASTING_HEX_FLAGS(p_ptr) &= ~(1L << HEX_INHAIL);
			add = FALSE;
		}
		break;

	case 11:
		if (name) return _("衰弱の霧", "Hypodynamic mist");
		if (desc) return _("視界内のモンスターに微弱量の衰弱属性のダメージを与える。",
			"Deals a little life-draining damage to all monsters in your sight.");
		power = (plev / 2) + 5;
		if (info) return info_damage(1, power, 0);
		if (cast || cont)
		{
			project_all_los(GF_HYPODYNAMIA, randint1(power));
		}
		break;

	case 12:
		if (name) return _("魔剣化", "Swords to runeswords");
		if (desc) return _("武器の攻撃力を上げる。切れ味を得、呪いに応じて与えるダメージが上昇し、善良なモンスターに対するダメージが2倍になる。",
			"Gives vorpal ability to your weapon. Increases damage from your weapon acccording to curse of your weapon.");
		if (cast)
		{
#ifdef JP
			msg_print("あなたの武器が黒く輝いた。");
#else
			if (!empty_hands(FALSE))
				msg_print("Your weapons glow bright black.");
			else
				msg_print("Your weapon glows bright black.");
#endif
		}
		if (stop)
		{
#ifdef JP
			msg_print("武器の輝きが消え去った。");
#else
			msg_format("Your weapon%s.", (empty_hands(FALSE)) ? " no longer glows" : "s no longer glow");
#endif
		}
		break;

	case 13:
		if (name) return _("混乱の手", "Touch of confusion");
		if (desc) return _("攻撃した際モンスターを混乱させる。", "Confuses a monster when you attack.");
		if (cast)
		{
			msg_print(_("あなたの手が赤く輝き始めた。", "Your hands glow bright red."));
		}
		if (stop)
		{
			msg_print(_("手の輝きがなくなった。", "Your hands no longer glow."));
		}
		break;

	case 14:
		if (name) return _("肉体強化", "Building up");
		if (desc) return _("術者の腕力、器用さ、耐久力を上昇させる。攻撃回数の上限を 1 増加させる。",
			"Attempts to increases your strength, dexterity and constitusion.");
		if (cast)
		{
			msg_print(_("身体が強くなった気がした。", "You feel your body is more developed now."));
		}
		break;

	case 15:
		if (name) return _("反テレポート結界", "Anti teleport barrier");
		if (desc) return _("視界内のモンスターのテレポートを阻害するバリアを張る。", "Obstructs all teleportations by monsters in your sight.");
		power = plev * 3 / 2;
		if (info) return info_power(power);
		if (cast)
		{
			msg_print(_("テレポートを防ぐ呪いをかけた。", "You feel anyone can not teleport except you."));
		}
		break;

		/*** 3rd book (16-23) ***/
	case 16:
		if (name) return _("衝撃のクローク", "Cloak of shock");
		if (desc) return _("電気のオーラを身にまとい、動きが速くなる。", "Gives lightning aura and a bonus to speed.");
		if (cast)
		{
			msg_print(_("体が稲妻のオーラで覆われた。", "You are enveloped by an electrical aura!"));
		}
		if (stop)
		{
			msg_print(_("稲妻のオーラが消え去った。", "The electrical aura disappeared."));
		}
		break;

	case 17:
		if (name) return _("致命傷の治癒", "Cure critical wounds");
		if (desc) return _("体力や傷を回復させる。", "Heals cuts and HP greatly.");
		if (info) return info_heal(4, 10, 0);
		if (cast)
		{
			msg_print(_("気分が良くなってくる。", "You feel much better."));
		}
		if (cast || cont) (void)cure_critical_wounds(damroll(4, 10));
		break;

	case 18:
		if (name) return _("呪力封入", "Recharging");
		if (desc) return _("魔法の道具に魔力を再充填する。", "Recharges a magic device.");
		power = plev * 2;
		if (info) return info_power(power);
		if (cast)
		{
			if (!recharge(power)) return NULL;
			add = FALSE;
		}
		break;

	case 19:
		if (name) return _("死者復活", "Animate Dead");
		if (desc) return _("死体を蘇らせてペットにする。", "Raises corpses and skeletons from dead.");
		if (cast)
		{
			msg_print(_("死者への呼びかけを始めた。", "You start to call the dead.!"));
		}
		if (cast || cont)
		{
			animate_dead(0, p_ptr->y, p_ptr->x);
		}
		break;

	case 20:
		if (name) return _("防具呪縛", "Curse armor");
		if (desc) return _("装備している防具に呪いをかける。", "Curse a piece of armour that you wielding.");
		if (cast)
		{
			OBJECT_IDX item;
			concptr q, s;
			GAME_TEXT o_name[MAX_NLEN];
			object_type *o_ptr;
			u32b f[TR_FLAG_SIZE];

			item_tester_hook = object_is_armour;
			q = _("どれを呪いますか？", "Which piece of armour do you curse?");
			s = _("防具を装備していない。", "You're not wearing any armor.");

			o_ptr = choose_object(&item, q, s, (USE_EQUIP));
			if (!o_ptr) return FALSE;

			o_ptr = &inventory[item];
			object_desc(o_name, o_ptr, OD_NAME_ONLY);
			object_flags(o_ptr, f);

			if (!get_check(format(_("本当に %s を呪いますか？", "Do you curse %s, really？"), o_name))) return FALSE;

			if (!one_in_(3) &&
				(object_is_artifact(o_ptr) || have_flag(f, TR_BLESSED)))
			{
				msg_format(_("%s は呪いを跳ね返した。", "%s resists the effect."), o_name);
				if (one_in_(3))
				{
					if (o_ptr->to_d > 0)
					{
						o_ptr->to_d -= randint1(3) % 2;
						if (o_ptr->to_d < 0) o_ptr->to_d = 0;
					}
					if (o_ptr->to_h > 0)
					{
						o_ptr->to_h -= randint1(3) % 2;
						if (o_ptr->to_h < 0) o_ptr->to_h = 0;
					}
					if (o_ptr->to_a > 0)
					{
						o_ptr->to_a -= randint1(3) % 2;
						if (o_ptr->to_a < 0) o_ptr->to_a = 0;
					}
					msg_format(_("%s は劣化してしまった。", "Your %s was disenchanted!"), o_name);
				}
			}
			else
			{
				int curse_rank = 0;
				msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
				o_ptr->curse_flags |= (TRC_CURSED);

				if (object_is_artifact(o_ptr) || object_is_ego(o_ptr))
				{

					if (one_in_(3)) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
					if (one_in_(666))
					{
						o_ptr->curse_flags |= (TRC_TY_CURSE);
						if (one_in_(666)) o_ptr->curse_flags |= (TRC_PERMA_CURSE);

						add_flag(o_ptr->art_flags, TR_AGGRAVATE);
						add_flag(o_ptr->art_flags, TR_RES_POIS);
						add_flag(o_ptr->art_flags, TR_RES_DARK);
						add_flag(o_ptr->art_flags, TR_RES_NETHER);
						msg_print(_("血だ！血だ！血だ！", "Blood, Blood, Blood!"));
						curse_rank = 2;
					}
				}

				o_ptr->curse_flags |= get_curse(curse_rank, o_ptr);
			}

			p_ptr->update |= (PU_BONUS);
			add = FALSE;
		}
		break;

	case 21:
		if (name) return _("影のクローク", "Cloak of shadow");
		if (desc) return _("影のオーラを身にまとい、敵に影のダメージを与える。", "Gives aura of shadow.");
		if (cast)
		{
			object_type *o_ptr = &inventory[INVEN_OUTER];

			if (!o_ptr->k_idx)
			{
				msg_print(_("クロークを身につけていない！", "You are not wearing a cloak."));
				return NULL;
			}
			else if (!object_is_cursed(o_ptr))
			{
				msg_print(_("クロークは呪われていない！", "Your cloak is not cursed."));
				return NULL;
			}
			else
			{
				msg_print(_("影のオーラを身にまとった。", "You are enveloped by a shadowy aura!"));
			}
		}
		if (cont)
		{
			object_type *o_ptr = &inventory[INVEN_OUTER];

			if ((!o_ptr->k_idx) || (!object_is_cursed(o_ptr)))
			{
				do_spell(REALM_HEX, spell, SPELL_STOP);
				CASTING_HEX_FLAGS(p_ptr) &= ~(1L << spell);
				CASTING_HEX_NUM(p_ptr)--;
				if (!SINGING_SONG_ID(p_ptr)) set_action(ACTION_NONE);
			}
		}
		if (stop)
		{
			msg_print(_("影のオーラが消え去った。", "The shadowy aura disappeared."));
		}
		break;

	case 22:
		if (name) return _("苦痛を魔力に", "Pain to mana");
		if (desc) return _("視界内のモンスターに精神ダメージ与え、魔力を吸い取る。", "Deals psychic damage to all monsters in sight and drains some mana.");
		power = plev * 3 / 2;
		if (info) return info_damage(1, power, 0);
		if (cast || cont)
		{
			project_all_los(GF_PSI_DRAIN, randint1(power));
		}
		break;

	case 23:
		if (name) return _("目には目を", "Eye for an eye");
		if (desc) return _("打撃や魔法で受けたダメージを、攻撃元のモンスターにも与える。", "Returns same damage which you got to the monster which damaged you.");
		if (cast)
		{
			msg_print(_("復讐したい欲望にかられた。", "You feel very vengeful."));
		}
		break;

		/*** 4th book (24-31) ***/
	case 24:
		if (name) return _("反増殖結界", "Anti multiply barrier");
		if (desc) return _("その階の増殖するモンスターの増殖を阻止する。", "Obstructs all multiplying by monsters in entire floor.");
		if (cast)
		{
			msg_print(_("増殖を阻止する呪いをかけた。", "You feel anyone can not multiply."));
		}
		break;

	case 25:
		if (name) return _("全復活", "Restoration");
		if (desc) return _("経験値を徐々に復活し、減少した能力値を回復させる。", "Restores experience and status.");
		if (cast)
		{
			msg_print(_("体が元の活力を取り戻し始めた。", "You feel your lost status starting to return."));
		}
		if (cast || cont)
		{
			bool flag = FALSE;
			int d = (p_ptr->max_exp - p_ptr->exp);
			int r = (p_ptr->exp / 20);
			int i;

			if (d > 0)
			{
				if (d < r)
					p_ptr->exp = p_ptr->max_exp;
				else
					p_ptr->exp += r;

				/* Check the experience */
				check_experience();

				flag = TRUE;
			}
			for (i = A_STR; i < A_MAX; i++)
			{
				if (p_ptr->stat_cur[i] < p_ptr->stat_max[i])
				{
					if (p_ptr->stat_cur[i] < 18)
						p_ptr->stat_cur[i]++;
					else
						p_ptr->stat_cur[i] += 10;

					if (p_ptr->stat_cur[i] > p_ptr->stat_max[i])
						p_ptr->stat_cur[i] = p_ptr->stat_max[i];
					p_ptr->update |= (PU_BONUS);

					flag = TRUE;
				}
			}

			if (!flag)
			{
				msg_format(_("%sの呪文の詠唱をやめた。", "Finish casting '%^s'."), do_spell(REALM_HEX, HEX_RESTORE, SPELL_NAME));
				CASTING_HEX_FLAGS(p_ptr) &= ~(1L << HEX_RESTORE);
				if (cont) CASTING_HEX_NUM(p_ptr)--;
				if (CASTING_HEX_NUM(p_ptr)) p_ptr->action = ACTION_NONE;

				p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
				p_ptr->redraw |= (PR_EXTRA);

				return "";
			}
		}
		break;

	case 26:
		if (name) return _("呪力吸収", "Drain curse power");
		if (desc) return _("呪われた武器の呪いを吸収して魔力を回復する。", "Drains curse on your weapon and heals SP a little.");
		if (cast)
		{
			OBJECT_IDX item;
			concptr s, q;
			u32b f[TR_FLAG_SIZE];
			object_type *o_ptr;

			item_tester_hook = item_tester_hook_cursed;
			q = _("どの装備品から吸収しますか？", "Which cursed equipment do you drain mana from?");
			s = _("呪われたアイテムを装備していない。", "You have no cursed equipment.");

			o_ptr = choose_object(&item, q, s, (USE_EQUIP));
			if (!o_ptr) return FALSE;

			object_flags(o_ptr, f);

			p_ptr->csp += (p_ptr->lev / 5) + randint1(p_ptr->lev / 5);
			if (have_flag(f, TR_TY_CURSE) || (o_ptr->curse_flags & TRC_TY_CURSE)) p_ptr->csp += randint1(5);
			if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;

			if (o_ptr->curse_flags & TRC_PERMA_CURSE)
			{
				/* Nothing */
			}
			else if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
			{
				if (one_in_(7))
				{
					msg_print(_("呪いを全て吸い取った。", "A heavy curse vanished."));
					o_ptr->curse_flags = 0L;
				}
			}
			else if ((o_ptr->curse_flags & (TRC_CURSED)) && one_in_(3))
			{
				msg_print(_("呪いを全て吸い取った。", "A curse vanished."));
				o_ptr->curse_flags = 0L;
			}

			add = FALSE;
		}
		break;

	case 27:
		if (name) return _("吸血の刃", "Swords to vampires");
		if (desc) return _("吸血属性で攻撃する。", "Gives vampiric ability to your weapon.");
		if (cast)
		{
#ifdef JP
			msg_print("あなたの武器が血を欲している。");
#else
			if (!empty_hands(FALSE))
				msg_print("Your weapons want more blood now.");
			else
				msg_print("Your weapon wants more blood now.");
#endif
		}
		if (stop)
		{
#ifdef JP
			msg_print("武器の渇望が消え去った。");
#else
			msg_format("Your weapon%s less thirsty now.", (empty_hands(FALSE)) ? " is" : "s are");
#endif
		}
		break;

	case 28:
		if (name) return _("朦朧の言葉", "Word of stun");
		if (desc) return _("視界内のモンスターを朦朧とさせる。", "Stuns all monsters in your sight.");
		power = plev * 4;
		if (info) return info_power(power);
		if (cast || cont)
		{
			stun_monsters(power);
		}
		break;

	case 29:
		if (name) return _("影移動", "Moving into shadow");
		if (desc) return _("モンスターの隣のマスに瞬間移動する。", "Teleports you close to a monster.");
		if (cast)
		{
			int i, dir;
			POSITION y, x;
			bool flag;

			for (i = 0; i < 3; i++)
			{
				if (!tgt_pt(&x, &y)) return FALSE;

				flag = FALSE;

				for (dir = 0; dir < 8; dir++)
				{
					int dy = y + ddy_ddd[dir];
					int dx = x + ddx_ddd[dir];
					if (dir == 5) continue;
					if (current_floor_ptr->grid_array[dy][dx].m_idx) flag = TRUE;
				}

				if (!cave_empty_bold(y, x) || (current_floor_ptr->grid_array[y][x].info & CAVE_ICKY) ||
					(distance(y, x, p_ptr->y, p_ptr->x) > plev + 2))
				{
					msg_print(_("そこには移動できない。", "Can not teleport to there."));
					continue;
				}
				break;
			}

			if (flag && randint0(plev * plev / 2))
			{
				teleport_player_to(y, x, 0L);
			}
			else
			{
				msg_print(_("おっと！", "Oops!"));
				teleport_player(30, 0L);
			}

			add = FALSE;
		}
		break;

	case 30:
		if (name) return _("反魔法結界", "Anti magic barrier");
		if (desc) return _("視界内のモンスターの魔法を阻害するバリアを張る。", "Obstructs all magic spells of monsters in your sight.");
		power = plev * 3 / 2;
		if (info) return info_power(power);
		if (cast)
		{
			msg_print(_("魔法を防ぐ呪いをかけた。", "You feel anyone can not cast spells except you."));
		}
		break;

	case 31:
		if (name) return _("復讐の宣告", "Revenge sentence");
		if (desc) return _("数ターン後にそれまで受けたダメージに応じた威力の地獄の劫火の弾を放つ。",
			"Fires a ball of hell fire to try avenging damage from a few turns.");
		power = HEX_REVENGE_POWER(p_ptr);
		if (info) return info_damage(0, 0, power);
		if (cast)
		{
			MAGIC_NUM2 r;
			int a = 3 - (p_ptr->pspeed - 100) / 10;
			r = 1 + randint1(2) + MAX(0, MIN(3, a));

			if (HEX_REVENGE_TURN(p_ptr) > 0)
			{
				msg_print(_("すでに復讐は宣告済みだ。", "You've already declared your revenge."));
				return NULL;
			}

			HEX_REVENGE_TYPE(p_ptr) = 2;
			HEX_REVENGE_TURN(p_ptr) = r;
			msg_format(_("あなたは復讐を宣告した。あと %d ターン。", "You declare your revenge. %d turns left."), r);
			add = FALSE;
		}
		if (cont)
		{
			HEX_REVENGE_TURN(p_ptr)--;

			if (HEX_REVENGE_TURN(p_ptr) <= 0)
			{
				DIRECTION dir;

				if (power)
				{
					command_dir = 0;

					do
					{
						msg_print(_("復讐の時だ！", "Time for revenge!"));
					} while (!get_aim_dir(&dir));

					fire_ball(GF_HELL_FIRE, dir, power, 1);

					if (p_ptr->wizard)
					{
						msg_format(_("%d点のダメージを返した。", "You return %d damage."), power);
					}
				}
				else
				{
					msg_print(_("復讐する気が失せた。", "You are not in the mood for revenge."));
				}
				HEX_REVENGE_POWER(p_ptr) = 0;
			}
		}
		break;
	}

	/* start casting */
	if ((cast) && (add))
	{
		/* add spell */
		CASTING_HEX_FLAGS(p_ptr) |= 1L << (spell);
		CASTING_HEX_NUM(p_ptr)++;

		if (p_ptr->action != ACTION_SPELL) set_action(ACTION_SPELL);
	}

	if (!info)
	{
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
		p_ptr->redraw |= (PR_EXTRA | PR_HP | PR_MANA);
	}

	return "";
}
