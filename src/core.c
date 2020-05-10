﻿/*!
 * @brief Angbandゲームエンジン / Angband game engine
 * @date 2013/12/31
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "io/signal-handlers.h"
#include "util.h"
#include "core.h"
#include "core/angband-version.h"
#include "core/stuff-handler.h"
#include "inet.h"
#include "gameterm.h"
#include "chuukei.h"

#include "creature.h"
#include "birth/birth.h"
#include "market/building.h"
#include "io/write-diary.h"
#include "dungeon.h"
#include "feature.h"
#include "floor.h"
#include "floor-events.h"
#include "grid.h"
#include "object-flavor.h"
#include "market/store.h"
#include "spell/technic-info-table.h"
#include "spells-status.h"
#include "world.h"
#include "market/arena-info-table.h"
#include "market/store-util.h"
#include "quest.h"
#include "view/display-player.h"
#include "player/process-name.h"
#include "player-move.h"
#include "player-class.h"
#include "player-race.h"
#include "player-personality.h"
#include "player-effects.h"
#include "wild.h"
#include "monster-process.h"
#include "monster-status.h"
#include "floor-save.h"
#include "feature.h"
#include "player-skill.h"

#include "view/display-main-window.h"
#include "dungeon-file.h"
#include "io/read-pref-file.h"
#include "scores.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-reader-writer.h"
#include "save.h"
#include "realm/realm.h"
#include "realm/realm-song.h"
#include "targeting.h"
#include "core/output-updater.h"
#include "core/game-closer.h"
#include "core/turn-compensator.h"
#include "inventory/simple-appraiser.h"
#include "core/hp-mp-regenerator.h"
#include "core/hp-mp-processor.h"
#include "mutation/mutation-processor.h"
#include "object/lite-processor.h"
#include "core/magic-effects-timeout-reducer.h"
#include "inventory/inventory-curse.h"
#include "inventory/recharge-processor.h"
#include "io/input-key-processor.h"
#include "core/player-processor.h"
#include "player/digestion-processor.h"
#include "world/world-movement-processor.h"

#include "cmd/cmd-save.h"
#include "cmd/cmd-dump.h"


 /*!
  * todo どこからも呼ばれていない。main関数辺りに移設するか、そもそもコメントでいいと思われる
  * コピーライト情報 / Link a copyright message into the executable
  */
const concptr copyright[5] =
{
	"Copyright (c) 1989 James E. Wilson, Robert A. Keoneke",
	"",
	"This software may be copied and distributed for educational, research,",
	"and not for profit purposes provided that this copyright and statement",
	"are included in all such copies."
};

concptr ANGBAND_SYS = "xxx";

#ifdef JP
concptr ANGBAND_KEYBOARD = "JAPAN";
#else
concptr ANGBAND_KEYBOARD = "0";
#endif

concptr ANGBAND_GRAF = "ascii";

/*
 * Flags for initialization
 */
int init_flags;

