﻿#include "angband.h"
#include "generate.h"
#include "grid.h"
#include "rooms.h"
#include "store.h"
#include "trap.h"
#include "monster.h"
#include "feature.h"

/*
* This function creates a random vault that looks like a collection of bubbles.
* It works by getting a set of coordinates that represent the center of each
* bubble.  The entire room is made by seeing which bubble center is closest. If
* two centers are equidistant then the square is a wall, otherwise it is a floor.
* The only exception is for squares really near a center, these are always floor.
* (It looks better than without this check.)
*
* Note: If two centers are on the same point then this algorithm will create a
*       blank bubble filled with walls. - This is prevented from happening.
*/
static void build_bubble_vault(POSITION x0, POSITION y0, POSITION xsize, POSITION ysize)
{
#define BUBBLENUM 10		/* number of bubbles */

	/* array of center points of bubbles */
	coord center[BUBBLENUM];

	int i, j;
	POSITION x = 0, y = 0;
	u16b min1, min2, temp;
	bool done;

	/* Offset from center to top left hand corner */
	POSITION xhsize = xsize / 2;
	POSITION yhsize = ysize / 2;

	msg_print_wizard(CHEAT_DUNGEON, _("泡型ランダムVaultを生成しました。", "Room Vault."));

	/* Allocate center of bubbles */
	center[0].x = (byte)randint1(xsize - 3) + 1;
	center[0].y = (byte)randint1(ysize - 3) + 1;

	for (i = 1; i < BUBBLENUM; i++)
	{
		done = FALSE;

		/* get center and check to see if it is unique */
		while (!done)
		{
			done = TRUE;

			x = randint1(xsize - 3) + 1;
			y = randint1(ysize - 3) + 1;

			for (j = 0; j < i; j++)
			{
				/* rough test to see if there is an overlap */
				if ((x == center[j].x) && (y == center[j].y)) done = FALSE;
			}
		}

		center[i].x = x;
		center[i].y = y;
	}


	/* Top and bottom boundaries */
	for (i = 0; i < xsize; i++)
	{
		int side_x = x0 - xhsize + i;

		place_outer_noperm_bold(y0 - yhsize + 0, side_x);
		current_floor_ptr->grid_array[y0 - yhsize + 0][side_x].info |= (CAVE_ROOM | CAVE_ICKY);
		place_outer_noperm_bold(y0 - yhsize + ysize - 1, side_x);
		current_floor_ptr->grid_array[y0 - yhsize + ysize - 1][side_x].info |= (CAVE_ROOM | CAVE_ICKY);
	}

	/* Left and right boundaries */
	for (i = 1; i < ysize - 1; i++)
	{
		int side_y = y0 - yhsize + i;

		place_outer_noperm_bold(side_y, x0 - xhsize + 0);
		current_floor_ptr->grid_array[side_y][x0 - xhsize + 0].info |= (CAVE_ROOM | CAVE_ICKY);
		place_outer_noperm_bold(side_y, x0 - xhsize + xsize - 1);
		current_floor_ptr->grid_array[side_y][x0 - xhsize + xsize - 1].info |= (CAVE_ROOM | CAVE_ICKY);
	}

	/* Fill in middle with bubbles */
	for (x = 1; x < xsize - 1; x++)
	{
		for (y = 1; y < ysize - 1; y++)
		{
			/* Get distances to two closest centers */

			min1 = (u16b)distance(x, y, center[0].x, center[0].y);
			min2 = (u16b)distance(x, y, center[1].x, center[1].y);

			if (min1 > min2)
			{
				/* swap if in wrong order */
				temp = min1;
				min1 = min2;
				min2 = temp;
			}

			/* Scan the rest */
			for (i = 2; i < BUBBLENUM; i++)
			{
				temp = (u16b)distance(x, y, center[i].x, center[i].y);

				if (temp < min1)
				{
					/* smallest */
					min2 = min1;
					min1 = temp;
				}
				else if (temp < min2)
				{
					/* second smallest */
					min2 = temp;
				}
			}
			if (((min2 - min1) <= 2) && (!(min1 < 3)))
			{
				/* Boundary at midpoint+ not at inner region of bubble */
				place_outer_noperm_bold(y0 - yhsize + y, x0 - xhsize + x);
			}
			else
			{
				/* middle of a bubble */
				place_floor_bold(y0 - yhsize + y, x0 - xhsize + x);
			}

			/* clean up rest of flags */
			current_floor_ptr->grid_array[y0 - yhsize + y][x0 - xhsize + x].info |= (CAVE_ROOM | CAVE_ICKY);
		}
	}

	/* Try to add some random doors */
	for (i = 0; i < 500; i++)
	{
		x = randint1(xsize - 3) - xhsize + x0 + 1;
		y = randint1(ysize - 3) - yhsize + y0 + 1;
		add_door(x, y);
	}

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(x0 - xhsize + 1, x0 - xhsize + xsize - 2, y0 - yhsize + 1, y0 - yhsize + ysize - 2, randint1(5));
}

