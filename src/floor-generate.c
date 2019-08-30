﻿/*!
 * @file generate.c
 * @brief ダンジョンの生成 / Dungeon generation
 * @date 2014/01/04
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen. \n
 * @details
 * Note that Level generation is *not* an important bottleneck,\n
 * though it can be annoyingly slow on older machines...  Thus\n
 * we emphasize "simplicity" and "correctness" over "speed".\n
 *\n
 * This entire file is only needed for generating levels.\n
 * This may allow smart compilers to only load it when needed.\n
 *\n
 * Consider the "v_info.txt" file for vault generation.\n
 *\n
 * In this file, we use the "special" granite and perma-wall sub-types,\n
 * where "basic" is normal, "inner" is inside a room, "outer" is the\n
 * outer wall of a room, and "solid" is the outer wall of the dungeon\n
 * or any walls that may not be pierced by corridors.  Thus the only\n
 * wall type that may be pierced by a corridor is the "outer granite"\n
 * type.  The "basic granite" type yields the "actual" corridors.\n
 *\n
 * Note that we use the special "solid" granite wall type to prevent\n
 * multiple corridors from piercing a wall in two adjacent locations,\n
 * which would be messy, and we use the special "outer" granite wall\n
 * to indicate which walls "surround" rooms, and may thus be "pierced"\n
 * by corridors entering or leaving the room.\n
 *\n
 * Note that a tunnel which attempts to leave a room near the "edge"\n
 * of the dungeon in a direction toward that edge will cause "silly"\n
 * wall piercings, but will have no permanently incorrect effects,\n
 * as long as the tunnel can *eventually* exit from another side.\n
 * And note that the wall may not come back into the room by the\n
 * hole it left through, so it must bend to the left or right and\n
 * then optionally re-enter the room (at least 2 grids away).  This\n
 * is not a problem since every room that is large enough to block\n
 * the passage of tunnels is also large enough to allow the tunnel\n
 * to pierce the room itself several times.\n
 *\n
 * Note that no two corridors may enter a room through adjacent grids,\n
 * they must either share an entryway or else use entryways at least\n
 * two grids apart.  This prevents "large" (or "silly") doorways.\n
 *\n
 * To create rooms in the dungeon, we first divide the dungeon up\n
 * into "blocks" of 11x11 grids each, and require that all rooms\n
 * occupy a rectangular group of blocks.  As long as each room type\n
 * reserves a sufficient number of blocks, the room building routines\n
 * will not need to check bounds.  Note that most of the normal rooms\n
 * actually only use 23x11 grids, and so reserve 33x11 grids.\n
 *\n
 * Note that the use of 11x11 blocks (instead of the old 33x11 blocks)\n
 * allows more variability in the horizontal placement of rooms, and\n
 * at the same time has the disadvantage that some rooms (two thirds\n
 * of the normal rooms) may be "split" by panel boundaries.  This can\n
 * induce a situation where a player is in a room and part of the room\n
 * is off the screen.  It may be annoying enough to go back to 33x11\n
 * blocks to prevent this visual situation.\n
 *\n
 * Note that the dungeon generation routines are much different (2.7.5)\n
 * and perhaps "DUN_ROOMS" should be less than 50.\n
 *\n
 * Note that it is possible to create a room which is only\n
 * connected to itself, because the "tunnel generation" code allows a\n
 * tunnel to leave a room, wander around, and then re-enter the room.\n
 *\n
 * Note that it is possible to create a set of rooms which\n
 * are only connected to other rooms in that set, since there is nothing\n
 * explicit in the code to prevent this from happening.  But this is less\n
 * likely than the "isolated room" problem, because each room attempts to\n
 * connect to another room, in a giant cycle, thus requiring at least two\n
 * bizarre occurances to create an isolated section of the dungeon.\n
 *\n
 * Note that (2.7.9) monster pits have been split into monster "nests"\n
 * and monster "pits".  The "nests" have a collection of monsters of a\n
 * given type strewn randomly around the room (jelly, animal, or undead),\n
 * while the "pits" have a collection of monsters of a given type placed\n
 * around the room in an organized manner (orc, troll, giant, dragon, or\n
 * demon).  Note that both "nests" and "pits" are now "level dependant",\n
 * and both make 16 "expensive" calls to the "get_mon_num()" function.\n
 *\n
 * Note that the grid flags changed in a rather drastic manner\n
 * for Angband 2.8.0 (and 2.7.9+), in particular, dungeon terrain\n
 * features, such as doors and stairs and traps and rubble and walls,\n
 * are all handled as a set of 64 possible "terrain features", and\n
 * not as "fake" objects (440-479) as in pre-2.8.0 versions.\n
 *\n
 * The 64 new "dungeon features" will also be used for "visual display"\n
 * but we must be careful not to allow, for example, the user to display\n
 * hidden traps in a different way from floors, or secret doors in a way\n
 * different from granite walls, or even permanent granite in a different\n
 * way from granite.  \n
 */

#include "angband.h"
#include "generate.h"
#include "grid.h"
#include "rooms.h"
#include "floor-streams.h"
#include "floor-generate.h"
#include "trap.h"
#include "monster.h"
#include "quest.h"
#include "player-status.h"
#include "wild.h"
#include "monster-status.h"

int dun_tun_rnd; 
int dun_tun_chg;
int dun_tun_con;
int dun_tun_pen;
int dun_tun_jct;


/*!
 * Dungeon generation data -- see "cave_gen()"
 */
dun_data *dun;


/*!
 * @brief 上下左右の外壁数をカウントする / Count the number of walls adjacent to the given grid.
 * @param y 基準のy座標
 * @param x 基準のx座標
 * @return 隣接する外壁の数
 * @note Assumes "in_bounds(y, x)"
 * @details We count only granite walls and permanent walls.
 */
static int next_to_walls(POSITION y, POSITION x)
{
	int k = 0;

	if (in_bounds(y + 1, x) && is_extra_bold(y + 1, x)) k++;
	if (in_bounds(y - 1, x) && is_extra_bold(y - 1, x)) k++;
	if (in_bounds(y, x + 1) && is_extra_bold(y, x + 1)) k++;
	if (in_bounds(y, x - 1) && is_extra_bold(y, x - 1)) k++;

	return (k);
}

/*!
 * @brief alloc_stairs()の補助として指定の位置に階段を生成できるかの判定を行う / Helper function for alloc_stairs(). Is this a good location for stairs?
 * @param y 基準のy座標
 * @param x 基準のx座標
 * @param walls 最低減隣接させたい外壁の数
 * @return 階段を生成して問題がないならばTRUEを返す。
 */
static bool alloc_stairs_aux(POSITION y, POSITION x, int walls)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Require "naked" floor grid */
	if (!is_floor_grid(g_ptr)) return FALSE;
	if (pattern_tile(y, x)) return FALSE;
	if (g_ptr->o_idx || g_ptr->m_idx) return FALSE;

	/* Require a certain number of adjacent walls */
	if (next_to_walls(y, x) < walls) return FALSE;

	return TRUE;
}


