/* File: floors.c */

/* Purpose: management of the saved floor */

/*
 * Copyright (c) 2002  Mogami
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"
#include "grid.h"


static s16b new_floor_id;       /* floor_id of the destination */
static u32b change_floor_mode;  /* Mode flags for changing floor */
static u32b latest_visit_mark;  /* Max number of visit_mark */


/*
 * Initialize saved_floors array.  Make sure that old temporal files
 * are not remaining as gurbages.
 */
void init_saved_floors(void)
{
	char floor_savefile[1024];
	int i;
	int fd = -1;
	int mode = 0644;
	bool force = FALSE;

#ifdef SET_UID
# ifdef SECURE
	/* Get "games" permissions */
	beGames();
# endif
#endif

	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];

		if (savefile[0]) {
			/* File name */
			sprintf(floor_savefile, "%s.F%02d", savefile, i);

			/* Grab permissions */
			safe_setuid_grab();

			/* Try to create the file */
			fd = fd_make(floor_savefile, mode);

			/* Drop permissions */
			safe_setuid_drop();

			/* Failed! */
			if (fd < 0)
			{
				if (!force)
				{
#ifdef JP
					msg_print("���顼���Ť��ƥ�ݥ�ꡦ�ե����뤬�ĤäƤ��ޤ���");
					msg_print("�Ѷ����ܤ���Ť˵�ư���Ƥ��ʤ�����ǧ���Ƥ���������");
					msg_print("�����Ѷ����ܤ�����å��夷�����ϰ���ե������");
					msg_print("����Ū�˺�����Ƽ¹Ԥ�³�����ޤ���");
					if (!get_check("����Ū�˺�����Ƥ��������Ǥ�����")) quit("�¹����");
#else
					msg_print("Error: There are old temporal files.");
					msg_print("Make sure you are not running two game processes simultaneously.");
					msg_print("If the temporal files are garbages of old crashed process, ");
					msg_print("you can delete it safely.");
					if (!get_check("Do you delete old temporal files? ")) quit("Aborted.");
#endif
					force = TRUE;
				}
			}
			else
			{
				/* Close the "fd" */
				(void)fd_close(fd);
			}

			/* Grab permissions */
			safe_setuid_grab();

			/* Simply kill the temporal file */
			(void)fd_kill(floor_savefile);

			/* Drop permissions */
			safe_setuid_drop();
		}

		sf_ptr->floor_id = 0;
	}

	/* No floor_id used yet (No.0 is reserved to indicate non existance) */
	max_floor_id = 1;

	/* vist_mark is from 1 */
	latest_visit_mark = 1;

	/* A sign to mark temporal files */
	saved_floor_file_sign = time(NULL);

	/* No next floor yet */
	new_floor_id = 0;

	/* No change floor mode yet */
	change_floor_mode = 0;

#ifdef SET_UID
# ifdef SECURE
	/* Drop "games" permissions */
	bePlayer();
# endif
#endif
}


/*
 * Kill temporal files
 * Should be called just before the game quit
 * and before new game discarding saved game.
 */
void clear_saved_floor_files(void)
{
	char floor_savefile[1024];
	int i;

#ifdef SET_UID
# ifdef SECURE
	/* Get "games" permissions */
	beGames();
# endif
#endif

	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];

		/* No temporal file */
		if (!sf_ptr->floor_id) continue;
		if (sf_ptr->floor_id == p_ptr->floor_id) continue;

		/* File name */
		sprintf(floor_savefile, "%s.F%02d", savefile, i);

		/* Grab permissions */
		safe_setuid_grab();

		/* Simply kill the temporal file */ 
		(void)fd_kill(floor_savefile);

		/* Drop permissions */
		safe_setuid_drop();
	}

#ifdef SET_UID
# ifdef SECURE
	/* Drop "games" permissions */
	bePlayer();
# endif
#endif
}


/*
 * Get a pointer for an item of the saved_floors array.
 */
saved_floor_type *get_sf_ptr(s16b floor_id)
{
	int i;

	/* floor_id No.0 indicates no floor */
	if (!floor_id) return NULL;

	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];

		if (sf_ptr->floor_id == floor_id) return sf_ptr;
	}

	/* None found */
	return NULL;
}


/*
 * kill a saved floor and get an empty space
 */