/* Create a random vault that looks like a collection of overlapping rooms */
static void build_room_vault(POSITION x0, POSITION y0, POSITION xsize, POSITION ysize)
{
	POSITION x1, x2, y1, y2, xhsize, yhsize;
	int i;

	/* get offset from center */
	xhsize = xsize / 2;
	yhsize = ysize / 2;

	msg_print_wizard(CHEAT_DUNGEON, _("部屋型ランダムVaultを生成しました。", "Room Vault."));

	/* fill area so don't get problems with arena levels */
	for (x1 = 0; x1 < xsize; x1++)
	{
		POSITION x = x0 - xhsize + x1;

		for (y1 = 0; y1 < ysize; y1++)
		{
			POSITION y = y0 - yhsize + y1;

			place_extra_bold(y, x);
			current_floor_ptr->grid_array[y][x].info &= (~CAVE_ICKY);
		}
	}

	/* add ten random rooms */
	for (i = 0; i < 10; i++)
	{
		x1 = randint1(xhsize) * 2 + x0 - xhsize;
		x2 = randint1(xhsize) * 2 + x0 - xhsize;
		y1 = randint1(yhsize) * 2 + y0 - yhsize;
		y2 = randint1(yhsize) * 2 + y0 - yhsize;
		build_room(x1, x2, y1, y2);
	}

	/* Add some random doors */
	for (i = 0; i < 500; i++)
	{
		x1 = randint1(xsize - 3) - xhsize + x0 + 1;
		y1 = randint1(ysize - 3) - yhsize + y0 + 1;
		add_door(x1, y1);
	}

	/* Fill with monsters and treasure, high difficulty */
	fill_treasure(x0 - xhsize + 1, x0 - xhsize + xsize - 2, y0 - yhsize + 1, y0 - yhsize + ysize - 2, randint1(5) + 5);
}


/* Create a random vault out of a fractal current_floor_ptr->grid_array */
static void build_cave_vault(POSITION x0, POSITION y0, POSITION xsiz, POSITION ysiz)
{
	int grd, roug, cutoff;
	bool done, light, room;
	POSITION xhsize, yhsize, xsize, ysize, x, y;

	/* round to make sizes even */
	xhsize = xsiz / 2;
	yhsize = ysiz / 2;
	xsize = xhsize * 2;
	ysize = yhsize * 2;

	msg_print_wizard(CHEAT_DUNGEON, _("洞穴ランダムVaultを生成しました。", "Cave Vault."));

	light = done = FALSE;
	room = TRUE;

	while (!done)
	{
		/* testing values for these parameters feel free to adjust */
		grd = 1 << randint0(4);

		/* want average of about 16 */
		roug = randint1(8) * randint1(4);

		/* about size/2 */
		cutoff = randint1(xsize / 4) + randint1(ysize / 4) +
			randint1(xsize / 4) + randint1(ysize / 4);

		/* make it */
		generate_hmap(y0, x0, xsize, ysize, grd, roug, cutoff);

		/* Convert to normal format+ clean up */
		done = generate_fracave(y0, x0, xsize, ysize, cutoff, light, room);
	}

	/* Set icky flag because is a vault */
	for (x = 0; x <= xsize; x++)
	{
		for (y = 0; y <= ysize; y++)
		{
			current_floor_ptr->grid_array[y0 - yhsize + y][x0 - xhsize + x].info |= CAVE_ICKY;
		}
	}

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(x0 - xhsize + 1, x0 - xhsize + xsize - 1, y0 - yhsize + 1, y0 - yhsize + ysize - 1, randint1(5));
}



/*!
* @brief Vault地形を回転、上下左右反転するための座標変換を返す / coordinate translation code
* @param x 変換したい点のX座標参照ポインタ
* @param y 変換したい点のY座標参照ポインタ
* @param xoffset Vault生成時の基準X座標
* @param yoffset Vault生成時の基準Y座標
* @param transno 処理ID
* @return なし
*/
static void coord_trans(POSITION *x, POSITION *y, POSITION xoffset, POSITION yoffset, int transno)
{
	int i;
	int temp;

	/*
	* transno specifies what transformation is required. (0-7)
	* The lower two bits indicate by how much the vault is rotated,
	* and the upper bit indicates a reflection.
	* This is done by using rotation matrices... however since
	* these are mostly zeros for rotations by 90 degrees this can
	* be expressed simply in terms of swapping and inverting the
	* x and y coordinates.
	*/
	for (i = 0; i < transno % 4; i++)
	{
		/* rotate by 90 degrees */
		temp = *x;
		*x = -(*y);
		*y = temp;
	}

	if (transno / 4)
	{
		/* Reflect depending on status of 3rd bit. */
		*x = -(*x);
	}

	/* Add offsets so vault stays in the first quadrant */
	*x += xoffset;
	*y += yoffset;
}