/*!
 * @brief 外壁に隣接させて階段を生成する / Places some staircases near walls
 * @param feat 配置したい地形ID
 * @param num 配置したい階段の数
 * @param walls 最低減隣接させたい外壁の数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
static bool alloc_stairs(IDX feat, int num, int walls)
{
	int i;
	int shaft_num = 0;

	feature_type *f_ptr = &f_info[feat];

	if (have_flag(f_ptr->flags, FF_LESS))
	{
		/* No up stairs in town or in ironman mode */
		if (ironman_downward || !current_floor_ptr->dun_level) return TRUE;

		if (current_floor_ptr->dun_level > d_info[p_ptr->dungeon_idx].mindepth)
			shaft_num = (randint1(num+1))/2;
	}
	else if (have_flag(f_ptr->flags, FF_MORE))
	{
		QUEST_IDX q_idx = quest_number(current_floor_ptr->dun_level);

		/* No downstairs on quest levels */
		if (current_floor_ptr->dun_level > 1 && q_idx)
		{
			monster_race *r_ptr = &r_info[quest[q_idx].r_idx];

			/* The quest monster(s) is still alive? */
			if (!(r_ptr->flags1 & RF1_UNIQUE) || 0 < r_ptr->max_num)
				return TRUE;
		}

		/* No downstairs at the bottom */
		if (current_floor_ptr->dun_level >= d_info[p_ptr->dungeon_idx].maxdepth) return TRUE;

		if ((current_floor_ptr->dun_level < d_info[p_ptr->dungeon_idx].maxdepth-1) && !quest_number(current_floor_ptr->dun_level+1))
			shaft_num = (randint1(num)+1)/2;
	}
	else return FALSE;


	/* Place "num" stairs */
	for (i = 0; i < num; i++)
	{
		while (TRUE)
		{
			POSITION y, x = 0;
			grid_type *g_ptr;

			int candidates = 0;
			int pick;

			for (y = 1; y < current_floor_ptr->height - 1; y++)
			{
				for (x = 1; x < current_floor_ptr->width - 1; x++)
				{
					if (alloc_stairs_aux(y, x, walls))
					{
						/* A valid space found */
						candidates++;
					}
				}
			}

			/* No valid place! */
			if (!candidates)
			{
				/* There are exactly no place! */
				if (walls <= 0) return FALSE;

				/* Decrease walls limit, and try again */
				walls--;
				continue;
			}

			/* Choose a random one */
			pick = randint1(candidates);

			for (y = 1; y < current_floor_ptr->height - 1; y++)
			{
				for (x = 1; x < current_floor_ptr->width - 1; x++)
				{
					if (alloc_stairs_aux(y, x, walls))
					{
						pick--;

						/* Is this a picked one? */
						if (!pick) break;
					}
				}

				if (!pick) break;
			}
			g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Clear possible garbage of hidden trap */
			g_ptr->mimic = 0;

			/* Clear previous contents, add stairs */
			g_ptr->feat = (i < shaft_num) ? feat_state(feat, FF_SHAFT) : feat;

			/* No longer "FLOOR" */
			g_ptr->info &= ~(CAVE_FLOOR);

			/* Success */
			break;
		}
	}
	return TRUE;
}

/*!
 * @brief フロア上のランダム位置に各種オブジェクトを配置する / Allocates some objects (using "place" and "type")
 * @param set 配置したい地形の種類
 * @param typ 配置したいオブジェクトの種類
 * @param num 配置したい数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
static void alloc_object(int set, EFFECT_ID typ, int num)
{
	POSITION y = 0, x = 0;
	int k;
	int dummy = 0;
	grid_type *g_ptr;

	/* A small level has few objects. */
	num = num * current_floor_ptr->height * current_floor_ptr->width / (MAX_HGT*MAX_WID) +1;

	/* Place some objects */
	for (k = 0; k < num; k++)
	{
		/* Pick a "legal" spot */
		while (dummy < SAFE_MAX_ATTEMPTS)
		{
			bool room;

			dummy++;

			y = randint0(current_floor_ptr->height);
			x = randint0(current_floor_ptr->width);

			g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Require "naked" floor grid */
			if (!is_floor_grid(g_ptr) || g_ptr->o_idx || g_ptr->m_idx) continue;

			/* Avoid player location */
			if (player_bold(y, x)) continue;

			/* Check for "room" */
			room = (current_floor_ptr->grid_array[y][x].info & CAVE_ROOM) ? TRUE : FALSE;

			/* Require corridor? */
			if ((set == ALLOC_SET_CORR) && room) continue;

			/* Require room? */
			if ((set == ALLOC_SET_ROOM) && !room) continue;

			/* Accept it */
			break;
		}

		if (dummy >= SAFE_MAX_ATTEMPTS)
		{
			msg_print_wizard(CHEAT_DUNGEON, _("アイテムの配置に失敗しました。", "Failed to place object."));
			return;
		}


		/* Place something */
		switch (typ)
		{
			case ALLOC_TYP_RUBBLE:
			{
				place_rubble(y, x);
				current_floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
				break;
			}

			case ALLOC_TYP_TRAP:
			{
				place_trap(y, x);
				current_floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
				break;
			}

			case ALLOC_TYP_GOLD:
			{
				place_gold(y, x);
				break;
			}

			case ALLOC_TYP_OBJECT:
			{
				place_object(y, x, 0L);
				break;
			}
		}
	}
}




/*!
 * @brief クエストに関わるモンスターの配置を行う / Place quest monsters
 * @return 成功したならばTRUEを返す
 */
bool place_quest_monsters(void)
{
	int i;

	/* Handle the quest monster placements */
	for (i = 0; i < max_q_idx; i++)
	{
		monster_race *r_ptr;
		BIT_FLAGS mode;
		int j;

		if (quest[i].status != QUEST_STATUS_TAKEN ||
		    (quest[i].type != QUEST_TYPE_KILL_LEVEL &&
		     quest[i].type != QUEST_TYPE_RANDOM) ||
		    quest[i].level != current_floor_ptr->dun_level ||
		    p_ptr->dungeon_idx != quest[i].dungeon ||
		    (quest[i].flags & QUEST_FLAG_PRESET))
		{
			/* Ignore it */
			continue;
		}

		r_ptr = &r_info[quest[i].r_idx];

		/* Hack -- "unique" monsters must be "unique" */
		if ((r_ptr->flags1 & RF1_UNIQUE) &&
		    (r_ptr->cur_num >= r_ptr->max_num)) continue;

		mode = (PM_NO_KAGE | PM_NO_PET);

		if (!(r_ptr->flags1 & RF1_FRIENDS))
			mode |= PM_ALLOW_GROUP;

		for (j = 0; j < (quest[i].max_num - quest[i].cur_num); j++)
		{
			int k;

			for (k = 0; k < SAFE_MAX_ATTEMPTS; k++)
			{
				POSITION x = 0, y = 0;
				int l;

				/* Find an empty grid */
				for (l = SAFE_MAX_ATTEMPTS; l > 0; l--)
				{
					grid_type    *g_ptr;
					feature_type *f_ptr;

					y = randint0(current_floor_ptr->height);
					x = randint0(current_floor_ptr->width);

					g_ptr = &current_floor_ptr->grid_array[y][x];
					f_ptr = &f_info[g_ptr->feat];

					if (!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY)) continue;
					if (!monster_can_enter(y, x, r_ptr, 0)) continue;
					if (distance(y, x, p_ptr->y, p_ptr->x) < 10) continue;
					if (g_ptr->info & CAVE_ICKY) continue;
					else break;
				}

				/* Failed to place */
				if (!l) return FALSE;

				/* Try to place the monster */
				if (place_monster_aux(0, y, x, quest[i].r_idx, mode))
				{
					/* Success */
					break;
				}
				else
				{
					/* Failure - Try again */
					continue;
				}
			}

			/* Failed to place */
			if (k == SAFE_MAX_ATTEMPTS) return FALSE;
		}
	}

	return TRUE;
}

/*!
 * @brief フロアに洞窟や湖を配置する / Generate various caverns and lakes
 * @details There were moved from cave_gen().
 * @return なし
 */
