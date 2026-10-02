#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "slic3r/Utils/LoadedFilamentColor.hpp"

using namespace Slic3r;

static nlohmann::json declaration()
{
    return {{"schema_version", 1}, {"slots", {{{"slot", 0}, {"material", "PLA"},
        {"color", "#000000"}, {"source", "user_declared"}}}}};
}

TEST_CASE("Loaded color preserves black, white and unknown", "[LoadedFilamentColor]")
{
    auto root = declaration();
    auto parsed = parse_loaded_filament_color(root.dump());
    REQUIRE(parsed.color == "#000000");
    REQUIRE(parsed.material == "PLA");
    root["slots"][0]["color"] = "#ffffff";
    REQUIRE(parse_loaded_filament_color(root.dump()).color == "#FFFFFF");
    root["slots"][0]["color"] = "#a1B2c3";
    REQUIRE(parse_loaded_filament_color(root.dump()).color == "#A1B2C3");
    root["slots"][0]["color"] = nullptr;
    REQUIRE_FALSE(parse_loaded_filament_color(root.dump()).color.has_value());
    REQUIRE(parse_loaded_filament_color(root.dump()).material == "PLA");
    root["slots"][0]["material"] = nullptr;
    REQUIRE_FALSE(parse_loaded_filament_color(root.dump()).material.has_value());
}

TEST_CASE("Unsupported or ambiguous filament schemas are rejected", "[LoadedFilamentColor]")
{
    auto root = declaration();
    SECTION("Future schema") { root["schema_version"] = 2; }
    SECTION("String schema") { root["schema_version"] = "1"; }
    SECTION("Float schema") { root["schema_version"] = 1.0; }
    SECTION("Missing schema") { root.erase("schema_version"); }
    SECTION("No spool") { root["slots"] = nlohmann::json::array(); }
    SECTION("Multiple spools") { root["slots"].push_back(root["slots"][0]); }
    SECTION("Nonzero slot") { root["slots"][0]["slot"] = 1; }
    SECTION("String slot") { root["slots"][0]["slot"] = "0"; }
    SECTION("Float slot") { root["slots"][0]["slot"] = 0.0; }
    SECTION("Sensor declaration") { root["slots"][0]["source"] = "sensor"; }
    SECTION("Missing color") { root["slots"][0].erase("color"); }
    SECTION("Missing material") { root["slots"][0].erase("material"); }
    SECTION("Numeric color") { root["slots"][0]["color"] = 0; }
    SECTION("RGB without hash") { root["slots"][0]["color"] = "FFFFFF"; }
    SECTION("Short RGB") { root["slots"][0]["color"] = "#FFF"; }
    SECTION("Non hex RGB") { root["slots"][0]["color"] = "#FFGGFF"; }
    SECTION("Numeric material") { root["slots"][0]["material"] = 1; }
    REQUIRE_THROWS(parse_loaded_filament_color(root.dump()));
}

TEST_CASE("Malformed, duplicate and oversized responses are rejected", "[LoadedFilamentColor]")
{
    REQUIRE_THROWS(parse_loaded_filament_color("<html>Not found</html>"));
    REQUIRE_THROWS(parse_loaded_filament_color("null"));
    REQUIRE_THROWS(parse_loaded_filament_color("[]"));
    REQUIRE_THROWS(parse_loaded_filament_color(declaration().dump() + " trailing"));
    REQUIRE_THROWS(parse_loaded_filament_color(std::string(4097, ' ')));
    REQUIRE_THROWS(parse_loaded_filament_color(R"({"schema_version":2,"schema_version":1,"slots":[]})"));
    REQUIRE_THROWS(parse_loaded_filament_color(R"({"schema_version":1,"slots":[{"slot":0,"material":null,"color":null,"color":"#FF0000","source":"user_declared"}]})"));
}


static nlohmann::json indx_declaration()
{
    using nlohmann::json;
    json slots = json::array();
    for (int i = 0; i < 8; ++i)
        slots.push_back({{"slot", i}, {"virtual_tool", i}, {"enabled", true}, {"loaded", true},
            {"material", i % 2 ? "PETG" : "PLA"}, {"color", "#123456"}, {"source", "user_declared"}});
    return {{"schema_version", 2}, {"printer_model", "COREONE_INDX"}, {"indexing", "physical_tools"},
            {"tool_count", 8}, {"slots", slots}};
}

TEST_CASE("INDX response keeps eight stable indices including disabled and unknown tools", "[LoadedFilamentColor]")
{
    auto data = indx_declaration();
    data["slots"][2]["enabled"] = false;
    data["slots"][2]["loaded"] = false;
    data["slots"][2]["material"] = nullptr;
    data["slots"][2]["color"] = nullptr;
    data["slots"][6]["color"] = nullptr;
    std::reverse(data["slots"].begin(), data["slots"].end());
    const auto parsed = parse_loaded_filaments(data.dump());
    REQUIRE(parsed.schema_version == 2);
    REQUIRE(parsed.slots.size() == 8);
    for (size_t i = 0; i < 8; ++i) REQUIRE(parsed.slots[i].slot == i);
    REQUIRE_FALSE(parsed.slots[2].enabled);
    REQUIRE_FALSE(parsed.slots[2].loaded);
    REQUIRE_FALSE(parsed.slots[2].filament.material);
    REQUIRE_FALSE(parsed.slots[6].filament.color);
    REQUIRE(parsed.slots[7].filament.material == "PETG");
    REQUIRE(parse_loaded_filaments(declaration().dump()).schema_version == 1);
}

TEST_CASE("INDX rejects mismapped tools, incomplete data and inconsistent loaded states", "[LoadedFilamentColor]")
{
    const auto valid = indx_declaration();
    for (const auto key : {"printer_model", "indexing", "tool_count", "slots"}) {
        auto bad = valid; bad.erase(key); REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    }
    for (const auto key : {"slot", "virtual_tool", "enabled", "loaded", "material", "color", "source"}) {
        auto bad = valid; bad["slots"][5].erase(key); REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    }
    for (const auto model : {"XL", "MK4", "COREONEL_INDX"}) {
        auto bad = valid; bad["printer_model"] = model; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    }
    auto bad = valid; bad["slots"][4]["slot"] = 3; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    bad = valid; bad["slots"][4]["virtual_tool"] = 3; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    bad = valid; bad["slots"].erase(4); REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    bad = valid; bad["slots"][4]["enabled"] = false; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    bad = valid; bad["slots"][4]["loaded"] = false; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    bad = valid; bad["slots"][4]["material"] = nullptr; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    bad = valid; bad["slots"][4]["color"] = "#FFGG00"; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    bad = valid; bad["slots"][4]["slot"] = -1; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    bad = valid; bad["slots"][4]["slot"] = 8; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    bad = valid; bad["tool_count"] = 4; REQUIRE_THROWS(parse_loaded_filaments(bad.dump()));
    REQUIRE_THROWS(parse_loaded_filaments(R"({"schema_version":1,"schema_version":2})"));
    REQUIRE_THROWS(parse_loaded_filaments(std::string(4097, ' ')));
}
