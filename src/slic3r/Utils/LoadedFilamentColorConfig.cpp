// PrusaSlicer is released under the terms of the AGPLv3 or higher.
#include "LoadedFilamentColor.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Preset.hpp"
#include <algorithm>

namespace Slic3r {

LoadedFilamentMaterialSelection resolve_loaded_filament_material(
    const Preset& current, bool current_compatible,
    const std::vector<const Preset*>& compatible_presets,
    const std::optional<std::string>& material)
{
    LoadedFilamentMaterialSelection result;
    if (!material) return result;
    auto matches = [&material](const Preset& preset) {
        const auto* type = preset.config.option<ConfigOptionStrings>("filament_type");
        return type && type->values.size() == 1 && type->values.front() == *material;
    };
    if (current_compatible && matches(current)) {
        result.keep_current = true;
        return result;
    }
    std::vector<std::string> generic;
    const std::string generic_name = "Generic " + *material;
    for (const Preset* preset : compatible_presets) {
        if (!preset || !preset->loaded || !preset->is_visible || preset->is_default || !matches(*preset))
            continue;
        result.candidates.push_back(preset->name);
        if (preset->is_system && (preset->alias == generic_name || preset->name == generic_name ||
            preset->name.compare(0, generic_name.size() + 2, generic_name + " @") == 0))
            generic.push_back(preset->name);
    }
    std::sort(result.candidates.begin(), result.candidates.end());
    if (generic.size() == 1) result.preferred_profile = generic.front();
    return result;
}

bool apply_loaded_filament_color(DynamicPrintConfig& config, const LoadedFilamentColor& filament)
{
    auto* colors = config.option<ConfigOptionStrings>("extruder_colour");
    if (!filament.color || !colors || colors->values.size() != 1 || colors->values.front() == *filament.color)
        return false;
    colors->values.front() = *filament.color;
    return true;
}

bool apply_loaded_filament_colors(DynamicPrintConfig& config, const LoadedFilaments& filaments)
{
    auto* colors = config.option<ConfigOptionStrings>("extruder_colour");
    if (filaments.schema_version == 1)
        return filaments.slots.size() == 1 && apply_loaded_filament_color(config, filaments.slots.front().filament);
    if (filaments.schema_version != 2 || filaments.slots.size() != 8 || !colors || colors->values.size() != 8)
        return false;
    // Validate the complete mapping before any mutation, including callers that
    // construct a snapshot directly rather than going through the JSON parser.
    for (size_t i = 0; i < 8; ++i)
        if (filaments.slots[i].slot != i) return false;
    bool changed = false;
    for (const auto& slot : filaments.slots) {
        if (!slot.enabled || !slot.loaded || !slot.filament.color) continue;
        if (colors->values[slot.slot] != *slot.filament.color) {
            colors->values[slot.slot] = *slot.filament.color;
            changed = true;
        }
    }
    return changed;
}

}