static void gen_caverns_and_lakes(void)
{
#ifdef ALLOW_CAVERNS_AND_LAKES
	/* Possible "destroyed" level */
	if ((current_floor_ptr->dun_level > 30) && one_in_(DUN_DEST*2) && (small_levels) && (d_info[p_ptr->dungeon_idx].flags1 & DF1_DESTROY))
	{
		dun->destroyed = TRUE;

		/* extra rubble around the place looks cool */
		build_lake(one_in_(2) ? LAKE_T_CAVE : LAKE_T_EARTH_VAULT);
	}

	/* Make a lake some of the time */
	if (one_in_(LAKE_LEVEL) && !dun->empty_level && !dun->destroyed &&
	    (d_info[p_ptr->dungeon_idx].flags1 & DF1_LAKE_MASK))
	{
		int count = 0;
		if (d_info[p_ptr->dungeon_idx].flags1 & DF1_LAKE_WATER) count += 3;
		if (d_info[p_ptr->dungeon_idx].flags1 & DF1_LAKE_LAVA) count += 3;
		if (d_info[p_ptr->dungeon_idx].flags1 & DF1_LAKE_RUBBLE) count += 3;
		if (d_info[p_ptr->dungeon_idx].flags1 & DF1_LAKE_TREE) count += 3;

		if (d_info[p_ptr->dungeon_idx].flags1 & DF1_LAKE_LAVA)
		{
			/* Lake of Lava */
			if ((current_floor_ptr->dun_level > 80) && (randint0(count) < 2)) dun->laketype = LAKE_T_LAVA;
			count -= 2;

			/* Lake of Lava2 */
			if (!dun->laketype && (current_floor_ptr->dun_level > 80) && one_in_(count)) dun->laketype = LAKE_T_FIRE_VAULT;
			count--;
		}

		if ((d_info[p_ptr->dungeon_idx].flags1 & DF1_LAKE_WATER) && !dun->laketype)
		{
			/* Lake of Water */
			if ((current_floor_ptr->dun_level > 50) && randint0(count) < 2) dun->laketype = LAKE_T_WATER;
			count -= 2;

			/* Lake of Water2 */
			if (!dun->laketype && (current_floor_ptr->dun_level > 50) && one_in_(count)) dun->laketype = LAKE_T_WATER_VAULT;
			count--;
		}

		if ((d_info[p_ptr->dungeon_idx].flags1 & DF1_LAKE_RUBBLE) && !dun->laketype)
		{
			/* Lake of rubble */
			if ((current_floor_ptr->dun_level > 35) && (randint0(count) < 2)) dun->laketype = LAKE_T_CAVE;
			count -= 2;

			/* Lake of rubble2 */
			if (!dun->laketype && (current_floor_ptr->dun_level > 35) && one_in_(count)) dun->laketype = LAKE_T_EARTH_VAULT;
			count--;
		}

		/* Lake of tree */
		if ((current_floor_ptr->dun_level > 5) && (d_info[p_ptr->dungeon_idx].flags1 & DF1_LAKE_TREE) && !dun->laketype) dun->laketype = LAKE_T_AIR_VAULT;

		if (dun->laketype)
		{
			msg_print_wizard(CHEAT_DUNGEON, _("湖を生成します。", "Lake on the level."));
			build_lake(dun->laketype);
		}
	}

	if ((current_floor_ptr->dun_level > DUN_CAVERN) && !dun->empty_level &&
	    (d_info[p_ptr->dungeon_idx].flags1 & DF1_CAVERN) &&
	    !dun->laketype && !dun->destroyed && (randint1(1000) < current_floor_ptr->dun_level))
	{
		dun->cavern = TRUE;

		/* make a large fractal current_floor_ptr->grid_array in the middle of the dungeon */

		msg_print_wizard(CHEAT_DUNGEON, _("洞窟を生成。", "Cavern on level."));
		build_cavern();
	}
#endif /* ALLOW_CAVERNS_AND_LAKES */

	/* Hack -- No destroyed "quest" levels */
	if (quest_number(current_floor_ptr->dun_level)) dun->destroyed = FALSE;
}


/*!
 * @brief ダンジョン生成のメインルーチン / Generate a new dungeon level
 * @details Note that "dun_body" adds about 4000 bytes of memory to the stack.
 * @return ダンジョン生成が全て無事に成功したらTRUEを返す。
 */