static void kill_saved_floor(saved_floor_type *sf_ptr)
{
	char floor_savefile[1024];

	/* Already empty */
	if (!sf_ptr->floor_id) return;

	if (sf_ptr->floor_id == p_ptr->floor_id)
	{
		/* Kill current floor */
		p_ptr->floor_id = 0;

		/* Current floor doesn't have temporal file */
	}
	else 
	{
		/* File name */
		sprintf(floor_savefile, "%s.F%02d", savefile, (int)sf_ptr->savefile_id);

		/* Grab permissions */
		safe_setuid_grab();

		/* Simply kill the temporal file */ 
		(void)fd_kill(floor_savefile);

		/* Drop permissions */
		safe_setuid_drop();
	}

	/* No longer exists */
	sf_ptr->floor_id = 0;
}


/*
 * Initialize new saved floor and get its floor id.  If number of
 * saved floors are already MAX_SAVED_FLOORS, kill the oldest one.
 */
s16b get_new_floor_id(void)
{
	saved_floor_type *sf_ptr;
	int i;

	/* Look for empty space */
	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		sf_ptr = &saved_floors[i];

		if (!sf_ptr->floor_id) break;
	}

	/* None found */
	if (i == MAX_SAVED_FLOORS)
	{
		int oldest = 0;
		u32b oldest_visit = 0xffffffffL;

		/* Search for oldest */
		for (i = 0; i < MAX_SAVED_FLOORS; i++)
		{
			sf_ptr = &saved_floors[i];

			/* Don't kill current floor */
			if (sf_ptr->floor_id == p_ptr->floor_id) continue;

			/* Don't kill newer */
			if (sf_ptr->visit_mark > oldest_visit) continue;

			oldest = i;
			oldest_visit = sf_ptr->visit_mark;
		}

		/* Kill oldest saved floor */
		sf_ptr = &saved_floors[oldest];
		kill_saved_floor(sf_ptr);

		/* Use it */
		i = oldest;
	}

	/* Prepare new floor data */
	sf_ptr->savefile_id = i;
	sf_ptr->floor_id = max_floor_id;
	sf_ptr->last_visit = 0;
	sf_ptr->upper_floor_id = 0;
	sf_ptr->lower_floor_id = 0;
	sf_ptr->visit_mark = latest_visit_mark++;

	/* sf_ptr->dun_level is not yet decided */


	/* Increment number of floor_id */
	if (max_floor_id < MAX_SHORT) max_floor_id++;

	/* 32767 floor_ids are all used up!  Re-use ancient IDs */
	else max_floor_id = 1;

	return sf_ptr->floor_id;
}


/*
 * Prepare mode flags of changing floor
 */
void prepare_change_floor_mode(u32b mode)
{
	change_floor_mode |= mode;
}


/*
 * Builds the dead end
 */
static void build_dead_end(void)
{
	int x,y;

	/* Clear and empty the cave */
	clear_cave();

	/* Fill the arrays of floors and walls in the good proportions */
	set_floor_and_wall(0);

	/* Smallest area */
	cur_hgt = SCREEN_HGT;
	cur_wid = SCREEN_WID;

	/* Filled with permanent walls */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			/* Create "solid" perma-wall */
			cave[y][x].feat = FEAT_PERM_SOLID;
		}
	}

	/* Place at center of the floor */
	py = cur_hgt / 2;
	px = cur_wid / 2;

	/* Give one square */
	place_floor_bold(py, px);

	wipe_generate_cave_flags();
}


/*
 * Preserve_pets
 */
