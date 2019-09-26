﻿#include "angband.h"
#include "cmd-spell.h"
#include "spells-summon.h"
#include "projection.h"
#include "spells-status.h"



/*!
* @brief 破邪領域魔法の各処理を行う
* @param spell 魔法ID
* @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST)
* @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST時はNULL文字列を返す。
*/
concptr do_crusade_spell(SPELL_IDX spell, BIT_FLAGS mode)
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
		if (name) return _("懲罰", "Punishment");
		if (desc) return _("電撃のボルトもしくはビームを放つ。", "Fires a bolt or beam of lightning.");
		{
			DICE_NUMBER dice = 3 + (plev - 1) / 5;
			DICE_SID sides = 4;
			if (info) return info_damage(dice, sides, 0);
			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				fire_bolt_or_beam(beam_chance() - 10, GF_ELEC, dir, damroll(dice, sides));
			}
		}
		break;

	case 1:
		if (name) return _("邪悪存在感知", "Detect Evil");
		if (desc) return _("近くの邪悪なモンスターを感知する。", "Detects all evil monsters in your vicinity.");
		{
			POSITION rad = DETECT_RAD_DEFAULT;
			if (info) return info_radius(rad);
			if (cast)
			{
				detect_monsters_evil(rad);
			}
		}
		break;

	case 2:
		if (name) return _("恐怖除去", "Remove Fear");
		if (desc) return _("恐怖を取り除く。", "Removes fear.");
		{
			if (cast) set_afraid(0);
		}
		break;

	case 3:
		if (name) return _("威圧", "Scare Monster");
		if (desc) return _("モンスター1体を恐怖させる。抵抗されると無効。", "Attempts to scare a monster.");

		{
			PLAYER_LEVEL power = plev;
			if (info) return info_power(power);
			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				fear_monster(dir, power);
			}
		}
		break;

	case 4:
		if (name) return _("聖域", "Sanctuary");
		if (desc) return _("隣接した全てのモンスターを眠らせる。抵抗されると無効。", "Attempts to sleep monsters in the adjacent squares.");
		{
			PLAYER_LEVEL power = plev;
			if (info) return info_power(power);
			if (cast) sleep_monsters_touch();
		}
		break;

	case 5:
		if (name) return _("入口", "Portal");
		if (desc) return _("中距離のテレポートをする。", "Teleports you a medium distance.");

		{
			POSITION range = 25 + plev / 2;
			if (info) return info_range(range);
			if (cast) teleport_player(range, 0L);
		}
		break;

	case 6:
		if (name) return _("スターダスト", "Star Dust");
		if (desc) return _("ターゲット付近に閃光のボルトを連射する。", "Fires many bolts of light near the target.");

		{
			DICE_NUMBER dice = 3 + (plev - 1) / 9;
			DICE_SID sides = 2;
			if (info) return info_multi_damage_dice(dice, sides);
			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				fire_blast(GF_LITE, dir, dice, sides, 10, 3);
			}
		}
		break;

	case 7:
		if (name) return _("身体浄化", "Purify");
		if (desc) return _("傷、毒、朦朧から全快する。", "Heals all cut, stun and poison status.");
		{
			if (cast)
			{
				set_cut(0);
				set_poisoned(0);
				set_stun(0);
			}
		}
		break;

	case 8:
		if (name) return _("邪悪飛ばし", "Scatter Evil");
		if (desc) return _("邪悪なモンスター1体をテレポートさせる。抵抗されると無効。", "Attempts to teleport an evil monster away.");

		{
			int power = MAX_SIGHT * 5;
			if (info) return info_power(power);
			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				fire_ball(GF_AWAY_EVIL, dir, power, 0);
			}
		}
		break;

	case 9:
		if (name) return _("聖なる光球", "Holy Orb");
		if (desc) return _("聖なる力をもつ宝珠を放つ。邪悪なモンスターに対して大きなダメージを与えるが、善良なモンスターには効果がない。",
			"Fires a ball with holy power. Hurts evil monsters greatly, but don't effect good monsters.");

		{
			DICE_NUMBER dice = 3;
			DICE_SID sides = 6;
			POSITION rad = (plev < 30) ? 2 : 3;
			int base;
			if (p_ptr->pclass == CLASS_PRIEST ||
				p_ptr->pclass == CLASS_HIGH_MAGE ||
				p_ptr->pclass == CLASS_SORCERER)
				base = plev + plev / 2;
			else
				base = plev + plev / 4;


			if (info) return info_damage(dice, sides, base);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_ball(GF_HOLY_FIRE, dir, damroll(dice, sides) + base, rad);
			}
		}
		break;

	case 10:
		if (name) return _("悪魔払い", "Exorcism");
		if (desc) return _("視界内の全てのアンデッド及び悪魔にダメージを与え、邪悪なモンスターを恐怖させる。",
			"Damages all undead and demons in sight, and scares all evil monsters in sight.");
		{
			DICE_SID sides = plev;
			int power = plev;
			if (info) return info_damage(1, sides, 0);
			if (cast)
			{
				dispel_undead(randint1(sides));
				dispel_demons(randint1(sides));
				turn_evil(power);
			}
		}
		break;

	case 11:
		if (name) return _("解呪", "Remove Curse");
		if (desc) return _("アイテムにかかった弱い呪いを解除する。", "Removes normal curses from equipped items.");
		{
			if (cast) (void)remove_curse();
		}
		break;

	case 12:
		if (name) return _("透明視認", "Sense Unseen");
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

	case 13:
		if (name) return _("対邪悪結界", "Protection from Evil");
		if (desc) return _("邪悪なモンスターの攻撃を防ぐバリアを張る。", "Gives aura which protect you from evil monster's physical attack.");

		{
			int base = 25;
			DICE_SID sides = 3 * plev;

			if (info) return info_duration(base, sides);

			if (cast)
			{
				set_protevil(randint1(sides) + base, FALSE);
			}
		}
		break;

	case 14:
		if (name) return _("裁きの雷", "Judgment Thunder");
		if (desc) return _("強力な電撃のボルトを放つ。", "Fires a powerful bolt of lightning.");

		{
			HIT_POINT dam = plev * 5;

			if (info) return info_damage(0, 0, dam);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				fire_bolt(GF_ELEC, dir, dam);
			}
		}
		break;

	case 15:
		if (name) return _("聖なる御言葉", "Holy Word");
		if (desc) return _("視界内の邪悪な存在に大きなダメージを与え、体力を回復し、毒、恐怖、朦朧状態、負傷から全快する。",
			"Damages all evil monsters in sight, heals HP somewhat, and completely heals poison, fear, stun and cut status.");

		{
			int dam_sides = plev * 6;
			int heal = 100;

			if (info) return format(_("損:1d%d/回%d", "dam:d%d/h%d"), dam_sides, heal);
			if (cast)
			{
				dispel_evil(randint1(dam_sides));
				hp_player(heal);
				set_afraid(0);
				set_poisoned(0);
				set_stun(0);
				set_cut(0);
			}
		}
		break;

	case 16:
		if (name) return _("開かれた道", "Unbarring Ways");
		if (desc) return _("一直線上の全ての罠と扉を破壊する。", "Fires a beam which destroy traps and doors.");

		{
			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				destroy_door(dir);
			}
		}
		break;

	case 17:
		if (name) return _("封魔", "Arrest");
		if (desc) return _("邪悪なモンスターの動きを止める。", "Attempts to paralyze an evil monster.");

		{
			int power = plev * 2;

			if (info) return info_power(power);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;
				stasis_evil(dir);
			}
		}
		break;

	case 18:
		if (name) return _("聖なるオーラ", "Holy Aura");
		if (desc) return _("一定時間、邪悪なモンスターを傷つける聖なるオーラを得る。",
			"Gives aura of holy power which injures evil monsters which attacked you for a while.");

		{
			int base = 20;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_tim_sh_holy(randint1(base) + base, FALSE);
			}
		}
		break;

	case 19:
		if (name) return _("アンデッド&悪魔退散", "Dispel Undead & Demons");
		if (desc) return _("視界内の全てのアンデッド及び悪魔にダメージを与える。", "Damages all undead and demons in sight.");

		{
			DICE_SID sides = plev * 4;

			if (info) return info_damage(1, sides, 0);

			if (cast)
			{
				dispel_undead(randint1(sides));
				dispel_demons(randint1(sides));
			}
		}
		break;

	case 20:
		if (name) return _("邪悪退散", "Dispel Evil");
		if (desc) return _("視界内の全ての邪悪なモンスターにダメージを与える。", "Damages all evil monsters in sight.");

		{
			DICE_SID sides = plev * 4;

			if (info) return info_damage(1, sides, 0);

			if (cast)
			{
				dispel_evil(randint1(sides));
			}
		}
		break;

	case 21:
		if (name) return _("聖なる刃", "Holy Blade");
		if (desc) return _("通常の武器に滅邪の属性をつける。", "Makes current weapon especially deadly against evil monsters.");

		{
			if (cast)
			{
				brand_weapon(13);
			}
		}
		break;

	case 22:
		if (name) return _("スターバースト", "Star Burst");
		if (desc) return _("巨大な閃光の球を放つ。", "Fires a huge ball of powerful light.");

		{
			HIT_POINT dam = 100 + plev * 2;
			POSITION rad = 4;

			if (info) return info_damage(0, 0, dam);

			if (cast)
			{
				if (!get_aim_dir(&dir)) return NULL;

				fire_ball(GF_LITE, dir, dam, rad);
			}
		}
		break;

	case 23:
		if (name) return _("天使召喚", "Summon Angel");
		if (desc) return _("天使を1体召喚する。", "Summons an angel.");

		{
			if (cast)
			{
				bool pet = !one_in_(3);
				u32b flg = 0L;

				if (pet) flg |= PM_FORCE_PET;
				else flg |= PM_NO_PET;
				if (!(pet && (plev < 50))) flg |= PM_ALLOW_GROUP;

				if (summon_specific((pet ? -1 : 0), p_ptr->y, p_ptr->x, (plev * 3) / 2, SUMMON_ANGEL, flg, '\0'))
				{
					if (pet)
					{
						msg_print(_("「ご用でございますか、ご主人様」", "'What is thy bidding... Master?'"));
					}
					else
					{
						msg_print(_("「我は汝の下僕にあらず！ 悪行者よ、悔い改めよ！」", "Mortal! Repent of thy impiousness."));
					}
				}
			}
		}
		break;

	case 24:
		if (name) return _("士気高揚", "Heroism");
		if (desc) return _("一定時間、ヒーロー気分になる。", "Removes fear, and gives bonus to hit and 10 more HP for a while.");

		{
			int base = 25;

			if (info) return info_duration(base, base);

			if (cast)
			{
				(void)heroism(base);
			}
		}
		break;

	case 25:
		if (name) return _("呪い退散", "Dispel Curse");
		if (desc) return _("アイテムにかかった強力な呪いを解除する。", "Removes normal and heavy curse from equipped items.");

		{
			if (cast) (void)remove_all_curse();
		}
		break;

	case 26:
		if (name) return _("邪悪追放", "Banish Evil");
		if (desc) return _("視界内の全ての邪悪なモンスターをテレポートさせる。抵抗されると無効。",
			"Teleports all evil monsters in sight away unless resisted.");

		{
			int power = 100;

			if (info) return info_power(power);

			if (cast)
			{
				if (banish_evil(power))
				{
					msg_print(_("神聖な力が邪悪を打ち払った！", "The holy power banishes evil!"));
				}
			}
		}
		break;

	case 27:
		if (name) return _("ハルマゲドン", "Armageddon");
		if (desc) return _("周辺のアイテム、モンスター、地形を破壊する。", "Destroys everything in nearby area.");

		{
			int base = 12;
			DICE_SID sides = 4;

			if (cast)
			{
				destroy_area(p_ptr->y, p_ptr->x, base + randint1(sides), FALSE);
			}
		}
		break;

	case 28:
		if (name) return _("目には目を", "An Eye for an Eye");
		if (desc) return _("一定時間、自分がダメージを受けたときに攻撃を行ったモンスターに対して同等のダメージを与える。",
			"Gives special aura for a while. When you are attacked by a monster, the monster are injured with same amount of damage as you take.");

		{
			int base = 10;

			if (info) return info_duration(base, base);

			if (cast)
			{
				set_tim_eyeeye(randint1(base) + base, FALSE);
			}
		}
		break;

	case 29:
		if (name) return _("神の怒り", "Wrath of the God");
		if (desc) return _("ターゲットの周囲に分解の球を多数落とす。", "Drops many balls of disintegration near the target.");

		{
			HIT_POINT dam = plev * 3 + 25;
			POSITION rad = 2;

			if (info) return info_multi_damage(dam);

			if (cast)
			{
				if (!cast_wrath_of_the_god(dam, rad)) return NULL;
			}
		}
		break;

	case 30:
		if (name) return _("神威", "Divine Intervention");
		if (desc) return _("隣接するモンスターに聖なるダメージを与え、視界内のモンスターにダメージ、減速、朦朧、混乱、恐怖、眠りを与える。さらに体力を回復する。",
			"Damages all adjacent monsters with holy power. Damages and attempt to slow, stun, confuse, scare and freeze all monsters in sight. And heals HP.");

		{
			int b_dam = plev * 11;
			int d_dam = plev * 4;
			int heal = 100;
			int power = plev * 4;

			if (info) return format(_("回%d/損%d+%d", "h%d/dm%d+%d"), heal, d_dam, b_dam / 2);
			if (cast)
			{
				project(0, 1, p_ptr->y, p_ptr->x, b_dam, GF_HOLY_FIRE, PROJECT_KILL, -1);
				dispel_monsters(d_dam);
				slow_monsters(plev);
				stun_monsters(power);
				confuse_monsters(power);
				turn_monsters(power);
				stasis_monsters(power);
				hp_player(heal);
			}
		}
		break;

	case 31:
		if (name) return _("聖戦", "Crusade");
		if (desc) return _("視界内の善良なモンスターをペットにしようとし、ならなかった場合及び善良でないモンスターを恐怖させる。さらに多数の加速された騎士を召喚し、ヒーロー、祝福、加速、対邪悪結界を得る。",
			"Attempts to charm all good monsters in sight, and scares all non-charmed monsters, and summons great number of knights, and gives heroism, bless, speed and protection from evil.");

		{
			if (cast)
			{
				int base = 25;
				int sp_sides = 20 + plev;
				int sp_base = plev;

				int i;
				crusade();
				for (i = 0; i < 12; i++)
				{
					int attempt = 10;
					POSITION my = 0, mx = 0;

					while (attempt--)
					{
						scatter(&my, &mx, p_ptr->y, p_ptr->x, 4, 0);

						/* Require empty grids */
						if (cave_empty_bold2(my, mx)) break;
					}
					if (attempt < 0) continue;
					summon_specific(-1, my, mx, plev, SUMMON_KNIGHTS, (PM_ALLOW_GROUP | PM_FORCE_PET | PM_HASTE), '\0');
				}
				set_hero(randint1(base) + base, FALSE);
				set_blessed(randint1(base) + base, FALSE);
				set_fast(randint1(sp_sides) + sp_base, FALSE);
				set_protevil(randint1(base) + base, FALSE);
				set_afraid(0);
			}
		}
		break;
	}

	return "";
}

