﻿#include "angband.h"
#include "cmd-spell.h"
#include "selfinfo.h"
#include "projection.h"
#include "spells-object.h"
#include "spells-summon.h"
#include "spells-status.h"
#include "mutation.h"



/*!
* @brief 匠領域魔法の各処理を行う
* @param spell 魔法ID
* @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST)
* @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST時はNULL文字列を返す。
*/
concptr do_craft_spell(SPELL_IDX spell, BIT_FLAGS mode)
{
	bool name = (mode == SPELL_NAME) ? TRUE : FALSE;
	bool desc = (mode == SPELL_DESC) ? TRUE : FALSE;
	bool info = (mode == SPELL_INFO) ? TRUE : FALSE;
	bool cast = (mode == SPELL_CAST) ? TRUE : FALSE;

	PLAYER_LEVEL plev = p_ptr->lev;

	switch (spell)
	{
	case 0:
		if (name) return _("赤外線視力", "Infravision");
		if (desc) return _("一定時間、赤外線視力が増強される。", "Gives infravision for a while.");

		{
			int base = 100;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_tim_infra(base + randint1(base), FALSE);
			}
		}
		break;

	case 1:
		if (name) return _("回復力強化", "Regeneration");
		if (desc) return _("一定時間、回復力が増強される。", "Gives regeneration ability for a while.");

		{
			int base = 80;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_tim_regen(base + randint1(base), FALSE);
			}
		}
		break;

	case 2:
		if (name) return _("空腹充足", "Satisfy Hunger");
		if (desc) return _("満腹になる。", "Satisfies hunger.");

		{
			if (cast)
			{
				set_food(PY_FOOD_MAX - 1);
			}
		}
		break;

	case 3:
		if (name) return _("耐冷気", "Resist Cold");
		if (desc) return _("一定時間、冷気への耐性を得る。装備による耐性に累積する。",
			"Gives resistance to cold. This resistance can be added to that from equipment for more powerful resistance.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_oppose_cold(randint1(base) + base, FALSE);
			}
		}
		break;

	case 4:
		if (name) return _("耐火炎", "Resist Fire");
		if (desc) return _("一定時間、炎への耐性を得る。装備による耐性に累積する。",
			"Gives resistance to fire. This resistance can be added to that from equipment for more powerful resistance.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_oppose_fire(randint1(base) + base, FALSE);
			}
		}
		break;

	case 5:
		if (name) return _("士気高揚", "Heroism");
		if (desc) return _("一定時間、ヒーロー気分になる。", "Removes fear, and gives a bonus to hit and 10 more HP for a while.");

		{
			int base = 25;

			if (info) return info_duration(base, base);

			if (cast)
			{
				(void)heroism(base);
			}
		}
		break;

	case 6:
		if (name) return _("耐電撃", "Resist Lightning");
		if (desc) return _("一定時間、電撃への耐性を得る。装備による耐性に累積する。",
			"Gives resistance to electricity. This resistance can be added to that from equipment for more powerful resistance.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_oppose_elec(randint1(base) + base, FALSE);
			}
		}
		break;

	case 7:
		if (name) return _("耐酸", "Resist Acid");
		if (desc) return _("一定時間、酸への耐性を得る。装備による耐性に累積する。",
			"Gives resistance to acid. This resistance can be added to that from equipment for more powerful resistance.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_oppose_acid(randint1(base) + base, FALSE);
			}
		}
		break;

	case 8:
		if (name) return _("透明視認", "See Invisibility");
		if (desc) return _("一定時間、透明なものが見えるようになる。", "Gives see invisible for a while.");

		{
			int base = 24;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_tim_invis(randint1(base) + base, FALSE);
			}
		}
		break;

	case 9:
		if (name) return _("解呪", "Remove Curse");
		if (desc) return _("アイテムにかかった弱い呪いを解除する。", "Removes normal curses from equipped items.");

		{
			if (cast) (void)remove_curse();
		}
		break;

	case 10:
		if (name) return _("耐毒", "Resist Poison");
		if (desc) return _("一定時間、毒への耐性を得る。装備による耐性に累積する。",
			"Gives resistance to poison. This resistance can be added to that from equipment for more powerful resistance.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_oppose_pois(randint1(base) + base, FALSE);
			}
		}
		break;

	case 11:
		if (name) return _("狂戦士化", "Berserk");
		if (desc) return _("狂戦士化し、恐怖を除去する。", "Gives a bonus to hit and HP, immunity to fear for a while. But decreases AC.");

		{
			int base = 25;

			if (info) return info_duration(base, base);

			if (cast)
			{
				(void)berserk(base + randint1(base));
			}
		}
		break;

	case 12:
		if (name) return _("自己分析", "Self Knowledge");
		if (desc) return _("現在の自分の状態を完全に知る。",
			"Gives you useful info regarding your current resistances, the powers of your weapon and maximum limits of your stats.");

		{
			if (cast)
			{
				self_knowledge();
			}
		}
		break;

	case 13:
		if (name) return _("対邪悪結界", "Protection from Evil");
		if (desc) return _("邪悪なモンスターの攻撃を防ぐバリアを張る。", "Gives aura which protect you from evil monster's physical attack.");

		{
			int base = 3 * plev;
			DICE_SID sides = 25;

			if (info) return info_duration(base, sides);

			if (cast)
			{
				set_protevil(randint1(sides) + base, FALSE);
			}
		}
		break;

	case 14:
		if (name) return _("癒し", "Cure");
		if (desc) return _("毒、朦朧状態、負傷を全快させ、幻覚を直す。", "Heals poison, stun, cut and hallucination completely.");

		{
			if (cast)
			{
				(void)true_healing(0);
			}
		}
		break;

	case 15:
		if (name) return _("魔法剣", "Mana Branding");
		if (desc) return _("一定時間、武器に冷気、炎、電撃、酸、毒のいずれかの属性をつける。武器を持たないと使えない。",
			"Causes current weapon to temporarily do additional damage from cold, fire, lightning, acid or poison. You must be wielding one or more weapons.");

		{
			int base = plev / 2;

			if (info) return info_duration(base, base);

			if (cast)
			{
				if (!choose_ele_attack()) return NULL;
			}
		}
		break;

	case 16:
		if (name) return _("テレパシー", "Telepathy");
		if (desc) return _("一定時間、テレパシー能力を得る。", "Gives telepathy for a while.");

		{
			int base = 25;
			DICE_SID sides = 30;

			if (info) return info_duration(base, sides);

			if (cast)
			{
				set_tim_esp(randint1(sides) + base, FALSE);
			}
		}
		break;

	case 17:
		if (name) return _("肌石化", "Stone Skin");
		if (desc) return _("一定時間、ACを上昇させる。", "Gives a bonus to AC for a while.");

		{
			int base = 30;
			DICE_SID sides = 20;

			if (info) return info_duration(base, sides);

			if (cast)
			{
				set_shield(randint1(sides) + base, FALSE);
			}
		}
		break;

	case 18:
		if (name) return _("全耐性", "Resistance");
		if (desc) return _("一定時間、酸、電撃、炎、冷気、毒に対する耐性を得る。装備による耐性に累積する。",
			"Gives resistance to fire, cold, electricity, acid and poison for a while. These resistances can be added to those from equipment for more powerful resistances.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_oppose_acid(randint1(base) + base, FALSE);
				set_oppose_elec(randint1(base) + base, FALSE);
				set_oppose_fire(randint1(base) + base, FALSE);
				set_oppose_cold(randint1(base) + base, FALSE);
				set_oppose_pois(randint1(base) + base, FALSE);
			}
		}
		break;

	case 19:
		if (name) return _("スピード", "Haste Self");
		if (desc) return _("一定時間、加速する。", "Hastes you for a while.");

		{
			int base = plev;
			DICE_SID sides = 20 + plev;

			if (info) return info_duration(base, sides);

			if (cast)
			{
				set_fast(randint1(sides) + base, FALSE);
			}
		}
		break;

	case 20:
		if (name) return _("壁抜け", "Walk through Wall");
		if (desc) return _("一定時間、半物質化し壁を通り抜けられるようになる。", "Gives ability to pass walls for a while.");

		{
			int base = plev / 2;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_kabenuke(randint1(base) + base, FALSE);
			}
		}
		break;

	case 21:
		if (name) return _("盾磨き", "Polish Shield");
		if (desc) return _("盾に反射の属性をつける。", "Makes a shield a shield of reflection.");

		{
			if (cast)
			{
				pulish_shield();
			}
		}
		break;

	case 22:
		if (name) return _("ゴーレム製造", "Create Golem");
		if (desc) return _("1体のゴーレムを製造する。", "Creates a golem.");

		{
			if (cast)
			{
				if (summon_specific(-1, p_ptr->y, p_ptr->x, plev, SUMMON_GOLEM, PM_FORCE_PET, '\0'))
				{
					msg_print(_("ゴーレムを作った。", "You make a golem."));
				}
				else
				{
					msg_print(_("うまくゴーレムを作れなかった。", "No Golems arrive."));
				}
			}
		}
		break;

	case 23:
		if (name) return _("魔法の鎧", "Magical armor");
		if (desc) return _("一定時間、魔法防御力とACが上がり、混乱と盲目の耐性、反射能力、麻痺知らず、浮遊を得る。",
			"Gives resistance to magic, bonus to AC, resistance to confusion, blindness, reflection, free action and levitation for a while.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_magicdef(randint1(base) + base, FALSE);
			}
		}
		break;

	case 24:
		if (name) return _("装備無力化", "Remove Enchantment");
		if (desc) return _("武器・防具にかけられたあらゆる魔力を完全に解除する。", "Removes all magics completely from any weapon or armor.");

		{
			if (cast)
			{
				if (!mundane_spell(TRUE)) return NULL;
			}
		}
		break;

	case 25:
		if (name) return _("呪い粉砕", "Remove All Curse");
		if (desc) return _("アイテムにかかった強力な呪いを解除する。", "Removes normal and heavy curse from equipped items.");

		{
			if (cast) (void)remove_all_curse();
		}
		break;

	case 26:
		if (name) return _("完全なる知識", "Knowledge True");
		if (desc) return _("アイテムの持つ能力を完全に知る。", "*Identifies* an item.");

		{
			if (cast)
			{
				if (!identify_fully(FALSE)) return NULL;
			}
		}
		break;

	case 27:
		if (name) return _("武器強化", "Enchant Weapon");
		if (desc) return _("武器の命中率修正とダメージ修正を強化する。", "Attempts to increase +to-hit, +to-dam of a weapon.");

		{
			if (cast)
			{
				if (!enchant_spell(randint0(4) + 1, randint0(4) + 1, 0)) return NULL;
			}
		}
		break;

	case 28:
		if (name) return _("防具強化", "Enchant Armor");
		if (desc) return _("鎧の防御修正を強化する。", "Attempts to increase +AC of an armor.");

		{
			if (cast)
			{
				if (!enchant_spell(0, 0, randint0(3) + 2)) return NULL;
			}
		}
		break;

	case 29:
		if (name) return _("武器属性付与", "Brand Weapon");
		if (desc) return _("武器にランダムに属性をつける。", "Makes current weapon a random ego weapon.");

		{
			if (cast)
			{
				brand_weapon(randint0(18));
			}
		}
		break;

	case 30:
		if (name) return _("人間トランプ", "Living Trump");
		if (desc) return _("ランダムにテレポートする突然変異か、自分の意思でテレポートする突然変異が身につく。",
			"Gives mutation which makes you teleport randomly or makes you able to teleport at will.");
		if (cast) become_living_trump(p_ptr);
		break;

	case 31:
		if (name) return _("属性への免疫", "Immunity");
		if (desc) return _("一定時間、冷気、炎、電撃、酸のいずれかに対する免疫を得る。",
			"Gives an immunity to fire, cold, electricity or acid for a while.");

		{
			int base = 13;

			if (info) return info_duration(base, base);

			if (cast)
			{
				if (!choose_ele_immune(base + randint1(base))) return NULL;
			}
		}
		break;
	}

	return "";
}
