﻿#include "angband.h"
#include "grid.h"
#include "generate.h"
#include "rooms.h"
#include "rooms-city.h"
#include "store.h"


ugbldg_type *ugbldg = NULL;


/*
* Precalculate buildings' location of underground arcade
*/
static bool precalc_ugarcade(int town_hgt, int town_wid, int n)
{
	POSITION i, y, x, center_y, center_x;
	int tmp, attempt = 10000;
	POSITION max_bldg_hgt = 3 * town_hgt / MAX_TOWN_HGT;
	POSITION max_bldg_wid = 5 * town_wid / MAX_TOWN_WID;
	ugbldg_type *cur_ugbldg;
	bool **ugarcade_used, abort;

	/* Allocate "ugarcade_used" array (2-dimension) */
	C_MAKE(ugarcade_used, town_hgt, bool *);
	C_MAKE(*ugarcade_used, town_hgt * town_wid, bool);
	for (y = 1; y < town_hgt; y++) ugarcade_used[y] = *ugarcade_used + y * town_wid;

	/* Calculate building locations */
	for (i = 0; i < n; i++)
	{
		cur_ugbldg = &ugbldg[i];
		(void)WIPE(cur_ugbldg, ugbldg_type);

		do
		{
			/* Find the "center" of the store */
			center_y = rand_range(2, town_hgt - 3);
			center_x = rand_range(2, town_wid - 3);

			/* Determine the store boundaries */
			tmp = center_y - randint1(max_bldg_hgt);
			cur_ugbldg->y0 = MAX(tmp, 1);
			tmp = center_x - randint1(max_bldg_wid);
			cur_ugbldg->x0 = MAX(tmp, 1);
			tmp = center_y + randint1(max_bldg_hgt);
			cur_ugbldg->y1 = MIN(tmp, town_hgt - 2);
			tmp = center_x + randint1(max_bldg_wid);
			cur_ugbldg->x1 = MIN(tmp, town_wid - 2);

			/* Scan this building's area */
			for (abort = FALSE, y = cur_ugbldg->y0; (y <= cur_ugbldg->y1) && !abort; y++)
			{
				for (x = cur_ugbldg->x0; x <= cur_ugbldg->x1; x++)
				{
					if (ugarcade_used[y][x])
					{
						abort = TRUE;
						break;
					}
				}
			}

			attempt--;
		} while (abort && attempt); /* Accept this building if no overlapping */

									/* Failed to generate underground arcade */
		if (!attempt) break;

		/*
		* Mark to ugarcade_used[][] as "used"
		* Note: Building-adjacent grids are included for preventing
		* connected bulidings.
		*/
		for (y = cur_ugbldg->y0 - 1; y <= cur_ugbldg->y1 + 1; y++)
		{
			for (x = cur_ugbldg->x0 - 1; x <= cur_ugbldg->x1 + 1; x++)
			{
				ugarcade_used[y][x] = TRUE;
			}
		}
	}

	/* Free "ugarcade_used" array (2-dimension) */
	C_KILL(*ugarcade_used, town_hgt * town_wid, bool);
	C_KILL(ugarcade_used, town_hgt, bool *);

	/* If i < n, generation is not allowed */
	return i == n;
}