/*!
* @brief Vaultをフロアに配置する / Hack -- fill in "vault" rooms
* @param yval 生成基準Y座標
* @param xval 生成基準X座標
* @param ymax VaultのYサイズ
* @param xmax VaultのXサイズ
* @param data Vaultのデータ文字列
* @param xoffset 変換基準X座標
* @param yoffset 変換基準Y座標
* @param transno 変換ID
* @return なし
*/
static void build_vault(POSITION yval, POSITION xval, POSITION ymax, POSITION xmax, concptr data,
	POSITION xoffset, POSITION yoffset, int transno)
{
	POSITION dx, dy, x, y, i, j;
	concptr t;
	grid_type *g_ptr;

	/* Place dungeon features and objects */
	for (t = data, dy = 0; dy < ymax; dy++)
	{
		for (dx = 0; dx < xmax; dx++, t++)
		{
			/* prevent loop counter from being overwritten */
			i = dx;
			j = dy;

			/* Flip / rotate */
			coord_trans(&i, &j, xoffset, yoffset, transno);

			/* Extract the location */
			if (transno % 2 == 0)
			{
				/* no swap of x/y */
				x = xval - (xmax / 2) + i;
				y = yval - (ymax / 2) + j;
			}
			else
			{
				/* swap of x/y */
				x = xval - (ymax / 2) + i;
				y = yval - (xmax / 2) + j;
			}

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;
			g_ptr = &current_floor_ptr->grid_array[y][x];

			/* Lay down a floor */
			place_floor_grid(g_ptr);

			/* Remove any mimic */
			g_ptr->mimic = 0;

			/* Part of a vault */
			g_ptr->info |= (CAVE_ROOM | CAVE_ICKY);

			/* Analyze the grid */
			switch (*t)
			{
				/* Granite wall (outer) */
			case '%':
				place_outer_noperm_grid(g_ptr);
				break;

				/* Granite wall (inner) */
			case '#':
				place_inner_grid(g_ptr);
				break;

				/* Glass wall (inner) */
			case '$':
				place_inner_grid(g_ptr);
				g_ptr->feat = feat_glass_wall;
				break;

				/* Permanent wall (inner) */
			case 'X':
				place_inner_perm_grid(g_ptr);
				break;

				/* Permanent glass wall (inner) */
			case 'Y':
				place_inner_perm_grid(g_ptr);
				g_ptr->feat = feat_permanent_glass_wall;
				break;

				/* Treasure/trap */
			case '*':
				if (randint0(100) < 75)
				{
					place_object(y, x, 0L);
				}
				else
				{
					place_trap(y, x);
				}
				break;

				/* Treasure */
			case '[':
				place_object(y, x, 0L);
				break;

				/* Tree */
			case ':':
				g_ptr->feat = feat_tree;
				break;

				/* Secret doors */
			case '+':
				place_secret_door(y, x, DOOR_DEFAULT);
				break;

				/* Secret glass doors */
			case '-':
				place_secret_door(y, x, DOOR_GLASS_DOOR);
				if (is_closed_door(g_ptr->feat)) g_ptr->mimic = feat_glass_wall;
				break;

				/* Curtains */
			case '\'':
				place_secret_door(y, x, DOOR_CURTAIN);
				break;

				/* Trap */
			case '^':
				place_trap(y, x);
				break;

				/* Black market in a dungeon */
			case 'S':
				set_cave_feat(y, x, feat_black_market);
				store_init(NO_TOWN, STORE_BLACK);
				break;

				/* The Pattern */
			case 'p':
				set_cave_feat(y, x, feat_pattern_start);
				break;

			case 'a':
				set_cave_feat(y, x, feat_pattern_1);
				break;

			case 'b':
				set_cave_feat(y, x, feat_pattern_2);
				break;

			case 'c':
				set_cave_feat(y, x, feat_pattern_3);
				break;

			case 'd':
				set_cave_feat(y, x, feat_pattern_4);
				break;

			case 'P':
				set_cave_feat(y, x, feat_pattern_end);
				break;

			case 'B':
				set_cave_feat(y, x, feat_pattern_exit);
				break;

			case 'A':
				/* Reward for Pattern walk */
				current_floor_ptr->object_level = current_floor_ptr->base_level + 12;
				place_object(y, x, AM_GOOD | AM_GREAT);
				current_floor_ptr->object_level = current_floor_ptr->base_level;
				break;

			case '~':
				set_cave_feat(y, x, feat_shallow_water);
				break;

			case '=':
				set_cave_feat(y, x, feat_deep_water);
				break;

			case 'v':
				set_cave_feat(y, x, feat_shallow_lava);
				break;

			case 'w':
				set_cave_feat(y, x, feat_deep_lava);
				break;

			case 'f':
				set_cave_feat(y, x, feat_shallow_acid_puddle);
				break;

			case 'F':
				set_cave_feat(y, x, feat_deep_acid_puddle);
				break;

			case 'g':
				set_cave_feat(y, x, feat_shallow_poisonous_puddle);
				break;

			case 'G':
				set_cave_feat(y, x, feat_deep_poisonous_puddle);
				break;

			case 'h':
				set_cave_feat(y, x, feat_cold_zone);
				break;

			case 'H':
				set_cave_feat(y, x, feat_heavy_cold_zone);
				break;

			case 'i':
				set_cave_feat(y, x, feat_electrical_zone);
				break;

			case 'I':
				set_cave_feat(y, x, feat_heavy_electrical_zone);
				break;

			}
		}
	}


	/* Place dungeon monsters and objects */
	for (t = data, dy = 0; dy < ymax; dy++)
	{
		for (dx = 0; dx < xmax; dx++, t++)
		{
			/* prevent loop counter from being overwritten */
			i = dx;
			j = dy;

			/* Flip / rotate */
			coord_trans(&i, &j, xoffset, yoffset, transno);

			/* Extract the location */
			if (transno % 2 == 0)
			{
				/* no swap of x/y */
				x = xval - (xmax / 2) + i;
				y = yval - (ymax / 2) + j;
			}
			else
			{
				/* swap of x/y */
				x = xval - (ymax / 2) + i;
				y = yval - (xmax / 2) + j;
			}

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Analyze the symbol */
			switch (*t)
			{
				case '&':
				{
					current_floor_ptr->monster_level = current_floor_ptr->base_level + 5;
					place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
					current_floor_ptr->monster_level = current_floor_ptr->base_level;
					break;
				}

				/* Meaner monster */
				case '@':
				{
					current_floor_ptr->monster_level = current_floor_ptr->base_level + 11;
					place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
					current_floor_ptr->monster_level = current_floor_ptr->base_level;
					break;
				}

				/* Meaner monster, plus treasure */
				case '9':
				{
					current_floor_ptr->monster_level = current_floor_ptr->base_level + 9;
					place_monster(y, x, PM_ALLOW_SLEEP);
					current_floor_ptr->monster_level = current_floor_ptr->base_level;
					current_floor_ptr->object_level = current_floor_ptr->base_level + 7;
					place_object(y, x, AM_GOOD);
					current_floor_ptr->object_level = current_floor_ptr->base_level;
					break;
				}

				/* Nasty monster and treasure */
				case '8':
				{
					current_floor_ptr->monster_level = current_floor_ptr->base_level + 40;
					place_monster(y, x, PM_ALLOW_SLEEP);
					current_floor_ptr->monster_level = current_floor_ptr->base_level;
					current_floor_ptr->object_level = current_floor_ptr->base_level + 20;
					place_object(y, x, AM_GOOD | AM_GREAT);
					current_floor_ptr->object_level = current_floor_ptr->base_level;
					break;
				}

				/* Monster and/or object */
				case ',':
				{
					if (randint0(100) < 50)
					{
						current_floor_ptr->monster_level = current_floor_ptr->base_level + 3;
						place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
						current_floor_ptr->monster_level = current_floor_ptr->base_level;
					}
					if (randint0(100) < 50)
					{
						current_floor_ptr->object_level = current_floor_ptr->base_level + 7;
						place_object(y, x, 0L);
						current_floor_ptr->object_level = current_floor_ptr->base_level;
					}
					break;
				}

			}
		}
	}
}



