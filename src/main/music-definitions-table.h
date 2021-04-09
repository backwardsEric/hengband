﻿#pragma once

enum music_type {
	MUSIC_BASIC_DEFAULT = 0,
    MUSIC_BASIC_GAMEOVER = 1,
    MUSIC_BASIC_EXIT = 2,
    MUSIC_BASIC_TOWN = 3,
    MUSIC_BASIC_FIELD1 = 4,
    MUSIC_BASIC_FIELD2 = 5,
    MUSIC_BASIC_FIELD3 = 6,
    MUSIC_BASIC_DUN_LOW = 7,
    MUSIC_BASIC_DUN_MED = 8,
    MUSIC_BASIC_DUN_HIGH = 9,
    MUSIC_BASIC_DUN_FEEL1 = 10,
    MUSIC_BASIC_DUN_FEEL2 = 11,
    MUSIC_BASIC_WINNER = 12,
    MUSIC_BASIC_BUILD = 13,
    MUSIC_BASIC_WILD = 14,
    MUSIC_BASIC_QUEST = 15,
    MUSIC_BASIC_ARENA = 16,
    MUSIC_BASIC_BATTLE = 17,
    MUSIC_BASIC_QUEST_CLEAR = 18,
    MUSIC_BASIC_FINAL_QUEST_CLEAR = 19,
    MUSIC_BASIC_AMBUSH = 20,
    MUSIC_BASIC_MAX = 21, /*!< BGM定義の最大数 */
};

extern const concptr angband_music_basic_name[MUSIC_BASIC_MAX];