static void preserve_pet(void)
{
	int num = 1, i;

	if (p_ptr->riding)
	{
		/* Paranoia - prevent loss of rided monster */
		for (num = 0; (num < MAX_PARTY_MON) && party_mon[num].r_idx; num++) /* Count forward */ ;
		if (num < MAX_PARTY_MON) COPY(&party_mon[num++], &m_list[p_ptr->riding], monster_type);

		/* Delete from this floor */
		delete_monster_idx(p_ptr->riding);
	}

	/*
	 * If player is in wild mode, no pets are preserved
	 * except a monster whom player riding
	 */
	if (!p_ptr->wild_mode)
	{
		for (i = m_max - 1; (i >= 1 && num < MAX_PARTY_MON); i--)
		{
			monster_type *m_ptr = &m_list[i];

			if (!m_ptr->r_idx) continue;
			if (!is_pet(m_ptr)) continue;
			if (i == p_ptr->riding) continue;

			if (reinit_wilderness || p_ptr->inside_arena || p_ptr->inside_battle)
			{
				/* Don't lose sight of pets when getting a Quest */
			}
			else
			{
				int dis = distance(py, px, m_ptr->fy, m_ptr->fx);

				/*
				 * Pets with nickname will follow even from 3 blocks away
				 * when you or the pet can see the other.
				 */
				if (m_ptr->nickname && 
				    (player_has_los_bold(m_ptr->fy, m_ptr->fx) ||
				     los(m_ptr->fy, m_ptr->fx, py, px)))
				{
					if (dis > 3) continue;
				}
				else
				{
					if (dis > 1) continue;
				}
				if (m_ptr->confused || m_ptr->stunned || m_ptr->csleep) continue;
			}

			for (; (num < MAX_PARTY_MON) && party_mon[num].r_idx; num++) /* Count forward */ ;
			if (num < MAX_PARTY_MON) COPY(&party_mon[num++], &m_list[i], monster_type);

			/* Delete from this floor */
			delete_monster_idx(i);
		}
	}

	if (record_named_pet)
	{
		for (i = m_max - 1; i >=1; i--)
		{
			monster_type *m_ptr = &m_list[i];
			char m_name[80];

			if (!m_ptr->r_idx) continue;
			if (!is_pet(m_ptr)) continue;
			if (!m_ptr->nickname) continue;
			if (p_ptr->riding == i) continue;

			monster_desc(m_name, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
			do_cmd_write_nikki(NIKKI_NAMED_PET, 4, m_name);
		}
	}
}


/*
 * Place preserved pet monsters on new floor
 */
static void place_pet(void)
{
	int i;
	int max_num = p_ptr->wild_mode ? 1 : MAX_PARTY_MON;

	for (i = 0; i < max_num; i++)
	{
		int cy, cx, m_idx;

		if (!(party_mon[i].r_idx)) continue;

		if (i == 0)
		{
			m_idx = m_pop();
			p_ptr->riding = m_idx;
			if (m_idx)
			{
				cy = py;
				cx = px;
			}
		}
		else
		{
			int j, d;

			/* Don't place in arena except rided one */
			if (p_ptr->inside_arena || p_ptr->inside_battle) continue;

			for (d = 1; d < 6; d++)
			{
				for (j = 1000; j > 0; j--)
				{
					scatter(&cy, &cx, py, px, d, 0);
					if ((cave_floor_bold(cy, cx) || (cave[cy][cx].feat == FEAT_TREES)) && !cave[cy][cx].m_idx && !player_bold(cy, cx)) break;
				}
				if (j) break;
			}
			m_idx = (d == 6) ? 0 : m_pop();
		}

		if (m_idx)
		{
			monster_type *m_ptr = &m_list[m_idx];
			monster_race *r_ptr;

			cave[cy][cx].m_idx = m_idx;

			m_ptr->r_idx = party_mon[i].r_idx;

			/* Copy all member of the structure */
			*m_ptr = party_mon[i];
			r_ptr = real_r_ptr(m_ptr);

			m_ptr->fy = cy;
			m_ptr->fx = cx;
			m_ptr->ml = TRUE;
			m_ptr->csleep = 0;

			/* Paranoia */
			m_ptr->hold_o_idx = 0;
			m_ptr->target_y = 0;

			if ((r_ptr->flags1 & RF1_FORCE_SLEEP) && !ironman_nightmare)
			{
				/* Monster is still being nice */
				m_ptr->mflag |= (MFLAG_NICE);

				/* Must repair monsters */
				repair_monsters = TRUE;
			}

			/* Update the monster */
			update_mon(m_idx, TRUE);
			lite_spot(cy, cx);

			/* Pre-calculated in precalc_cur_num_of_pet() */
			/* r_ptr->cur_num++; */

			/* Hack -- Count the number of "reproducers" */
			if (r_ptr->flags2 & RF2_MULTIPLY) num_repro++;

			/* Hack -- Notice new multi-hued monsters */
			{
				monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
				if (ap_r_ptr->flags1 & (RF1_ATTR_MULTI | RF1_SHAPECHANGER))
					shimmer_monsters = TRUE;
			}
		}
		else
		{
			monster_type *m_ptr = &party_mon[i];
			monster_race *r_ptr = real_r_ptr(m_ptr);
			char m_name[80];

			monster_desc(m_name, m_ptr, 0);
#ifdef JP
			msg_format("%s�ȤϤ���Ƥ��ޤä���", m_name);
#else
			msg_format("You have lost sight of %s.", m_name);
#endif
			if (record_named_pet && m_ptr->nickname)
			{
				monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
				do_cmd_write_nikki(NIKKI_NAMED_PET, 5, m_name);
			}

			/* Pre-calculated in precalc_cur_num_of_pet(), but need to decrease */
			if (r_ptr->cur_num) r_ptr->cur_num--;
		}

		/* For accuracy of precalc_cur_num_of_pet() */
		WIPE(&party_mon[i], monster_type);
	}
}


/*
 * Hack -- Update location of unique monsters and artifacts
 *
 * The r_ptr->floor_id and a_ptr->floor_id are not updated correctly
 * while new floor creation since dungeons may be re-created by
 * auto-scum option.
 */
static void update_unique_artifact(s16b cur_floor_id)
{
	int i;

	/* Maintain unique monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_race *r_ptr;
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Extract real monster race */
		r_ptr = real_r_ptr(m_ptr);

		/* Memorize location of the unique monster */
		if ((r_ptr->flags1 & RF1_UNIQUE) ||
		    (r_ptr->flags7 & RF7_NAZGUL))
		{
			r_ptr->floor_id = cur_floor_id;
		}
	}

	/* Maintain artifatcs */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Memorize location of the artifact */
		if (artifact_p(o_ptr))
		{
			a_info[o_ptr->name1].floor_id = cur_floor_id;
		}
	}
}