/*!
* @brief タイプ7の部屋…v_info.txtより小型vaultを生成する / Type 7 -- simple vaults (see "v_info.txt")
* @return なし
*/
bool build_type7(void)
{
	vault_type *v_ptr = NULL;
	int dummy;
	POSITION x, y;
	POSITION xval, yval;
	POSITION xoffset, yoffset;
	int transno;

	/* Pick a lesser vault */
	for (dummy = 0; dummy < SAFE_MAX_ATTEMPTS; dummy++)
	{
		/* Access a random vault record */
		v_ptr = &v_info[randint0(max_v_idx)];

		/* Accept the first lesser vault */
		if (v_ptr->typ == 7) break;
	}

	/* No lesser vault found */
	if (dummy >= SAFE_MAX_ATTEMPTS)
	{
		msg_print_wizard(CHEAT_DUNGEON, _("小型固定Vaultを配置できませんでした。", "Could not place lesser vault."));
		return FALSE;
	}

	/* pick type of transformation (0-7) */
	transno = randint0(8);

	/* calculate offsets */
	x = v_ptr->wid;
	y = v_ptr->hgt;

	/* Some huge vault cannot be ratated to fit in the dungeon */
	if (x + 2 > current_floor_ptr->height - 2)
	{
		/* Forbid 90 or 270 degree ratation */
		transno &= ~1;
	}

	coord_trans(&x, &y, 0, 0, transno);

	if (x < 0)
	{
		xoffset = -x - 1;
	}
	else
	{
		xoffset = 0;
	}

	if (y < 0)
	{
		yoffset = -y - 1;
	}
	else
	{
		yoffset = 0;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&yval, &xval, abs(y), abs(x))) return FALSE;

