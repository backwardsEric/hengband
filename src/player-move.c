﻿/*!
 *  @file cmd1.c
 *  @brief プレイヤーのコマンド処理1 / Movement commands (part 1)
 *  @date 2014/01/02
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * @note
 * <pre>
 * The running algorithm:                       -CJS-
 *
 * In the diagrams below, the player has just arrived in the
 * grid marked as '@', and he has just come from a grid marked
 * as 'o', and he is about to enter the grid marked as 'x'.
 *
 * Of course, if the "requested" move was impossible, then you
 * will of course be blocked, and will stop.
 *
 * Overview: You keep moving until something interesting happens.
 * If you are in an enclosed space, you follow corners. This is
 * the usual corridor scheme. If you are in an open space, you go
 * straight, but stop before entering enclosed space. This is
 * analogous to reaching doorways. If you have enclosed space on
 * one side only (that is, running along side a wall) stop if
 * your wall opens out, or your open space closes in. Either case
 * corresponds to a doorway.
 *
 * What happens depends on what you can really SEE. (i.e. if you
 * have no light, then running along a dark corridor is JUST like
 * running in a dark room.) The algorithm works equally well in
 * corridors, rooms, mine tailings, earthquake rubble, etc, etc.
 *
 * These conditions are kept in static memory:
 * find_openarea         You are in the open on at least one
 * side.
 * find_breakleft        You have a wall on the left, and will
 * stop if it opens
 * find_breakright       You have a wall on the right, and will
 * stop if it opens
 *
 * To initialize these conditions, we examine the grids adjacent
 * to the grid marked 'x', two on each side (marked 'L' and 'R').
 * If either one of the two grids on a given side is seen to be
 * closed, then that side is considered to be closed. If both
 * sides are closed, then it is an enclosed (corridor) run.
 *
 * LL           L
 * @@x          LxR
 * RR          @@R
 *
 * Looking at more than just the immediate squares is
 * significant. Consider the following case. A run along the
 * corridor will stop just before entering the center point,
 * because a choice is clearly established. Running in any of
 * three available directions will be defined as a corridor run.
 * Note that a minor hack is inserted to make the angled corridor
 * entry (with one side blocked near and the other side blocked
 * further away from the runner) work correctly. The runner moves
 * diagonally, but then saves the previous direction as being
 * straight into the gap. Otherwise, the tail end of the other
 * entry would be perceived as an alternative on the next move.
 *
 * \#.\#
 * \#\#.\#\#
 * \.\@x..
 * \#\#.\#\#
 * \#.\#
 *
 * Likewise, a run along a wall, and then into a doorway (two
 * runs) will work correctly. A single run rightwards from \@ will
 * stop at 1. Another run right and down will enter the corridor
 * and make the corner, stopping at the 2.
 *
 * \#\#\#\#\#\#\#\#\#\#\#\#\#\#\#\#\#\#
 * o@@x       1
 * \#\#\#\#\#\#\#\#\#\#\# \#\#\#\#\#\#
 * \#2          \#
 * \#\#\#\#\#\#\#\#\#\#\#\#\#
 *
 * After any move, the function area_affect is called to
 * determine the new surroundings, and the direction of
 * subsequent moves. It examines the current player location
 * (at which the runner has just arrived) and the previous
 * direction (from which the runner is considered to have come).
 *
 * Moving one square in some direction places you adjacent to
 * three or five new squares (for straight and diagonal moves
 * respectively) to which you were not previously adjacent,
 * marked as '!' in the diagrams below.
 *
 *   ...!              ...
 *   .o@@!  (normal)    .o.!  (diagonal)
 *   ...!  (east)      ..@@!  (south east)
 *                      !!!
 *
 * You STOP if any of the new squares are interesting in any way:
 * for example, if they contain visible monsters or treasure.
 *
 * You STOP if any of the newly adjacent squares seem to be open,
 * and you are also looking for a break on that side. (that is,
 * find_openarea AND find_break).
 *
 * You STOP if any of the newly adjacent squares do NOT seem to be
 * open and you are in an open area, and that side was previously
 * entirely open.
 *
 * Corners: If you are not in the open (i.e. you are in a corridor)
 * and there is only one way to go in the new squares, then turn in
 * that direction. If there are more than two new ways to go, STOP.
 * If there are two ways to go, and those ways are separated by a
 * square which does not seem to be open, then STOP.
 *
 * Otherwise, we have a potential corner. There are two new open
 * squares, which are also adjacent. One of the new squares is
 * diagonally located, the other is straight on (as in the diagram).
 * We consider two more squares further out (marked below as ?).
 *
 * We assign "option" to the straight-on grid, and "option2" to the
 * diagonal grid, and "check_dir" to the grid marked 's'.
 *
 * \#\#s
 * @@x?
 * \#.?
 *
 * If they are both seen to be closed, then it is seen that no benefit
 * is gained from moving straight. It is a known corner.  To cut the
 * corner, go diagonally, otherwise go straight, but pretend you
 * stepped diagonally into that next location for a full view next
 * time. Conversely, if one of the ? squares is not seen to be closed,
 * then there is a potential choice. We check to see whether it is a
 * potential corner or an intersection/room entrance.  If the square
 * two spaces straight ahead, and the space marked with 's' are both
 * unknown space, then it is a potential corner and enter if
 * find_examine is set, otherwise must stop because it is not a
 * corner. (find_examine option is removed and always is TRUE.)
 * </pre>
 */

#include "angband.h"
#include "melee.h"
#include "grid.h"
#include "trap.h"
#include "projection.h"
#include "quest.h"
#include "artifact.h"
#include "player-move.h"
#include "player-status.h"
#include "spells-floor.h"
#include "feature.h"
#include "warning.h"
#include "monster.h"
#include "monster-spell.h"



/*!
 * @brief 地形やその上のアイテムの隠された要素を明かす /
 * Search for hidden things
 * @param y 対象となるマスのY座標
 * @param x 対象となるマスのX座標
 * @return なし
 */
static void discover_hidden_things(POSITION y, POSITION x)
{
	OBJECT_IDX this_o_idx, next_o_idx = 0;
	grid_type *g_ptr;
	g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Invisible trap */
	if (g_ptr->mimic && is_trap(g_ptr->feat))
	{
		/* Pick a trap */
		disclose_grid(y, x);

		msg_print(_("トラップを発見した。", "You have found a trap."));

		disturb(FALSE, TRUE);
	}

	/* Secret door */
	if (is_hidden_door(g_ptr))
	{
		msg_print(_("隠しドアを発見した。", "You have found a secret door."));

		/* Disclose */
		disclose_grid(y, x);

		disturb(FALSE, FALSE);
	}

	/* Scan all objects in the grid */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

		/* Skip non-chests */
		if (o_ptr->tval != TV_CHEST) continue;

		/* Skip non-trapped chests */
		if (!chest_traps[o_ptr->pval]) continue;

		/* Identify once */
		if (!object_is_known(o_ptr))
		{
			msg_print(_("箱に仕掛けられたトラップを発見した！", "You have discovered a trap on the chest!"));

			/* Know the trap */
			object_known(o_ptr);

			/* Notice it */
			disturb(FALSE, FALSE);
		}
	}
}

/*!
 * @brief プレイヤーの探索処理判定
 * @return なし
 */
void search(void)
{
	DIRECTION i;
	PERCENTAGE chance;

	/* Start with base search ability */
	chance = p_ptr->skill_srh;

	/* Penalize various conditions */
	if (p_ptr->blind || no_lite()) chance = chance / 10;
	if (p_ptr->confused || p_ptr->image) chance = chance / 10;

	/* Search the nearby grids, which are always in bounds */
	for (i = 0; i < 9; ++ i)
	{
		/* Sometimes, notice things */
		if (randint0(100) < chance)
		{
			discover_hidden_things(p_ptr->y + ddy_ddd[i], p_ptr->x + ddx_ddd[i]);
		}
	}
}


/*!
 * @brief プレイヤーがオブジェクトを拾った際のメッセージ表示処理 /
 * Helper routine for py_pickup() and py_pickup_floor().
 * @param o_idx 取得したオブジェクトの参照ID
 * @return なし
 * @details
 * アイテムを拾った際に「２つのケーキを持っている」\n
 * "You have two cakes." とアイテムを拾った後の合計のみの表示がオリジナル\n
 * だが、違和感が\n
 * あるという指摘をうけたので、「～を拾った、～を持っている」という表示\n
 * にかえてある。そのための配列。\n
 * Add the given dungeon object to the character's inventory.\n
 * Delete the object afterwards.\n
 */
