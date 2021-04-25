﻿#pragma once
/*!
 * @file random-art-generator.h
 * @brief ランダムアーティファクトの生成メインヘッダ / Artifact code
 */

#include "system/angband.h"

bool become_random_artifact(player_type *player_ptr, object_type *o_ptr, bool a_scroll);