#ifdef FORCE_V_IDX
	v_ptr = &v_info[2];
#endif

	msg_format_wizard(CHEAT_DUNGEON, _("小型Vault(%s)を生成しました。", "Lesser vault (%s)."), v_name + v_ptr->name);

	/* Hack -- Build the vault */
	build_vault(yval, xval, v_ptr->hgt, v_ptr->wid,
		v_text + v_ptr->text, xoffset, yoffset, transno);

	return TRUE;
}

/*!
* @brief タイプ8の部屋…v_info.txtより大型vaultを生成する / Type 8 -- greater vaults (see "v_info.txt")
* @return なし
*/
bool build_type8(void)
{
	vault_type *v_ptr;
	int dummy;
	POSITION xval, yval;
	POSITION x, y;
	int transno;
	POSITION xoffset, yoffset;

	/* Pick a greater vault */
	for (dummy = 0; dummy < SAFE_MAX_ATTEMPTS; dummy++)
	{
		/* Access a random vault record */
		v_ptr = &v_info[randint0(max_v_idx)];

		/* Accept the first greater vault */
		if (v_ptr->typ == 8) break;
	}

	/* No greater vault found */
	if (dummy >= SAFE_MAX_ATTEMPTS)
	{
		msg_print_wizard(CHEAT_DUNGEON, _("大型固定Vaultを配置できませんでした。", "Could not place greater vault."));
		return FALSE;
	}

	/* pick type of transformation (0-7) */
	transno = randint0(8);

	/* calculate offsets */
	x = v_ptr->wid;
	y = v_ptr->hgt;

	/* Some huge vault cannot be ratated to fit in the dungeon */
	if (x + 2 > current_floor_ptr->height - 2)
	{
		/* Forbid 90 or 270 degree ratation */
		transno &= ~1;
	}

	coord_trans(&x, &y, 0, 0, transno);

	if (x < 0)
	{
		xoffset = -x - 1;
	}
	else
	{
		xoffset = 0;
	}

	if (y < 0)
	{
		yoffset = -y - 1;
	}
	else
	{
		yoffset = 0;
	}

	/*
	* Try to allocate space for room.  If fails, exit
	*
	* Hack -- Prepare a bit larger space (+2, +2) to
	* prevent generation of vaults with no-entrance.
	*/
	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&yval, &xval, (POSITION)(abs(y) + 2), (POSITION)(abs(x) + 2))) return FALSE;

#ifdef FORCE_V_IDX
	v_ptr = &v_info[76 + randint1(3)];
#endif

	msg_format_wizard(CHEAT_DUNGEON, _("大型固定Vault(%s)を生成しました。", "Greater vault (%s)."), v_name + v_ptr->name);

	/* Hack -- Build the vault */
	build_vault(yval, xval, v_ptr->hgt, v_ptr->wid,
		v_text + v_ptr->text, xoffset, yoffset, transno);

	return TRUE;
}