void py_pickup_aux(OBJECT_IDX o_idx)
{
	INVENTORY_IDX slot;

#ifdef JP
	GAME_TEXT o_name[MAX_NLEN];
	GAME_TEXT old_name[MAX_NLEN];
	char kazu_str[80];
	int hirottakazu;
#else
	GAME_TEXT o_name[MAX_NLEN];
#endif

	object_type *o_ptr;

	o_ptr = &current_floor_ptr->o_list[o_idx];

#ifdef JP
	object_desc(old_name, o_ptr, OD_NAME_ONLY);
	object_desc_kosuu(kazu_str, o_ptr);
	hirottakazu = o_ptr->number;
#endif
	/* Carry the object */
	slot = inven_carry(o_ptr);

	/* Get the object again */
	o_ptr = &inventory[slot];

	delete_object_idx(o_idx);

	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN)
	{
		bool old_known = identify_item(o_ptr);

		/* Auto-inscription/destroy */
		autopick_alter_item(slot, (bool)(destroy_identify && !old_known));

		/* If it is destroyed, don't pick it up */
		if (o_ptr->marked & OM_AUTODESTROY) return;
	}

	object_desc(o_name, o_ptr, 0);

#ifdef JP
	if ((o_ptr->name1 == ART_CRIMSON) && (p_ptr->pseikaku == SEIKAKU_COMBAT))
	{
		msg_format("こうして、%sは『クリムゾン』を手に入れた。", p_ptr->name);
		msg_print("しかし今、『混沌のサーペント』の放ったモンスターが、");
		msg_format("%sに襲いかかる．．．", p_ptr->name);
	}
	else
	{
		if (plain_pickup)
		{
			msg_format("%s(%c)を持っている。",o_name, index_to_label(slot));
		}
		else
		{
			if (o_ptr->number > hirottakazu) {
			    msg_format("%s拾って、%s(%c)を持っている。",
			       kazu_str, o_name, index_to_label(slot));
			} else {
				msg_format("%s(%c)を拾った。", o_name, index_to_label(slot));
			}
		}
	}
	strcpy(record_o_name, old_name);
#else
	msg_format("You have %s (%c).", o_name, index_to_label(slot));
	strcpy(record_o_name, o_name);
#endif
	record_turn = current_world_ptr->game_turn;


	check_find_art_quest_completion(o_ptr);
}


/*!
 * @brief プレイヤーがオブジェクト上に乗った際の表示処理
 * @param pickup 自動拾い処理を行うならばTRUEとする
 * @return なし
 * @details
 * Player "wants" to pick up an object or gold.
 * Note that we ONLY handle things that can be picked up.
 * See "move_player()" for handling of other things.
 */
void carry(bool pickup)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x];

	OBJECT_IDX this_o_idx, next_o_idx = 0;

	GAME_TEXT o_name[MAX_NLEN];

	/* Recenter the map around the player */
	verify_panel();

	p_ptr->update |= (PU_MONSTERS);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD);
	handle_stuff();

	/* Automatically pickup/destroy/inscribe items */
	autopick_pickup_items(g_ptr);

	if (easy_floor)
	{
		py_pickup_floor(pickup);
		return;
	}

	/* Scan the pile of objects */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[this_o_idx];

#ifdef ALLOW_EASY_SENSE /* TNB */

		/* Option: Make item sensing easy */
		if (easy_sense)
		{
			/* Sense the object */
			(void)sense_object(o_ptr);
		}

#endif /* ALLOW_EASY_SENSE -- TNB */

		object_desc(o_name, o_ptr, 0);
		next_o_idx = o_ptr->next_o_idx;

		disturb(FALSE, FALSE);

		/* Pick up gold */
		if (o_ptr->tval == TV_GOLD)
		{
			int value = (long)o_ptr->pval;

			/* Delete the gold */
			delete_object_idx(this_o_idx);

			msg_format(_(" $%ld の価値がある%sを見つけた。", "You collect %ld gold pieces worth of %s."),
			   (long)value, o_name);

			sound(SOUND_SELL);

			/* Collect the gold */
			p_ptr->au += value;

			/* Redraw gold */
			p_ptr->redraw |= (PR_GOLD);
			p_ptr->window |= (PW_PLAYER);
		}

		/* Pick up objects */
		else
		{
			/* Hack - some objects were handled in autopick_pickup_items(). */
			if (o_ptr->marked & OM_NOMSG)
			{
				/* Clear the flag. */
				o_ptr->marked &= ~OM_NOMSG;
			}
			else if (!pickup)
			{
				msg_format(_("%sがある。", "You see %s."), o_name);
			}

			/* Note that the pack is too full */
			else if (!inven_carry_okay(o_ptr))
			{
				msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
			}

			/* Pick up the item (if requested and allowed) */
			else
			{
				int okay = TRUE;

				/* Hack -- query every item */
				if (carry_query_flag)
				{
					char out_val[MAX_NLEN+20];
					sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
					okay = get_check(out_val);
				}

				/* Attempt to pick up an object. */
				if (okay)
				{
					/* Pick up the object */
					py_pickup_aux(this_o_idx);
				}
			}
		}
	}
}


/*!
 * @brief パターンによる移動制限処理
 * @param c_y プレイヤーの移動元Y座標
 * @param c_x プレイヤーの移動元X座標
 * @param n_y プレイヤーの移動先Y座標
 * @param n_x プレイヤーの移動先X座標
 * @return 移動処理が可能である場合（可能な場合に選択した場合）TRUEを返す。
 */