/*
 * When a monster is at a place where player will return,
 * Get out of the my way!
 */
static void get_out_monster(void)
{
	int tries = 0;
	int dis = 1;
	int oy = py;
	int ox = px;
	int m_idx = cave[oy][ox].m_idx;

	/* Nothing to do if no monster */
	if (!m_idx) return;

	/* Look until done */
	while (TRUE)
	{
		monster_type *m_ptr;

		/* Pick a (possibly illegal) location */
		int ny = rand_spread(oy, dis);
		int nx = rand_spread(ox, dis);

		tries++;

		/* Stop after 1000 tries */
		if (tries > 10000) return;

		/*
		 * Increase distance after doing enough tries
		 * compared to area of possible space
		 */
		if (tries > 20 * dis * dis) dis++;

		/* Ignore illegal locations */
		if (!in_bounds(ny, nx)) continue;

		/* Require "empty" floor space */
		if (!cave_empty_bold(ny, nx)) continue;

		/* Hack -- no teleport onto glyph of warding */
		if (is_glyph_grid(&cave[ny][nx])) continue;
		if (is_explosive_rune_grid(&cave[ny][nx])) continue;

		/* ...nor onto the Pattern */
		if ((cave[ny][nx].feat >= FEAT_PATTERN_START) &&
		    (cave[ny][nx].feat <= FEAT_PATTERN_XTRA2)) continue;

		/*** It's a good place ***/

		m_ptr = &m_list[m_idx];

		/* Update the new location */
		cave[ny][nx].m_idx = m_idx;

		/* Update the old location */
		cave[oy][ox].m_idx = 0;

		/* Move the monster */
		m_ptr->fy = ny;
		m_ptr->fx = nx; 

		/* No need to do update_mon() */

		/* Success */
		return;
	}
}


/*
 * Maintain quest monsters, mark next floor_id at stairs, save current
 * floor, and prepare to enter next floor.
 */
