﻿#include <vector>

#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/line-of-sight.h"
#include "game-option/cheat-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/input-key-requester.h"
#include "io/screen-util.h"
#include "main/sound-of-music.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-describer.h"
#include "target/target-preparation.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "window/display-sub-windows.h"
#include "window/main-window-util.h"

// "interesting" な座標たちを記録する配列。
// ang_sort() を利用する関係上、y/x座標それぞれについて配列を作る。
static std::vector<POSITION> ys_interest;
static std::vector<POSITION> xs_interest;

// Target Setter.
typedef struct ts_type {
    target_type mode;
    POSITION y;
    POSITION x;
    POSITION y2; // panel_row_min 退避用
    POSITION x2; // panel_col_min 退避用
    bool done;
    bool flag; // 移動コマンド入力時、"interesting" な座標へ飛ぶかどうか
    char query;
    char info[80];
    grid_type *g_ptr;
    TERM_LEN wid, hgt;
    int m; // "interesting" な座標たちのうち現在ターゲットしているもののインデックス
    int distance; // カーソルの移動方向 (1,2,3,4,6,7,8,9)
    int target_num; // target_pick() の結果
    bool move_fast; // カーソル移動を粗くする(1マスずつ移動しない)
} ts_type;

static ts_type *initialize_target_set_type(player_type *creature_ptr, ts_type *ts_ptr, target_type mode)
{
    ts_ptr->mode = mode;
    ts_ptr->y = creature_ptr->y;
    ts_ptr->x = creature_ptr->x;
    ts_ptr->done = FALSE;
    ts_ptr->flag = TRUE;
    get_screen_size(&ts_ptr->wid, &ts_ptr->hgt);
    ts_ptr->m = 0;
    return ts_ptr;
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y 変更先のフロアY座標
 * @param x 変更先のフロアX座標
 * @details
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
static bool change_panel_xy(player_type *creature_ptr, POSITION y, POSITION x)
{
    POSITION dy = 0, dx = 0;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    if (y < panel_row_min)
        dy = -1;

    if (y > panel_row_max)
        dy = 1;

    if (x < panel_col_min)
        dx = -1;

    if (x > panel_col_max)
        dx = 1;

    if (!dy && !dx)
        return FALSE;

    return change_panel(creature_ptr, dy, dx);
}

/*!
 * @brief "interesting" な座標たちのうち、(y1,x1) から (dy,dx) 方向にある最も近いもののインデックスを得る。
 * @param y1 現在地座標y
 * @param x1 現在地座標x
 * @param dy 現在地からの向きy [-1,1]
 * @param dx 現在地からの向きx [-1,1]
 * @return 最も近い座標のインデックス。適切なものがない場合 -1
 */
static POSITION_IDX target_pick(const POSITION y1, const POSITION x1, const POSITION dy, const POSITION dx)
{
    // 最も近いもののインデックスとその距離。
    POSITION_IDX b_i = -1, b_v = 9999;

    for (POSITION_IDX i = 0; i < (POSITION_IDX)size(ys_interest); i++) {
        const POSITION x2 = xs_interest[i];
        const POSITION y2 = ys_interest[i];

        // (y1,x1) から (y2,x2) へ向かうベクトル。
        const POSITION x3 = (x2 - x1);
        const POSITION y3 = (y2 - y1);

        // (dy,dx) 方向にないものを除外する。

        // dx > 0 のとき、x3 <= 0 なるものは除外。
        // dx < 0 のとき、x3 >= 0 なるものは除外。
        if (dx && (x3 * dx <= 0))
            continue;

        // dy > 0 のとき、y3 <= 0 なるものは除外。
        // dy < 0 のとき、y3 >= 0 なるものは除外。
        if (dy && (y3 * dy <= 0))
            continue;

        const POSITION x4 = ABS(x3);
        const POSITION y4 = ABS(y3);

        // (dy,dx) が (-1,0) or (1,0) のとき、|x3| > |y3| なるものは除外。
        if (dy && !dx && (x4 > y4))
            continue;

        // (dy,dx) が (0,-1) or (0,1) のとき、|y3| > |x3| なるものは除外。
        if (dx && !dy && (y4 > x4))
            continue;

        // (y1,x1), (y2,x2) 間の距離を求め、最も近いものを更新する。
        // 距離の定義は v の式を参照。
        const POSITION_IDX v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));
        if ((b_i >= 0) && (v >= b_v))
            continue;

        b_i = i;
        b_v = v;
    }

    return b_i;
}