bool pattern_seq(POSITION c_y, POSITION c_x, POSITION n_y, POSITION n_x)
{
	feature_type *cur_f_ptr = &f_info[current_floor_ptr->grid_array[c_y][c_x].feat];
	feature_type *new_f_ptr = &f_info[current_floor_ptr->grid_array[n_y][n_x].feat];
	bool is_pattern_tile_cur = have_flag(cur_f_ptr->flags, FF_PATTERN);
	bool is_pattern_tile_new = have_flag(new_f_ptr->flags, FF_PATTERN);
	int pattern_type_cur, pattern_type_new;

	if (!is_pattern_tile_cur && !is_pattern_tile_new) return TRUE;

	pattern_type_cur = is_pattern_tile_cur ? cur_f_ptr->subtype : NOT_PATTERN_TILE;
	pattern_type_new = is_pattern_tile_new ? new_f_ptr->subtype : NOT_PATTERN_TILE;

	if (pattern_type_new == PATTERN_TILE_START)
	{
		if (!is_pattern_tile_cur && !p_ptr->confused && !p_ptr->stun && !p_ptr->image)
		{
			if (get_check(_("パターンの上を歩き始めると、全てを歩かなければなりません。いいですか？", 
							"If you start walking the Pattern, you must walk the whole way. Ok? ")))
				return TRUE;
			else
				return FALSE;
		}
		else
			return TRUE;
	}
	else if ((pattern_type_new == PATTERN_TILE_OLD) ||
		 (pattern_type_new == PATTERN_TILE_END) ||
		 (pattern_type_new == PATTERN_TILE_WRECKED))
	{
		if (is_pattern_tile_cur)
		{
			return TRUE;
		}
		else
		{
			msg_print(_("パターンの上を歩くにはスタート地点から歩き始めなくてはなりません。",
						"You must start walking the Pattern from the startpoint."));

			return FALSE;
		}
	}
	else if ((pattern_type_new == PATTERN_TILE_TELEPORT) ||
		 (pattern_type_cur == PATTERN_TILE_TELEPORT))
	{
		return TRUE;
	}
	else if (pattern_type_cur == PATTERN_TILE_START)
	{
		if (is_pattern_tile_new)
			return TRUE;
		else
		{
			msg_print(_("パターンの上は正しい順序で歩かねばなりません。", "You must walk the Pattern in correct order."));
			return FALSE;
		}
	}
	else if ((pattern_type_cur == PATTERN_TILE_OLD) ||
		 (pattern_type_cur == PATTERN_TILE_END) ||
		 (pattern_type_cur == PATTERN_TILE_WRECKED))
	{
		if (!is_pattern_tile_new)
		{
			msg_print(_("パターンを踏み外してはいけません。", "You may not step off from the Pattern."));
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	else
	{
		if (!is_pattern_tile_cur)
		{
			msg_print(_("パターンの上を歩くにはスタート地点から歩き始めなくてはなりません。",
						"You must start walking the Pattern from the startpoint."));

			return FALSE;
		}
		else
		{
			byte ok_move = PATTERN_TILE_START;
			switch (pattern_type_cur)
			{
				case PATTERN_TILE_1:
					ok_move = PATTERN_TILE_2;
					break;
				case PATTERN_TILE_2:
					ok_move = PATTERN_TILE_3;
					break;
				case PATTERN_TILE_3:
					ok_move = PATTERN_TILE_4;
					break;
				case PATTERN_TILE_4:
					ok_move = PATTERN_TILE_1;
					break;
				default:
					if (p_ptr->wizard)
						msg_format(_("おかしなパターン歩行、%d。", "Funny Pattern walking, %d."), pattern_type_cur);

					return TRUE; /* Goof-up */
			}

			if ((pattern_type_new == ok_move) || (pattern_type_new == pattern_type_cur))
				return TRUE;
			else
			{
				if (!is_pattern_tile_new)
					msg_print(_("パターンを踏み外してはいけません。", "You may not step off from the Pattern."));
				else
					msg_print(_("パターンの上は正しい順序で歩かねばなりません。", "You must walk the Pattern in correct order."));

				return FALSE;
			}
		}
	}
}


/*!
 * @brief プレイヤーが地形踏破可能かを返す
 * @param feature 判定したい地形ID
 * @param mode 移動に関するオプションフラグ
 * @return 移動可能ならばTRUEを返す
 */
bool player_can_enter(FEAT_IDX feature, BIT_FLAGS16 mode)
{
	feature_type *f_ptr = &f_info[feature];

	if (p_ptr->riding) return monster_can_cross_terrain(feature, &r_info[current_floor_ptr->m_list[p_ptr->riding].r_idx], mode | CEM_RIDING);

	if (have_flag(f_ptr->flags, FF_PATTERN))
	{
		if (!(mode & CEM_P_CAN_ENTER_PATTERN)) return FALSE;
	}

	if (have_flag(f_ptr->flags, FF_CAN_FLY) && p_ptr->levitation) return TRUE;
	if (have_flag(f_ptr->flags, FF_CAN_SWIM) && p_ptr->can_swim) return TRUE;
	if (have_flag(f_ptr->flags, FF_CAN_PASS) && p_ptr->pass_wall) return TRUE;

	if (!have_flag(f_ptr->flags, FF_MOVE)) return FALSE;

	return TRUE;
}


/*!
 * @brief 移動に伴うプレイヤーのステータス変化処理
 * @param ny 移動先Y座標
 * @param nx 移動先X座標
 * @param mpe_mode 移動オプションフラグ
 * @return プレイヤーが死亡やフロア離脱を行わず、実際に移動が可能ならばTRUEを返す。
 */
bool move_player_effect(POSITION ny, POSITION nx, BIT_FLAGS mpe_mode)
{
	POSITION oy = p_ptr->y;
	POSITION ox = p_ptr->x;
	grid_type *g_ptr = &current_floor_ptr->grid_array[ny][nx];
	grid_type *oc_ptr = &current_floor_ptr->grid_array[oy][ox];
	feature_type *f_ptr = &f_info[g_ptr->feat];
	feature_type *of_ptr = &f_info[oc_ptr->feat];

	if (!(mpe_mode & MPE_STAYING))
	{
		MONSTER_IDX om_idx = oc_ptr->m_idx;
		MONSTER_IDX nm_idx = g_ptr->m_idx;

		p_ptr->y = ny;
		p_ptr->x = nx;

		/* Hack -- For moving monster or riding player's moving */
		if (!(mpe_mode & MPE_DONT_SWAP_MON))
		{
			/* Swap two monsters */
			g_ptr->m_idx = om_idx;
			oc_ptr->m_idx = nm_idx;

			if (om_idx > 0) /* Monster on old spot (or p_ptr->riding) */
			{
				monster_type *om_ptr = &current_floor_ptr->m_list[om_idx];
				om_ptr->fy = ny;
				om_ptr->fx = nx;
				update_monster(om_idx, TRUE);
			}

			if (nm_idx > 0) /* Monster on new spot */
			{
				monster_type *nm_ptr = &current_floor_ptr->m_list[nm_idx];
				nm_ptr->fy = oy;
				nm_ptr->fx = ox;
				update_monster(nm_idx, TRUE);
			}
		}

		lite_spot(oy, ox);
		lite_spot(ny, nx);

		/* Check for new panel (redraw map) */
		verify_panel();

		if (mpe_mode & MPE_FORGET_FLOW)
		{
			forget_flow();

			p_ptr->update |= (PU_UN_VIEW);
			p_ptr->redraw |= (PR_MAP);
		}

		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_DISTANCE);
		p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

		/* Remove "unsafe" flag */
		if ((!p_ptr->blind && !no_lite()) || !is_trap(g_ptr->feat)) g_ptr->info &= ~(CAVE_UNSAFE);

		/* For get everything when requested hehe I'm *NASTY* */
		if (current_floor_ptr->dun_level && (d_info[p_ptr->dungeon_idx].flags1 & DF1_FORGET)) wiz_dark();
		if (mpe_mode & MPE_HANDLE_STUFF) handle_stuff();

		if (p_ptr->pclass == CLASS_NINJA)
		{
			if (g_ptr->info & (CAVE_GLOW)) set_superstealth(FALSE);
			else if (p_ptr->cur_lite <= 0) set_superstealth(TRUE);
		}

		if ((p_ptr->action == ACTION_HAYAGAKE) &&
		    (!have_flag(f_ptr->flags, FF_PROJECT) ||
		     (!p_ptr->levitation && have_flag(f_ptr->flags, FF_DEEP))))
		{
			msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
			set_action(ACTION_NONE);
		}
		if (p_ptr->prace == RACE_MERFOLK)
		{
			if(have_flag(f_ptr->flags, FF_WATER) ^ have_flag(of_ptr->flags, FF_WATER))
			{
				p_ptr->update |= PU_BONUS;
				update_creature(p_ptr);
			}
		}
	}

	if (mpe_mode & MPE_ENERGY_USE)
	{
		if (music_singing(MUSIC_WALL))
		{
			(void)project(0, 0, p_ptr->y, p_ptr->x, (60 + p_ptr->lev), GF_DISINTEGRATE,
				PROJECT_KILL | PROJECT_ITEM, -1);

			if (!player_bold(ny, nx) || p_ptr->is_dead || p_ptr->leaving) return FALSE;
		}

		/* Spontaneous Searching */
		if ((p_ptr->skill_fos >= 50) || (0 == randint0(50 - p_ptr->skill_fos)))
		{
			search();
		}

		/* Continuous Searching */
		if (p_ptr->action == ACTION_SEARCH)
		{
			search();
		}
	}

	/* Handle "objects" */
	if (!(mpe_mode & MPE_DONT_PICKUP))
	{
		carry((mpe_mode & MPE_DO_PICKUP) ? TRUE : FALSE);
	}

	/* Handle "store doors" */
	if (have_flag(f_ptr->flags, FF_STORE))
	{
		disturb(FALSE, TRUE);

		free_turn(p_ptr);
		/* Hack -- Enter store */
		command_new = SPECIAL_KEY_STORE;
	}

	/* Handle "building doors" -KMW- */
	else if (have_flag(f_ptr->flags, FF_BLDG))
	{
		disturb(FALSE, TRUE);

		free_turn(p_ptr);
		/* Hack -- Enter building */
		command_new = SPECIAL_KEY_BUILDING;
	}

	/* Handle quest areas -KMW- */
	else if (have_flag(f_ptr->flags, FF_QUEST_ENTER))
	{
		disturb(FALSE, TRUE);

		free_turn(p_ptr);
		/* Hack -- Enter quest level */
		command_new = SPECIAL_KEY_QUEST;
	}

	else if (have_flag(f_ptr->flags, FF_QUEST_EXIT))
	{
		if (quest[p_ptr->inside_quest].type == QUEST_TYPE_FIND_EXIT)
		{
			complete_quest(p_ptr->inside_quest);
		}

		leave_quest_check();

		p_ptr->inside_quest = g_ptr->special;
		current_floor_ptr->dun_level = 0;
		p_ptr->oldpx = 0;
		p_ptr->oldpy = 0;

		p_ptr->leaving = TRUE;
	}

	/* Set off a trap */
	else if (have_flag(f_ptr->flags, FF_HIT_TRAP) && !(mpe_mode & MPE_STAYING))
	{
		disturb(FALSE, TRUE);

		/* Hidden trap */
		if (g_ptr->mimic || have_flag(f_ptr->flags, FF_SECRET))
		{
			msg_print(_("トラップだ！", "You found a trap!"));

			/* Pick a trap */
			disclose_grid(p_ptr->y, p_ptr->x);
		}

		/* Hit the trap */
		hit_trap((mpe_mode & MPE_BREAK_TRAP) ? TRUE : FALSE);

		if (!player_bold(ny, nx) || p_ptr->is_dead || p_ptr->leaving) return FALSE;
	}

	/* Warn when leaving trap detected region */
	if (!(mpe_mode & MPE_STAYING) && (disturb_trap_detect || alert_trap_detect)
	    && p_ptr->dtrap && !(g_ptr->info & CAVE_IN_DETECT))
	{
		/* No duplicate warning */
		p_ptr->dtrap = FALSE;

		/* You are just on the edge */
		if (!(g_ptr->info & CAVE_UNSAFE))
		{
			if (alert_trap_detect)
			{
				msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
			}

			if (disturb_trap_detect) disturb(FALSE, TRUE);
		}
	}

	return player_bold(ny, nx) && !p_ptr->is_dead && !p_ptr->leaving;
}

/*!
 * @brief 該当地形のトラップがプレイヤーにとって無効かどうかを判定して返す
 * @param feat 地形ID
 * @return トラップが自動的に無効ならばTRUEを返す
 */
bool trap_can_be_ignored(FEAT_IDX feat)
{
	feature_type *f_ptr = &f_info[feat];

	if (!have_flag(f_ptr->flags, FF_TRAP)) return TRUE;

	switch (f_ptr->subtype)
	{
	case TRAP_TRAPDOOR:
	case TRAP_PIT:
	case TRAP_SPIKED_PIT:
	case TRAP_POISON_PIT:
		if (p_ptr->levitation) return TRUE;
		break;
	case TRAP_TELEPORT:
		if (p_ptr->anti_tele) return TRUE;
		break;
	case TRAP_FIRE:
		if (p_ptr->immune_fire) return TRUE;
		break;
	case TRAP_ACID:
		if (p_ptr->immune_acid) return TRUE;
		break;
	case TRAP_BLIND:
		if (p_ptr->resist_blind) return TRUE;
		break;
	case TRAP_CONFUSE:
		if (p_ptr->resist_conf) return TRUE;
		break;
	case TRAP_POISON:
		if (p_ptr->resist_pois) return TRUE;
		break;
	case TRAP_SLEEP:
		if (p_ptr->free_act) return TRUE;
		break;
	}

	return FALSE;
}


/*
 * Determine if a "boundary" grid is "floor mimic"
 */
#define boundary_floor(C, F, MF) \
	((C)->mimic && permanent_wall(F) && \
	 (have_flag((MF)->flags, FF_MOVE) || have_flag((MF)->flags, FF_CAN_FLY)) && \
	 have_flag((MF)->flags, FF_PROJECT) && \
	 !have_flag((MF)->flags, FF_OPEN))


/*!
 * @brief 該当地形のトラップがプレイヤーにとって無効かどうかを判定して返す /
 * Move player in the given direction, with the given "pickup" flag.
 * @param dir 移動方向ID
 * @param do_pickup 罠解除を試みながらの移動ならばTRUE
 * @param break_trap トラップ粉砕処理を行うならばTRUE
 * @return 実際に移動が行われたならばTRUEを返す。
 * @note
 * This routine should (probably) always induce energy expenditure.\n
 * @details
 * Note that moving will *always* take a turn, and will *always* hit\n
 * any monster which might be in the destination grid.  Previously,\n
 * moving into walls was "free" and did NOT hit invisible monsters.\n
 */
void move_player(DIRECTION dir, bool do_pickup, bool break_trap)
{
	/* Find the result of moving */
	POSITION y = p_ptr->y + ddy[dir];
	POSITION x = p_ptr->x + ddx[dir];

	/* Examine the destination */
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

	feature_type *f_ptr = &f_info[g_ptr->feat];

	monster_type *m_ptr;

	monster_type *riding_m_ptr = &current_floor_ptr->m_list[p_ptr->riding];
	monster_race *riding_r_ptr = &r_info[p_ptr->riding ? riding_m_ptr->r_idx : 0];

	GAME_TEXT m_name[MAX_NLEN];

	bool p_can_enter = player_can_enter(g_ptr->feat, CEM_P_CAN_ENTER_PATTERN);
	bool p_can_kill_walls = FALSE;
	bool stormbringer = FALSE;

	bool oktomove = TRUE;
	bool do_past = FALSE;

	/* Exit the area */
	if (!current_floor_ptr->dun_level && !p_ptr->wild_mode &&
		((x == 0) || (x == MAX_WID - 1) ||
		 (y == 0) || (y == MAX_HGT - 1)))
	{
		/* Can the player enter the grid? */
		if (g_ptr->mimic && player_can_enter(g_ptr->mimic, 0))
		{
			/* Hack: move to new area */
			if ((y == 0) && (x == 0))
			{
				p_ptr->wilderness_y--;
				p_ptr->wilderness_x--;
				p_ptr->oldpy = current_floor_ptr->height - 2;
				p_ptr->oldpx = current_floor_ptr->width - 2;
				p_ptr->ambush_flag = FALSE;
			}

			else if ((y == 0) && (x == MAX_WID - 1))
			{
				p_ptr->wilderness_y--;
				p_ptr->wilderness_x++;
				p_ptr->oldpy = current_floor_ptr->height - 2;
				p_ptr->oldpx = 1;
				p_ptr->ambush_flag = FALSE;
			}

			else if ((y == MAX_HGT - 1) && (x == 0))
			{
				p_ptr->wilderness_y++;
				p_ptr->wilderness_x--;
				p_ptr->oldpy = 1;
				p_ptr->oldpx = current_floor_ptr->width - 2;
				p_ptr->ambush_flag = FALSE;
			}

			else if ((y == MAX_HGT - 1) && (x == MAX_WID - 1))
			{
				p_ptr->wilderness_y++;
				p_ptr->wilderness_x++;
				p_ptr->oldpy = 1;
				p_ptr->oldpx = 1;
				p_ptr->ambush_flag = FALSE;
			}

			else if (y == 0)
			{
				p_ptr->wilderness_y--;
				p_ptr->oldpy = current_floor_ptr->height - 2;
				p_ptr->oldpx = x;
				p_ptr->ambush_flag = FALSE;
			}

			else if (y == MAX_HGT - 1)
			{
				p_ptr->wilderness_y++;
				p_ptr->oldpy = 1;
				p_ptr->oldpx = x;
				p_ptr->ambush_flag = FALSE;
			}

			else if (x == 0)
			{
				p_ptr->wilderness_x--;
				p_ptr->oldpx = current_floor_ptr->width - 2;
				p_ptr->oldpy = y;
				p_ptr->ambush_flag = FALSE;
			}

			else if (x == MAX_WID - 1)
			{
				p_ptr->wilderness_x++;
				p_ptr->oldpx = 1;
				p_ptr->oldpy = y;
				p_ptr->ambush_flag = FALSE;
			}

			p_ptr->leaving = TRUE;
			take_turn(p_ptr, 100);

			return;
		}

		/* "Blocked" message appears later */
		/* oktomove = FALSE; */
		p_can_enter = FALSE;
	}

	m_ptr = &current_floor_ptr->m_list[g_ptr->m_idx];

	if (inventory[INVEN_RARM].name1 == ART_STORMBRINGER) stormbringer = TRUE;
	if (inventory[INVEN_LARM].name1 == ART_STORMBRINGER) stormbringer = TRUE;

	/* Player can not walk through "walls"... */
	/* unless in Shadow Form */
	p_can_kill_walls = p_ptr->kill_wall && have_flag(f_ptr->flags, FF_HURT_DISI) &&
		(!p_can_enter || !have_flag(f_ptr->flags, FF_LOS)) &&
		!have_flag(f_ptr->flags, FF_PERMANENT);

	/* Hack -- attack monsters */
	if (g_ptr->m_idx && (m_ptr->ml || p_can_enter || p_can_kill_walls))
	{
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Attack -- only if we can see it OR it is not in a wall */
		if (!is_hostile(m_ptr) &&
		    !(p_ptr->confused || p_ptr->image || !m_ptr->ml || p_ptr->stun ||
		    ((p_ptr->muta2 & MUT2_BERS_RAGE) && p_ptr->shero)) &&
		    pattern_seq(p_ptr->y, p_ptr->x, y, x) && (p_can_enter || p_can_kill_walls))
		{
			/* Disturb the monster */
			(void)set_monster_csleep(g_ptr->m_idx, 0);

			/* Extract monster name (or "it") */
			monster_desc(m_name, m_ptr, 0);

			if (m_ptr->ml)
			{
				/* Auto-Recall if possible and visible */
				if (!p_ptr->image) monster_race_track(m_ptr->ap_r_idx);

				/* Track a new monster */
				health_track(g_ptr->m_idx);
			}

			/* displace? */
			if ((stormbringer && (randint1(1000) > 666)) || (p_ptr->pclass == CLASS_BERSERKER))
			{
				py_attack(y, x, 0);
				oktomove = FALSE;
			}
			else if (monster_can_cross_terrain(current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].feat, r_ptr, 0))
			{
				do_past = TRUE;
			}
			else
			{
				msg_format(_("%^sが邪魔だ！", "%^s is in your way!"), m_name);
				free_turn(p_ptr);
				oktomove = FALSE;
			}

			/* now continue on to 'movement' */
		}
		else
		{
			py_attack(y, x, 0);
			oktomove = FALSE;
		}
	}

	if (oktomove && p_ptr->riding)
	{
		if (riding_r_ptr->flags1 & RF1_NEVER_MOVE)
		{
			msg_print(_("動けない！", "Can't move!"));
			free_turn(p_ptr);
			oktomove = FALSE;
			disturb(FALSE, TRUE);
		}
		else if (MON_MONFEAR(riding_m_ptr))
		{
			GAME_TEXT steed_name[MAX_NLEN];
			monster_desc(steed_name, riding_m_ptr, 0);
			msg_format(_("%sが恐怖していて制御できない。", "%^s is too scared to control."), steed_name);
			oktomove = FALSE;
			disturb(FALSE, TRUE);
		}
		else if (p_ptr->riding_ryoute)
		{
			oktomove = FALSE;
			disturb(FALSE, TRUE);
		}
		else if (have_flag(f_ptr->flags, FF_CAN_FLY) && (riding_r_ptr->flags7 & RF7_CAN_FLY))
		{
			/* Allow moving */
		}
		else if (have_flag(f_ptr->flags, FF_CAN_SWIM) && (riding_r_ptr->flags7 & RF7_CAN_SWIM))
		{
			/* Allow moving */
		}
		else if (have_flag(f_ptr->flags, FF_WATER) &&
			!(riding_r_ptr->flags7 & RF7_AQUATIC) &&
			(have_flag(f_ptr->flags, FF_DEEP) || (riding_r_ptr->flags2 & RF2_AURA_FIRE)))
		{
			msg_format(_("%sの上に行けない。", "Can't swim."), f_name + f_info[get_feat_mimic(g_ptr)].name);
			free_turn(p_ptr);
			oktomove = FALSE;
			disturb(FALSE, TRUE);
		}
		else if (!have_flag(f_ptr->flags, FF_WATER) && (riding_r_ptr->flags7 & RF7_AQUATIC))
		{
			msg_format(_("%sから上がれない。", "Can't land."), f_name + f_info[get_feat_mimic(&current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])].name);
			free_turn(p_ptr);
			oktomove = FALSE;
			disturb(FALSE, TRUE);
		}
		else if (have_flag(f_ptr->flags, FF_LAVA) && !(riding_r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK))
		{
			msg_format(_("%sの上に行けない。", "Too hot to go through."), f_name + f_info[get_feat_mimic(g_ptr)].name);
			free_turn(p_ptr);
			oktomove = FALSE;
			disturb(FALSE, TRUE);
		}

		if (oktomove && MON_STUNNED(riding_m_ptr) && one_in_(2))
		{
			GAME_TEXT steed_name[MAX_NLEN];
			monster_desc(steed_name, riding_m_ptr, 0);
			msg_format(_("%sが朦朧としていてうまく動けない！", "You cannot control stunned %s!"), steed_name);
			oktomove = FALSE;
			disturb(FALSE, TRUE);
		}
	}

	if (!oktomove)
	{
	}

	else if (!have_flag(f_ptr->flags, FF_MOVE) && have_flag(f_ptr->flags, FF_CAN_FLY) && !p_ptr->levitation)
	{
		msg_format(_("空を飛ばないと%sの上には行けない。", "You need to fly to go through the %s."), f_name + f_info[get_feat_mimic(g_ptr)].name);
		free_turn(p_ptr);
		running = 0;
		oktomove = FALSE;
	}

	/*
	 * Player can move through trees and
	 * has effective -10 speed
	 * Rangers can move without penality
	 */
	else if (have_flag(f_ptr->flags, FF_TREE) && !p_can_kill_walls)
	{
		if ((p_ptr->pclass != CLASS_RANGER) && !p_ptr->levitation && (!p_ptr->riding || !(riding_r_ptr->flags8 & RF8_WILD_WOOD))) p_ptr->energy_use *= 2;
	}


	/* Disarm a visible trap */
	else if ((do_pickup != easy_disarm) && have_flag(f_ptr->flags, FF_DISARM) && !g_ptr->mimic)
	{
		if (!trap_can_be_ignored(g_ptr->feat))
		{
			(void)do_cmd_disarm_aux(y, x, dir);
			return;
		}
	}


	/* Player can not walk through "walls" unless in wraith form...*/
	else if (!p_can_enter && !p_can_kill_walls)
	{
		/* Feature code (applying "mimic" field) */
		FEAT_IDX feat = get_feat_mimic(g_ptr);
		feature_type *mimic_f_ptr = &f_info[feat];
		concptr name = f_name + mimic_f_ptr->name;

		oktomove = FALSE;

		/* Notice things in the dark */
		if (!(g_ptr->info & CAVE_MARK) && !player_can_see_bold(y, x))
		{
			/* Boundary floor mimic */
			if (boundary_floor(g_ptr, f_ptr, mimic_f_ptr))
			{
				msg_print(_("それ以上先には進めないようだ。", "You feel you cannot go any more."));
			}

			/* Wall (or secret door) */
			else
			{
#ifdef JP
				msg_format("%sが行く手をはばんでいるようだ。", name);
#else
				msg_format("You feel %s %s blocking your way.",
					is_a_vowel(name[0]) ? "an" : "a", name);
#endif

				g_ptr->info |= (CAVE_MARK);
				lite_spot(y, x);
			}
		}

		/* Notice things */
		else
		{
			/* Boundary floor mimic */
			if (boundary_floor(g_ptr, f_ptr, mimic_f_ptr))
			{
				msg_print(_("それ以上先には進めない。", "You cannot go any more."));
				if (!(p_ptr->confused || p_ptr->stun || p_ptr->image))
					free_turn(p_ptr);
			}

			/* Wall (or secret door) */
			else
			{
				/* Closed doors */
				if (easy_open && is_closed_door(feat) && easy_open_door(y, x)) return;

#ifdef JP
				msg_format("%sが行く手をはばんでいる。", name);
#else
				msg_format("There is %s %s blocking your way.",
					is_a_vowel(name[0]) ? "an" : "a", name);
#endif

				/*
				 * Well, it makes sense that you lose time bumping into
				 * a wall _if_ you are confused, stunned or blind; but
				 * typing mistakes should not cost you a turn...
				 */
				if (!(p_ptr->confused || p_ptr->stun || p_ptr->image))
					free_turn(p_ptr);
			}
		}

		disturb(FALSE, TRUE);

		if (!boundary_floor(g_ptr, f_ptr, mimic_f_ptr)) sound(SOUND_HITWALL);
	}

	/* Normal movement */
	if (oktomove && !pattern_seq(p_ptr->y, p_ptr->x, y, x))
	{
		if (!(p_ptr->confused || p_ptr->stun || p_ptr->image))
		{
			free_turn(p_ptr);
		}

		/* To avoid a loop with running */
		disturb(FALSE, TRUE);

		oktomove = FALSE;
	}

	/* Normal movement */
	if (oktomove)
	{
		u32b mpe_mode = MPE_ENERGY_USE;

		if (p_ptr->warning)
		{
			if (!process_warning(x, y))
			{
				p_ptr->energy_use = 25;
				return;
			}
		}

		if (do_past)
		{
			msg_format(_("%sを押し退けた。", "You push past %s."), m_name);
		}

		/* Change oldpx and oldpy to place the player well when going back to big mode */
		if (p_ptr->wild_mode)
		{
			if (ddy[dir] > 0)  p_ptr->oldpy = 1;
			if (ddy[dir] < 0)  p_ptr->oldpy = MAX_HGT - 2;
			if (ddy[dir] == 0) p_ptr->oldpy = MAX_HGT / 2;
			if (ddx[dir] > 0)  p_ptr->oldpx = 1;
			if (ddx[dir] < 0)  p_ptr->oldpx = MAX_WID - 2;
			if (ddx[dir] == 0) p_ptr->oldpx = MAX_WID / 2;
		}

		if (p_can_kill_walls)
		{
			cave_alter_feat(y, x, FF_HURT_DISI);

			/* Update some things -- similar to GF_KILL_WALL */
			p_ptr->update |= (PU_FLOW);
		}

		/* sound(SOUND_WALK); */

		if (do_pickup != always_pickup) mpe_mode |= MPE_DO_PICKUP;
		if (break_trap) mpe_mode |= MPE_BREAK_TRAP;

		(void)move_player_effect(y, x, mpe_mode);
	}
}