void leave_floor(void)
{
	cave_type *c_ptr = NULL;
	saved_floor_type *sf_ptr;
	int quest_r_idx = 0;
	int i;

	/* Preserve pets and prepare to take these to next floor */
	preserve_pet();

	/* Remove all mirrors without explosion */
	remove_all_mirrors(FALSE);

	if (p_ptr->special_defense & NINJA_S_STEALTH) set_superstealth(FALSE);

	/* New floor is not yet prepared */
	new_floor_id = 0;


	/* From somewhere of the surface to somewhere of the surface */
	if (!dungeon_type)
	{
		/* No need to save current floor */
		return;
	}

	/* Search the quest monster index */
	for (i = 0; i < max_quests; i++)
	{
		if ((quest[i].status == QUEST_STATUS_TAKEN) &&
		    ((quest[i].type == QUEST_TYPE_KILL_LEVEL) ||
		    (quest[i].type == QUEST_TYPE_RANDOM)) &&
		    (quest[i].level == dun_level) &&
		    (dungeon_type == quest[i].dungeon) &&
		    !(quest[i].flags & QUEST_FLAG_PRESET))
		{
			quest_r_idx = quest[i].r_idx;
		}
	}

	/* Maintain quest monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_race *r_ptr;
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Only maintain quest monsters */
		if (quest_r_idx != m_ptr->r_idx) continue;

		/* Extract real monster race */
		r_ptr = real_r_ptr(m_ptr);

		/* Ignore unique monsters */
		if ((r_ptr->flags1 & RF1_UNIQUE) ||
		    (r_ptr->flags7 & RF7_NAZGUL)) continue;

		/* Delete non-unique quest monsters */
		delete_monster_idx(i);
	}

	/* Check if there is a same item */
	for (i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Delete old memorized location of the artifact */
		if (artifact_p(o_ptr))
		{
			a_info[o_ptr->name1].floor_id = 0;
		}
	}

	/* Extract current floor info or NULL */
	sf_ptr = get_sf_ptr(p_ptr->floor_id);

	/* Choose random stairs */
	if ((change_floor_mode & CFM_RAND_CONNECT) && p_ptr->floor_id)
	{
		int x, y, sx = 0, sy = 0;
		int x_table[20];
		int y_table[20];
		int num = 0;
		int i;

		/* Search usable stairs */
		for (y = 0; y < cur_hgt; y++)
		{
			for (x = 0; x < cur_wid; x++)
			{
				cave_type *c_ptr = &cave[y][x];
				bool ok = FALSE;

				if (change_floor_mode & CFM_UP)
				{
					/* Found fixed stairs */
					if (c_ptr->special &&
					    c_ptr->special == sf_ptr->upper_floor_id)
					{
						sx = x;
						sy = y;
					}

					if (c_ptr->feat == FEAT_LESS ||
					    c_ptr->feat == FEAT_LESS_LESS)
						ok = TRUE;
				}
				else if (change_floor_mode & CFM_DOWN)
				{
					/* Found fixed stairs */
					if (c_ptr->special &&
					    c_ptr->special == sf_ptr->lower_floor_id)
					{
						sx = x;
						sy = y;
					}

					if (c_ptr->feat == FEAT_MORE ||
					    c_ptr->feat == FEAT_MORE_MORE)
						ok = TRUE;
				}

				if (ok && num < 20)
				{
					x_table[num] = x;
					y_table[num] = y;
					num++;
				}
			}
		}

		if (sx)
		{
			/* Already fixed */
			py = sy;
			px = sx;
		}
		else if (!num)
		{
			/* No stairs found! -- No return */
			prepare_change_floor_mode(CFM_RAND_PLACE | CFM_NO_RETURN);

			/* Mega Hack -- It's not the stairs you enter.  Disable it.  */
			cave[py][px].special = 0;
		}
		else
		{
			/* Choose random one */
			i = randint0(num);

			/* Point stair location */
			py = y_table[i];
			px = x_table[i];
		}
	}

	/* Extract new dungeon level */
	if (!(change_floor_mode & CFM_CLEAR_ALL))
	{
		int move_num = 0;

		/* Extract stair position */
		c_ptr = &cave[py][px];

		/* Get back to old saved floor? */
		if (dun_level && c_ptr->special && get_sf_ptr(c_ptr->special))
		{
			/* Saved floor is exist.  Use it. */
			new_floor_id = c_ptr->special;
		}

		/* Extract level movement number */
		if (change_floor_mode & CFM_DOWN) move_num = 1;
		else if (change_floor_mode & CFM_UP) move_num = -1;
		
		/* Mark shaft up/down */
		if (c_ptr->feat == FEAT_LESS_LESS ||
		    c_ptr->feat == FEAT_MORE_MORE)
		{
			prepare_change_floor_mode(CFM_SHAFT);
			move_num += SGN(move_num);
		}

		/* Get out from or Enter the dungeon */
		if (change_floor_mode & CFM_DOWN)
		{
			if (!dun_level)
				move_num = d_info[c_ptr->special].mindepth;
		}
		else if (change_floor_mode & CFM_UP)
		{
			if (dun_level + move_num < d_info[dungeon_type].mindepth)
				move_num = -dun_level;
		}
		
		dun_level += move_num;
	}

	/* Leaving the dungeon to town */
	if (!dun_level && dungeon_type)
	{
		p_ptr->leaving_dungeon = TRUE;
		if (!vanilla_town && !lite_town)
		{
			p_ptr->wilderness_y = d_info[dungeon_type].dy;
			p_ptr->wilderness_x = d_info[dungeon_type].dx;
		}
		p_ptr->recall_dungeon = dungeon_type;
		dungeon_type = 0;

		/* Reach to the surface -- Clear all saved floors */
		prepare_change_floor_mode(CFM_CLEAR_ALL);
	}

	if (change_floor_mode & CFM_CLEAR_ALL)
	{
		int i;

		/* Kill all saved floors */
		for (i = 0; i < MAX_SAVED_FLOORS; i++)
			kill_saved_floor(&saved_floors[i]);

		/* Reset visit_mark count */
		latest_visit_mark = 1;
	}
	else if (change_floor_mode & CFM_NO_RETURN)
	{
		/* Kill current floor */
		kill_saved_floor(sf_ptr);
	}

	/* No current floor -- Left/Enter dungeon etc... */
	if (!p_ptr->floor_id)
	{
		/* No longer need to save current floor */
		return;
	}


	/* Mark next floor_id on the previous floor */
	if (!new_floor_id)
	{
		/* Get new id */
		new_floor_id = get_new_floor_id();

		/* Connect from here */
		if (c_ptr)
		{
			c_ptr->special = new_floor_id;
		}

		/* Record new dungeon level */
		get_sf_ptr(new_floor_id)->dun_level = dun_level;
	}

	/* Fix connection -- level teleportation or trap door */
	if (change_floor_mode & CFM_RAND_CONNECT)
	{
		if (change_floor_mode & CFM_UP)
			sf_ptr->upper_floor_id = new_floor_id;
		else if (change_floor_mode & CFM_DOWN)
			sf_ptr->lower_floor_id = new_floor_id;
	}

	/* If you can return, you need to save previous floor */
	if (!(change_floor_mode & (CFM_NO_RETURN | CFM_CLEAR_ALL)))
	{
		/* Get out of the my way! */
		get_out_monster();

		/* Record the last visit turn of current floor */
		sf_ptr->last_visit = turn;

		/* Forget the lite */
		forget_lite();

		/* Forget the view */
		forget_view();

		/* Forget the view */
		clear_mon_lite();

		/* Save current floor */
		if (!save_floor(sf_ptr, 0))
		{
			/* Save failed -- No return */
			prepare_change_floor_mode(CFM_NO_RETURN);

			/* Kill current floor */
			kill_saved_floor(get_sf_ptr(p_ptr->floor_id));
		}
	}
}


