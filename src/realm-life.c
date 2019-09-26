﻿#include "angband.h"
#include "cmd-spell.h"
#include "projection.h"
#include "spells-status.h"
#include "spells-floor.h"

/*!
* @brief 生命領域魔法の各処理を行う
* @param spell 魔法ID
* @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST)
* @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST時はNULL文字列を返す。
*/
concptr do_life_spell(SPELL_IDX spell, BIT_FLAGS mode)
{
	bool name = (mode == SPELL_NAME) ? TRUE : FALSE;
	bool desc = (mode == SPELL_DESC) ? TRUE : FALSE;
	bool info = (mode == SPELL_INFO) ? TRUE : FALSE;
	bool cast = (mode == SPELL_CAST) ? TRUE : FALSE;

	DIRECTION dir;
	PLAYER_LEVEL plev = p_ptr->lev;

	switch (spell)
	{
	case 0:
		if (name) return _("軽傷の治癒", "Cure Light Wounds");
		if (desc) return _("怪我と体力を少し回復させる。", "Heals cut and HP a little.");
		{
			DICE_NUMBER dice = 2;
			DICE_SID sides = 10;
			if (info) return info_heal(dice, sides, 0);
			if (cast) (void)cure_light_wounds(dice, sides);			
		}
		break;

	case 1:
		if (name) return _("祝福", "Bless");
		if (desc) return _("一定時間、命中率とACにボーナスを得る。", "Gives a bonus to hit and AC for a few turns.");
		{
			int base = 12;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_blessed(randint1(base) + base, FALSE);
			}
		}
		break;

	case 2:
		if (name) return _("軽傷", "Cause Light Wounds");
		if (desc) return _("1体のモンスターに小ダメージを与える。抵抗されると無効。", "Wounds a monster a little unless resisted.");
		{
			DICE_NUMBER dice = 3 + (plev - 1) / 5;
			DICE_SID sides = 4;

			if (info) return info_damage(dice, sides, 0);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				fire_ball_hide(GF_WOUNDS, dir, damroll(dice, sides), 0);
			}
		}
		break;

	case 3:
		if (name) return _("光の召喚", "Call Light");
		if (desc) return _("光源が照らしている範囲か部屋全体を永久に明るくする。", "Lights up nearby area and the inside of a room permanently.");
		{
			DICE_NUMBER dice = 2;
			DICE_SID sides = plev / 2;
			POSITION rad = plev / 10 + 1;

			if (info) return info_damage(dice, sides, 0);

			if (cast)
			{
				lite_area(damroll(dice, sides), rad);
			}
		}
		break;

	case 4:
		if (name) return _("罠 & 隠し扉感知", "Detect Doors & Traps");
		if (desc) return _("近くの全ての罠と扉と階段を感知する。", "Detects traps, doors, and stairs in your vicinity.");
		{
			POSITION rad = DETECT_RAD_DEFAULT;

			if (info) return info_radius(rad);

			if (cast)
			{
				detect_traps(rad, TRUE);
				detect_doors(rad);
				detect_stairs(rad);
			}
		}
		break;

	case 5:
		if (name) return _("重傷の治癒", "Cure Medium Wounds");
		if (desc) return _("怪我と体力を中程度回復させる。", "Heals cut and HP more.");
		{
			DICE_NUMBER dice = 4;
			DICE_SID sides = 10;

			if (info) return info_heal(dice, sides, 0);
			if (cast) (void)cure_serious_wounds(dice, sides);
		}
		break;

	case 6:
		if (name) return _("解毒", "Cure Poison");
		if (desc) return _("体内の毒を取り除く。", "Cures poison status.");
		{
			if (cast)
			{
				set_poisoned(0);
			}
		}
		break;

	case 7:
		if (name) return _("空腹充足", "Satisfy Hunger");
		if (desc) return _("満腹にする。", "Satisfies hunger.");
		{
			if (cast)
			{
				set_food(PY_FOOD_MAX - 1);
			}
		}
		break;

	case 8:
		if (name) return _("解呪", "Remove Curse");
		if (desc) return _("アイテムにかかった弱い呪いを解除する。", "Removes normal curses from equipped items.");
		{
			if (cast) (void)remove_curse();
		}
		break;

	case 9:
		if (name) return _("重傷", "Cause Medium Wounds");
		if (desc) return _("1体のモンスターに中ダメージを与える。抵抗されると無効。", "Wounds a monster unless resisted.");
		{
			DICE_SID sides = 8 + (plev - 5) / 4;
			DICE_NUMBER dice = 8;

			if (info) return info_damage(dice, sides, 0);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				fire_ball_hide(GF_WOUNDS, dir, damroll(dice, sides), 0);
			}
		}
		break;

	case 10:
		if (name) return _("致命傷の治癒", "Cure Critical Wounds");
		if (desc) return _("体力を大幅に回復させ、負傷と朦朧状態も全快する。", "Heals cut, stun and HP greatly.");
		{
			DICE_NUMBER dice = 8;
			DICE_SID sides = 10;

			if (info) return info_heal(dice, sides, 0);
			if (cast) (void)cure_critical_wounds(damroll(dice, sides));
		}
		break;

	case 11:
		if (name) return _("耐熱耐寒", "Resist Heat and Cold");
		if (desc) return _("一定時間、火炎と冷気に対する耐性を得る。装備による耐性に累積する。",
			"Gives resistance to fire and cold. These resistances can be added to those from equipment for more powerful resistances.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_oppose_cold(randint1(base) + base, FALSE);
				set_oppose_fire(randint1(base) + base, FALSE);
			}
		}
		break;

	case 12:
		if (name) return _("周辺感知", "Sense Surroundings");
		if (desc) return _("周辺の地形を感知する。", "Maps nearby area.");

		{
			POSITION rad = DETECT_RAD_MAP;

			if (info) return info_radius(rad);

			if (cast)
			{
				map_area(rad);
			}
		}
		break;

	case 13:
		if (name) return _("パニック・アンデッド", "Turn Undead");
		if (desc) return _("視界内のアンデッドを恐怖させる。抵抗されると無効。", "Attempts to scare undead monsters in sight.");

		{
			if (cast)
			{
				turn_undead();
			}
		}
		break;

	case 14:
		if (name) return _("体力回復", "Healing");
		if (desc) return _("極めて強力な回復呪文で、負傷と朦朧状態も全快する。", "Much powerful healing magic, and heals cut and stun completely.");

		{
			int heal = 300;
			if (info) return info_heal(0, 0, heal);
			if (cast) (void)cure_critical_wounds(heal);
		}
		break;

	case 15:
		if (name) return _("結界の紋章", "Glyph of Warding");
		if (desc) return _("自分のいる床の上に、モンスターが通り抜けたり召喚されたりすることができなくなるルーンを描く。",
			"Sets a glyph on the floor beneath you. Monsters cannot attack you if you are on a glyph, but can try to break glyph.");

		{
			if (cast)
			{
				warding_glyph();
			}
		}
		break;

	case 16:
		if (name) return _("*解呪*", "Dispel Curse");
		if (desc) return _("アイテムにかかった強力な呪いを解除する。", "Removes normal and heavy curse from equipped items.");
		{
			if (cast) (void)remove_all_curse();
		}
		break;

	case 17:
		if (name) return _("鑑識", "Perception");
		if (desc) return _("アイテムを識別する。", "Identifies an item.");

		{
			if (cast)
			{
				if (!ident_spell(FALSE)) return NULL;
			}
		}
		break;

	case 18:
		if (name) return _("アンデッド退散", "Dispel Undead");
		if (desc) return _("視界内の全てのアンデッドにダメージを与える。", "Damages all undead monsters in sight.");

		{
			DICE_NUMBER dice = 1;
			DICE_SID sides = plev * 5;

			if (info) return info_damage(dice, sides, 0);

			if (cast)
			{
				dispel_undead(damroll(dice, sides));
			}
		}
		break;

	case 19:
		if (name) return _("凪の刻", "Day of the Dove");
		if (desc) return _("視界内の全てのモンスターを魅了する。抵抗されると無効。", "Attempts to charm all monsters in sight.");

		{
			int power = plev * 2;

			if (info) return info_power(power);

			if (cast)
			{
				charm_monsters(power);
			}
		}
		break;

	case 20:
		if (name) return _("致命傷", "Cause Critical Wounds");
		if (desc) return _("1体のモンスターに大ダメージを与える。抵抗されると無効。", "Wounds a monster critically unless resisted.");

		{
			DICE_NUMBER dice = 5 + (plev - 5) / 3;
			DICE_SID sides = 15;

			if (info) return info_damage(dice, sides, 0);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				fire_ball_hide(GF_WOUNDS, dir, damroll(dice, sides), 0);
			}
		}
		break;

	case 21:
		if (name) return _("帰還の詔", "Word of Recall");
		if (desc) return _("地上にいるときはダンジョンの最深階へ、ダンジョンにいるときは地上へと移動する。", "Recalls player from dungeon to town, or from town to the deepest level of dungeon.");

		{
			int base = 15;
			DICE_SID sides = 20;

			if (info) return info_delay(base, sides);

			if (cast)
			{
				if (!recall_player(p_ptr, randint0(21) + 15)) return NULL;
			}
		}
		break;

	case 22:
		if (name) return _("真実の祭壇", "Alter Reality");
		if (desc) return _("現在の階を再構成する。", "Recreates current dungeon level.");

		{
			int base = 15;
			DICE_SID sides = 20;

			if (info) return info_delay(base, sides);

			if (cast)
			{
				alter_reality();
			}
		}
		break;

	case 23:
		if (name) return _("真・結界", "Warding True");
		if (desc) return _("自分のいる床と周囲8マスの床の上に、モンスターが通り抜けたり召喚されたりすることができなくなるルーンを描く。", "Creates glyphs in all adjacent squares and under you.");

		{
			POSITION rad = 1;

			if (info) return info_radius(rad);

			if (cast)
			{
				warding_glyph();
				glyph_creation();
			}
		}
		break;

	case 24:
		if (name) return _("不毛化", "Sterilization");
		if (desc) return _("この階の増殖するモンスターが増殖できなくなる。", "Prevents any breeders on current level from breeding.");

		{
			if (cast)
			{
				current_floor_ptr->num_repro += MAX_REPRO;
			}
		}
		break;

	case 25:
		if (name) return _("全感知", "Detection");
		if (desc) return _("近くの全てのモンスター、罠、扉、階段、財宝、そしてアイテムを感知する。", "Detects all monsters, traps, doors, stairs, treasures and items in your vicinity.");

		{
			POSITION rad = DETECT_RAD_DEFAULT;

			if (info) return info_radius(rad);

			if (cast)
			{
				detect_all(rad);
			}
		}
		break;

	case 26:
		if (name) return _("アンデッド消滅", "Annihilate Undead");
		if (desc) return _("自分の周囲にいるアンデッドを現在の階から消し去る。抵抗されると無効。",
			"Eliminates all nearby undead monsters, exhausting you.  Powerful or unique monsters may be able to resist.");

		{
			int power = plev + 50;

			if (info) return info_power(power);

			if (cast)
			{
				mass_genocide_undead(power, TRUE);
			}
		}
		break;

	case 27:
		if (name) return _("千里眼", "Clairvoyance");
		if (desc) return _("その階全体を永久に照らし、ダンジョン内すべてのアイテムを感知する。", "Maps and lights whole dungeon level. Knows all objects location. And gives telepathy for a while.");

		{
			if (cast)
			{
				wiz_lite(FALSE);
			}
		}
		break;

	case 28:
		if (name) return _("全復活", "Restoration");
		if (desc) return _("すべてのステータスと経験値を回復する。", "Restores all stats and experience.");

		{
			if (cast)
			{
				(void)restore_all_status();
				restore_level();
			}
		}
		break;

	case 29:
		if (name) return _("*体力回復*", "Healing True");
		if (desc) return _("最強の治癒の魔法で、負傷と朦朧状態も全快する。", "The greatest healing magic. Heals all HP, cut and stun.");

		{
			int heal = 2000;
			if (info) return info_heal(0, 0, heal);
			if (cast) (void)cure_critical_wounds(heal);
		}
		break;

	case 30:
		if (name) return _("聖なるビジョン", "Holy Vision");
		if (desc) return _("アイテムの持つ能力を完全に知る。", "*Identifies* an item.");

		{
			if (cast)
			{
				if (!identify_fully(FALSE)) return NULL;
			}
		}
		break;

	case 31:
		if (name) return _("究極の耐性", "Ultimate Resistance");
		if (desc) return _("一定時間、あらゆる耐性を付け、ACと魔法防御能力を上昇させる。", "Gives ultimate resistance, bonus to AC and speed.");

		{
			TIME_EFFECT base = (TIME_EFFECT)plev / 2;

			if (info) return info_duration(base, base);

			if (cast)
			{
				TIME_EFFECT v = randint1(base) + base;
				set_fast(v, FALSE);
				set_oppose_acid(v, FALSE);
				set_oppose_elec(v, FALSE);
				set_oppose_fire(v, FALSE);
				set_oppose_cold(v, FALSE);
				set_oppose_pois(v, FALSE);
				set_ultimate_res(v, FALSE);
			}
		}
		break;
	}

	return "";
}