static bool ignore_avoid_run;

/*!
 * @brief ダッシュ移動処理中、移動先のマスが既知の壁かどうかを判定する /
 * Hack -- Check for a "known wall" (see below)
 * @param dir 想定する移動方向ID
 * @param y 移動元のY座標
 * @param x 移動元のX座標
 * @return 移動先が既知の壁ならばTRUE
 */
static bool see_wall(DIRECTION dir, POSITION y, POSITION x)
{
	grid_type *g_ptr;

	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are not known walls */
	if (!in_bounds2(y, x)) return (FALSE);

	/* Access grid */
	g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Must be known to the player */
	if (g_ptr->info & (CAVE_MARK))
	{
		/* Feature code (applying "mimic" field) */
		s16b         feat = get_feat_mimic(g_ptr);
		feature_type *f_ptr = &f_info[feat];

		/* Wall grids are known walls */
		if (!player_can_enter(feat, 0)) return !have_flag(f_ptr->flags, FF_DOOR);

		/* Don't run on a tree unless explicitly requested */
		if (have_flag(f_ptr->flags, FF_AVOID_RUN) && !ignore_avoid_run)
			return TRUE;

		/* Don't run in a wall */
		if (!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY))
			return !have_flag(f_ptr->flags, FF_DOOR);
	}

	return FALSE;
}