static bool cave_gen(void)
{
	int i, k;
	POSITION y, x;
	dun_data dun_body;

	current_floor_ptr->lite_n = 0;
	current_floor_ptr->mon_lite_n = 0;
	current_floor_ptr->redraw_n = 0;
	current_floor_ptr->view_n = 0;

	/* Global data */
	dun = &dun_body;

	dun->destroyed = FALSE;
	dun->empty_level = FALSE;
	dun->cavern = FALSE;
	dun->laketype = 0;

	/* Fill the arrays of floors and walls in the good proportions */
	set_floor_and_wall(p_ptr->dungeon_idx);
	get_mon_num_prep(get_monster_hook(), NULL);

	/* Randomize the dungeon creation values */
	dun_tun_rnd = rand_range(DUN_TUN_RND_MIN, DUN_TUN_RND_MAX);
	dun_tun_chg = rand_range(DUN_TUN_CHG_MIN, DUN_TUN_CHG_MAX);
	dun_tun_con = rand_range(DUN_TUN_CON_MIN, DUN_TUN_CON_MAX);
	dun_tun_pen = rand_range(DUN_TUN_PEN_MIN, DUN_TUN_PEN_MAX);
	dun_tun_jct = rand_range(DUN_TUN_JCT_MIN, DUN_TUN_JCT_MAX);

	/* Actual maximum number of rooms on this level */
	dun->row_rooms = current_floor_ptr->height / BLOCK_HGT;
	dun->col_rooms = current_floor_ptr->width / BLOCK_WID;

	/* Initialize the room table */
	for (y = 0; y < dun->row_rooms; y++)
	{
		for (x = 0; x < dun->col_rooms; x++)
		{
			dun->room_map[y][x] = FALSE;
		}
	}

	/* No rooms yet */
	dun->cent_n = 0;

	/* Empty arena levels */
	if (ironman_empty_levels || ((d_info[p_ptr->dungeon_idx].flags1 & DF1_ARENA) && (empty_levels && one_in_(EMPTY_LEVEL))))
	{
		dun->empty_level = TRUE;
		msg_print_wizard(CHEAT_DUNGEON, _("アリーナレベルを生成。", "Arena level."));
	}

	if (dun->empty_level)
	{
		/* Start with floors */
		for (y = 0; y < current_floor_ptr->height; y++)
		{
			for (x = 0; x < current_floor_ptr->width; x++)
			{
				place_floor_bold(y, x);
			}
		}

		/* Special boundary walls -- Top and bottom */
		for (x = 0; x < current_floor_ptr->width; x++)
		{
			place_extra_bold(0, x);
			place_extra_bold(current_floor_ptr->height - 1, x);
		}

		/* Special boundary walls -- Left and right */
		for (y = 1; y < (current_floor_ptr->height - 1); y++)
		{
			place_extra_bold(y, 0);
			place_extra_bold(y, current_floor_ptr->width - 1);
		}
	}
	else
	{
		/* Start with walls */
		for (y = 0; y < current_floor_ptr->height; y++)
		{
			for (x = 0; x < current_floor_ptr->width; x++)
			{
				place_extra_bold(y, x);
			}
		}
	}

	/* Generate various caverns and lakes */
	gen_caverns_and_lakes();

	/* Build maze */
	if (d_info[p_ptr->dungeon_idx].flags1 & DF1_MAZE)
	{
		build_maze_vault(current_floor_ptr->width/2-1, current_floor_ptr->height/2-1, current_floor_ptr->width-4, current_floor_ptr->height-4, FALSE);

		/* Place 3 or 4 down stairs near some walls */
		if (!alloc_stairs(feat_down_stair, rand_range(2, 3), 3)) return FALSE;

		/* Place 1 or 2 up stairs near some walls */
		if (!alloc_stairs(feat_up_stair, 1, 3)) return FALSE;
	}

	/* Build some rooms */
	else
	{
		int tunnel_fail_count = 0;

		/*
		 * Build each type of room in turn until we cannot build any more.
		 */
		if (!generate_rooms()) return FALSE;


		/* Make a hole in the dungeon roof sometimes at level 1 */
		if (current_floor_ptr->dun_level == 1)
		{
			while (one_in_(DUN_MOS_DEN))
			{
				place_trees(randint1(current_floor_ptr->width - 2), randint1(current_floor_ptr->height - 2));
			}
		}

		/* Destroy the level if necessary */
		if (dun->destroyed) destroy_level();

		/* Hack -- Add some rivers */
		if (one_in_(3) && (randint1(current_floor_ptr->dun_level) > 5))
		{
			FEAT_IDX feat1 = 0, feat2 = 0;

			/* Choose water mainly */
			if ((randint1(MAX_DEPTH * 2) - 1 > current_floor_ptr->dun_level) && (d_info[p_ptr->dungeon_idx].flags1 & DF1_WATER_RIVER))
			{
				feat1 = feat_deep_water;
				feat2 = feat_shallow_water;
			}
			else /* others */
			{
				FEAT_IDX select_deep_feat[10];
				FEAT_IDX select_shallow_feat[10];
				int select_id_max = 0, selected;

				if (d_info[p_ptr->dungeon_idx].flags1 & DF1_LAVA_RIVER)
				{
					select_deep_feat[select_id_max] = feat_deep_lava;
					select_shallow_feat[select_id_max] = feat_shallow_lava;
					select_id_max++;
				}
				if (d_info[p_ptr->dungeon_idx].flags1 & DF1_POISONOUS_RIVER)
				{
					select_deep_feat[select_id_max] = feat_deep_poisonous_puddle;
					select_shallow_feat[select_id_max] = feat_shallow_poisonous_puddle;
					select_id_max++;
				}
				if (d_info[p_ptr->dungeon_idx].flags1 & DF1_ACID_RIVER)
				{
					select_deep_feat[select_id_max] = feat_deep_acid_puddle;
					select_shallow_feat[select_id_max] = feat_shallow_acid_puddle;
					select_id_max++;
				}

				selected = randint0(select_id_max);
				feat1 = select_deep_feat[selected];
				feat2 = select_shallow_feat[selected];
			}

			if (feat1)
			{
				feature_type *f_ptr = &f_info[feat1];

				/* Only add river if matches lake type or if have no lake at all */
				if (((dun->laketype == LAKE_T_LAVA) && have_flag(f_ptr->flags, FF_LAVA)) ||
				    ((dun->laketype == LAKE_T_WATER) && have_flag(f_ptr->flags, FF_WATER)) ||
				     !dun->laketype)
				{
					add_river(feat1, feat2);
				}
			}
		}

		/* Hack -- Scramble the room order */
		for (i = 0; i < dun->cent_n; i++)
		{
			POSITION ty, tx;
			int pick = rand_range(0, i);

			ty = dun->cent[i].y;
			tx = dun->cent[i].x;
			dun->cent[i].y = dun->cent[pick].y;
			dun->cent[i].x = dun->cent[pick].x;
			dun->cent[pick].y = ty;
			dun->cent[pick].x = tx;
		}

		/* Start with no tunnel doors */
		dun->door_n = 0;

		/* Hack -- connect the first room to the last room */
		y = dun->cent[dun->cent_n-1].y;
		x = dun->cent[dun->cent_n-1].x;

		/* Connect all the rooms together */
		for (i = 0; i < dun->cent_n; i++)
		{
			int j;

			/* Reset the arrays */
			dun->tunn_n = 0;
			dun->wall_n = 0;

			/* Connect the room to the previous room */
			if (randint1(current_floor_ptr->dun_level) > d_info[p_ptr->dungeon_idx].tunnel_percent)
			{
				/* make cavelike tunnel */
				(void)build_tunnel2(dun->cent[i].x, dun->cent[i].y, x, y, 2, 2);
			}
			else
			{
				/* make normal tunnel */
				if (!build_tunnel(dun->cent[i].y, dun->cent[i].x, y, x)) tunnel_fail_count++;
			}

			if (tunnel_fail_count >= 2) return FALSE;

			/* Turn the tunnel into corridor */
			for (j = 0; j < dun->tunn_n; j++)
			{
				grid_type *g_ptr;
				feature_type *f_ptr;
				y = dun->tunn[j].y;
				x = dun->tunn[j].x;
				g_ptr = &current_floor_ptr->grid_array[y][x];
				f_ptr = &f_info[g_ptr->feat];

				/* Clear previous contents (if not a lake), add a floor */
				if (!have_flag(f_ptr->flags, FF_MOVE) || (!have_flag(f_ptr->flags, FF_WATER) && !have_flag(f_ptr->flags, FF_LAVA)))
				{
					/* Clear mimic type */
					g_ptr->mimic = 0;

					place_floor_grid(g_ptr);
				}
			}

			/* Apply the piercings that we found */
			for (j = 0; j < dun->wall_n; j++)
			{
				grid_type *g_ptr;
				y = dun->wall[j].y;
				x = dun->wall[j].x;
				g_ptr = &current_floor_ptr->grid_array[y][x];

				/* Clear mimic type */
				g_ptr->mimic = 0;

				/* Clear previous contents, add up floor */
				place_floor_grid(g_ptr);

				/* Occasional doorway */
				if ((randint0(100) < dun_tun_pen) && !(d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_DOORS))
				{
					/* Place a random door */
					place_random_door(y, x, TRUE);
				}
			}

			/* Remember the "previous" room */
			y = dun->cent[i].y;
			x = dun->cent[i].x;
		}

		/* Place intersection doors */
		for (i = 0; i < dun->door_n; i++)
		{
			/* Extract junction location */
			y = dun->door[i].y;
			x = dun->door[i].x;

			/* Try placing doors */
			try_door(y, x - 1);
			try_door(y, x + 1);
			try_door(y - 1, x);
			try_door(y + 1, x);
		}

		/* Place 3 or 4 down stairs near some walls */
		if (!alloc_stairs(feat_down_stair, rand_range(3, 4), 3)) return FALSE;

		/* Place 1 or 2 up stairs near some walls */
		if (!alloc_stairs(feat_up_stair, rand_range(1, 2), 3)) return FALSE;
	}

	if (!dun->laketype)
	{
		if (d_info[p_ptr->dungeon_idx].stream2)
		{
			/* Hack -- Add some quartz streamers */
			for (i = 0; i < DUN_STR_QUA; i++)
			{
				build_streamer(d_info[p_ptr->dungeon_idx].stream2, DUN_STR_QC);
			}
		}

		if (d_info[p_ptr->dungeon_idx].stream1)
		{
			/* Hack -- Add some magma streamers */
			for (i = 0; i < DUN_STR_MAG; i++)
			{
				build_streamer(d_info[p_ptr->dungeon_idx].stream1, DUN_STR_MC);
			}
		}
	}

	/* Special boundary walls -- Top and bottom */
	for (x = 0; x < current_floor_ptr->width; x++)
	{
		place_bound_perm_wall(&current_floor_ptr->grid_array[0][x]);
		place_bound_perm_wall(&current_floor_ptr->grid_array[current_floor_ptr->height - 1][x]);
	}

	/* Special boundary walls -- Left and right */
	for (y = 1; y < (current_floor_ptr->height - 1); y++)
	{
		place_bound_perm_wall(&current_floor_ptr->grid_array[y][0]);
		place_bound_perm_wall(&current_floor_ptr->grid_array[y][current_floor_ptr->width - 1]);
	}

	/* Determine the character location */
	if (!new_player_spot()) return FALSE;

	if (!place_quest_monsters()) return FALSE;

	/* Basic "amount" */
	k = (current_floor_ptr->dun_level / 3);
	if (k > 10) k = 10;
	if (k < 2) k = 2;

	/* Pick a base number of monsters */
	i = d_info[p_ptr->dungeon_idx].min_m_alloc_level;

	/* To make small levels a bit more playable */
	if (current_floor_ptr->height < MAX_HGT || current_floor_ptr->width < MAX_WID)
	{
		int small_tester = i;

		i = (i * current_floor_ptr->height) / MAX_HGT;
		i = (i * current_floor_ptr->width) / MAX_WID;
		i += 1;

		if (i > small_tester) i = small_tester;
		else msg_format_wizard(CHEAT_DUNGEON,
			_("モンスター数基本値を %d から %d に減らします", "Reduced monsters base from %d to %d"), small_tester, i);

	}

	i += randint1(8);

	/* Put some monsters in the dungeon */
	for (i = i + k; i > 0; i--)
	{
		(void)alloc_monster(0, PM_ALLOW_SLEEP);
	}

	/* Place some traps in the dungeon */
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

	/* Put some rubble in corridors (except NO_CAVE dungeon (Castle)) */
	if (!(d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_CAVE)) alloc_object(ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint1(k));

	/* Mega Hack -- No object at first level of deeper dungeon */
	if (p_ptr->enter_dungeon && current_floor_ptr->dun_level > 1)
	{
		/* No stair scum! */
		current_floor_ptr->object_level = 1;
	}

	/* Put some objects in rooms */
	alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3));

	/* Put some objects/gold in the dungeon */
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3));
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3));

	/* Set back to default */
	current_floor_ptr->object_level = current_floor_ptr->base_level;

	/* Put the Guardian */
	if (!alloc_guardian(TRUE)) return FALSE;

	if (dun->empty_level && (!one_in_(DARK_EMPTY) || (randint1(100) > current_floor_ptr->dun_level)) && !(d_info[p_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
	{
		/* Lite the current_floor_ptr->grid_array */
		for (y = 0; y < current_floor_ptr->height; y++)
		{
			for (x = 0; x < current_floor_ptr->width; x++)
			{
				current_floor_ptr->grid_array[y][x].info |= (CAVE_GLOW);
			}
		}
	}

	return TRUE;
}

/*!
 * @brief 闘技場用のアリーナ地形を作成する / Builds the arena after it is entered -KMW-
 * @return なし
 */
static void build_arena(void)
{
	POSITION yval, y_height, y_depth, xval, x_left, x_right;
	register int i, j;

	yval = SCREEN_HGT / 2;
	xval = SCREEN_WID / 2;
	y_height = yval - 10;
	y_depth = yval + 10;
	x_left = xval - 32;
	x_right = xval + 32;

	for (i = y_height; i <= y_height + 5; i++)
		for (j = x_left; j <= x_right; j++)
		{
			place_extra_perm_bold(i, j);
			current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (i = y_depth; i >= y_depth - 5; i--)
		for (j = x_left; j <= x_right; j++)
		{
			place_extra_perm_bold(i, j);
			current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_left; j <= x_left + 17; j++)
		for (i = y_height; i <= y_depth; i++)
		{
			place_extra_perm_bold(i, j);
			current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_right; j >= x_right - 17; j--)
		for (i = y_height; i <= y_depth; i++)
		{
			place_extra_perm_bold(i, j);
			current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}

	place_extra_perm_bold(y_height+6, x_left+18);
	current_floor_ptr->grid_array[y_height+6][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_depth-6, x_left+18);
	current_floor_ptr->grid_array[y_depth-6][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_height+6, x_right-18);
	current_floor_ptr->grid_array[y_height+6][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_depth-6, x_right-18);
	current_floor_ptr->grid_array[y_depth-6][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);

	i = y_height + 5;
	j = xval;
	current_floor_ptr->grid_array[i][j].feat = f_tag_to_index("ARENA_GATE");
	current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
	player_place(i, j);
}

/*!
 * @brief 挑戦時闘技場への入場処理 / Town logic flow for generation of arena -KMW-
 * @return なし
 */
static void generate_challenge_arena(void)
{
	POSITION y, x;
	POSITION qy = 0;
	POSITION qx = 0;

	/* Smallest area */
	current_floor_ptr->height = SCREEN_HGT;
	current_floor_ptr->width = SCREEN_WID;

	/* Start with solid walls */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			/* Create "solid" perma-wall */
			place_solid_perm_bold(y, x);

			/* Illuminate and memorize the walls */
			current_floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
		}
	}

	/* Then place some floors */
	for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++)
	{
		for (x = qx + 1; x < qx + SCREEN_WID - 1; x++)
		{
			/* Create empty floor */
			current_floor_ptr->grid_array[y][x].feat = feat_floor;
		}
	}

	build_arena();

	if(!place_monster_aux(0, p_ptr->y + 5, p_ptr->x, arena_info[p_ptr->arena_number].r_idx, (PM_NO_KAGE | PM_NO_PET)))
	{
		p_ptr->exit_bldg = TRUE;
		p_ptr->arena_number++;
		msg_print(_("相手は欠場した。あなたの不戦勝だ。", "The enemy is unable appear. You won by default."));
	}

}