/*!
 * @brief 10ゲームターンが進行する毎にゲーム世界全体の処理を行う。
 * / Handle certain things once every 10 game turns
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void process_world(player_type *player_ptr)
{
	const s32b A_DAY = TURNS_PER_TICK * TOWN_DAWN;
	s32b prev_turn_in_today = ((current_world_ptr->game_turn - TURNS_PER_TICK) % A_DAY + A_DAY / 4) % A_DAY;
	int prev_min = (1440 * prev_turn_in_today / A_DAY) % 60;

	int day, hour, min;
	extract_day_hour_min(player_ptr, &day, &hour, &min);
	update_dungeon_feeling(player_ptr);

	/* 帰還無しモード時のレベルテレポバグ対策 / Fix for level teleport bugs on ironman_downward.*/
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (ironman_downward && (player_ptr->dungeon_idx != DUNGEON_ANGBAND && player_ptr->dungeon_idx != 0))
	{
		floor_ptr->dun_level = 0;
		player_ptr->dungeon_idx = 0;
		prepare_change_floor_mode(player_ptr, CFM_FIRST_FLOOR | CFM_RAND_PLACE);
		floor_ptr->inside_arena = FALSE;
		player_ptr->wild_mode = FALSE;
		player_ptr->leaving = TRUE;
	}

	if (player_ptr->phase_out && !player_ptr->leaving)
	{
		int win_m_idx = 0;
		int number_mon = 0;
		for (int i2 = 0; i2 < floor_ptr->width; ++i2)
		{
			for (int j2 = 0; j2 < floor_ptr->height; j2++)
			{
				grid_type *g_ptr = &floor_ptr->grid_array[j2][i2];
				if ((g_ptr->m_idx > 0) && (g_ptr->m_idx != player_ptr->riding))
				{
					number_mon++;
					win_m_idx = g_ptr->m_idx;
				}
			}
		}

		if (number_mon == 0)
		{
			msg_print(_("相打ちに終わりました。", "Nothing survived."));
			msg_print(NULL);
			player_ptr->energy_need = 0;
			update_gambling_monsters(player_ptr);
		}
		else if ((number_mon - 1) == 0)
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_type *wm_ptr;
			wm_ptr = &floor_ptr->m_list[win_m_idx];
			monster_desc(player_ptr, m_name, wm_ptr, 0);
			msg_format(_("%sが勝利した！", "%s won!"), m_name);
			msg_print(NULL);

			if (win_m_idx == (sel_monster + 1))
			{
				msg_print(_("おめでとうございます。", "Congratulations."));
				msg_format(_("%d＄を受け取った。", "You received %d gold."), battle_odds);
				player_ptr->au += battle_odds;
			}
			else
			{
				msg_print(_("残念でした。", "You lost gold."));
			}

			msg_print(NULL);
			player_ptr->energy_need = 0;
			update_gambling_monsters(player_ptr);
		}
		else if (current_world_ptr->game_turn - floor_ptr->generated_turn == 150 * TURNS_PER_TICK)
		{
			msg_print(_("申し分けありませんが、この勝負は引き分けとさせていただきます。", "This battle ended in a draw."));
			player_ptr->au += kakekin;
			msg_print(NULL);
			player_ptr->energy_need = 0;
			update_gambling_monsters(player_ptr);
		}
	}

	if (current_world_ptr->game_turn % TURNS_PER_TICK) return;

	if (autosave_t && autosave_freq && !player_ptr->phase_out)
	{
		if (!(current_world_ptr->game_turn % ((s32b)autosave_freq * TURNS_PER_TICK)))
			do_cmd_save_game(player_ptr, TRUE);
	}

	if (floor_ptr->monster_noise && !ignore_unview)
	{
		msg_print(_("何かが聞こえた。", "You hear noise."));
	}

	if (!floor_ptr->dun_level && !floor_ptr->inside_quest && !player_ptr->phase_out && !floor_ptr->inside_arena)
	{
		if (!(current_world_ptr->game_turn % ((TURNS_PER_TICK * TOWN_DAWN) / 2)))
		{
			bool dawn = (!(current_world_ptr->game_turn % (TURNS_PER_TICK * TOWN_DAWN)));
			if (dawn) day_break(player_ptr);
			else night_falls(player_ptr);

		}
	}
	else if ((vanilla_town || (lite_town && !floor_ptr->inside_quest && !player_ptr->phase_out && !floor_ptr->inside_arena)) && floor_ptr->dun_level)
	{
		if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * STORE_TICKS)))
		{
			if (one_in_(STORE_SHUFFLE))
			{
				int n;
				do
				{
					n = randint0(MAX_STORES);
				} while ((n == STORE_HOME) || (n == STORE_MUSEUM));

				for (FEAT_IDX i = 1; i < max_f_idx; i++)
				{
					feature_type *f_ptr = &f_info[i];
					if (!f_ptr->name) continue;
					if (!have_flag(f_ptr->flags, FF_STORE)) continue;

					if (f_ptr->subtype == n)
					{
						if (cheat_xtra)
							msg_format(_("%sの店主をシャッフルします。", "Shuffle a Shopkeeper of %s."), f_name + f_ptr->name);

						store_shuffle(player_ptr, n);
						break;
					}
				}
			}
		}
	}

	if (one_in_(d_info[player_ptr->dungeon_idx].max_m_alloc_chance) &&
		!floor_ptr->inside_arena && !floor_ptr->inside_quest && !player_ptr->phase_out)
	{
		(void)alloc_monster(player_ptr, MAX_SIGHT + 5, 0);
	}

	if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 10)) && !player_ptr->phase_out)
		regenerate_monsters(player_ptr);
	if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 3)))
		regenerate_captured_monsters(player_ptr);

	if (!player_ptr->leaving)
	{
		for (int i = 0; i < MAX_MTIMED; i++)
		{
			if (floor_ptr->mproc_max[i] > 0) process_monsters_mtimed(player_ptr, i);
		}
	}

	if (!hour && !min)
	{
		if (min != prev_min)
		{
			exe_write_diary(player_ptr, DIARY_DIALY, 0, NULL);
			determine_daily_bounty(player_ptr, FALSE);
		}
	}

	/*
	 * Nightmare mode activates the TY_CURSE at midnight
	 * Require exact minute -- Don't activate multiple times in a minute
	 */
	if (ironman_nightmare && (min != prev_min))
	{
		if ((hour == 23) && !(min % 15))
		{
			disturb(player_ptr, FALSE, TRUE);
			switch (min / 15)
			{
			case 0:
				msg_print(_("遠くで不気味な鐘の音が鳴った。", "You hear a distant bell toll ominously."));
				break;

			case 1:
				msg_print(_("遠くで鐘が二回鳴った。", "A distant bell sounds twice."));
				break;

			case 2:
				msg_print(_("遠くで鐘が三回鳴った。", "A distant bell sounds three times."));
				break;

			case 3:
				msg_print(_("遠くで鐘が四回鳴った。", "A distant bell tolls four times."));
				break;
			}
		}

		if (!hour && !min)
		{
			disturb(player_ptr, TRUE, TRUE);
			msg_print(_("遠くで鐘が何回も鳴り、死んだような静けさの中へ消えていった。", "A distant bell tolls many times, fading into an deathly silence."));
			if (player_ptr->wild_mode)
			{
				player_ptr->oldpy = randint1(MAX_HGT - 2);
				player_ptr->oldpx = randint1(MAX_WID - 2);
				change_wild_mode(player_ptr, TRUE);
				take_turn(player_ptr, 100);

			}

			player_ptr->invoking_midnight_curse = TRUE;
		}
	}

	starve_player(player_ptr);
	process_player_hp_mp(player_ptr);
	reduce_magic_effects_timeout(player_ptr);
	reduce_lite_life(player_ptr);
	process_world_aux_mutation(player_ptr);
	execute_cursed_items_effect(player_ptr);
	recharge_magic_items(player_ptr);
	sense_inventory1(player_ptr);
	sense_inventory2(player_ptr);
	process_world_aux_movement(player_ptr);
}


