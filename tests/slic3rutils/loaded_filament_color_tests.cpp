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