/*
 * Enter new floor.  If the floor is an old saved floor, it will be
 * restored from the temporal file.  If the floor is new one, new cave
 * will be generated.
 */
void change_floor(void)
{
	saved_floor_type *sf_ptr;
	bool loaded = FALSE;

	/* The dungeon is not ready */
	character_dungeon = FALSE;

	/* No longer in the trap detecteded region */
	p_ptr->dtrap = FALSE;

	/* Mega-Hack -- no panel yet */
	panel_row_min = 0;
	panel_row_max = 0;
	panel_col_min = 0;
	panel_col_max = 0;

	/* Mega-Hack -- not ambushed on the wildness? */
	ambush_flag = FALSE;

	/* On the surface */
	if (!dungeon_type)
	{
		/* Create cave */
		generate_cave();

		/* Paranoia -- Now on the surface */
		new_floor_id = 0;
	}

	/* In the dungeon */
	else
	{
		/* No floor_id yet */
		if (!new_floor_id)
		{
			/* Get new id */
			new_floor_id = get_new_floor_id();
		}

		/* Pointer for infomations of new floor */
		sf_ptr = get_sf_ptr(new_floor_id);

		/* Try to restore old floor */
		if (sf_ptr->last_visit)
		{
			/* Old saved floor is exist */
			if (load_floor(sf_ptr, 0))
			{
				loaded = TRUE;

				/* Forbid return stairs */
				if (change_floor_mode & CFM_NO_RETURN)
				{
					cave_type *c_ptr = &cave[py][px];

					/* Reset to floor */
					place_floor_grid(c_ptr);
					c_ptr->special = 0;
				}
			}
		}

		/*
		 * Set lower/upper_floor_id of new floor when the new
		 * floor is right-above/right-under the current floor.
		 *
		 * Stair creation/Teleport level/Trap door will take
		 * you the same floor when you used it later again.
		 */
		if (p_ptr->floor_id)
		{
			saved_floor_type *cur_sf_ptr = get_sf_ptr(p_ptr->floor_id);

			if (change_floor_mode & CFM_UP)
			{
				/* New floor is right-above */
				if (cur_sf_ptr->upper_floor_id == new_floor_id)
					sf_ptr->lower_floor_id = p_ptr->floor_id;
			}
			else if (change_floor_mode & CFM_DOWN)
			{
				/* New floor is right-under */
				if (cur_sf_ptr->lower_floor_id == new_floor_id)
					sf_ptr->upper_floor_id = p_ptr->floor_id;
			}
		}

		/* Break connection to killed floor */
		else
		{
			if (change_floor_mode & CFM_UP)
				sf_ptr->lower_floor_id = 0;
			else if (change_floor_mode & CFM_DOWN)
				sf_ptr->upper_floor_id = 0;
		}

		/* Maintain monsters and artifacts */
		if (loaded)
		{
			int i;
			s32b tmp_last_visit = sf_ptr->last_visit;
			s32b absence_ticks;
			int alloc_chance = d_info[dungeon_type].max_m_alloc_chance;
			int alloc_times;

			while (tmp_last_visit > turn) tmp_last_visit -= TURNS_PER_TICK * TOWN_DAWN;
			absence_ticks = (turn - tmp_last_visit) / TURNS_PER_TICK;

			/* Maintain monsters */
			for (i = 1; i < m_max; i++)
			{
				monster_race *r_ptr;
				monster_type *m_ptr = &m_list[i];

				/* Skip dead monsters */
				if (!m_ptr->r_idx) continue;

				if (!is_pet(m_ptr))
				{
					/* Restore HP */
					m_ptr->hp = m_ptr->maxhp = m_ptr->max_maxhp;

					/* Remove fear */
					m_ptr->monfear = 0;

					/* Remove invulnerability */
					m_ptr->invulner = 0;

					/* Remove fast status */
					m_ptr->fast = 0;

					/* Remove slow status */
					m_ptr->slow = 0;

					/* Remove stun */
					m_ptr->stunned = 0;

					/* Remove confusion */
					m_ptr->confused = 0;
				}

				/* Extract real monster race */
				r_ptr = real_r_ptr(m_ptr);

				/* Ignore non-unique */
				if (!(r_ptr->flags1 & RF1_UNIQUE) &&
				    !(r_ptr->flags7 & RF7_NAZGUL)) continue;

				/* Appear at a different floor? */
				if (r_ptr->floor_id != new_floor_id)
				{
					/* Disapper from here */
					delete_monster_idx(i);
				}
			}

			/* Maintain artifatcs */
			for (i = 1; i < o_max; i++)
			{
				object_type *o_ptr = &o_list[i];

				/* Skip dead objects */
				if (!o_ptr->k_idx) continue;

				/* Ignore non-artifact */
				if (!artifact_p(o_ptr)) continue;

				/* Appear at a different floor? */
				if (a_info[o_ptr->name1].floor_id != new_floor_id)
				{
					/* Disappear from here */
					delete_object_idx(i);
				}
				else
				{
					/* Cancel preserve */
					a_info[o_ptr->name1].cur_num = 1;
				}
			}

			(void)place_quest_monsters();

			/* Place some random monsters */
			alloc_times = absence_ticks / alloc_chance;

			if (randint0(alloc_chance) < (absence_ticks % alloc_chance))
				alloc_times++;

			for (i = 0; i < alloc_times; i++)
			{
				/* Make a (group of) new monster */
				(void)alloc_monster(0, 0);
			}
		}

		/* New floor_id or failed to restore */
		else /* if (!loaded) */
		{
			if (sf_ptr->last_visit)
			{
				/* Temporal file is broken? */
#ifdef JP
				msg_print("���ʤϹԤ��ߤޤ���ä���");
#else
				msg_print("The staircases come to a dead end...");
#endif

				/* Create simple dead end */
				build_dead_end();

				/* Break connection */
				if (change_floor_mode & CFM_UP)
				{
					sf_ptr->upper_floor_id = 0;
				}
				else if (change_floor_mode & CFM_DOWN)
				{
					sf_ptr->lower_floor_id = 0;
				}
			}
			else
			{
				/* Newly create cave */
				generate_cave();
			}

			/* Record last visit turn */
			sf_ptr->last_visit = turn;

			/* Set correct dun_level value */
			sf_ptr->dun_level = dun_level;

			/* Creat connected stairs */
			if (!(change_floor_mode & (CFM_NO_RETURN | CFM_CLEAR_ALL)) && dun_level)
			{
				bool ok = TRUE;

				/* Extract stair position */
				cave_type *c_ptr = &cave[py][px];

				/*** Create connected stairs ***/

				/* No stairs down from Quest */
				if ((change_floor_mode & CFM_UP) && !quest_number(dun_level))
				{
					if (change_floor_mode & CFM_SHAFT)
						c_ptr->feat = FEAT_MORE_MORE;
					else
						c_ptr->feat = FEAT_MORE;
				}

				/* No stairs up when ironman_downward */
				else if ((change_floor_mode & CFM_DOWN) && !ironman_downward)
				{
					if (change_floor_mode & CFM_SHAFT)
						c_ptr->feat = FEAT_LESS_LESS;
					else
						c_ptr->feat = FEAT_LESS;
				}
				else
				{
					/* Hum??? */
					ok = FALSE;
				}

				if (ok)
				{
					/* Paranoia -- Clear mimic */
					c_ptr->mimic = 0;

					/* Connect to previous floor */
					c_ptr->special = p_ptr->floor_id;
				}
			}
		}

		/* Arrive at random grid */
		if (change_floor_mode & (CFM_RAND_PLACE))
		{
			(void)new_player_spot();
		}

		/* You see stairs blocked */
		else if (change_floor_mode & CFM_NO_RETURN)
		{
			if (!p_ptr->blind)
			{
#ifdef JP
				msg_print("�������ʤ��ɤ���Ƥ��ޤä���");
#else
				msg_print("Suddenly the stairs is blocked!");
#endif
			}
			else
			{
#ifdef JP
				msg_print("���ȥ��ȤȲ�������������");
#else
				msg_print("You hear some noises.");
#endif
			}
		}

		/*
		 * Update visit mark
		 *
		 * The "turn" is not always different number because
		 * the level teleport doesn't take any turn.  Use
		 * visit mark instead of last visit turn to find the
		 * oldest saved floor.
		 */
		sf_ptr->visit_mark = latest_visit_mark++;
	}

	/* Place preserved pet monsters */
	place_pet();

	/* Hack -- maintain unique and artifacts */
	update_unique_artifact(new_floor_id);

	/* Now the player is in new floor */
	p_ptr->floor_id = new_floor_id;

	/* The dungeon is ready */
	character_dungeon = TRUE;

	/* Hack -- Munchkin characters always get whole map */
	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN)
		wiz_lite((bool)(p_ptr->pclass == CLASS_NINJA));

	/* Remember when this level was "created" */
	old_turn = turn;

	/* Clear all flags */
	change_floor_mode = 0L;
}