/*!
 * @brief ダッシュ移動処理中、移動先のマスか未知の地形かどうかを判定する /
 * Hack -- Check for an "unknown corner" (see below)
 * @param dir 想定する移動方向ID
 * @param y 移動元のY座標
 * @param x 移動元のX座標
 * @return 移動先が未知の地形ならばTRUE
 */
static bool see_nothing(DIRECTION dir, POSITION y, POSITION x)
{
	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are unknown */
	if (!in_bounds2(y, x)) return (TRUE);

	/* Memorized grids are always known */
	if (current_floor_ptr->grid_array[y][x].info & (CAVE_MARK)) return (FALSE);

	/* Viewable door/wall grids are known */
	if (player_can_see_bold(y, x)) return (FALSE);

	/* Default */
	return (TRUE);
}


/*
 * Hack -- allow quick "cycling" through the legal directions
 */
static byte cycle[] = { 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/*
 * Hack -- map each direction into the "middle" of the "cycle[]" array
 */
static byte chome[] = { 0, 8, 9, 10, 7, 0, 11, 6, 5, 4 };

/*
 * The direction we are running
 */
static DIRECTION find_current;

/*
 * The direction we came from
 */
static DIRECTION find_prevdir;

/*
 * We are looking for open area
 */
static bool find_openarea;

/*
 * We are looking for a break
 */
static bool find_breakright;
static bool find_breakleft;

/*!
 * @brief ダッシュ処理の導入 /
 * Initialize the running algorithm for a new direction.
 * @param dir 導入の移動先
 * @details
 * Diagonal Corridor -- allow diaginal entry into corridors.\n
 *\n
 * Blunt Corridor -- If there is a wall two spaces ahead and\n
 * we seem to be in a corridor, then force a turn into the side\n
 * corridor, must be moving straight into a corridor here. ???\n
 *\n
 * Diagonal Corridor    Blunt Corridor (?)\n
 *       \# \#                  \#\n
 *       \#x\#                  \@x\#\n
 *       \@\@p.                  p\n
 */
static void run_init(DIRECTION dir)
{
	int row, col, deepleft, deepright;
	int i, shortleft, shortright;

	/* Save the direction */
	find_current = dir;

	/* Assume running straight */
	find_prevdir = dir;

	/* Assume looking for open area */
	find_openarea = TRUE;

	/* Assume not looking for breaks */
	find_breakright = find_breakleft = FALSE;

	/* Assume no nearby walls */
	deepleft = deepright = FALSE;
	shortright = shortleft = FALSE;

	p_ptr->run_py = p_ptr->y;
	p_ptr->run_px = p_ptr->x;

	/* Find the destination grid */
	row = p_ptr->y + ddy[dir];
	col = p_ptr->x + ddx[dir];

	ignore_avoid_run = cave_have_flag_bold(row, col, FF_AVOID_RUN);

	/* Extract cycle index */
	i = chome[dir];

	/* Check for walls */
	if (see_wall(cycle[i+1], p_ptr->y, p_ptr->x))
	{
		find_breakleft = TRUE;
		shortleft = TRUE;
	}
	else if (see_wall(cycle[i+1], row, col))
	{
		find_breakleft = TRUE;
		deepleft = TRUE;
	}

	/* Check for walls */
	if (see_wall(cycle[i-1], p_ptr->y, p_ptr->x))
	{
		find_breakright = TRUE;
		shortright = TRUE;
	}
	else if (see_wall(cycle[i-1], row, col))
	{
		find_breakright = TRUE;
		deepright = TRUE;
	}

	/* Looking for a break */
	if (find_breakleft && find_breakright)
	{
		/* Not looking for open area */
		find_openarea = FALSE;

		/* Hack -- allow angled corridor entry */
		if (dir & 0x01)
		{
			if (deepleft && !deepright)
			{
				find_prevdir = cycle[i - 1];
			}
			else if (deepright && !deepleft)
			{
				find_prevdir = cycle[i + 1];
			}
		}

		/* Hack -- allow blunt corridor entry */
		else if (see_wall(cycle[i], row, col))
		{
			if (shortleft && !shortright)
			{
				find_prevdir = cycle[i - 2];
			}
			else if (shortright && !shortleft)
			{
				find_prevdir = cycle[i + 2];
			}
		}
	}
}


/*!
 * @brief ダッシュ移動が継続できるかどうかの判定 /
 * Update the current "run" path
 * @return
 * ダッシュ移動が継続できるならばTRUEを返す。
 * Return TRUE if the running should be stopped
 */
static bool run_test(void)
{
	DIRECTION prev_dir, new_dir, check_dir = 0;
	int row, col;
	int i, max, inv;
	int option = 0, option2 = 0;
	grid_type *g_ptr;
	FEAT_IDX feat;
	feature_type *f_ptr;

	/* Where we came from */
	prev_dir = find_prevdir;


	/* Range of newly adjacent grids */
	max = (prev_dir & 0x01) + 1;

	/* break run when leaving trap detected region */
	if ((disturb_trap_detect || alert_trap_detect)
	    && p_ptr->dtrap && !(current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info & CAVE_IN_DETECT))
	{
		/* No duplicate warning */
		p_ptr->dtrap = FALSE;

		/* You are just on the edge */
		if (!(current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info & CAVE_UNSAFE))
		{
			if (alert_trap_detect)
			{
				msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
			}

			if (disturb_trap_detect)
			{
				/* Break Run */
				return(TRUE);
			}
		}
	}

	/* Look at every newly adjacent square. */
	for (i = -max; i <= max; i++)
	{
		OBJECT_IDX this_o_idx, next_o_idx = 0;

		/* New direction */
		new_dir = cycle[chome[prev_dir] + i];

		/* New location */
		row = p_ptr->y + ddy[new_dir];
		col = p_ptr->x + ddx[new_dir];

		/* Access grid */
		g_ptr = &current_floor_ptr->grid_array[row][col];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);
		f_ptr = &f_info[feat];

		/* Visible monsters abort running */
		if (g_ptr->m_idx)
		{
			monster_type *m_ptr = &current_floor_ptr->m_list[g_ptr->m_idx];

			/* Visible monster */
			if (m_ptr->ml) return (TRUE);
		}

		/* Visible objects abort running */
		for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;
			o_ptr = &current_floor_ptr->o_list[this_o_idx];
			next_o_idx = o_ptr->next_o_idx;

			/* Visible object */
			if (o_ptr->marked & OM_FOUND) return (TRUE);
		}

		/* Assume unknown */
		inv = TRUE;

		/* Check memorized grids */
		if (g_ptr->info & (CAVE_MARK))
		{
			bool notice = have_flag(f_ptr->flags, FF_NOTICE);

			if (notice && have_flag(f_ptr->flags, FF_MOVE))
			{
				/* Open doors */
				if (find_ignore_doors && have_flag(f_ptr->flags, FF_DOOR) && have_flag(f_ptr->flags, FF_CLOSE))
				{
					/* Option -- ignore */
					notice = FALSE;
				}

				/* Stairs */
				else if (find_ignore_stairs && have_flag(f_ptr->flags, FF_STAIRS))
				{
					/* Option -- ignore */
					notice = FALSE;
				}

				/* Lava */
				else if (have_flag(f_ptr->flags, FF_LAVA) && (p_ptr->immune_fire || IS_INVULN()))
				{
					/* Ignore */
					notice = FALSE;
				}

				/* Deep water */
				else if (have_flag(f_ptr->flags, FF_WATER) && have_flag(f_ptr->flags, FF_DEEP) &&
				         (p_ptr->levitation || p_ptr->can_swim || (p_ptr->total_weight <= weight_limit())))
				{
					/* Ignore */
					notice = FALSE;
				}
			}

			/* Interesting feature */
			if (notice) return (TRUE);

			/* The grid is "visible" */
			inv = FALSE;
		}

		/* Analyze unknown grids and floors considering mimic */
		if (inv || !see_wall(0, row, col))
		{
			/* Looking for open area */
			if (find_openarea)
			{
				/* Nothing */
			}

			/* The first new direction. */
			else if (!option)
			{
				option = new_dir;
			}

			/* Three new directions. Stop running. */
			else if (option2)
			{
				return (TRUE);
			}

			/* Two non-adjacent new directions.  Stop running. */
			else if (option != cycle[chome[prev_dir] + i - 1])
			{
				return (TRUE);
			}

			/* Two new (adjacent) directions (case 1) */
			else if (new_dir & 0x01)
			{
				check_dir = cycle[chome[prev_dir] + i - 2];
				option2 = new_dir;
			}

			/* Two new (adjacent) directions (case 2) */
			else
			{
				check_dir = cycle[chome[prev_dir] + i + 1];
				option2 = option;
				option = new_dir;
			}
		}

		/* Obstacle, while looking for open area */
		else
		{
			if (find_openarea)
			{
				if (i < 0)
				{
					/* Break to the right */
					find_breakright = TRUE;
				}

				else if (i > 0)
				{
					/* Break to the left */
					find_breakleft = TRUE;
				}
			}
		}
	}

	/* Looking for open area */
	if (find_openarea)
	{
		/* Hack -- look again */
		for (i = -max; i < 0; i++)
		{
			/* Unknown grid or non-wall */
			if (!see_wall(cycle[chome[prev_dir] + i], p_ptr->y, p_ptr->x))
			{
				/* Looking to break right */
				if (find_breakright)
				{
					return (TRUE);
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break left */
				if (find_breakleft)
				{
					return (TRUE);
				}
			}
		}

		/* Hack -- look again */
		for (i = max; i > 0; i--)
		{
			/* Unknown grid or non-wall */
			if (!see_wall(cycle[chome[prev_dir] + i], p_ptr->y, p_ptr->x))
			{
				/* Looking to break left */
				if (find_breakleft)
				{
					return (TRUE);
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break right */
				if (find_breakright)
				{
					return (TRUE);
				}
			}
		}
	}

	/* Not looking for open area */
	else
	{
		/* No options */
		if (!option)
		{
			return (TRUE);
		}

		/* One option */
		else if (!option2)
		{
			/* Primary option */
			find_current = option;

			/* No other options */
			find_prevdir = option;
		}

		/* Two options, examining corners */
		else if (!find_cut)
		{
			/* Primary option */
			find_current = option;

			/* Hack -- allow curving */
			find_prevdir = option2;
		}

		/* Two options, pick one */
		else
		{
			/* Get next location */
			row = p_ptr->y + ddy[option];
			col = p_ptr->x + ddx[option];

			/* Don't see that it is closed off. */
			/* This could be a potential corner or an intersection. */
			if (!see_wall(option, row, col) ||
			    !see_wall(check_dir, row, col))
			{
				/* Can not see anything ahead and in the direction we */
				/* are turning, assume that it is a potential corner. */
				if (see_nothing(option, row, col) &&
				    see_nothing(option2, row, col))
				{
					find_current = option;
					find_prevdir = option2;
				}

				/* STOP: we are next to an intersection or a room */
				else
				{
					return (TRUE);
				}
			}

			/* This corner is seen to be enclosed; we cut the corner. */
			else if (find_cut)
			{
				find_current = option2;
				find_prevdir = option2;
			}

			/* This corner is seen to be enclosed, and we */
			/* deliberately go the long way. */
			else
			{
				find_current = option;
				find_prevdir = option2;
			}
		}
	}

	/* About to hit a known wall, stop */
	if (see_wall(find_current, p_ptr->y, p_ptr->x))
	{
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}



/*!
 * @brief 継続的なダッシュ処理 /
 * Take one step along the current "run" path
 * @param dir 移動を試みる方向ID
 * @return なし
 */
void run_step(DIRECTION dir)
{
	/* Start running */
	if (dir)
	{
		/* Ignore AVOID_RUN on a first step */
		ignore_avoid_run = TRUE;

		/* Hack -- do not start silly run */
		if (see_wall(dir, p_ptr->y, p_ptr->x))
		{
			sound(SOUND_HITWALL);

			msg_print(_("その方向には走れません。", "You cannot run in that direction."));

			disturb(FALSE, FALSE);

			return;
		}

		run_init(dir);
	}

	/* Keep running */
	else
	{
		/* Update run */
		if (run_test())
		{
			disturb(FALSE, FALSE);

			return;
		}
	}

	/* Decrease the run counter */
	if (--running <= 0) return;

	/* Take time */
	take_turn(p_ptr, 100);

	/* Move the player, using the "pickup" flag */
	move_player(find_current, FALSE, FALSE);

	if (player_bold(p_ptr->run_py, p_ptr->run_px))
	{
		p_ptr->run_py = 0;
		p_ptr->run_px = 0;
		disturb(FALSE, FALSE);
	}
}


#ifdef TRAVEL

/*!
 * @brief トラベル機能の判定処理 /
 * Test for traveling
 * @param prev_dir 前回移動を行った元の方角ID
 * @return 次の方向
 */
static DIRECTION travel_test(DIRECTION prev_dir)
{
	DIRECTION new_dir = 0;
	int i, max;
	const grid_type *g_ptr;
	int cost;

	/* Cannot travel when blind */
	if (p_ptr->blind || no_lite())
	{
		msg_print(_("目が見えない！", "You cannot see!"));
		return (0);
	}

	/* break run when leaving trap detected region */
	if ((disturb_trap_detect || alert_trap_detect)
	    && p_ptr->dtrap && !(current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info & CAVE_IN_DETECT))
	{
		/* No duplicate warning */
		p_ptr->dtrap = FALSE;

		/* You are just on the edge */
		if (!(current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info & CAVE_UNSAFE))
		{
			if (alert_trap_detect)
			{
				msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
			}

			if (disturb_trap_detect)
			{
				/* Break Run */
				return (0);
			}
		}
	}

	/* Range of newly adjacent grids */
	max = (prev_dir & 0x01) + 1;

	/* Look at every newly adjacent square. */
	for (i = -max; i <= max; i++)
	{
		/* New direction */
		DIRECTION dir = cycle[chome[prev_dir] + i];

		/* New location */
		POSITION row = p_ptr->y + ddy[dir];
		POSITION col = p_ptr->x + ddx[dir];

		/* Access grid */
		g_ptr = &current_floor_ptr->grid_array[row][col];

		/* Visible monsters abort running */
		if (g_ptr->m_idx)
		{
			monster_type *m_ptr = &current_floor_ptr->m_list[g_ptr->m_idx];

			/* Visible monster */
			if (m_ptr->ml) return (0);
		}

	}

	/* Travel cost of current grid */
	cost = travel.cost[p_ptr->y][p_ptr->x];

	/* Determine travel direction */
	for (i = 0; i < 8; ++ i) {
		int dir_cost = travel.cost[p_ptr->y+ddy_ddd[i]][p_ptr->x+ddx_ddd[i]];

		if (dir_cost < cost)
		{
			new_dir = ddd[i];
			cost = dir_cost;
		}
	}

	if (!new_dir) return (0);

	/* Access newly move grid */
	g_ptr = &current_floor_ptr->grid_array[p_ptr->y+ddy[new_dir]][p_ptr->x+ddx[new_dir]];

	/* Close door abort traveling */
	if (!easy_open && is_closed_door(g_ptr->feat)) return (0);

	/* Visible and unignorable trap abort tarveling */
	if (!g_ptr->mimic && !trap_can_be_ignored(g_ptr->feat)) return (0);

	/* Move new grid */
	return (new_dir);
}


/*!
 * @brief トラベル機能の実装 /
 * Travel command
 * @return なし
 */
void travel_step(void)
{
	/* Get travel direction */
	travel.dir = travel_test(travel.dir);

	if (!travel.dir)
	{
		if (travel.run == 255)
		{
			msg_print(_("道筋が見つかりません！", "No route is found!"));
			travel.y = travel.x = 0;
		}
		disturb(FALSE, TRUE);
		return;
	}

	take_turn(p_ptr, 100);

	move_player(travel.dir, always_pickup, FALSE);

	if ((p_ptr->y == travel.y) && (p_ptr->x == travel.x))
	{
		travel.run = 0;
		travel.y = travel.x = 0;
	}
	else if (travel.run > 0)
		travel.run--;

	/* Travel Delay */
	Term_xtra(TERM_XTRA_DELAY, delay_factor);
}
#endif


#ifdef TRAVEL
/*
 * Hack: travel command
 */
#define TRAVEL_UNABLE 9999

static int flow_head = 0;
static int flow_tail = 0;
static POSITION temp2_x[MAX_SHORT];
static POSITION temp2_y[MAX_SHORT];

/*!
 * @brief トラベル処理の記憶配列を初期化する Hack: forget the "flow" information
 * @return なし
 */
void forget_travel_flow(void)
{
	POSITION x, y;
	/* Check the entire dungeon / Forget the old data */
	for (y = 0; y < current_floor_ptr->height; y++)
	{
		for (x = 0; x < current_floor_ptr->width; x++)
		{

			travel.cost[y][x] = MAX_SHORT;
		}
	}
	travel.y = travel.x = 0;
}

/*!
 * @brief トラベル処理中に地形に応じた移動コスト基準を返す
 * @param y 該当地点のY座標
 * @param x 該当地点のX座標
 * @return コスト値
 */
static int travel_flow_cost(POSITION y, POSITION x)
{
	feature_type *f_ptr = &f_info[current_floor_ptr->grid_array[y][x].feat];
	int cost = 1;

	/* Avoid obstacles (ex. trees) */
	if (have_flag(f_ptr->flags, FF_AVOID_RUN)) cost += 1;

	/* Water */
	if (have_flag(f_ptr->flags, FF_WATER))
	{
		if (have_flag(f_ptr->flags, FF_DEEP) && !p_ptr->levitation) cost += 5;
	}

	/* Lava */
	if (have_flag(f_ptr->flags, FF_LAVA))
	{
		int lava = 2;
		if (!p_ptr->resist_fire) lava *= 2;
		if (!p_ptr->levitation) lava *= 2;
		if (have_flag(f_ptr->flags, FF_DEEP)) lava *= 2;

		cost += lava;
	}

	/* Detected traps and doors */
	if (current_floor_ptr->grid_array[y][x].info & (CAVE_MARK))
	{
		if (have_flag(f_ptr->flags, FF_DOOR)) cost += 1;
		if (have_flag(f_ptr->flags, FF_TRAP)) cost += 10;
	}

	return (cost);
}

/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のサブルーチン
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 * @param n 現在のコスト
 * @param wall プレイヤーが壁の中にいるならばTRUE
 * @return なし
 */
static void travel_flow_aux(POSITION y, POSITION x, int n, bool wall)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[g_ptr->feat];
	int old_head = flow_head;
	int add_cost = 1;
	int base_cost = (n % TRAVEL_UNABLE);
	int from_wall = (n / TRAVEL_UNABLE);
	int cost;

	/* Ignore out of bounds */
	if (!in_bounds(y, x)) return;

	/* Ignore unknown grid except in wilderness */
	if (current_floor_ptr->dun_level > 0 && !(g_ptr->info & CAVE_KNOWN)) return;

	/* Ignore "walls" and "rubble" (include "secret doors") */
	if (have_flag(f_ptr->flags, FF_WALL) ||
		have_flag(f_ptr->flags, FF_CAN_DIG) ||
		(have_flag(f_ptr->flags, FF_DOOR) && current_floor_ptr->grid_array[y][x].mimic) ||
		(!have_flag(f_ptr->flags, FF_MOVE) && have_flag(f_ptr->flags, FF_CAN_FLY) && !p_ptr->levitation))
	{
		if (!wall || !from_wall) return;
		add_cost += TRAVEL_UNABLE;
	}
	else
	{
		add_cost = travel_flow_cost(y, x);
	}

	cost = base_cost + add_cost;

	/* Ignore lower cost entries */
	if (travel.cost[y][x] <= cost) return;

	/* Save the flow cost */
	travel.cost[y][x] = cost;

	/* Enqueue that entry */
	temp2_y[flow_head] = y;
	temp2_x[flow_head] = x;

	/* Advance the queue */
	if (++flow_head == MAX_SHORT) flow_head = 0;

	/* Hack -- notice overflow by forgetting new entry */
	if (flow_head == flow_tail) flow_head = old_head;

	return;
}

/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のメインルーチン
 * @param ty 目標地点のY座標
 * @param tx 目標地点のX座標
 * @return なし
 */
static void travel_flow(POSITION ty, POSITION tx)
{
	POSITION x, y;
	DIRECTION d;
	bool wall = FALSE;
	feature_type *f_ptr = &f_info[current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].feat];

	/* Reset the "queue" */
	flow_head = flow_tail = 0;

	/* is player in the wall? */
	if (!have_flag(f_ptr->flags, FF_MOVE)) wall = TRUE;

	/* Start at the target grid */
	travel_flow_aux(ty, tx, 0, wall);

	/* Now process the queue */
	while (flow_head != flow_tail)
	{
		/* Extract the next entry */
		y = temp2_y[flow_tail];
		x = temp2_x[flow_tail];

		/* Forget that entry */
		if (++flow_tail == MAX_SHORT) flow_tail = 0;

		/* Ignore too far entries */
		//if (distance(ty, tx, y, x) > 100) continue;

		/* Add the "children" */
		for (d = 0; d < 8; d++)
		{
			/* Add that child if "legal" */
			travel_flow_aux(y + ddy_ddd[d], x + ddx_ddd[d], travel.cost[y][x], wall);
		}
	}

	/* Forget the flow info */
	flow_head = flow_tail = 0;
}