/*!
* @brief タイプ16の部屋…地下都市生成のサブルーチン / Actually create buildings
* @return なし
* @param ltcy 生成基準Y座標
* @param ltcx 生成基準X座標
* @param stotes[] 生成する店舗のリスト
* @param n 生成する店舗の数
* @note
* Note: ltcy and ltcx indicate "left top corner".
*/
static void build_stores(POSITION ltcy, POSITION ltcx, int stores[], int n)
{
	int i;
	POSITION y, x;
	FEAT_IDX j;
	ugbldg_type *cur_ugbldg;

	for (i = 0; i < n; i++)
	{
		cur_ugbldg = &ugbldg[i];

		/* Generate new room */
		generate_room_floor(
			ltcy + cur_ugbldg->y0 - 2, ltcx + cur_ugbldg->x0 - 2,
			ltcy + cur_ugbldg->y1 + 2, ltcx + cur_ugbldg->x1 + 2,
			FALSE);
	}

	for (i = 0; i < n; i++)
	{
		cur_ugbldg = &ugbldg[i];

		/* Build an invulnerable rectangular building */
		generate_fill_perm_bold(
			ltcy + cur_ugbldg->y0, ltcx + cur_ugbldg->x0,
			ltcy + cur_ugbldg->y1, ltcx + cur_ugbldg->x1);

		/* Pick a door direction (S,N,E,W) */
		switch (randint0(4))
		{
			/* Bottom side */
		case 0:
			y = cur_ugbldg->y1;
			x = rand_range(cur_ugbldg->x0, cur_ugbldg->x1);
			break;

			/* Top side */
		case 1:
			y = cur_ugbldg->y0;
			x = rand_range(cur_ugbldg->x0, cur_ugbldg->x1);
			break;

			/* Right side */
		case 2:
			y = rand_range(cur_ugbldg->y0, cur_ugbldg->y1);
			x = cur_ugbldg->x1;
			break;

			/* Left side */
		default:
			y = rand_range(cur_ugbldg->y0, cur_ugbldg->y1);
			x = cur_ugbldg->x0;
			break;
		}

		for (j = 0; j < max_f_idx; j++)
		{
			if (have_flag(f_info[j].flags, FF_STORE))
			{
				if (f_info[j].subtype == stores[i]) break;
			}
		}

		/* Clear previous contents, add a store door */
		if (j < max_f_idx)
		{
			cave_set_feat(ltcy + y, ltcx + x, j);

			/* Init store */
			store_init(NO_TOWN, stores[i]);
		}
	}
}


/*!
* @brief タイプ16の部屋…地下都市の生成 / Type 16 -- Underground Arcade
* @return なし
* @details
* Town logic flow for generation of new town\n
* Originally from Vanilla 3.0.3\n
*\n
* We start with a fully wiped current_floor_ptr->grid_array of normal floors.\n
*\n
* Note that town_gen_hack() plays games with the R.N.G.\n
*\n
* This function does NOT do anything about the owners of the stores,\n
* nor the contents thereof.  It only handles the physical layout.\n
*/
bool build_type16(void)
{
	int stores[] =
	{
		STORE_GENERAL, STORE_ARMOURY, STORE_WEAPON, STORE_TEMPLE,
		STORE_ALCHEMIST, STORE_MAGIC, STORE_BLACK, STORE_BOOK,
	};
	int n = sizeof stores / sizeof(int);
	POSITION i, y, x, y1, x1, yval, xval;
	int town_hgt = rand_range(MIN_TOWN_HGT, MAX_TOWN_HGT);
	int town_wid = rand_range(MIN_TOWN_WID, MAX_TOWN_WID);
	bool prevent_bm = FALSE;

	/* Hack -- If already exist black market, prevent building */
	for (y = 0; (y < current_floor_ptr->height) && !prevent_bm; y++)
	{
		for (x = 0; x < current_floor_ptr->width; x++)
		{
			if (current_floor_ptr->grid_array[y][x].feat == FF_STORE)
			{
				prevent_bm = (f_info[current_floor_ptr->grid_array[y][x].feat].subtype == STORE_BLACK);
				break;
			}
		}
	}
	for (i = 0; i < n; i++)
	{
		if ((stores[i] == STORE_BLACK) && prevent_bm) stores[i] = stores[--n];
	}
	if (!n) return FALSE;

	/* Allocate buildings array */
	C_MAKE(ugbldg, n, ugbldg_type);

	/* If cannot build stores, abort */
	if (!precalc_ugarcade(town_hgt, town_wid, n))
	{
		/* Free buildings array */
		C_KILL(ugbldg, n, ugbldg_type);
		return FALSE;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&yval, &xval, town_hgt + 4, town_wid + 4))
	{
		/* Free buildings array */
		C_KILL(ugbldg, n, ugbldg_type);
		return FALSE;
	}

	/* Get top left corner */
	y1 = yval - (town_hgt / 2);
	x1 = xval - (town_wid / 2);

	/* Generate new room */
	generate_room_floor(
		y1 + town_hgt / 3, x1 + town_wid / 3,
		y1 + town_hgt * 2 / 3, x1 + town_wid * 2 / 3, FALSE);

	/* Build stores */
	build_stores(y1, x1, stores, n);

	msg_print_wizard(CHEAT_DUNGEON, _("地下街を生成しました", "Underground arcade was generated."));

	/* Free buildings array */
	C_KILL(ugbldg, n, ugbldg_type);

	return TRUE;
}