/*!
 * @brief モンスター闘技場のフロア生成 / Builds the arena after it is entered -KMW-
 * @return なし
 */
static void build_battle(void)
{
	POSITION yval, y_height, y_depth, xval, x_left, x_right;
	register int i, j;

	yval = SCREEN_HGT / 2;
	xval = SCREEN_WID / 2;
	y_height = yval - 10;
	y_depth = yval + 10;
	x_left = xval - 32;
	x_right = xval + 32;

	for (i = y_height; i <= y_height + 5; i++)
		for (j = x_left; j <= x_right; j++)
		{
			place_extra_perm_bold(i, j);
			current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (i = y_depth; i >= y_depth - 3; i--)
		for (j = x_left; j <= x_right; j++)
		{
			place_extra_perm_bold(i, j);
			current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_left; j <= x_left + 17; j++)
		for (i = y_height; i <= y_depth; i++)
		{
			place_extra_perm_bold(i, j);
			current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_right; j >= x_right - 17; j--)
		for (i = y_height; i <= y_depth; i++)
		{
			place_extra_perm_bold(i, j);
			current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}

	place_extra_perm_bold(y_height+6, x_left+18);
	current_floor_ptr->grid_array[y_height+6][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_depth-4, x_left+18);
	current_floor_ptr->grid_array[y_depth-4][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_height+6, x_right-18);
	current_floor_ptr->grid_array[y_height+6][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_depth-4, x_right-18);
	current_floor_ptr->grid_array[y_depth-4][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);

	for (i = y_height + 1; i <= y_height + 5; i++)
		for (j = x_left + 20 + 2 * (y_height + 5 - i); j <= x_right - 20 - 2 * (y_height + 5 - i); j++)
		{
			current_floor_ptr->grid_array[i][j].feat = feat_permanent_glass_wall;
		}

	i = y_height + 1;
	j = xval;
	current_floor_ptr->grid_array[i][j].feat = f_tag_to_index("BUILDING_3");
	current_floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
	player_place(i, j);
}

/*!
 * @brief モンスター闘技場への導入処理 / Town logic flow for generation of arena -KMW-
 * @return なし
 */
static void generate_gambling_arena(void)
{
	POSITION y, x;
	MONSTER_IDX i;
	POSITION qy = 0;
	POSITION qx = 0;

	/* Start with solid walls */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			/* Create "solid" perma-wall */
			place_solid_perm_bold(y, x);

			/* Illuminate and memorize the walls */
			current_floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
		}
	}

	/* Then place some floors */
	for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++)
	{
		for (x = qx + 1; x < qx + SCREEN_WID - 1; x++)
		{
			/* Create empty floor */
			current_floor_ptr->grid_array[y][x].feat = feat_floor;
		}
	}

	build_battle();

	for(i = 0; i < 4; i++)
	{
		place_monster_aux(0, p_ptr->y + 8 + (i/2)*4, p_ptr->x - 2 + (i%2)*4, battle_mon[i], (PM_NO_KAGE | PM_NO_PET));
		set_friendly(&current_floor_ptr->m_list[current_floor_ptr->grid_array[p_ptr->y+8+(i/2)*4][p_ptr->x-2+(i%2)*4].m_idx]);
	}
	for(i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[i];

		if (!monster_is_valid(m_ptr)) continue;

		m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
		update_monster(i, FALSE);
	}
}

/*!
 * @brief 固定マップクエストのフロア生成 / Generate a quest level
 * @return なし
 */
static void generate_fixed_floor(void)
{
	POSITION x, y;

	/* Start with perm walls */
	for (y = 0; y < current_floor_ptr->height; y++)
	{
		for (x = 0; x < current_floor_ptr->width; x++)
		{
			place_solid_perm_bold(y, x);
		}
	}

	/* Set the quest level */
	current_floor_ptr->base_level = quest[p_ptr->inside_quest].level;
	current_floor_ptr->dun_level = current_floor_ptr->base_level;
	current_floor_ptr->object_level = current_floor_ptr->base_level;
	current_floor_ptr->monster_level = current_floor_ptr->base_level;

	if (record_stair) do_cmd_write_nikki(NIKKI_TO_QUEST, p_ptr->inside_quest, NULL);
	get_mon_num_prep(get_monster_hook(), NULL);

	init_flags = INIT_CREATE_DUNGEON;

	process_dungeon_file("q_info.txt", 0, 0, MAX_HGT, MAX_WID);
}

/*!
 * @brief ダンジョン時のランダムフロア生成 / Make a real level
 * @return フロアの生成に成功したらTRUE
 */
