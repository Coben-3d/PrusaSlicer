// PrusaSlicer is released under the terms of the AGPLv3 or higher.
#include "LoadedFilamentColor.hpp"
#include "libslic3r/PrintConfig.hpp"

namespace Slic3r {

bool apply_loaded_filament_color(DynamicPrintConfig& config, const LoadedFilamentColor& filament)
{
    auto* colors = config.option<ConfigOptionStrings>("extruder_colour");
    if (!filament.color || !colors || colors->values.size() != 1 || colors->values.front() == *filament.color)
        return false;
    colors->values.front() = *filament.color;
    return true;
}

}
