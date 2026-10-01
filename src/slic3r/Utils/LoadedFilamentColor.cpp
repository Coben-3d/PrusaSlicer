// PrusaSlicer is released under the terms of the AGPLv3 or higher.
#include "LoadedFilamentColor.hpp"

#include <stdexcept>
#include <set>
#include <vector>
#include <nlohmann/json.hpp>

namespace Slic3r {

LoadedFilamentColor parse_loaded_filament_color(const std::string& body)
{
    using nlohmann::json;
    if (body.size() > 4096)
        throw std::runtime_error("Filament response too large");
    std::vector<std::set<std::string>> object_keys;
    const json root = json::parse(body, [&object_keys](int, json::parse_event_t event, json& value) {
        if (event == json::parse_event_t::object_start) object_keys.emplace_back();
        else if (event == json::parse_event_t::key &&
                 !object_keys.back().insert(value.get<std::string>()).second)
            throw std::runtime_error("Duplicate filament response key");
        else if (event == json::parse_event_t::object_end) object_keys.pop_back();
        return true;
    });
    if (!root.is_object() || !root.contains("schema_version") ||
        !root.at("schema_version").is_number_integer() || root.at("schema_version") != 1 ||
        !root.contains("slots") || !root.at("slots").is_array() || root.at("slots").size() != 1)
        throw std::runtime_error("Unsupported filament schema");
    const json& slot = root.at("slots").at(0);
    if (!slot.is_object() || !slot.contains("slot") ||
        !slot.at("slot").is_number_integer() || slot.at("slot") != 0 ||
        !slot.contains("source") || slot.at("source") != "user_declared" ||
        !slot.contains("material") || !slot.contains("color"))
        throw std::runtime_error("Unsupported filament slot");
    LoadedFilamentColor result;
    if (!slot.at("material").is_null()) {
        if (!slot.at("material").is_string())
            throw std::runtime_error("Invalid filament material");
        result.material = slot.at("material").get<std::string>();
        if (result.material->empty() || result.material->size() > 64)
            throw std::runtime_error("Invalid filament material");
    }
    if (!slot.at("color").is_null()) {
        if (!slot.at("color").is_string())
            throw std::runtime_error("Invalid filament color");
        std::string color = slot.at("color").get<std::string>();
        if (color.size() != 7 || color.front() != '#')
            throw std::runtime_error("Invalid filament color");
        for (size_t i = 1; i < color.size(); ++i) {
            char& c = color[i];
            if (c >= 'a' && c <= 'f') c -= 'a' - 'A';
            else if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')))
                throw std::runtime_error("Invalid filament color");
        }
        result.color = std::move(color);
    }
    return result;
}

}
