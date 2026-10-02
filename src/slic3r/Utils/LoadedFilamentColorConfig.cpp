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

}