/*
* Build target vault.
* This is made by two concentric "crypts" with perpendicular
* walls creating the cross-hairs.
*/
static void build_target_vault(POSITION x0, POSITION y0, POSITION xsize, POSITION ysize)
{
	POSITION rad, x, y;

	/* Make a random metric */
	POSITION h1, h2, h3, h4;
	h1 = randint1(32) - 16;
	h2 = randint1(16);
	h3 = randint1(32);
	h4 = randint1(32) - 16;

	msg_print_wizard(CHEAT_DUNGEON, _("対称形ランダムVaultを生成しました。", "Elemental Vault"));

	/* work out outer radius */
	if (xsize > ysize)
	{
		rad = ysize / 2;
	}
	else
	{
		rad = xsize / 2;
	}

	/* Make floor */
	for (x = x0 - rad; x <= x0 + rad; x++)
	{
		for (y = y0 - rad; y <= y0 + rad; y++)
		{
			/* clear room flag */
			current_floor_ptr->grid_array[y][x].info &= ~(CAVE_ROOM);

			/* Vault - so is "icky" */
			current_floor_ptr->grid_array[y][x].info |= CAVE_ICKY;

			if (dist2(y0, x0, y, x, h1, h2, h3, h4) <= rad - 1)
			{
				/* inside- so is floor */
				place_floor_bold(y, x);
			}
			else
			{
				/* make granite outside so arena works */
				place_extra_bold(y, x);
			}

			/* proper boundary for arena */
			if (((y + rad) == y0) || ((y - rad) == y0) ||
				((x + rad) == x0) || ((x - rad) == x0))
			{
				place_extra_bold(y, x);
			}
		}
	}

	/* Find visible outer walls and set to be FEAT_OUTER */
	add_outer_wall(x0, y0, FALSE, x0 - rad - 1, y0 - rad - 1,
		x0 + rad + 1, y0 + rad + 1);

	/* Add inner wall */
	for (x = x0 - rad / 2; x <= x0 + rad / 2; x++)
	{
		for (y = y0 - rad / 2; y <= y0 + rad / 2; y++)
		{
			if (dist2(y0, x0, y, x, h1, h2, h3, h4) == rad / 2)
			{
				/* Make an internal wall */
				place_inner_bold(y, x);
			}
		}
	}

	/* Add perpendicular walls */
	for (x = x0 - rad; x <= x0 + rad; x++)
	{
		place_inner_bold(y0, x);
	}

	for (y = y0 - rad; y <= y0 + rad; y++)
	{
		place_inner_bold(y, x0);
	}

	/* Make inner vault */
	for (y = y0 - 1; y <= y0 + 1; y++)
	{
		place_inner_bold(y, x0 - 1);
		place_inner_bold(y, x0 + 1);
	}
	for (x = x0 - 1; x <= x0 + 1; x++)
	{
		place_inner_bold(y0 - 1, x);
		place_inner_bold(y0 + 1, x);
	}

	place_floor_bold(y0, x0);


	/* Add doors to vault */
	/* get two distances so can place doors relative to centre */
	x = (rad - 2) / 4 + 1;
	y = rad / 2 + x;

	add_door(x0 + x, y0);
	add_door(x0 + y, y0);
	add_door(x0 - x, y0);
	add_door(x0 - y, y0);
	add_door(x0, y0 + x);
	add_door(x0, y0 + y);
	add_door(x0, y0 - x);
	add_door(x0, y0 - y);

	/* Fill with stuff - medium difficulty */
	fill_treasure(x0 - rad, x0 + rad, y0 - rad, y0 + rad, randint1(3) + 3);
}


#ifdef ALLOW_CAVERNS_AND_LAKES
/*
* This routine uses a modified version of the lake code to make a
* distribution of some terrain type over the vault.  This type
* depends on the dungeon depth.
*
* Miniture rooms are then scattered across the vault.
*/
static void build_elemental_vault(POSITION x0, POSITION y0, POSITION xsiz, POSITION ysiz)
{
	int grd, roug;
	int c1, c2, c3;
	bool done = FALSE;
	POSITION xsize, ysize, xhsize, yhsize, x, y;
	int i;
	int type;

	msg_print_wizard(CHEAT_DUNGEON, _("精霊界ランダムVaultを生成しました。", "Elemental Vault"));

	/* round to make sizes even */
	xhsize = xsiz / 2;
	yhsize = ysiz / 2;
	xsize = xhsize * 2;
	ysize = yhsize * 2;

	if (current_floor_ptr->dun_level < 25)
	{
		/* Earth vault  (Rubble) */
		type = LAKE_T_EARTH_VAULT;
	}
	else if (current_floor_ptr->dun_level < 50)
	{
		/* Air vault (Trees) */
		type = LAKE_T_AIR_VAULT;
	}
	else if (current_floor_ptr->dun_level < 75)
	{
		/* Water vault (shallow water) */
		type = LAKE_T_WATER_VAULT;
	}
	else
	{
		/* Fire vault (shallow lava) */
		type = LAKE_T_FIRE_VAULT;
	}

	while (!done)
	{
		/* testing values for these parameters: feel free to adjust */
		grd = 1 << (randint0(3));

		/* want average of about 16 */
		roug = randint1(8) * randint1(4);

		/* Make up size of various componants */
		/* Floor */
		c3 = 2 * xsize / 3;

		/* Deep water/lava */
		c1 = randint0(c3 / 2) + randint0(c3 / 2) - 5;

		/* Shallow boundary */
		c2 = (c1 + c3) / 2;

		/* make it */
		generate_hmap(y0, x0, xsize, ysize, grd, roug, c3);

		/* Convert to normal format+ clean up */
		done = generate_lake(y0, x0, xsize, ysize, c1, c2, c3, type);
	}

	/* Set icky flag because is a vault */
	for (x = 0; x <= xsize; x++)
	{
		for (y = 0; y <= ysize; y++)
		{
			current_floor_ptr->grid_array[y0 - yhsize + y][x0 - xhsize + x].info |= CAVE_ICKY;
		}
	}

	/* make a few rooms in the vault */
	for (i = 1; i <= (xsize * ysize) / 50; i++)
	{
		build_small_room(x0 + randint0(xsize - 4) - xsize / 2 + 2,
			y0 + randint0(ysize - 4) - ysize / 2 + 2);
	}

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(x0 - xhsize + 1, x0 - xhsize + xsize - 1,
		y0 - yhsize + 1, y0 - yhsize + ysize - 1, randint1(5));
}
#endif /* ALLOW_CAVERNS_AND_LAKES */