static bool level_gen(concptr *why)
{
	int level_height, level_width;

	if ((always_small_levels || ironman_small_levels ||
	    (one_in_(SMALL_LEVEL) && small_levels) ||
	     (d_info[p_ptr->dungeon_idx].flags1 & DF1_BEGINNER) ||
	    (d_info[p_ptr->dungeon_idx].flags1 & DF1_SMALLEST)) &&
	    !(d_info[p_ptr->dungeon_idx].flags1 & DF1_BIG))
	{
		if (d_info[p_ptr->dungeon_idx].flags1 & DF1_SMALLEST)
		{
			level_height = 1;
			level_width = 1;
		}
		else if (d_info[p_ptr->dungeon_idx].flags1 & DF1_BEGINNER)
		{
			level_height = 2;
			level_width = 2;
		}
		else
		{
			do
			{
				level_height = randint1(MAX_HGT/SCREEN_HGT);
				level_width = randint1(MAX_WID/SCREEN_WID);
			}
			while ((level_height == MAX_HGT/SCREEN_HGT) && (level_width == MAX_WID/SCREEN_WID));
		}

		current_floor_ptr->height = level_height * SCREEN_HGT;
		current_floor_ptr->width = level_width * SCREEN_WID;

		/* Assume illegal panel */
		panel_row_min = current_floor_ptr->height;
		panel_col_min = current_floor_ptr->width;

		msg_format_wizard(CHEAT_DUNGEON,
			_("小さなフロア: X:%d, Y:%d", "A 'small' dungeon level: X:%d, Y:%d."),
			current_floor_ptr->width, current_floor_ptr->height);
	}
	else
	{
		/* Big dungeon */
		current_floor_ptr->height = MAX_HGT;
		current_floor_ptr->width = MAX_WID;

		/* Assume illegal panel */
		panel_row_min = current_floor_ptr->height;
		panel_col_min = current_floor_ptr->width;
	}

	/* Make a dungeon */
	if (!cave_gen())
	{
		*why = _("ダンジョン生成に失敗", "could not place player");
		return FALSE;
	}
	else return TRUE;
}

/*!
 * @brief フロアに存在する全マスの記憶状態を初期化する / Wipe all unnecessary flags after current_floor_ptr->grid_array generation
 * @return なし
 */
void wipe_generate_random_floor_flags(void)
{
	POSITION x, y;

	for (y = 0; y < current_floor_ptr->height; y++)
	{
		for (x = 0; x < current_floor_ptr->width; x++)
		{
			/* Wipe unused flags */
			current_floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
		}
	}

	if (current_floor_ptr->dun_level)
	{
		for (y = 1; y < current_floor_ptr->height - 1; y++)
		{
			for (x = 1; x < current_floor_ptr->width - 1; x++)
			{
				/* There might be trap */
				current_floor_ptr->grid_array[y][x].info |= CAVE_UNSAFE;
			}
		}
	}
}

/*!
 * @brief フロアの全情報を初期化する / Clear and empty the current_floor_ptr->grid_array
 * @return なし
 */
void clear_cave(void)
{
	POSITION x, y;
	int i;

	/* Very simplified version of wipe_o_list() */
	(void)C_WIPE(current_floor_ptr->o_list, o_max, object_type);
	o_max = 1;
	o_cnt = 0;

	/* Very simplified version of wipe_m_list() */
	for (i = 1; i < max_r_idx; i++)
		r_info[i].cur_num = 0;
	(void)C_WIPE(current_floor_ptr->m_list, m_max, monster_type);
	m_max = 1;
	m_cnt = 0;
	for (i = 0; i < MAX_MTIMED; i++) current_floor_ptr->mproc_max[i] = 0;

	/* Pre-calc cur_num of pets in party_mon[] */
	precalc_cur_num_of_pet();


	/* Start with a blank current_floor_ptr->grid_array */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
			g_ptr->info = 0;
			g_ptr->feat = 0;
			g_ptr->o_idx = 0;
			g_ptr->m_idx = 0;
			g_ptr->special = 0;
			g_ptr->mimic = 0;
			g_ptr->cost = 0;
			g_ptr->dist = 0;
			g_ptr->when = 0;
		}
	}

	/* Mega-Hack -- no player yet */
	p_ptr->x = p_ptr->y = 0;

	/* Set the base level */
	current_floor_ptr->base_level = current_floor_ptr->dun_level;

	/* Reset the monster generation level */
	current_floor_ptr->monster_level = current_floor_ptr->base_level;

	/* Reset the object generation level */
	current_floor_ptr->object_level = current_floor_ptr->base_level;
}


/*!
 * ダンジョンのランダムフロアを生成する / Generates a random dungeon level -RAK-
 * @return なし
 * @note Hack -- regenerate any "overflow" levels
 */
void generate_random_floor(void)
{
	int num;

	/* Fill the arrays of floors and walls in the good proportions */
	set_floor_and_wall(p_ptr->dungeon_idx);

	/* Generate */
	for (num = 0; TRUE; num++)
	{
		bool okay = TRUE;

		concptr why = NULL;

		/* Clear and empty the current_floor_ptr->grid_array */
		clear_cave();

		/* Build the arena -KMW- */
		if (p_ptr->inside_arena)
		{
			/* Small arena */
			generate_challenge_arena();
		}

		/* Build the battle -KMW- */
		else if (p_ptr->inside_battle)
		{
			/* Small arena */
			generate_gambling_arena();
		}

		else if (p_ptr->inside_quest)
		{
			generate_fixed_floor();
		}

		/* Build the town */
		else if (!current_floor_ptr->dun_level)
		{
			/* Make the wilderness */
			if (p_ptr->wild_mode) wilderness_gen_small();
			else wilderness_gen();
		}

		/* Build a real level */
		else
		{
			okay = level_gen(&why);
		}


		/* Prevent object over-flow */
		if (o_max >= current_floor_ptr->max_o_idx)
		{
			why = _("アイテムが多すぎる", "too many objects");
			okay = FALSE;
		}
		/* Prevent monster over-flow */
		else if (m_max >= current_floor_ptr->max_m_idx)
		{
			why = _("モンスターが多すぎる", "too many monsters");
			okay = FALSE;
		}

		/* Accept */
		if (okay) break;

		if (why) msg_format(_("生成やり直し(%s)", "Generation restarted (%s)"), why);

		wipe_o_list();
		wipe_m_list();
	}

	/* Glow deep lava and building entrances */
	glow_deep_lava_and_bldg();

	/* Reset flag */
	p_ptr->enter_dungeon = FALSE;

	wipe_generate_random_floor_flags();
}

/*!
 * @brief build_tunnel用に通路を掘るための方向をランダムに決める / Pick a random direction
 * @param rdir Y方向に取るべきベクトル値を返す参照ポインタ
 * @param cdir X方向に取るべきベクトル値を返す参照ポインタ
 * @return なし
 */
static void rand_dir(POSITION *rdir, POSITION *cdir)
{
	/* Pick a random direction */
	int i = randint0(4);

	/* Extract the dy/dx components */
	*rdir = ddy_ddd[i];
	*cdir = ddx_ddd[i];
}

/*!
 * @brief build_tunnel用に通路を掘るための方向を位置関係通りに決める / Always picks a correct direction
 * @param rdir Y方向に取るべきベクトル値を返す参照ポインタ
 * @param cdir X方向に取るべきベクトル値を返す参照ポインタ
 * @param y1 始点Y座標
 * @param x1 始点X座標
 * @param y2 終点Y座標
 * @param x2 終点X座標
 * @return なし
 */
static void correct_dir(POSITION *rdir, POSITION *cdir, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	/* Extract vertical and horizontal directions */
	*rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
	*cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;

	/* Never move diagonally */
	if (*rdir && *cdir)
	{
		if (randint0(100) < 50)
			*rdir = 0;
		else
			*cdir = 0;
	}
}

