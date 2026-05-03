#include "info-reader/dungeon-reader.h"
#include "external-lib/include-json.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "io/tokenizer.h"
#include "main/angband-headers.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <memory>
#include <span>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_dungeon_flag(DungeonDefinition &dungeon, std::string_view what)
{
    if (EnumClassFlagGroup<DungeonFeatureType>::grab_one_flag(dungeon.flags, dungeon_flags, what)) {
        return true;
    }

    msg_format(_("未知のダンジョン・フラグ '%s'。", "Unknown dungeon type flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_basic_monster_flag(DungeonDefinition &dungeon, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterResistanceType>::grab_one_flag(dungeon.mon_resistance_flags, r_info_flagsr, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBehaviorType>::grab_one_flag(dungeon.mon_behavior_flags, r_info_behavior_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterVisualType>::grab_one_flag(dungeon.mon_visual_flags, r_info_visual_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterKindType>::grab_one_flag(dungeon.mon_kind_flags, r_info_kind_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterDropType>::grab_one_flag(dungeon.mon_drop_flags, r_info_drop_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterWildernessType>::grab_one_flag(dungeon.mon_wilderness_flags, r_info_wilderness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterFeatureType>::grab_one_flag(dungeon.mon_feature_flags, r_info_feature_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterPopulationType>::grab_one_flag(dungeon.mon_population_flags, r_info_population_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpeakType>::grab_one_flag(dungeon.mon_speak_flags, r_info_speak_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBrightnessType>::grab_one_flag(dungeon.mon_brightness_flags, r_info_brightness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpecialType>::grab_one_flag(dungeon.mon_special_flags, r_info_special_flags, what)) {
        return true;
    }
    if (EnumClassFlagGroup<MonsterMiscType>::grab_one_flag(dungeon.mon_misc_flags, r_info_misc_flags, what)) {
        return true;
    }

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_spell_monster_flag(DungeonDefinition &dungeon, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterAbilityType>::grab_one_flag(dungeon.mon_ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

static tl::optional<ProbabilityTable<short>> parse_terrain_probability(const nlohmann::json &tiles_obj)
{
    if (tiles_obj.is_null() || !tiles_obj.is_array()) {
        return tl::nullopt;
    }

    const auto &terrains = TerrainList::get_instance();
    ProbabilityTable<short> prob_table;
    for (const auto &tile_obj : tiles_obj) {
        if (!tile_obj.is_object() || !tile_obj["type"].is_string() || !tile_obj["rate"].is_number_integer()) {
            return tl::nullopt;
        }

        try {
            const auto terrain_id = terrains.get_terrain_id(tile_obj["type"].get<std::string>());
            const auto prob = tile_obj["rate"].get<short>();
            prob_table.entry_item(terrain_id, prob);
        } catch (const std::exception &) {
            return tl::nullopt;
        }
    }

    return prob_table;
}

static errr set_dungeon_description(const nlohmann::json &description_obj, DungeonDefinition &dungeon)
{
    return info_set_string(description_obj, dungeon.text, false);
}

static errr set_dungeon_generation(const nlohmann::json &generation_obj, DungeonDefinition &dungeon)
{
    if (generation_obj.is_null() || !generation_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    if (auto err = info_set_integer(generation_obj["minDepth"], dungeon.mindepth, true)) {
        return err;
    }
    if (auto err = info_set_integer(generation_obj["maxDepth"], dungeon.maxdepth, true)) {
        return err;
    }
    if (auto err = info_set_integer(generation_obj["minPlayerLevel"], dungeon.min_plev, true)) {
        return err;
    }
    int mode = 0;
    if (auto err = info_set_integer(generation_obj["flagsMode"], mode, true, Range(0, 4))) {
        return err;
    }
    dungeon.mode = static_cast<DungeonMode>(mode);
    if (auto err = info_set_integer(generation_obj["minAlloc"], dungeon.min_m_alloc_level, true)) {
        return err;
    }
    if (auto err = info_set_integer(generation_obj["maxAllocChance"], dungeon.max_m_alloc_chance, true)) {
        return err;
    }
    if (auto err = info_set_integer(generation_obj["objGood"], dungeon.obj_good, true)) {
        return err;
    }
    if (auto err = info_set_integer(generation_obj["objGreat"], dungeon.obj_great, true)) {
        return err;
    }

    try {
        dungeon.pit = static_cast<BIT_FLAGS16>(std::stoul(generation_obj["pit"].get<std::string>(), nullptr, 16));
        dungeon.nest = static_cast<BIT_FLAGS16>(std::stoul(generation_obj["nest"].get<std::string>(), nullptr, 16));
    } catch (const std::exception &) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    return PARSE_ERROR_NONE;
}

static errr set_dungeon_floor(const nlohmann::json &floor_obj, DungeonDefinition &dungeon)
{
    if (floor_obj.is_null() || !floor_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    auto prob_table = parse_terrain_probability(floor_obj["tiles"]);
    if (!prob_table) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }
    dungeon.prob_table_floor = std::move(*prob_table);

    return info_set_integer(floor_obj["tunnelRate"], dungeon.tunnel_percent, true);
}

static errr set_dungeon_wall(const nlohmann::json &wall_obj, DungeonDefinition &dungeon)
{
    if (wall_obj.is_null() || !wall_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    auto prob_table = parse_terrain_probability(wall_obj["tiles"]);
    if (!prob_table) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }
    dungeon.prob_table_wall = std::move(*prob_table);

    const auto &terrains = TerrainList::get_instance();
    try {
        dungeon.outer_wall = terrains.get_terrain_id(wall_obj["outer"].get<std::string>());
        dungeon.inner_wall = terrains.get_terrain_id(wall_obj["inner"].get<std::string>());
        dungeon.stream1 = terrains.get_terrain_id(wall_obj["stream1"].get<std::string>());
        dungeon.stream2 = terrains.get_terrain_id(wall_obj["stream2"].get<std::string>());
    } catch (const std::exception &) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }

    return PARSE_ERROR_NONE;
}

static errr set_dungeon_flags(const nlohmann::json &flags_obj, DungeonDefinition &dungeon)
{
    if (flags_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flags_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &f_obj : flags_obj) {
        if (!f_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto f = f_obj.get<std::string>();
        if (f.empty()) {
            continue;
        }

        const auto &f_tokens = str_split(f, '_');
        if (f_tokens.size() == 3) {
            if (f_tokens[0] == "FINAL" && f_tokens[1] == "ARTIFACT") {
                info_set_value(dungeon.final_artifact, f_tokens[2]);
                continue;
            }
            if (f_tokens[0] == "FINAL" && f_tokens[1] == "OBJECT") {
                info_set_value(dungeon.final_object, f_tokens[2]);
                continue;
            }
            if (f_tokens[0] == "FINAL" && f_tokens[1] == "GUARDIAN") {
                info_set_value(dungeon.final_guardian, f_tokens[2]);
                continue;
            }
            if (f_tokens[0] == "MONSTER" && f_tokens[1] == "DIV") {
                info_set_value(dungeon.special_div, f_tokens[2]);
                continue;
            }
        }

        if (!grab_one_dungeon_flag(dungeon, f)) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }

    return PARSE_ERROR_NONE;
}

static errr set_dungeon_monster_flags(const nlohmann::json &flags_obj, DungeonDefinition &dungeon)
{
    if (flags_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flags_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &f_obj : flags_obj) {
        if (!f_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto f = f_obj.get<std::string>();
        if (f.empty()) {
            continue;
        }

        const auto &m_tokens = str_split(f, '_');
        if (m_tokens.size() >= 3 && m_tokens[0] == "R" && m_tokens[1] == "CHAR") {
            dungeon.r_chars.insert(dungeon.r_chars.end(), m_tokens[2].begin(), m_tokens[2].end());
            continue;
        }

        if (!grab_one_basic_monster_flag(dungeon, f)) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }

    return PARSE_ERROR_NONE;
}

static errr set_dungeon_monster_spells(const nlohmann::json &spells_obj, DungeonDefinition &dungeon)
{
    if (spells_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!spells_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &s_obj : spells_obj) {
        if (!s_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto s = s_obj.get<std::string>();
        if (s.empty()) {
            continue;
        }

        const auto &s_tokens = str_split(s, '_');
        if (s_tokens.size() == 3 && s_tokens[1] == "IN") {
            if (s_tokens[0] != "1") {
                return PARSE_ERROR_GENERIC;
            }
            continue;
        }

        if (!grab_one_spell_monster_flag(dungeon, s)) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }

    return PARSE_ERROR_NONE;
}

errr parse_dungeons_info(nlohmann::json &element, angband_header *)
{
    if (element.is_null() || !element.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    int id = 0;
    if (auto err = info_set_integer(element["id"], id, true, Range(0, 9999))) {
        return err;
    }
    if (id < error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }
    error_idx = id;

    DungeonDefinition dungeon;
    if (auto err = info_set_string(element["name"], dungeon.name, true)) {
        return err;
    }
    if (auto err = set_dungeon_description(element["description"], dungeon)) {
        return err;
    }

    const auto &position_obj = element["position"];
    if (position_obj.is_null() || !position_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    int wild_y = 0;
    int wild_x = 0;
    if (auto err = info_set_integer(position_obj["min"], wild_y, true)) {
        return err;
    }
    if (auto err = info_set_integer(position_obj["max"], wild_x, true)) {
        return err;
    }
    dungeon.initialize_position({ wild_y, wild_x });

    if (auto err = set_dungeon_generation(element["generation"], dungeon)) {
        return err;
    }
    if (auto err = set_dungeon_floor(element["floor"], dungeon)) {
        return err;
    }
    if (auto err = set_dungeon_wall(element["wall"], dungeon)) {
        return err;
    }
    if (auto err = set_dungeon_flags(element["flags"], dungeon)) {
        return err;
    }
    if (auto err = set_dungeon_monster_flags(element["monsterFlags"], dungeon)) {
        return err;
    }
    if (auto err = set_dungeon_monster_spells(element["monsterSpells"], dungeon)) {
        return err;
    }

    auto &dungeons = DungeonList::get_instance();
    dungeons.emplace(i2enum<DungeonId>(id), std::move(dungeon));
    return PARSE_ERROR_NONE;
}