static void describe_projectablity(player_type *creature_ptr, ts_type *ts_ptr)
{
    ts_ptr->y = ys_interest[ts_ptr->m];
    ts_ptr->x = xs_interest[ts_ptr->m];
    change_panel_xy(creature_ptr, ts_ptr->y, ts_ptr->x);
    if ((ts_ptr->mode & TARGET_LOOK) == 0)
        print_path(creature_ptr, ts_ptr->y, ts_ptr->x);

    ts_ptr->g_ptr = &creature_ptr->current_floor_ptr->grid_array[ts_ptr->y][ts_ptr->x];
    if (target_able(creature_ptr, ts_ptr->g_ptr->m_idx))
        strcpy(ts_ptr->info, _("q止 t決 p自 o現 +次 -前", "q,t,p,o,+,-,<dir>"));
    else
        strcpy(ts_ptr->info, _("q止 p自 o現 +次 -前", "q,p,o,+,-,<dir>"));

    if (!cheat_sight)
        return;

    char cheatinfo[30];
    sprintf(cheatinfo, " LOS:%d, PROJECTABLE:%d", los(creature_ptr, creature_ptr->y, creature_ptr->x, ts_ptr->y, ts_ptr->x),
        projectable(creature_ptr, creature_ptr->y, creature_ptr->x, ts_ptr->y, ts_ptr->x));
    strcat(ts_ptr->info, cheatinfo);
}

static void menu_target(ts_type *ts_ptr)
{
    if (!use_menu)
        return;

    if (ts_ptr->query == '\r')
        ts_ptr->query = 't';
}

static void switch_target_input(player_type *creature_ptr, ts_type *ts_ptr)
{
    ts_ptr->distance = 0;
    switch (ts_ptr->query) {
    case ESCAPE:
    case 'q':
        ts_ptr->done = TRUE;
        return;
    case 't':
    case '.':
    case '5':
    case '0':
        if (!target_able(creature_ptr, ts_ptr->g_ptr->m_idx)) {
            bell();
            return;
        }

        health_track(creature_ptr, ts_ptr->g_ptr->m_idx);
        target_who = ts_ptr->g_ptr->m_idx;
        target_row = ts_ptr->y;
        target_col = ts_ptr->x;
        ts_ptr->done = TRUE;
        return;
    case ' ':
    case '*':
    case '+':
        if (++ts_ptr->m != (int)size(ys_interest))
            return;

        ts_ptr->m = 0;
        if (!expand_list)
            ts_ptr->done = TRUE;

        return;
    case '-':
        if (ts_ptr->m-- != 0)
            return;

        ts_ptr->m = (int)size(ys_interest) - 1;
        if (!expand_list)
            ts_ptr->done = TRUE;

        return;
    case 'p': {
        verify_panel(creature_ptr);
        creature_ptr->update |= PU_MONSTERS;
        creature_ptr->redraw |= PR_MAP;
        creature_ptr->window_flags |= PW_OVERHEAD;
        handle_stuff(creature_ptr);
        target_set_prepare(creature_ptr, ys_interest, xs_interest, ts_ptr->mode);
        ts_ptr->y = creature_ptr->y;
        ts_ptr->x = creature_ptr->x;
    }
        /* Fall through */
    case 'o':
        ts_ptr->flag = FALSE;
        return;
    case 'm':
        return;
    default: {
        const char queried_command = rogue_like_commands ? 'x' : 'l';
        if (ts_ptr->query != queried_command) {
            ts_ptr->distance = get_keymap_dir(ts_ptr->query);
            if (ts_ptr->distance == 0)
                bell();

            return;
        }

        if (++ts_ptr->m != (int)size(ys_interest))
            return;

        ts_ptr->m = 0;
        if (!expand_list)
            ts_ptr->done = TRUE;

        return;
    }
    }
}