/*!
* @brief 部屋間のトンネルを生成する / Constructs a tunnel between two points
* @param row1 始点Y座標
* @param col1 始点X座標
* @param row2 終点Y座標
* @param col2 終点X座標
* @return 生成に成功したらTRUEを返す
* @details
* This function must be called BEFORE any streamers are created,\n
* since we use the special "granite wall" sub-types to keep track\n
* of legal places for corridors to pierce rooms.\n
*\n
* We use "door_flag" to prevent excessive construction of doors\n
* along overlapping corridors.\n
*\n
* We queue the tunnel grids to prevent door creation along a corridor\n
* which intersects itself.\n
*\n
* We queue the wall piercing grids to prevent a corridor from leaving\n
* a room and then coming back in through the same entrance.\n
*\n
* We "pierce" grids which are "outer" walls of rooms, and when we\n
* do so, we change all adjacent "outer" walls of rooms into "solid"\n
* walls so that no two corridors may use adjacent grids for exits.\n
*\n
* The "solid" wall check prevents corridors from "chopping" the\n
* corners of rooms off, as well as "silly" door placement, and\n
* "excessively wide" room entrances.\n
*\n
* Kind of walls:\n
*   extra -- walls\n
*   inner -- inner room walls\n
*   outer -- outer room walls\n
*   solid -- solid room walls\n
*/
bool build_tunnel(POSITION row1, POSITION col1, POSITION row2, POSITION col2)
{
	POSITION y, x;
	POSITION tmp_row, tmp_col;
	POSITION row_dir, col_dir;
	POSITION start_row, start_col;
	int main_loop_count = 0;

	bool door_flag = FALSE;

	grid_type *g_ptr;

	/* Save the starting location */
	start_row = row1;
	start_col = col1;

	/* Start out in the correct direction */
	correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

	/* Keep going until done (or bored) */
	while ((row1 != row2) || (col1 != col2))
	{
		/* Mega-Hack -- Paranoia -- prevent infinite loops */
		if (main_loop_count++ > 2000) return FALSE;

		/* Allow bends in the tunnel */
		if (randint0(100) < dun_tun_chg)
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (randint0(100) < dun_tun_rnd)
			{
				rand_dir(&row_dir, &col_dir);
			}
		}

		/* Get the next location */
		tmp_row = row1 + row_dir;
		tmp_col = col1 + col_dir;


		/* Extremely Important -- do not leave the dungeon */
		while (!in_bounds(tmp_row, tmp_col))
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (randint0(100) < dun_tun_rnd)
			{
				rand_dir(&row_dir, &col_dir);
			}

			/* Get the next location */
			tmp_row = row1 + row_dir;
			tmp_col = col1 + col_dir;
		}

		g_ptr = &current_floor_ptr->grid_array[tmp_row][tmp_col];

		/* Avoid "solid" walls */
		if (is_solid_grid(g_ptr)) continue;

		/* Pierce "outer" walls of rooms */
		if (is_outer_grid(g_ptr))
		{
			/* Acquire the "next" location */
			y = tmp_row + row_dir;
			x = tmp_col + col_dir;

			/* Hack -- Avoid outer/solid walls */
			if (is_outer_bold(y, x)) continue;
			if (is_solid_bold(y, x)) continue;

			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the wall location */
			if (dun->wall_n < WALL_MAX)
			{
				dun->wall[dun->wall_n].y = row1;
				dun->wall[dun->wall_n].x = col1;
				dun->wall_n++;
			}
			else return FALSE;

			/* Forbid re-entry near this piercing */
			for (y = row1 - 1; y <= row1 + 1; y++)
			{
				for (x = col1 - 1; x <= col1 + 1; x++)
				{
					/* Convert adjacent "outer" walls as "solid" walls */
					if (is_outer_bold(y, x))
					{
						/* Change the wall to a "solid" wall */
						place_solid_noperm_bold(y, x);
					}
				}
			}
		}

		/* Travel quickly through rooms */
		else if (g_ptr->info & (CAVE_ROOM))
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;
		}

		/* Tunnel through all other walls */
		else if (is_extra_grid(g_ptr) || is_inner_grid(g_ptr) || is_solid_grid(g_ptr))
		{
			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the tunnel location */
			if (dun->tunn_n < TUNN_MAX)
			{
				dun->tunn[dun->tunn_n].y = row1;
				dun->tunn[dun->tunn_n].x = col1;
				dun->tunn_n++;
			}
			else return FALSE;

			/* Allow door in next grid */
			door_flag = FALSE;
		}

		/* Handle corridor intersections or overlaps */
		else
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Collect legal door locations */
			if (!door_flag)
			{
				/* Save the door location */
				if (dun->door_n < DOOR_MAX)
				{
					dun->door[dun->door_n].y = row1;
					dun->door[dun->door_n].x = col1;
					dun->door_n++;
				}
				else return FALSE;

				/* No door in next grid */
				door_flag = TRUE;
			}

			/* Hack -- allow pre-emptive tunnel termination */
			if (randint0(100) >= dun_tun_con)
			{
				/* Distance between row1 and start_row */
				tmp_row = row1 - start_row;
				if (tmp_row < 0) tmp_row = (-tmp_row);

				/* Distance between col1 and start_col */
				tmp_col = col1 - start_col;
				if (tmp_col < 0) tmp_col = (-tmp_col);

				/* Terminate the tunnel */
				if ((tmp_row > 10) || (tmp_col > 10)) break;
			}
		}
	}

	return TRUE;
}


/*!
* @brief トンネル生成のための基準点を指定する。
* @param x 基準点を指定するX座標の参照ポインタ、適時値が修正される。
* @param y 基準点を指定するY座標の参照ポインタ、適時値が修正される。
* @param affectwall (調査中)
* @return なし
* @details
* This routine adds the square to the tunnel\n
* It also checks for SOLID walls - and returns a nearby\n
* non-SOLID square in (x,y) so that a simple avoiding\n
* routine can be used. The returned boolean value reflects\n
* whether or not this routine hit a SOLID wall.\n
*\n
* "affectwall" toggles whether or not this new square affects\n
* the boundaries of rooms. - This is used by the catacomb\n
* routine.\n
* @todo 特に詳細な処理の意味を調査すべし
*/
static bool set_tunnel(POSITION *x, POSITION *y, bool affectwall)
{
	int i, j, dx, dy;

	grid_type *g_ptr = &current_floor_ptr->grid_array[*y][*x];

	if (!in_bounds(*y, *x)) return TRUE;

	if (is_inner_grid(g_ptr))
	{
		return TRUE;
	}

	if (is_extra_bold(*y, *x))
	{
		/* Save the tunnel location */
		if (dun->tunn_n < TUNN_MAX)
		{
			dun->tunn[dun->tunn_n].y = *y;
			dun->tunn[dun->tunn_n].x = *x;
			dun->tunn_n++;

			return TRUE;
		}
		else return FALSE;
	}

	if (is_floor_bold(*y, *x))
	{
		/* Don't do anything */
		return TRUE;
	}

	if (is_outer_grid(g_ptr) && affectwall)
	{
		/* Save the wall location */
		if (dun->wall_n < WALL_MAX)
		{
			dun->wall[dun->wall_n].y = *y;
			dun->wall[dun->wall_n].x = *x;
			dun->wall_n++;
		}
		else return FALSE;

		/* Forbid re-entry near this piercing */
		for (j = *y - 1; j <= *y + 1; j++)
		{
			for (i = *x - 1; i <= *x + 1; i++)
			{
				/* Convert adjacent "outer" walls as "solid" walls */
				if (is_outer_bold(j, i))
				{
					/* Change the wall to a "solid" wall */
					place_solid_noperm_bold(j, i);
				}
			}
		}

		/* Clear mimic type */
		current_floor_ptr->grid_array[*y][*x].mimic = 0;

		place_floor_bold(*y, *x);

		return TRUE;
	}

	if (is_solid_grid(g_ptr) && affectwall)
	{
		/* cannot place tunnel here - use a square to the side */

		/* find usable square and return value in (x,y) */

		i = 50;

		dy = 0;
		dx = 0;
		while ((i > 0) && is_solid_bold(*y + dy, *x + dx))
		{
			dy = randint0(3) - 1;
			dx = randint0(3) - 1;

			if (!in_bounds(*y + dy, *x + dx))
			{
				dx = 0;
				dy = 0;
			}

			i--;
		}

		if (i == 0)
		{
			/* Failed for some reason: hack - ignore the solidness */
			place_outer_grid(g_ptr);
			dx = 0;
			dy = 0;
		}

		/* Give new, acceptable coordinate. */
		*x = *x + dx;
		*y = *y + dy;

		return FALSE;
	}

	return TRUE;
}


