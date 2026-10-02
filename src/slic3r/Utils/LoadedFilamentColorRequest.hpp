// PrusaSlicer is released under the terms of the AGPLv3 or higher.
#ifndef slic3r_LoadedFilamentColorRequest_hpp_
#define slic3r_LoadedFilamentColorRequest_hpp_

#include <atomic>
#include <functional>
#include <memory>
#include "Http.hpp"
#include "LoadedFilamentColor.hpp"

namespace Slic3r {

enum class FilamentColorError { None, Authentication, Unsupported, Transport, InvalidResponse };
struct FilamentColorResult {
    FilamentColorError error = FilamentColorError::None;
    LoadedFilamentColor filament; // Legacy MK4 callers.
    LoadedFilaments declarations;
};
using FilamentColorCallback = std::function<void(FilamentColorResult)>;

// The caller supplies the existing host URL/authentication. Callback runs on
// the HTTP worker; no printer writes, redirects, retries or response logging.
Http::Ptr request_loaded_filament_color(Http&& http, FilamentColorCallback callback,
                                      std::shared_ptr<std::atomic_bool> cancelled);

}
#endif
