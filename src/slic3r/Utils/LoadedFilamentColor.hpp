// PrusaSlicer is released under the terms of the AGPLv3 or higher.
#ifndef slic3r_LoadedFilamentColor_hpp_
#define slic3r_LoadedFilamentColor_hpp_

#include <optional>
#include <string>

namespace Slic3r {

// A declaration by the printer operator, not a filament sensor measurement.
struct LoadedFilamentColor {
    std::optional<std::string> material;
    std::optional<std::string> color;
};

class DynamicPrintConfig;
// Change only the single extruder display color; no-op for unknown colors.
bool apply_loaded_filament_color(DynamicPrintConfig& config, const LoadedFilamentColor& filament);

// Accept only the version 1, single-slot protocol. Throws std::runtime_error
// for invalid, ambiguous or unsupported responses. Unknown color is nullopt.
LoadedFilamentColor parse_loaded_filament_color(const std::string& body);

}
#endif