/* Build a "mini" checkerboard vault
*
* This is done by making a permanent wall maze and setting
* the diagonal sqaures of the checker board to be granite.
* The vault has two entrances on opposite sides to guarantee
* a way to get in even if the vault abuts a side of the dungeon.
*/
static void build_mini_c_vault(POSITION x0, POSITION y0, POSITION xsize, POSITION ysize)
{
	POSITION dy, dx;
	POSITION y1, x1, y2, x2, y, x, total;
	int m, n, num_vertices;
	int *visited;

	msg_print_wizard(CHEAT_DUNGEON, _("小型チェッカーランダムVaultを生成しました。", "Mini Checker Board Vault."));

	/* Pick a random room size */
	dy = ysize / 2 - 1;
	dx = xsize / 2 - 1;

	y1 = y0 - dy;
	x1 = x0 - dx;
	y2 = y0 + dy;
	x2 = x0 + dx;


	/* generate the room */
	for (x = x1 - 2; x <= x2 + 2; x++)
	{
		if (!in_bounds(y1 - 2, x)) break;

		current_floor_ptr->grid_array[y1 - 2][x].info |= (CAVE_ROOM | CAVE_ICKY);

		place_outer_noperm_bold(y1 - 2, x);
	}

	for (x = x1 - 2; x <= x2 + 2; x++)
	{
		if (!in_bounds(y2 + 2, x)) break;

		current_floor_ptr->grid_array[y2 + 2][x].info |= (CAVE_ROOM | CAVE_ICKY);

		place_outer_noperm_bold(y2 + 2, x);
	}

	for (y = y1 - 2; y <= y2 + 2; y++)
	{
		if (!in_bounds(y, x1 - 2)) break;

		current_floor_ptr->grid_array[y][x1 - 2].info |= (CAVE_ROOM | CAVE_ICKY);

		place_outer_noperm_bold(y, x1 - 2);
	}

	for (y = y1 - 2; y <= y2 + 2; y++)
	{
		if (!in_bounds(y, x2 + 2)) break;

		current_floor_ptr->grid_array[y][x2 + 2].info |= (CAVE_ROOM | CAVE_ICKY);

		place_outer_noperm_bold(y, x2 + 2);
	}

	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

			g_ptr->info |= (CAVE_ROOM | CAVE_ICKY);

			/* Permanent walls */
			place_inner_perm_grid(g_ptr);
		}
	}


	/* dimensions of vertex array */
	m = dx + 1;
	n = dy + 1;
	num_vertices = m * n;

	/* initialize array of visited vertices */
	C_MAKE(visited, num_vertices, int);

	/* traverse the graph to create a spannng tree, pick a random root */
	r_visit(y1, x1, y2, x2, randint0(num_vertices), 0, visited);

	/* Make it look like a checker board vault */
	for (x = x1; x <= x2; x++)
	{
		for (y = y1; y <= y2; y++)
		{
			total = x - x1 + y - y1;
			/* If total is odd- and is a floor then make a wall */
			if ((total % 2 == 1) && is_floor_bold(y, x))
			{
				place_inner_bold(y, x);
			}
		}
	}

	/* Make a couple of entrances */
	if (one_in_(2))
	{
		/* left and right */
		y = randint1(dy) + dy / 2;
		place_inner_bold(y1 + y, x1 - 1);
		place_inner_bold(y1 + y, x2 + 1);
	}
	else
	{
		/* top and bottom */
		x = randint1(dx) + dx / 2;
		place_inner_bold(y1 - 1, x1 + x);
		place_inner_bold(y2 + 1, x1 + x);
	}

	/* Fill with monsters and treasure, highest difficulty */
	fill_treasure(x1, x2, y1, y2, 10);

	C_KILL(visited, num_vertices, int);
}

