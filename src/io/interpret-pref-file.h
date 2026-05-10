#pragma once

#include <string>
#include <string_view>
#include <tl/optional.hpp>

extern tl::optional<std::string> histpref_buf;

class PlayerType;
int interpret_pref_file(PlayerType *player_ptr, std::string_view buf);