/*!
 * @brief トラベル処理のメインルーチン
 * @return なし
 */
void do_cmd_travel(void)
{
	POSITION x, y;
	int i;
	POSITION dx, dy, sx, sy;
	feature_type *f_ptr;

	if (travel.x != 0 && travel.y != 0 &&
		get_check(_("トラベルを継続しますか？", "Do you continue to travel? ")))
	{
		y = travel.y;
		x = travel.x;
	}
	else if (!tgt_pt(&x, &y)) return;

	if ((x == p_ptr->x) && (y == p_ptr->y))
	{
		msg_print(_("すでにそこにいます！", "You are already there!!"));
		return;
	}

	f_ptr = &f_info[current_floor_ptr->grid_array[y][x].feat];

	if ((current_floor_ptr->grid_array[y][x].info & CAVE_MARK) &&
		(have_flag(f_ptr->flags, FF_WALL) ||
			have_flag(f_ptr->flags, FF_CAN_DIG) ||
			(have_flag(f_ptr->flags, FF_DOOR) && current_floor_ptr->grid_array[y][x].mimic)))
	{
		msg_print(_("そこには行くことができません！", "You cannot travel there!"));
		return;
	}

	forget_travel_flow();
	travel_flow(y, x);

	travel.x = x;
	travel.y = y;

	/* Travel till 255 steps */
	travel.run = 255;
	travel.dir = 0;

	/* Decides first direction */
	dx = abs(p_ptr->x - x);
	dy = abs(p_ptr->y - y);
	sx = ((x == p_ptr->x) || (dx < dy)) ? 0 : ((x > p_ptr->x) ? 1 : -1);
	sy = ((y == p_ptr->y) || (dy < dx)) ? 0 : ((y > p_ptr->y) ? 1 : -1);

	for (i = 1; i <= 9; i++)
	{
		if ((sx == ddx[i]) && (sy == ddy[i])) travel.dir = i;
	}
}
#endif

