#include "info-reader/terrain-reader.h"
#include "info-reader/feature-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/gameterm.h"
#include "util/dice.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る
 */
static bool grab_one_feat_flag(TerrainType &terrain, std::string_view what)
{
    if (EnumClassFlagGroup<TerrainCharacteristics>::grab_one_flag(terrain.flags, f_info_flags, what)) {
        return true;
    }

    msg_format(_("未知の地形フラグ '%s'。", "Unknown feature flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る
 */
static std::optional<TerrainCharacteristics> parse_terrain_action(std::string_view what)
{
    if (auto it = f_info_flags.find(what); it != f_info_flags.end()) {
        return it->second;
    }

    return std::nullopt;
}

/*!
 * @brief JSON Objectから地形シンボルをセットする
 */
static errr set_terrain_symbol(const nlohmann::json &symbol_obj, TerrainType &terrain)
{
    if (symbol_obj.is_null() || !symbol_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &ch_obj = symbol_obj["character"];
    const auto &color_obj = symbol_obj["color"];
    const auto &lit_obj = symbol_obj["lit"];

    if (!ch_obj.is_string() || !color_obj.is_string() || !lit_obj.is_boolean()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto ch_str = ch_obj.get<std::string>();
    if (ch_str.empty()) {
        return PARSE_ERROR_GENERIC;
    }

    const auto color_name = color_obj.get<std::string>();
    const auto it = color_list.find(color_name);
    if (it == color_list.end()) {
        msg_format(_("未知の色 '%s'。", "Unknown color '%s'."), color_name.c_str());
        return PARSE_ERROR_INVALID_FLAG;
    }
    if (it->second > 127) {
        return PARSE_ERROR_GENERIC;
    }

    const auto character = ch_str.front();
    const auto color = it->second;

    terrain.symbol_definitions[F_LIT_STANDARD] = { color, character };

    const bool lit = lit_obj.get<bool>();
    if (lit) {
        terrain.reset_lighting(false);
        return PARSE_ERROR_NONE;
    }

    for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
        terrain.symbol_definitions[j] = { color, character };
    }

    return PARSE_ERROR_NONE;
}

static errr set_terrain_conversion(const nlohmann::json &convert_obj, TerrainType &terrain)
{
    if (convert_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!convert_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &type_obj = convert_obj["type"];
    if (!type_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    static const std::unordered_map<std::string_view, TerrainConversionType> conversion_types = {
        { "FLOOR", TerrainConversionType::FLOOR },
        { "WALL", TerrainConversionType::WALL },
        { "INNER", TerrainConversionType::INNER },
        { "OUTER", TerrainConversionType::OUTER },
        { "SOLID", TerrainConversionType::SOLID },
        { "STREAM", TerrainConversionType::STREAM },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = conversion_types.find(type);
    if (it == conversion_types.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    auto stream_index = -1;
    if (it->second == TerrainConversionType::STREAM) {
        if (auto err = info_set_integer(convert_obj["stream_index"], stream_index, true, Range(0, 254))) {
            return err;
        }
    }

    terrain.flags.set(TerrainCharacteristics::CONVERT);
    return terrain.init_conversion_type(it->second, stream_index) ? PARSE_ERROR_NONE : PARSE_ERROR_INVALID_VALUE;
}

static errr set_terrain_generation(const nlohmann::json &generation_obj, TerrainType &terrain)
{
    if (generation_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!generation_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &changes_obj = generation_obj["changes"];
    if (!changes_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    auto probability_sum = 0;
    for (const auto &change_obj : changes_obj) {
        if (!change_obj.is_object()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &terrain_obj = change_obj["terrain"];
        if (!terrain_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto change = TerrainGenerationChange{};
        change.result_tag = terrain_obj.get<std::string>();
        if (change.result_tag.empty()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        if (auto err = info_set_integer(change_obj["probability"], change.probability, true, Range(1, 100))) {
            return err;
        }

        probability_sum += change.probability;
        if (probability_sum > 100) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        terrain.generation_changes.push_back(change);
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief 地形の用途別強度をJSON定義から設定する
 * @param power_obj 強度値を表すJSON値
 * @param power 設定先の強度
 * @param is_required trueなら未定義時にエラーとする
 * @return パース結果
 */
static errr set_terrain_power(const nlohmann::json &power_obj, uint8_t &power, bool is_required = false)
{
    return info_set_integer(power_obj, power, is_required, Range(0, 255));
}

static errr set_terrain_trap(const nlohmann::json &trap_obj, TerrainType &terrain)
{
    if (trap_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!trap_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &type_obj = trap_obj["type"];
    if (!type_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (auto err = set_terrain_power(trap_obj["power"], terrain.trap_power, true)) {
        return err;
    }

    static const std::unordered_map<std::string_view, TrapType> trap_types = {
        { "TRAPDOOR", TrapType::TRAPDOOR },
        { "PIT", TrapType::PIT },
        { "SPIKED_PIT", TrapType::SPIKED_PIT },
        { "POISON_PIT", TrapType::POISON_PIT },
        { "TY_CURSE", TrapType::TY_CURSE },
        { "TELEPORT", TrapType::TELEPORT },
        { "FIRE", TrapType::FIRE },
        { "ACID", TrapType::ACID },
        { "SLOW", TrapType::SLOW },
        { "LOSE_STR", TrapType::LOSE_STR },
        { "LOSE_DEX", TrapType::LOSE_DEX },
        { "LOSE_CON", TrapType::LOSE_CON },
        { "BLIND", TrapType::BLIND },
        { "CONFUSE", TrapType::CONFUSE },
        { "POISON", TrapType::POISON },
        { "SLEEP", TrapType::SLEEP },
        { "TRAPS", TrapType::TRAPS },
        { "ALARM", TrapType::ALARM },
        { "OPEN", TrapType::OPEN },
        { "ARMAGEDDON", TrapType::ARMAGEDDON },
        { "PIRANHA", TrapType::PIRANHA },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = trap_types.find(type);
    if (it == trap_types.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    terrain.flags.set(TerrainCharacteristics::TRAP);
    return terrain.init_trap_type(it->second) ? PARSE_ERROR_NONE : PARSE_ERROR_INVALID_VALUE;
}

/*!
 * @brief 扉定義を地形情報へ反映する
 * @param door_obj 扉定義を表すJSONオブジェクト
 * @param terrain 設定先の地形情報
 * @return パース結果
 */
static errr set_terrain_door(const nlohmann::json &door_obj, TerrainType &terrain)
{
    if (door_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!door_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    terrain.flags.set(TerrainCharacteristics::DOOR);
    return set_terrain_power(door_obj["power"], terrain.door_power, true);
}

/*!
 * @brief 掘削定義を地形情報へ反映する
 * @param tunnel_obj 掘削定義を表すJSONオブジェクト
 * @param terrain 設定先の地形情報
 * @return パース結果
 */
static errr set_terrain_tunnel(const nlohmann::json &tunnel_obj, TerrainType &terrain)
{
    if (tunnel_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!tunnel_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    terrain.flags.set(TerrainCharacteristics::TUNNEL);
    return set_terrain_power(tunnel_obj["power"], terrain.tunnel_power, true);
}

static errr set_terrain_pattern(const nlohmann::json &pattern_obj, TerrainType &terrain)
{
    if (pattern_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!pattern_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &type_obj = pattern_obj["type"];
    if (!type_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    static const std::unordered_map<std::string_view, PatternTileType> pattern_types = {
        { "START", PatternTileType::START },
        { "TILE_1", PatternTileType::TILE_1 },
        { "TILE_2", PatternTileType::TILE_2 },
        { "TILE_3", PatternTileType::TILE_3 },
        { "TILE_4", PatternTileType::TILE_4 },
        { "END", PatternTileType::END },
        { "OLD", PatternTileType::OLD },
        { "TELEPORT", PatternTileType::TELEPORT },
        { "WRECKED", PatternTileType::WRECKED },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = pattern_types.find(type);
    if (it == pattern_types.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    terrain.flags.set(TerrainCharacteristics::PATTERN);
    return terrain.init_pattern_tile_type(it->second) ? PARSE_ERROR_NONE : PARSE_ERROR_INVALID_VALUE;
}

static errr set_terrain_store(const nlohmann::json &store_obj, TerrainType &terrain)
{
    if (store_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!store_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &type_obj = store_obj["type"];
    if (!type_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    static const std::unordered_map<std::string_view, StoreSaleType> store_types = {
        { "GENERAL", StoreSaleType::GENERAL },
        { "ARMOURY", StoreSaleType::ARMOURY },
        { "WEAPON", StoreSaleType::WEAPON },
        { "TEMPLE", StoreSaleType::TEMPLE },
        { "ALCHEMIST", StoreSaleType::ALCHEMIST },
        { "MAGIC", StoreSaleType::MAGIC },
        { "BLACK", StoreSaleType::BLACK },
        { "HOME", StoreSaleType::HOME },
        { "BOOK", StoreSaleType::BOOK },
        { "MUSEUM", StoreSaleType::MUSEUM },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = store_types.find(type);
    if (it == store_types.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    terrain.flags.set(TerrainCharacteristics::STORE);
    return terrain.init_store_sale_type(it->second) ? PARSE_ERROR_NONE : PARSE_ERROR_INVALID_VALUE;
}

static errr set_terrain_building(const nlohmann::json &building_obj, TerrainType &terrain)
{
    if (building_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!building_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &type_obj = building_obj["type"];
    if (!type_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    static const std::unordered_map<std::string_view, BuildingType> building_types = {
        { "BUILDING_00", BuildingType::BUILDING_00 },
        { "BUILDING_01", BuildingType::BUILDING_01 },
        { "BUILDING_02", BuildingType::BUILDING_02 },
        { "BUILDING_03", BuildingType::BUILDING_03 },
        { "BUILDING_04", BuildingType::BUILDING_04 },
        { "BUILDING_05", BuildingType::BUILDING_05 },
        { "BUILDING_06", BuildingType::BUILDING_06 },
        { "BUILDING_07", BuildingType::BUILDING_07 },
        { "BUILDING_08", BuildingType::BUILDING_08 },
        { "BUILDING_09", BuildingType::BUILDING_09 },
        { "BUILDING_10", BuildingType::BUILDING_10 },
        { "BUILDING_11", BuildingType::BUILDING_11 },
        { "BUILDING_12", BuildingType::BUILDING_12 },
        { "BUILDING_13", BuildingType::BUILDING_13 },
        { "BUILDING_14", BuildingType::BUILDING_14 },
        { "BUILDING_15", BuildingType::BUILDING_15 },
        { "BUILDING_16", BuildingType::BUILDING_16 },
        { "BUILDING_17", BuildingType::BUILDING_17 },
        { "BUILDING_18", BuildingType::BUILDING_18 },
        { "BUILDING_19", BuildingType::BUILDING_19 },
        { "BUILDING_20", BuildingType::BUILDING_20 },
        { "BUILDING_21", BuildingType::BUILDING_21 },
        { "BUILDING_22", BuildingType::BUILDING_22 },
        { "BUILDING_23", BuildingType::BUILDING_23 },
        { "BUILDING_24", BuildingType::BUILDING_24 },
        { "BUILDING_25", BuildingType::BUILDING_25 },
        { "BUILDING_26", BuildingType::BUILDING_26 },
        { "BUILDING_27", BuildingType::BUILDING_27 },
        { "BUILDING_28", BuildingType::BUILDING_28 },
        { "BUILDING_29", BuildingType::BUILDING_29 },
        { "BUILDING_30", BuildingType::BUILDING_30 },
        { "BUILDING_31", BuildingType::BUILDING_31 },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = building_types.find(type);
    if (it == building_types.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    terrain.flags.set(TerrainCharacteristics::BLDG);
    return terrain.init_building_type(it->second) ? PARSE_ERROR_NONE : PARSE_ERROR_INVALID_VALUE;
}

/*!
 * @brief 地形変化定義を地形情報へ反映する
 * @param interactions_obj 地形変化定義を表すJSONオブジェクト
 * @param terrain 設定先の地形情報
 * @return パース結果
 */
static errr set_terrain_interactions(const nlohmann::json &interactions_obj, TerrainType &terrain)
{
    if (interactions_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!interactions_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto it = interactions_obj.begin(); it != interactions_obj.end(); ++it) {
        const auto action = it.key();
        const auto &value_obj = it.value();
        if (!value_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        const auto result_tag = value_obj.get<std::string>();
        if (result_tag.empty()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        if (action == "DROP_GOLD") {
            Dice dice;
            if (auto err = info_set_dice(value_obj, dice, true)) {
                return err;
            }
            if (dice.is_valid()) {
                terrain.gold_drop = dice;
                terrain.flags.set(TerrainCharacteristics::HAS_GOLD);
            }
            continue;
        }

        if (action == "DESTROYED") {
            terrain.destroyed_tag = result_tag;
            continue;
        }

        const auto state_it = std::find_if(std::begin(terrain.state), std::end(terrain.state),
            [](const auto &s) { return s.action == TerrainCharacteristics::MAX; });
        if (state_it == std::end(terrain.state)) {
            return PARSE_ERROR_GENERIC;
        }

        const auto idx_state = static_cast<int>(std::distance(std::begin(terrain.state), state_it));

        if (auto val = parse_terrain_action(action)) {
            terrain.state[idx_state].action = *val;
            if (*val == TerrainCharacteristics::MAY_HAVE_GOLD) {
                terrain.flags.set(TerrainCharacteristics::MAY_HAVE_GOLD);
            }
        } else {
            msg_format(_("未知の地形アクション '%s'。", "Unknown feature action '%s'."), action.data());
            return PARSE_ERROR_INVALID_FLAG;
        }
        terrain.state[idx_state].result_tag = result_tag;
    }

    return PARSE_ERROR_NONE;
}

errr parse_terrains_json_info(nlohmann::json &element, angband_header *)
{
    if (element.is_null() || !element.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    int id = 0;
    if (auto err = info_set_integer(element["id"], id, true, Range(0, 9999))) {
        return err;
    }

    const auto &key_obj = element["key"];
    if (!key_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto key = key_obj.get<std::string>();
    if (key.empty()) {
        return PARSE_ERROR_GENERIC;
    }

    auto &terrains = TerrainList::get_instance();
    if (id >= static_cast<int>(terrains.size())) {
        terrains.resize(id + 1);
    }

    error_idx = id;

    const auto s = static_cast<short>(id);
    auto &terrain = terrains.get_terrain(s);

    terrain.idx = s;
    terrain.tag = key;

    const auto tag_it = terrain_tags.find(key);
    if (tag_it != terrain_tags.end()) {
        terrain.tag_enum = tag_it->second;
    }

    terrain.mimic = s;
    terrain.destroyed = s;
    terrain.gold_drop = Dice{};
    terrain.generation_changes.clear();
    for (auto j = 0; j < MAX_FEAT_STATES; j++) {
        terrain.state[j].action = TerrainCharacteristics::MAX;
        terrain.state[j].result_tag.clear();
    }

    if (auto err = info_set_string(element["name"], terrain.name, true)) {
        return err;
    }

    if (auto err = set_terrain_symbol(element["symbol"], terrain)) {
        return err;
    }

    const auto &mimic_obj = element["mimic"];
    if (!mimic_obj.is_null()) {
        if (!mimic_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        terrain.mimic_tag = mimic_obj.get<std::string>();
    }

    const auto &prio_obj = element["map_priority"];
    if (!prio_obj.is_null()) {
        if (auto err = info_set_integer(prio_obj, terrain.priority, true, Range(-9999, 9999))) {
            return err;
        }
    }

    const auto &flags_obj = element["flags"];
    if (flags_obj.is_null() || !flags_obj.is_array()) {
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

        if (!grab_one_feat_flag(terrain, f)) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    if (auto err = set_terrain_door(element["door"], terrain)) {
        return err;
    }
    if (auto err = set_terrain_tunnel(element["tunnel"], terrain)) {
        return err;
    }
    if (auto err = set_terrain_trap(element["trap"], terrain)) {
        return err;
    }
    if (auto err = set_terrain_pattern(element["pattern"], terrain)) {
        return err;
    }
    if (auto err = set_terrain_store(element["store"], terrain)) {
        return err;
    }
    if (auto err = set_terrain_building(element["building"], terrain)) {
        return err;
    }
    if (auto err = set_terrain_conversion(element["convert"], terrain)) {
        return err;
    }
    if (auto err = set_terrain_generation(element["generation"], terrain)) {
        return err;
    }
    if (auto err = set_terrain_interactions(element["interactions"], terrain)) {
        return err;
    }

    return PARSE_ERROR_NONE;
}
