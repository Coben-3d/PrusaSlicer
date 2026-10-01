#include <catch2/catch_test_macros.hpp>
#include "libslic3r/PrintConfig.hpp"
#include "slic3r/Utils/LoadedFilamentColor.hpp"

using namespace Slic3r;

TEST_CASE("Synchronization changes only the edited extruder color", "[LoadedFilamentColor]")
{
    auto original = DynamicPrintConfig::full_print_config();
    original.set_key_value("extruder_colour", new ConfigOptionStrings({"#0000FF"}));
    original.set_key_value("filament_colour", new ConfigOptionStrings({"#00FF00"}));
    auto updated = original;
    auto declaration = parse_loaded_filament_color(R"({"schema_version":1,"slots":[{"slot":0,"material":"PETG","color":"#FF0000","source":"user_declared"}]})");
    REQUIRE(apply_loaded_filament_color(updated, declaration));
    REQUIRE(updated.diff(original) == std::vector<std::string>{"extruder_colour"});
    REQUIRE(updated.option<ConfigOptionStrings>("extruder_colour")->values.front() == "#FF0000");
    REQUIRE_FALSE(apply_loaded_filament_color(updated, declaration));
    declaration.color.reset();
    auto before = updated;
    REQUIRE_FALSE(apply_loaded_filament_color(updated, declaration));
    REQUIRE(updated == before);
    updated.set_key_value("extruder_colour", new ConfigOptionStrings({"#000000", "#FFFFFF"}));
    before = updated;
    declaration.color = "#FF0000";
    REQUIRE_FALSE(apply_loaded_filament_color(updated, declaration));
    REQUIRE(updated == before);
}