/*
 * Something has happened to disturb the player.
 * The first arg indicates a major disturbance, which affects search.
 * The second arg is currently unused, but could induce output flush.
 * All disturbance cancels repeated commands, resting, and running.
 */
void disturb(bool stop_search, bool stop_travel)
{
#ifndef TRAVEL
	/* Unused */
	stop_travel = stop_travel;
#endif

	/* Cancel auto-commands */
	/* command_new = 0; */

	/* Cancel repeated commands */
	if (command_rep)
	{
		/* Cancel */
		command_rep = 0;

		/* Redraw the state (later) */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Cancel Resting */
	if ((p_ptr->action == ACTION_REST) || (p_ptr->action == ACTION_FISH) || (stop_search && (p_ptr->action == ACTION_SEARCH)))
	{
		/* Cancel */
		set_action(ACTION_NONE);
	}

	/* Cancel running */
	if (running)
	{
		/* Cancel */
		running = 0;

		/* Check for new panel if appropriate */
		if (center_player && !center_running) verify_panel();

		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);

		/* Update monster flow */
		p_ptr->update |= (PU_FLOW);
	}

#ifdef TRAVEL
	if (stop_travel)
	{
		/* Cancel */
		travel.run = 0;

		/* Check for new panel if appropriate */
		if (center_player && !center_running) verify_panel();

		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);
	}
#endif

	/* Flush the input if requested */
	if (flush_disturb) flush();
}