/*!
* @brief 外壁を削って「カタコンベ状」の通路を作成する / This routine creates the catacomb-like tunnels by removing extra rock.
* @param x 基準点のX座標
* @param y 基準点のY座標
* @return なし
* @details
* Note that this routine is only called on "even" squares - so it gives
* a natural checkerboard pattern.
*/
static void create_cata_tunnel(POSITION x, POSITION y)
{
	POSITION x1, y1;

	/* Build tunnel */
	x1 = x - 1;
	y1 = y;
	set_tunnel(&x1, &y1, FALSE);

	x1 = x + 1;
	y1 = y;
	set_tunnel(&x1, &y1, FALSE);

	x1 = x;
	y1 = y - 1;
	set_tunnel(&x1, &y1, FALSE);

	x1 = x;
	y1 = y + 1;
	set_tunnel(&x1, &y1, FALSE);
}


/*!
* @brief トンネル生成処理（詳細調査中）/ This routine does the bulk of the work in creating the new types of tunnels.
* @return なし
* @todo 詳細用調査
* @details
* It is designed to use very simple algorithms to go from (x1,y1) to (x2,y2)\n
* It doesn't need to add any complexity - straight lines are fine.\n
* The SOLID walls are avoided by a recursive algorithm which tries random ways\n
* around the obstical until it works.  The number of itterations is counted, and it\n
* this gets too large the routine exits. This should stop any crashes - but may leave\n
* small gaps in the tunnel where there are too many SOLID walls.\n
*\n
* Type 1 tunnels are extremely simple - straight line from A to B.  This is only used\n
* as a part of the dodge SOLID walls algorithm.\n
*\n
* Type 2 tunnels are made of two straight lines at right angles. When this is used with\n
* short line segments it gives the "cavelike" tunnels seen deeper in the dungeon.\n
*\n
* Type 3 tunnels are made of two straight lines like type 2, but with extra rock removed.\n
* This, when used with longer line segments gives the "catacomb-like" tunnels seen near\n
* the surface.\n
*/
static void short_seg_hack(POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int count, bool *fail)
{
	int i;
	POSITION x, y;
	int length;

	/* Check for early exit */
	if (!(*fail)) return;

	length = distance(x1, y1, x2, y2);

	count++;

	if ((type == 1) && (length != 0))
	{

		for (i = 0; i <= length; i++)
		{
			x = x1 + i * (x2 - x1) / length;
			y = y1 + i * (y2 - y1) / length;
			if (!set_tunnel(&x, &y, TRUE))
			{
				if (count > 50)
				{
					/* This isn't working - probably have an infinite loop */
					*fail = FALSE;
					return;
				}

				/* solid wall - so try to go around */
				short_seg_hack(x, y, x1 + (i - 1) * (x2 - x1) / length, y1 + (i - 1) * (y2 - y1) / length, 1, count, fail);
				short_seg_hack(x, y, x1 + (i + 1) * (x2 - x1) / length, y1 + (i + 1) * (y2 - y1) / length, 1, count, fail);
			}
		}
	}
	else if ((type == 2) || (type == 3))
	{
		if (x1 < x2)
		{
			for (i = x1; i <= x2; i++)
			{
				x = i;
				y = y1;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, i - 1, y1, 1, count, fail);
					short_seg_hack(x, y, i + 1, y1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(i, y1);
				}
			}
		}
		else
		{
			for (i = x2; i <= x1; i++)
			{
				x = i;
				y = y1;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, i - 1, y1, 1, count, fail);
					short_seg_hack(x, y, i + 1, y1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(i, y1);
				}
			}

		}
		if (y1 < y2)
		{
			for (i = y1; i <= y2; i++)
			{
				x = x2;
				y = i;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, x2, i - 1, 1, count, fail);
					short_seg_hack(x, y, x2, i + 1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(x2, i);
				}
			}
		}
		else
		{
			for (i = y2; i <= y1; i++)
			{
				x = x2;
				y = i;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, x2, i - 1, 1, count, fail);
					short_seg_hack(x, y, x2, i + 1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(x2, i);
				}
			}
		}
	}
}


/*!
* @brief 特定の壁(永久壁など)を避けながら部屋間の通路を作成する / This routine maps a path from (x1, y1) to (x2, y2) avoiding SOLID walls.
* @return なし
* @todo 詳細要調査
* @details
* Permanent rock is ignored in this path finding- sometimes there is no\n
* path around anyway -so there will be a crash if we try to find one.\n
* This routine is much like the river creation routine in Zangband.\n
* It works by dividing a line segment into two.  The segments are divided\n
* until they are less than "cutoff" - when the corresponding routine from\n
* "short_seg_hack" is called.\n
* Note it is VERY important that the "stop if hit another passage" logic\n
* stays as is.  Without this the dungeon turns into Swiss Cheese...\n
*/
bool build_tunnel2(POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int cutoff)
{
	POSITION x3, y3, dx, dy;
	POSITION changex, changey;
	int length;
	int i;
	bool retval, firstsuccede;
	grid_type *g_ptr;

	length = distance(x1, y1, x2, y2);

	if (length > cutoff)
	{
		/*
		* Divide path in half and call routine twice.
		*/
		dx = (x2 - x1) / 2;
		dy = (y2 - y1) / 2;

		/* perturbation perpendicular to path */
		changex = (randint0(abs(dy) + 2) * 2 - abs(dy) - 1) / 2;
		changey = (randint0(abs(dx) + 2) * 2 - abs(dx) - 1) / 2;

		/* Work out "mid" ponit */
		x3 = x1 + dx + changex;
		y3 = y1 + dy + changey;

		/* See if in bounds - if not - do not perturb point */
		if (!in_bounds(y3, x3))
		{
			x3 = (x1 + x2) / 2;
			y3 = (y1 + y2) / 2;
		}
		/* cache g_ptr */
		g_ptr = &current_floor_ptr->grid_array[y3][x3];
		if (is_solid_grid(g_ptr))
		{
			/* move midpoint a bit to avoid problem. */

			i = 50;

			dy = 0;
			dx = 0;
			while ((i > 0) && is_solid_bold(y3 + dy, x3 + dx))
			{
				dy = randint0(3) - 1;
				dx = randint0(3) - 1;
				if (!in_bounds(y3 + dy, x3 + dx))
				{
					dx = 0;
					dy = 0;
				}
				i--;
			}

			if (i == 0)
			{
				/* Failed for some reason: hack - ignore the solidness */
				place_outer_bold(y3, x3);
				dx = 0;
				dy = 0;
			}
			y3 += dy;
			x3 += dx;
			g_ptr = &current_floor_ptr->grid_array[y3][x3];
		}

		if (is_floor_grid(g_ptr))
		{
			if (build_tunnel2(x1, y1, x3, y3, type, cutoff))
			{
				if ((current_floor_ptr->grid_array[y3][x3].info & CAVE_ROOM) || (randint1(100) > 95))
				{
					/* do second half only if works + if have hit a room */
					retval = build_tunnel2(x3, y3, x2, y2, type, cutoff);
				}
				else
				{
					/* have hit another tunnel - make a set of doors here */
					retval = FALSE;

					/* Save the door location */
					if (dun->door_n < DOOR_MAX)
					{
						dun->door[dun->door_n].y = y3;
						dun->door[dun->door_n].x = x3;
						dun->door_n++;
					}
					else return FALSE;
				}
				firstsuccede = TRUE;
			}
			else
			{
				/* false- didn't work all the way */
				retval = FALSE;
				firstsuccede = FALSE;
			}
		}
		else
		{
			/* tunnel through walls */
			if (build_tunnel2(x1, y1, x3, y3, type, cutoff))
			{
				retval = build_tunnel2(x3, y3, x2, y2, type, cutoff);
				firstsuccede = TRUE;
			}
			else
			{
				/* false- didn't work all the way */
				retval = FALSE;
				firstsuccede = FALSE;
			}
		}
		if (firstsuccede)
		{
			/* only do this if the first half has worked */
			set_tunnel(&x3, &y3, TRUE);
		}
		/* return value calculated above */
		return retval;
	}
	else
	{
		/* Do a short segment */
		retval = TRUE;
		short_seg_hack(x1, y1, x2, y2, type, 0, &retval);

		/* Hack - ignore return value so avoid infinite loops */
		return TRUE;
	}
}