/*!
 * @brief カーソル移動に伴い、描画範囲、"interesting" 座標リスト、現在のターゲットを更新する。
 * @return カーソル移動によって描画範囲が変化したかどうか
 */
static bool check_panel_changed(player_type *creature_ptr, ts_type *ts_ptr)
{
    // カーソル移動によって描画範囲が変化しないなら何もせずその旨を返す。
    if (!change_panel(creature_ptr, ddy[ts_ptr->distance], ddx[ts_ptr->distance]))
        return FALSE;

    // 描画範囲が変化した場合、"interesting" 座標リストおよび現在のターゲットを更新する必要がある。

    // "interesting" 座標を探す起点。
    // ts_ptr->m が有効な座標を指していればそれを使う。
    // さもなくば (ts_ptr->y, ts_ptr->x) を使う。
    int v, u;
    if (ts_ptr->m < (int)size(ys_interest)) {
        v = ys_interest[ts_ptr->m];
        u = xs_interest[ts_ptr->m];
    } else {
        v = ts_ptr->y;
        u = ts_ptr->x;
    }

    // 新たな描画範囲を用いて "interesting" 座標リストを更新。
    target_set_prepare(creature_ptr, ys_interest, xs_interest, ts_ptr->mode);

    // 新たな "interesting" 座標リストからターゲットを探す。
    ts_ptr->flag = TRUE;
    ts_ptr->target_num = target_pick(v, u, ddy[ts_ptr->distance], ddx[ts_ptr->distance]);
    if (ts_ptr->target_num >= 0)
        ts_ptr->m = ts_ptr->target_num;

    return TRUE;
}

/*!
 * @brief カーソル移動方向に "interesting" な座標がなかったとき、画面外まで探す。
 *
 * 既に "interesting" な座標を発見している場合、この関数は何もしない。
 */
static void sweep_targets(player_type *creature_ptr, ts_type *ts_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    while (ts_ptr->flag && (ts_ptr->target_num < 0)) {
        // カーソル移動に伴い、必要なだけ描画範囲を更新。
        // "interesting" 座標リストおよび現在のターゲットも更新。
        if (check_panel_changed(creature_ptr, ts_ptr))
            continue;

        POSITION dx = ddx[ts_ptr->distance];
        POSITION dy = ddy[ts_ptr->distance];
        panel_row_min = ts_ptr->y2;
        panel_col_min = ts_ptr->x2;
        panel_bounds_center();
        creature_ptr->update |= PU_MONSTERS;
        creature_ptr->redraw |= PR_MAP;
        creature_ptr->window_flags |= PW_OVERHEAD;
        handle_stuff(creature_ptr);
        target_set_prepare(creature_ptr, ys_interest, xs_interest, ts_ptr->mode);
        ts_ptr->flag = FALSE;
        ts_ptr->x += dx;
        ts_ptr->y += dy;
        if (((ts_ptr->x < panel_col_min + ts_ptr->wid / 2) && (dx > 0)) || ((ts_ptr->x > panel_col_min + ts_ptr->wid / 2) && (dx < 0)))
            dx = 0;

        if (((ts_ptr->y < panel_row_min + ts_ptr->hgt / 2) && (dy > 0)) || ((ts_ptr->y > panel_row_min + ts_ptr->hgt / 2) && (dy < 0)))
            dy = 0;

        if ((ts_ptr->y >= panel_row_min + ts_ptr->hgt) || (ts_ptr->y < panel_row_min) || (ts_ptr->x >= panel_col_min + ts_ptr->wid)
            || (ts_ptr->x < panel_col_min)) {
            if (change_panel(creature_ptr, dy, dx))
                target_set_prepare(creature_ptr, ys_interest, xs_interest, ts_ptr->mode);
        }

        if (ts_ptr->x >= floor_ptr->width - 1)
            ts_ptr->x = floor_ptr->width - 2;
        else if (ts_ptr->x <= 0)
            ts_ptr->x = 1;

        if (ts_ptr->y >= floor_ptr->height - 1)
            ts_ptr->y = floor_ptr->height - 2;
        else if (ts_ptr->y <= 0)
            ts_ptr->y = 1;
    }
}

