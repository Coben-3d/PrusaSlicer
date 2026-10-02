// PrusaSlicer is released under the terms of the AGPLv3 or higher.
#ifndef slic3r_LoadedFilamentColor_hpp_
#define slic3r_LoadedFilamentColor_hpp_

#include <optional>
#include <string>
#include <vector>

namespace Slic3r {

// A declaration by the printer operator, not a filament sensor measurement.
struct LoadedFilamentColor {
    std::optional<std::string> material;
    std::optional<std::string> color;
};

class DynamicPrintConfig;
class Preset;

struct LoadedFilamentMaterialSelection {
    bool keep_current = false;
    std::optional<std::string> preferred_profile;
    std::vector<std::string> candidates;
};

// Candidates supplied by the caller must be compatible with the active extruder.
// Preserve an edited matching profile; auto-select only a unique system Generic profile.
LoadedFilamentMaterialSelection resolve_loaded_filament_material(
    const Preset& current, bool current_compatible,
    const std::vector<const Preset*>& compatible_presets,
    const std::optional<std::string>& material);

// Change only the single extruder display color; no-op for unknown colors.
bool apply_loaded_filament_color(DynamicPrintConfig& config, const LoadedFilamentColor& filament);

// Accept only the version 1, single-slot protocol. Throws std::runtime_error
// for invalid, ambiguous or unsupported responses. Unknown color is nullopt.
LoadedFilamentColor parse_loaded_filament_color(const std::string& body);

}
#endif