/* Build a castle */
/* Driver routine: clear the region and call the recursive
* room routine.
*
*This makes a vault that looks like a castle/ city in the dungeon.
*/
static void build_castle_vault(POSITION x0, POSITION y0, POSITION xsize, POSITION ysize)
{
	POSITION dy, dx;
	POSITION y1, x1, y2, x2;
	POSITION y, x;

	/* Pick a random room size */
	dy = ysize / 2 - 1;
	dx = xsize / 2 - 1;

	y1 = y0 - dy;
	x1 = x0 - dx;
	y2 = y0 + dy;
	x2 = x0 + dx;

	msg_print_wizard(CHEAT_DUNGEON, _("城型ランダムVaultを生成しました。", "Castle Vault"));

	/* generate the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			current_floor_ptr->grid_array[y][x].info |= (CAVE_ROOM | CAVE_ICKY);
			/* Make everything a floor */
			place_floor_bold(y, x);
		}
	}

	/* Make the castle */
	build_recursive_room(x1, y1, x2, y2, randint1(5));

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(x1, x2, y1, y2, randint1(3));
}



/*!
* @brief タイプ10の部屋…ランダム生成vault / Type 10 -- Random vaults
* @return なし
*/
bool build_type10(void)
{
	POSITION y0, x0, xsize, ysize, vtype;

	/* big enough to look good, small enough to be fairly common. */
	xsize = randint1(22) + 22;
	ysize = randint1(11) + 11;

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, ysize + 1, xsize + 1)) return FALSE;

	/* Select type of vault */
#ifdef ALLOW_CAVERNS_AND_LAKES
	do
	{
		vtype = randint1(15);
	} while ((d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_CAVE) &&
		((vtype == 1) || (vtype == 3) || (vtype == 8) || (vtype == 9) || (vtype == 11)));
#else /* ALLOW_CAVERNS_AND_LAKES */
	do
	{
		vtype = randint1(7);
	} while ((d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_CAVE) &&
		((vtype == 1) || (vtype == 3)));
#endif /* ALLOW_CAVERNS_AND_LAKES */

	switch (vtype)
	{
		/* Build an appropriate room */
	case 1: case  9: build_bubble_vault(x0, y0, xsize, ysize); break;
	case 2: case 10: build_room_vault(x0, y0, xsize, ysize); break;
	case 3: case 11: build_cave_vault(x0, y0, xsize, ysize); break;
	case 4: case 12: build_maze_vault(x0, y0, xsize, ysize, TRUE); break;
	case 5: case 13: build_mini_c_vault(x0, y0, xsize, ysize); break;
	case 6: case 14: build_castle_vault(x0, y0, xsize, ysize); break;
	case 7: case 15: build_target_vault(x0, y0, xsize, ysize); break;
#ifdef ALLOW_CAVERNS_AND_LAKES
	case 8: build_elemental_vault(x0, y0, xsize, ysize); break;
#endif /* ALLOW_CAVERNS_AND_LAKES */
		/* I know how to add a few more... give me some time. */
	default: return FALSE;
	}

	return TRUE;
}


/*!
* @brief タイプ17の部屋…v_info.txtより固定特殊部屋を生成する / Type 17 -- fixed special room (see "v_info.txt")
* @return なし
*/
bool build_type17(void)
{
	vault_type *v_ptr = NULL;
	int dummy;
	POSITION x, y;
	POSITION xval, yval;
	POSITION xoffset, yoffset;
	int transno;

	/* Pick a lesser vault */
	for (dummy = 0; dummy < SAFE_MAX_ATTEMPTS; dummy++)
	{
		/* Access a random vault record */
		v_ptr = &v_info[randint0(max_v_idx)];

		/* Accept the special fix room. */
		if (v_ptr->typ == 17) break;
	}

	/* No lesser vault found */
	if (dummy >= SAFE_MAX_ATTEMPTS)
	{
		msg_print_wizard(CHEAT_DUNGEON, _("固定特殊部屋を配置できませんでした。", "Could not place fixed special room."));
		return FALSE;
	}

	/* pick type of transformation (0-7) */
	transno = randint0(8);

	/* calculate offsets */
	x = v_ptr->wid;
	y = v_ptr->hgt;

	/* Some huge vault cannot be ratated to fit in the dungeon */
	if (x + 2 > current_floor_ptr->height - 2)
	{
		/* Forbid 90 or 270 degree ratation */
		transno &= ~1;
	}

	coord_trans(&x, &y, 0, 0, transno);

	if (x < 0)
	{
		xoffset = -x - 1;
	}
	else
	{
		xoffset = 0;
	}

	if (y < 0)
	{
		yoffset = -y - 1;
	}
	else
	{
		yoffset = 0;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&yval, &xval, abs(y), abs(x))) return FALSE;

#ifdef FORCE_V_IDX
	v_ptr = &v_info[2];
#endif

	msg_format_wizard(CHEAT_DUNGEON, _("特殊固定部屋(%s)を生成しました。", "Special Fixed Room (%s)."), v_name + v_ptr->name);

	/* Hack -- Build the vault */
	build_vault(yval, xval, v_ptr->hgt, v_ptr->wid,
		v_text + v_ptr->text, xoffset, yoffset, transno);

	return TRUE;
}