/*!
 * process_player()、process_world() をcore.c から移設するのが先.
 * process_upkeep_with_speed() はこの関数と同じところでOK
 * @brief 現在プレイヤーがいるダンジョンの全体処理 / Interact with the current dungeon level.
 * @return なし
 * @details
 * <p>
 * この関数から現在の階層を出る、プレイヤーがキャラが死ぬ、
 * ゲームを終了するかのいずれかまでループする。
 * </p>
 * <p>
 * This function will not exit until the level is completed,\n
 * the user dies, or the game is terminated.\n
 * </p>
 */
static void dungeon(player_type *player_ptr, bool load_game)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	floor_ptr->base_level = floor_ptr->dun_level;
	current_world_ptr->is_loading_now = FALSE;
	player_ptr->leaving = FALSE;

	command_cmd = 0;
	command_rep = 0;
	command_arg = 0;
	command_dir = 0;

	target_who = 0;
	player_ptr->pet_t_m_idx = 0;
	player_ptr->riding_t_m_idx = 0;
	player_ptr->ambush_flag = FALSE;
	health_track(player_ptr, 0);
	repair_monsters = TRUE;

	disturb(player_ptr, TRUE, TRUE);
	int quest_num = quest_num = quest_number(player_ptr, floor_ptr->dun_level);
	if (quest_num)
	{
		r_info[quest[quest_num].r_idx].flags1 |= RF1_QUESTOR;
	}

	if (player_ptr->max_plv < player_ptr->lev)
	{
		player_ptr->max_plv = player_ptr->lev;
	}

	if ((max_dlv[player_ptr->dungeon_idx] < floor_ptr->dun_level) && !floor_ptr->inside_quest)
	{
		max_dlv[player_ptr->dungeon_idx] = floor_ptr->dun_level;
		if (record_maxdepth) exe_write_diary(player_ptr, DIARY_MAXDEAPTH, floor_ptr->dun_level, NULL);
	}

	(void)calculate_upkeep(player_ptr);
	panel_bounds_center();
	verify_panel(player_ptr);
	msg_erase();

	current_world_ptr->character_xtra = TRUE;
	player_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER | PW_MONSTER | PW_OVERHEAD | PW_DUNGEON);
	player_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_EQUIPPY | PR_MAP);
	player_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_VIEW | PU_LITE | PU_MON_LITE | PU_TORCH | PU_MONSTERS | PU_DISTANCE | PU_FLOW);
	handle_stuff(player_ptr);

	current_world_ptr->character_xtra = FALSE;
	player_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	player_ptr->update |= (PU_COMBINE | PU_REORDER);
	handle_stuff(player_ptr);
	Term_fresh();

	if (quest_num && (is_fixed_quest_idx(quest_num) &&
		!((quest_num == QUEST_OBERON) || (quest_num == QUEST_SERPENT) ||
			!(quest[quest_num].flags & QUEST_FLAG_PRESET))))
		do_cmd_feeling(player_ptr);

	if (player_ptr->phase_out)
	{
		if (load_game)
		{
			player_ptr->energy_need = 0;
			update_gambling_monsters(player_ptr);
		}
		else
		{
			msg_print(_("試合開始！", "Ready..Fight!"));
			msg_print(NULL);
		}
	}

	if ((player_ptr->pclass == CLASS_BARD) && (SINGING_SONG_EFFECT(player_ptr) > MUSIC_DETECT))
		SINGING_SONG_EFFECT(player_ptr) = MUSIC_DETECT;

	if (!player_ptr->playing || player_ptr->is_dead) return;

	if (!floor_ptr->inside_quest && (player_ptr->dungeon_idx == DUNGEON_ANGBAND))
	{
		quest_discovery(random_quest_number(player_ptr, floor_ptr->dun_level));
		floor_ptr->inside_quest = random_quest_number(player_ptr, floor_ptr->dun_level);
	}
	if ((floor_ptr->dun_level == d_info[player_ptr->dungeon_idx].maxdepth) && d_info[player_ptr->dungeon_idx].final_guardian)
	{
		if (r_info[d_info[player_ptr->dungeon_idx].final_guardian].max_num)
#ifdef JP
			msg_format("この階には%sの主である%sが棲んでいる。",
				d_name + d_info[player_ptr->dungeon_idx].name,
				r_name + r_info[d_info[player_ptr->dungeon_idx].final_guardian].name);
#else
			msg_format("%^s lives in this level as the keeper of %s.",
				r_name + r_info[d_info[player_ptr->dungeon_idx].final_guardian].name,
				d_name + d_info[player_ptr->dungeon_idx].name);
#endif
	}

	if (!load_game && (player_ptr->special_defense & NINJA_S_STEALTH)) set_superstealth(player_ptr, FALSE);

	floor_ptr->monster_level = floor_ptr->base_level;
	floor_ptr->object_level = floor_ptr->base_level;
	current_world_ptr->is_loading_now = TRUE;
	if (player_ptr->energy_need > 0 && !player_ptr->phase_out &&
		(floor_ptr->dun_level || player_ptr->leaving_dungeon || floor_ptr->inside_arena))
		player_ptr->energy_need = 0;

	player_ptr->leaving_dungeon = FALSE;
	mproc_init(floor_ptr);

	while (TRUE)
	{
		if ((floor_ptr->m_cnt + 32 > current_world_ptr->max_m_idx) && !player_ptr->phase_out)
			compact_monsters(player_ptr, 64);

		if ((floor_ptr->m_cnt + 32 < floor_ptr->m_max) && !player_ptr->phase_out)
			compact_monsters(player_ptr, 0);

		if (floor_ptr->o_cnt + 32 > current_world_ptr->max_o_idx)
			compact_objects(player_ptr, 64);

		if (floor_ptr->o_cnt + 32 < floor_ptr->o_max)
			compact_objects(player_ptr, 0);

		process_player(player_ptr);
		process_upkeep_with_speed(player_ptr);
		handle_stuff(player_ptr);

		move_cursor_relative(player_ptr->y, player_ptr->x);
		if (fresh_after) Term_fresh();

		if (!player_ptr->playing || player_ptr->is_dead) break;

		process_monsters(player_ptr);
		handle_stuff(player_ptr);

		move_cursor_relative(player_ptr->y, player_ptr->x);
		if (fresh_after) Term_fresh();

		if (!player_ptr->playing || player_ptr->is_dead) break;

		process_world(player_ptr);
		handle_stuff(player_ptr);

		move_cursor_relative(player_ptr->y, player_ptr->x);
		if (fresh_after) Term_fresh();

		if (!player_ptr->playing || player_ptr->is_dead) break;

		current_world_ptr->game_turn++;
		if (current_world_ptr->dungeon_turn < current_world_ptr->dungeon_turn_limit)
		{
			if (!player_ptr->wild_mode || wild_regen) current_world_ptr->dungeon_turn++;
			else if (player_ptr->wild_mode && !(current_world_ptr->game_turn % ((MAX_HGT + MAX_WID) / 2))) current_world_ptr->dungeon_turn++;
		}

		prevent_turn_overflow(player_ptr);

		if (player_ptr->leaving) break;

		if (wild_regen) wild_regen--;
	}

	if (quest_num && !(r_info[quest[quest_num].r_idx].flags1 & RF1_UNIQUE))
	{
		r_info[quest[quest_num].r_idx].flags1 &= ~RF1_QUESTOR;
	}

	if (player_ptr->playing && !player_ptr->is_dead)
	{
		/*
		 * Maintain Unique monsters and artifact, save current
		 * floor, then prepare next floor
		 */
		leave_floor(player_ptr);
		reinit_wilderness = FALSE;
	}

	write_level = TRUE;
}