static bool set_target_grid(player_type *creature_ptr, ts_type *ts_ptr)
{
    if (!ts_ptr->flag || ys_interest.empty())
        return FALSE;

    describe_projectablity(creature_ptr, ts_ptr);
    fix_floor_item_list(creature_ptr, ts_ptr->y, ts_ptr->x);

    while (TRUE) {
        ts_ptr->query = examine_grid(creature_ptr, ts_ptr->y, ts_ptr->x, ts_ptr->mode, ts_ptr->info);
        if (ts_ptr->query)
            break;
    }

    menu_target(ts_ptr);
    switch_target_input(creature_ptr, ts_ptr);
    if (ts_ptr->distance == 0)
        return TRUE;

    ts_ptr->y2 = panel_row_min;
    ts_ptr->x2 = panel_col_min;
    {
        const POSITION y = ys_interest[ts_ptr->m];
        const POSITION x = xs_interest[ts_ptr->m];
        ts_ptr->target_num = target_pick(y, x, ddy[ts_ptr->distance], ddx[ts_ptr->distance]);
    }
    sweep_targets(creature_ptr, ts_ptr);
    ts_ptr->m = ts_ptr->target_num;
    return TRUE;
}

static void describe_grid_wizard(player_type *creature_ptr, ts_type *ts_ptr)
{
    if (!cheat_sight)
        return;

    char cheatinfo[100];
    sprintf(cheatinfo, " LOS:%d, PROJECTABLE:%d, SPECIAL:%d", los(creature_ptr, creature_ptr->y, creature_ptr->x, ts_ptr->y, ts_ptr->x),
        projectable(creature_ptr, creature_ptr->y, creature_ptr->x, ts_ptr->y, ts_ptr->x), ts_ptr->g_ptr->special);
    strcat(ts_ptr->info, cheatinfo);
}

static void switch_next_grid_command(player_type *creature_ptr, ts_type *ts_ptr)
{
    switch (ts_ptr->query) {
    case ESCAPE:
    case 'q':
        ts_ptr->done = TRUE;
        break;
    case 't':
    case '.':
    case '5':
    case '0':
        target_who = -1;
        target_row = ts_ptr->y;
        target_col = ts_ptr->x;
        ts_ptr->done = TRUE;
        break;
    case 'p':
        verify_panel(creature_ptr);
        creature_ptr->update |= PU_MONSTERS;
        creature_ptr->redraw |= PR_MAP;
        creature_ptr->window_flags |= PW_OVERHEAD;
        handle_stuff(creature_ptr);
        target_set_prepare(creature_ptr, ys_interest, xs_interest, ts_ptr->mode);
        ts_ptr->y = creature_ptr->y;
        ts_ptr->x = creature_ptr->x;
    case 'o':
        //!< @todo ↑元からbreakしていないがFall Throughを付けてよいか不明なので保留
        break;
    case ' ':
    case '*':
    case '+':
    case '-':
    case 'm': {
        ts_ptr->flag = TRUE;
        ts_ptr->m = 0;
        int bd = 999;
        for (size_t i = 0; i < size(ys_interest); i++) {
            const POSITION y = ys_interest[i];
            const POSITION x = xs_interest[i];
            int t = distance(ts_ptr->y, ts_ptr->x, y, x);
            if (t < bd) {
                ts_ptr->m = i;
                bd = t;
            }
        }

        if (bd == 999)
            ts_ptr->flag = FALSE;

        break;
    }
    default:
        ts_ptr->distance = get_keymap_dir(ts_ptr->query);
        if (isupper(ts_ptr->query))
            ts_ptr->move_fast = TRUE;

        if (!ts_ptr->distance)
            bell();

        break;
    }
}