/*
 * Create stairs at or move previously created stairs into the player
 * location.
 */
void stair_creation(void)
{
	saved_floor_type *sf_ptr;
	saved_floor_type *dest_sf_ptr;

	bool up = TRUE;
	bool down = TRUE;
	s16b dest_floor_id = 0;


	/* Forbid up staircases on Ironman mode */
	if (ironman_downward) up = FALSE;

	/* Forbid down staircases on quest level */
	if (quest_number(dun_level) || (dun_level >= d_info[dungeon_type].maxdepth)) down = FALSE;

	/* No effect out of standard dungeon floor */
	if (!dun_level || (!up && !down) ||
	    (p_ptr->inside_quest && is_fixed_quest_idx(p_ptr->inside_quest)) ||
	    p_ptr->inside_arena || p_ptr->inside_battle)
	{
		/* arena or quest */
#ifdef JP
		msg_print("���̤�����ޤ���");
#else
		msg_print("There is no effect!");
#endif
		return;
	}

	/* Artifacts resists */
	if (!cave_valid_bold(py, px))
	{
#ifdef JP
		msg_print("����Υ����ƥब��ʸ��ķ���֤�����");
#else
		msg_print("The object resists the spell.");
#endif

		return;
	}

	/* Destroy all objects in the grid */
	delete_object(py, px);

	/* Extract current floor data */
	sf_ptr = get_sf_ptr(p_ptr->floor_id);

	/* Choose randomly */
	if (up && down)
	{
		if (randint0(100) < 50) up = FALSE;
		else down = FALSE;
	}

	/* Destination is already fixed */
	if (up)
	{
		if (sf_ptr->upper_floor_id) dest_floor_id = sf_ptr->upper_floor_id;
	}
	else
	{
		if (sf_ptr->lower_floor_id) dest_floor_id = sf_ptr->lower_floor_id;
	}


	/* Search old stairs leading to the destination */
	if (dest_floor_id)
	{
		int x, y;

		for (y = 0; y < cur_hgt; y++)
		{
			for (x = 0; x < cur_wid; x++)
			{
				cave_type *c_ptr = &cave[y][x];

				if (!c_ptr->special) continue;
				if (c_ptr->special != dest_floor_id) continue;

				/* Remove old stairs */
				c_ptr->special = 0;
				cave_set_feat(y, x, floor_type[randint0(100)]);
			}
		}
	}

	/* No old destination -- Get new one now */
	else
	{
		dest_floor_id = get_new_floor_id();

		/* Fix it */
		if (up)
			sf_ptr->upper_floor_id = dest_floor_id;
		else
			sf_ptr->lower_floor_id = dest_floor_id;
	}

	/* Extract destination floor data */
	dest_sf_ptr = get_sf_ptr(dest_floor_id);


	/* Create a staircase */
	if (up)
	{
		if (dest_sf_ptr->last_visit && dest_sf_ptr->dun_level <= dun_level - 2)
			cave_set_feat(py, px, FEAT_LESS_LESS);
		else
			cave_set_feat(py, px, FEAT_LESS);
	}
	else
	{
		if (dest_sf_ptr->last_visit && dest_sf_ptr->dun_level >= dun_level + 2)
			cave_set_feat(py, px, FEAT_MORE_MORE);
		else
			cave_set_feat(py, px, FEAT_MORE);
	}


	/* Connect this stairs to the destination */
	cave[py][px].special = dest_floor_id;
}