/*!
 * @brief 全ユーザプロファイルをロードする / Load some "user pref files"
 * @paaram player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @note
 * Modified by Arcum Dagsson to support
 * separate macro files for different realms.
 */
static void load_all_pref_files(player_type *player_ptr)
{
	char buf[1024];
	sprintf(buf, "user.prf");
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	sprintf(buf, "user-%s.prf", ANGBAND_SYS);
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	sprintf(buf, "%s.prf", rp_ptr->title);
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	sprintf(buf, "%s.prf", cp_ptr->title);
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	sprintf(buf, "%s.prf", player_ptr->base_name);
	process_pref_file(player_ptr, buf, process_autopick_file_command);
	if (player_ptr->realm1 != REALM_NONE)
	{
		sprintf(buf, "%s.prf", realm_names[player_ptr->realm1]);
		process_pref_file(player_ptr, buf, process_autopick_file_command);
	}

	if (player_ptr->realm2 != REALM_NONE)
	{
		sprintf(buf, "%s.prf", realm_names[player_ptr->realm2]);
		process_pref_file(player_ptr, buf, process_autopick_file_command);
	}

	autopick_load_pref(player_ptr, FALSE);
}


/*!
 * @brief 1ゲームプレイの主要ルーチン / Actually play a game
 * @return なし
 * @note
 * If the "new_game" parameter is true, then, after loading the
 * savefile, we will commit suicide, if necessary, to allow the
 * player to start a new game.
 */