static void decide_change_panel(player_type *creature_ptr, ts_type *ts_ptr)
{
    if (ts_ptr->distance == 0)
        return;

    POSITION dx = ddx[ts_ptr->distance];
    POSITION dy = ddy[ts_ptr->distance];
    if (ts_ptr->move_fast) {
        int mag = MIN(ts_ptr->wid / 2, ts_ptr->hgt / 2);
        ts_ptr->x += dx * mag;
        ts_ptr->y += dy * mag;
    } else {
        ts_ptr->x += dx;
        ts_ptr->y += dy;
    }

    if (((ts_ptr->x < panel_col_min + ts_ptr->wid / 2) && (dx > 0)) || ((ts_ptr->x > panel_col_min + ts_ptr->wid / 2) && (dx < 0)))
        dx = 0;

    if (((ts_ptr->y < panel_row_min + ts_ptr->hgt / 2) && (dy > 0)) || ((ts_ptr->y > panel_row_min + ts_ptr->hgt / 2) && (dy < 0)))
        dy = 0;

    if ((ts_ptr->y >= panel_row_min + ts_ptr->hgt) || (ts_ptr->y < panel_row_min) || (ts_ptr->x >= panel_col_min + ts_ptr->wid)
        || (ts_ptr->x < panel_col_min)) {
        if (change_panel(creature_ptr, dy, dx))
            target_set_prepare(creature_ptr, ys_interest, xs_interest, ts_ptr->mode);
    }

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (ts_ptr->x >= floor_ptr->width - 1)
        ts_ptr->x = floor_ptr->width - 2;
    else if (ts_ptr->x <= 0)
        ts_ptr->x = 1;

    if (ts_ptr->y >= floor_ptr->height - 1)
        ts_ptr->y = floor_ptr->height - 2;
    else if (ts_ptr->y <= 0)
        ts_ptr->y = 1;
}

static void sweep_target_grids(player_type *creature_ptr, ts_type *ts_ptr)
{
    while (!ts_ptr->done) {
        if (set_target_grid(creature_ptr, ts_ptr))
            continue;

        ts_ptr->move_fast = FALSE;
        if ((ts_ptr->mode & TARGET_LOOK) == 0)
            print_path(creature_ptr, ts_ptr->y, ts_ptr->x);

        ts_ptr->g_ptr = &creature_ptr->current_floor_ptr->grid_array[ts_ptr->y][ts_ptr->x];
        strcpy(ts_ptr->info, _("q止 t決 p自 m近 +次 -前", "q,t,p,m,+,-,<dir>"));
        describe_grid_wizard(creature_ptr, ts_ptr);
        fix_floor_item_list(creature_ptr, ts_ptr->y, ts_ptr->x);

        /* Describe and Prompt (enable "TARGET_LOOK") */
        while ((ts_ptr->query = examine_grid(creature_ptr, ts_ptr->y, ts_ptr->x, static_cast<target_type>(ts_ptr->mode | TARGET_LOOK), ts_ptr->info)) == 0)
            ;

        ts_ptr->distance = 0;
        if (use_menu && (ts_ptr->query == '\r'))
            ts_ptr->query = 't';

        switch_next_grid_command(creature_ptr, ts_ptr);
        decide_change_panel(creature_ptr, ts_ptr);
    }
}

/*
 * Handle "target" and "look".
 */
bool target_set(player_type *creature_ptr, target_type mode)
{
    ts_type tmp_ts;
    ts_type *ts_ptr = initialize_target_set_type(creature_ptr, &tmp_ts, mode);
    target_who = 0;
    target_set_prepare(creature_ptr, ys_interest, xs_interest, mode);
    sweep_target_grids(creature_ptr, ts_ptr);
    prt("", 0, 0);
    verify_panel(creature_ptr);
    set_bits(creature_ptr->update, PU_MONSTERS);
    set_bits(creature_ptr->redraw, PR_MAP);
    set_bits(creature_ptr->window_flags, PW_OVERHEAD | PW_FLOOR_ITEM_LIST);
    handle_stuff(creature_ptr);
    return target_who != 0;
}