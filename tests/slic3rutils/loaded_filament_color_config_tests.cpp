#include <catch2/catch_test_macros.hpp>
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Preset.hpp"
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

static Preset material_preset(const std::string& name, const std::string& type, bool system = false)
{
    struct TestPreset : Preset {
        explicit TestPreset(const std::string& name) : Preset(Preset::TYPE_FILAMENT, name, false) {}
    };
    TestPreset preset(name);
    preset.loaded = true;
    preset.is_system = system;
    preset.config.set_key_value("filament_type", new ConfigOptionStrings({type}));
    return preset;
}

TEST_CASE("Loaded material preserves an edited matching profile", "[LoadedFilamentMaterial]")
{
    auto current = material_preset("My tuned PLA", "PLA");
    current.config.set_key_value("temperature", new ConfigOptionInts({233}));
    auto original = current.config;
    const auto result = resolve_loaded_filament_material(current, true, {}, "PLA");
    REQUIRE(result.keep_current);
    REQUIRE_FALSE(result.preferred_profile);
    REQUIRE(result.candidates.empty());
    REQUIRE(current.config == original);
    REQUIRE_FALSE(resolve_loaded_filament_material(current, false, {}, "PLA").keep_current);
    REQUIRE_FALSE(resolve_loaded_filament_material(current, true, {}, std::nullopt).keep_current);
    REQUIRE_FALSE(resolve_loaded_filament_material(current, true, {}, "PLA+").keep_current);
}

TEST_CASE("Loaded material auto-selects only one compatible system Generic profile", "[LoadedFilamentMaterial]")
{
    const auto current = material_preset("Current PETG", "PETG");
    auto generic = material_preset("Generic PLA @MK4", "PLA", true);
    auto brand = material_preset("Brand PLA", "PLA", true);
    auto silk = material_preset("Generic PLA Silk @MK4", "PLA", true);
    auto user = material_preset("Generic PLA - my settings", "PLA");
    std::vector<const Preset*> candidates{&generic, &brand, &silk, &user};
    auto result = resolve_loaded_filament_material(current, true, candidates, "PLA");
    REQUIRE_FALSE(result.keep_current);
    REQUIRE(result.preferred_profile == generic.name);
    REQUIRE(result.candidates.size() == 4);
    generic.is_visible = false;
    result = resolve_loaded_filament_material(current, true, candidates, "PLA");
    REQUIRE_FALSE(result.preferred_profile);
    REQUIRE(result.candidates.size() == 3);
    generic.is_visible = true;
    generic.loaded = false;
    REQUIRE_FALSE(resolve_loaded_filament_material(current, true, candidates, "PLA").preferred_profile);
    generic.loaded = true;
    generic.is_default = true;
    REQUIRE_FALSE(resolve_loaded_filament_material(current, true, candidates, "PLA").preferred_profile);
    generic.is_default = false;
    auto second = material_preset("Generic PLA @other", "PLA", true);
    candidates.push_back(&second);
    REQUIRE_FALSE(resolve_loaded_filament_material(current, true, candidates, "PLA").preferred_profile);
    REQUIRE(resolve_loaded_filament_material(current, true, candidates, "Unsupported material").candidates.empty());
    REQUIRE(resolve_loaded_filament_material(current, true, candidates, std::nullopt).candidates.empty());
}


TEST_CASE("INDX updates eight display colors without shifting disabled or empty tools", "[LoadedFilamentColor]")
{
    DynamicPrintConfig original = DynamicPrintConfig::full_print_config();
    original.set_key_value("extruder_colour", new ConfigOptionStrings(std::vector<std::string>(8, "#0000FF")));
    auto updated = original;
    LoadedFilaments snapshot; snapshot.schema_version = 2;
    const std::vector<std::string> rgb = {"#000000", "#FFFFFF", "#FF0000", "#00FF00", "#FFFF00", "#00FFFF", "#800080", "#123456"};
    for (size_t i = 0; i < 8; ++i) snapshot.slots.push_back({i, true, true, {{"PLA"}, {rgb[i]}}});
    snapshot.slots[2].enabled = false;
    snapshot.slots[4].loaded = false;
    snapshot.slots[6].filament.color.reset();
    REQUIRE(apply_loaded_filament_colors(updated, snapshot));
    REQUIRE(updated.diff(original) == std::vector<std::string>{"extruder_colour"});
    const auto& colors = updated.option<ConfigOptionStrings>("extruder_colour")->values;
    for (size_t i = 0; i < 8; ++i)
        REQUIRE(colors[i] == (i == 2 || i == 4 || i == 6 ? "#0000FF" : rgb[i]));
    REQUIRE_FALSE(apply_loaded_filament_colors(updated, snapshot));
    std::swap(snapshot.slots[2], snapshot.slots[3]);
    REQUIRE_FALSE(apply_loaded_filament_colors(original, snapshot));
    REQUIRE(original.option<ConfigOptionStrings>("extruder_colour")->values == std::vector<std::string>(8, "#0000FF"));
    auto single = DynamicPrintConfig::full_print_config();
    REQUIRE_FALSE(apply_loaded_filament_colors(single, snapshot));
}