void play_game(player_type *player_ptr, bool new_game)
{
	bool load_game = TRUE;
	bool init_random_seed = FALSE;

#ifdef CHUUKEI
	if (chuukei_client)
	{
		reset_visuals(player_ptr, process_autopick_file_command);
		browse_chuukei();
		return;
	}

	else if (chuukei_server)
	{
		prepare_chuukei_hooks();
	}
#endif

	if (browsing_movie)
	{
		reset_visuals(player_ptr, process_autopick_file_command);
		browse_movie();
		return;
	}

	player_ptr->hack_mutation = FALSE;
	current_world_ptr->character_icky = TRUE;
	Term_activate(angband_term[0]);
	angband_term[0]->resize_hook = resize_map;
	for (MONSTER_IDX i = 1; i < 8; i++)
	{
		if (angband_term[i])
		{
			angband_term[i]->resize_hook = redraw_window;
		}
	}

	(void)Term_set_cursor(0);
	if (!load_player(player_ptr))
	{
		quit(_("セーブファイルが壊れています", "broken savefile"));
	}

	extract_option_vars();
	if (player_ptr->wait_report_score)
	{
		char buf[1024];
		bool success;

		if (!get_check_strict(_("待機していたスコア登録を今行ないますか？", "Do you register score now? "), CHECK_NO_HISTORY))
			quit(0);

		player_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
		update_creature(player_ptr);
		player_ptr->is_dead = TRUE;
		current_world_ptr->start_time = (u32b)time(NULL);
		signals_ignore_tstp();
		current_world_ptr->character_icky = TRUE;
		path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
		highscore_fd = fd_open(buf, O_RDWR);

		/* 町名消失バグ対策(#38205)のためここで世界マップ情報を読み出す */
		process_dungeon_file(player_ptr, "w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);
		success = send_world_score(player_ptr, TRUE, update_playtime, display_player, map_name);

		if (!success && !get_check_strict(_("スコア登録を諦めますか？", "Do you give up score registration? "), CHECK_NO_HISTORY))
		{
			prt(_("引き続き待機します。", "standing by for future registration..."), 0, 0);
			(void)inkey();
		}
		else
		{
			player_ptr->wait_report_score = FALSE;
			top_twenty(player_ptr);
			if (!save_player(player_ptr)) msg_print(_("セーブ失敗！", "death save failed!"));
		}

		(void)fd_close(highscore_fd);
		highscore_fd = -1;
		signals_handle_tstp();

		quit(0);
	}

	current_world_ptr->creating_savefile = new_game;

	if (!current_world_ptr->character_loaded)
	{
		new_game = TRUE;
		current_world_ptr->character_dungeon = FALSE;
		init_random_seed = TRUE;
		init_saved_floors(player_ptr, FALSE);
	}
	else if (new_game)
	{
		init_saved_floors(player_ptr, TRUE);
	}

	if (!new_game)
	{
		process_player_name(player_ptr, FALSE);
	}

	if (init_random_seed)
	{
		Rand_state_init();
	}

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (new_game)
	{
		current_world_ptr->character_dungeon = FALSE;

		floor_ptr->dun_level = 0;
		floor_ptr->inside_quest = 0;
		floor_ptr->inside_arena = FALSE;
		player_ptr->phase_out = FALSE;
		write_level = TRUE;

		current_world_ptr->seed_flavor = randint0(0x10000000);
		current_world_ptr->seed_town = randint0(0x10000000);

		player_birth(player_ptr, process_autopick_file_command);
		counts_write(player_ptr, 2, 0);
		player_ptr->count = 0;
		load = FALSE;
		determine_bounty_uniques(player_ptr);
		determine_daily_bounty(player_ptr, FALSE);
		wipe_o_list(floor_ptr);
	}
	else
	{
		write_level = FALSE;
		exe_write_diary(player_ptr, DIARY_GAMESTART, 1,
			_("                            ----ゲーム再開----",
				"                            --- Restarted Game ---"));

		/*
		 * todo もう2.2.Xなので互換性は打ち切ってもいいのでは？
		 * 1.0.9 以前はセーブ前に player_ptr->riding = -1 としていたので、再設定が必要だった。
		 * もう不要だが、以前のセーブファイルとの互換のために残しておく。
		 */
		if (player_ptr->riding == -1)
		{
			player_ptr->riding = 0;
			for (MONSTER_IDX i = floor_ptr->m_max; i > 0; i--)
			{
				if (player_bold(player_ptr, floor_ptr->m_list[i].fy, floor_ptr->m_list[i].fx))
				{
					player_ptr->riding = i;
					break;
				}
			}
		}
	}

	current_world_ptr->creating_savefile = FALSE;

	player_ptr->teleport_town = FALSE;
	player_ptr->sutemi = FALSE;
	current_world_ptr->timewalk_m_idx = 0;
	player_ptr->now_damaged = FALSE;
	now_message = 0;
	current_world_ptr->start_time = time(NULL) - 1;
	record_o_name[0] = '\0';

	panel_row_min = floor_ptr->height;
	panel_col_min = floor_ptr->width;
	if (player_ptr->pseikaku == SEIKAKU_SEXY)
		s_info[player_ptr->pclass].w_max[TV_HAFTED - TV_WEAPON_BEGIN][SV_WHIP] = WEAPON_EXP_MASTER;

	set_floor_and_wall(player_ptr->dungeon_idx);
	flavor_init();
	prt(_("お待ち下さい...", "Please wait..."), 0, 0);
	Term_fresh();

	if (arg_wizard)
	{
		if (enter_wizard_mode(player_ptr))
		{
			current_world_ptr->wizard = TRUE;

			if (player_ptr->is_dead || !player_ptr->y || !player_ptr->x)
			{
				init_saved_floors(player_ptr, TRUE);
				floor_ptr->inside_quest = 0;
				player_ptr->y = player_ptr->x = 10;
			}
		}
		else if (player_ptr->is_dead)
		{
			quit("Already dead.");
		}
	}

	if (!floor_ptr->dun_level && !floor_ptr->inside_quest)
	{
		process_dungeon_file(player_ptr, "w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);
		init_flags = INIT_ONLY_BUILDINGS;
		process_dungeon_file(player_ptr, "t_info.txt", 0, 0, MAX_HGT, MAX_WID);
		select_floor_music(player_ptr);
	}

	if (!current_world_ptr->character_dungeon)
	{
		change_floor(player_ptr);
	}
	else
	{
		if (player_ptr->panic_save)
		{
			if (!player_ptr->y || !player_ptr->x)
			{
				msg_print(_("プレイヤーの位置がおかしい。フロアを再生成します。", "What a strange player location, regenerate the dungeon floor."));
				change_floor(player_ptr);
			}

			if (!player_ptr->y || !player_ptr->x) player_ptr->y = player_ptr->x = 10;

			player_ptr->panic_save = 0;
		}
	}

	current_world_ptr->character_generated = TRUE;
	current_world_ptr->character_icky = FALSE;

	if (new_game)
	{
		char buf[80];
		sprintf(buf, _("%sに降り立った。", "arrived in %s."), map_name(player_ptr));
		exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, buf);
	}

	player_ptr->playing = TRUE;
	reset_visuals(player_ptr, process_autopick_file_command);
	load_all_pref_files(player_ptr);
	if (new_game)
	{
		player_outfit(player_ptr);
	}

	Term_xtra(TERM_XTRA_REACT, 0);

	player_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
	player_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);
	handle_stuff(player_ptr);

	if (arg_force_original) rogue_like_commands = FALSE;
	if (arg_force_roguelike) rogue_like_commands = TRUE;

	if (player_ptr->chp < 0) player_ptr->is_dead = TRUE;

	if (player_ptr->prace == RACE_ANDROID) calc_android_exp(player_ptr);

	if (new_game && ((player_ptr->pclass == CLASS_CAVALRY) || (player_ptr->pclass == CLASS_BEASTMASTER)))
	{
		monster_type *m_ptr;
		MONRACE_IDX pet_r_idx = ((player_ptr->pclass == CLASS_CAVALRY) ? MON_HORSE : MON_YASE_HORSE);
		monster_race *r_ptr = &r_info[pet_r_idx];
		place_monster_aux(player_ptr, 0, player_ptr->y, player_ptr->x - 1, pet_r_idx,
			(PM_FORCE_PET | PM_NO_KAGE));
		m_ptr = &floor_ptr->m_list[hack_m_idx_ii];
		m_ptr->mspeed = r_ptr->speed;
		m_ptr->maxhp = r_ptr->hdice*(r_ptr->hside + 1) / 2;
		m_ptr->max_maxhp = m_ptr->maxhp;
		m_ptr->hp = r_ptr->hdice*(r_ptr->hside + 1) / 2;
		m_ptr->dealt_damage = 0;
		m_ptr->energy_need = ENERGY_NEED() + ENERGY_NEED();
	}

	(void)combine_and_reorder_home(STORE_HOME);
	(void)combine_and_reorder_home(STORE_MUSEUM);
	select_floor_music(player_ptr);

	while (TRUE)
	{
		dungeon(player_ptr, load_game);
		current_world_ptr->character_xtra = TRUE;
		handle_stuff(player_ptr);

		current_world_ptr->character_xtra = FALSE;
		target_who = 0;
		health_track(player_ptr, 0);
		forget_lite(floor_ptr);
		forget_view(floor_ptr);
		clear_mon_lite(floor_ptr);
		if (!player_ptr->playing && !player_ptr->is_dead) break;

		wipe_o_list(floor_ptr);
		if (!player_ptr->is_dead) wipe_monsters_list(player_ptr);

		msg_print(NULL);
		load_game = FALSE;
		if (player_ptr->playing && player_ptr->is_dead)
		{
			if (floor_ptr->inside_arena)
			{
				floor_ptr->inside_arena = FALSE;
				if (player_ptr->arena_number > MAX_ARENA_MONS)
					player_ptr->arena_number++;
				else
					player_ptr->arena_number = -1 - player_ptr->arena_number;
				player_ptr->is_dead = FALSE;
				player_ptr->chp = 0;
				player_ptr->chp_frac = 0;
				player_ptr->exit_bldg = TRUE;
				reset_tim_flags(player_ptr);
				prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_RAND_CONNECT);
				leave_floor(player_ptr);
			}
			else
			{
				if ((current_world_ptr->wizard || cheat_live) && !get_check(_("死にますか? ", "Die? ")))
				{
					cheat_death(player_ptr);
				}
			}
		}

		if (player_ptr->is_dead) break;

		change_floor(player_ptr);
	}

	close_game(player_ptr);
	quit(NULL);
}
