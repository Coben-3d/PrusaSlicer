// PrusaSlicer is released under the terms of the AGPLv3 or higher.
#include "LoadedFilamentColorRequest.hpp"

#include <exception>

namespace Slic3r {

Http::Ptr request_loaded_filament_color(Http&& http, FilamentColorCallback callback,
                                      std::shared_ptr<std::atomic_bool> cancelled)
{
    return http.timeout_connect(3).timeout_max(8).size_limit(4096)
        .follow_redirects(false).sensitive(true)
        .on_progress([cancelled](Http::Progress, bool& abort) { abort = cancelled->load(); })
        .on_error([callback, cancelled](std::string, std::string, unsigned status) {
            if (cancelled->load()) return;
            FilamentColorResult result;
            result.error = status == 401 || status == 403 ? FilamentColorError::Authentication :
                           status == 404 ? FilamentColorError::Unsupported : FilamentColorError::Transport;
            callback(std::move(result));
        })
        .on_complete([callback, cancelled](std::string body, unsigned status) {
            if (cancelled->load()) return;
            FilamentColorResult result;
            if (status != 200)
                result.error = FilamentColorError::InvalidResponse;
            else {
                try { result.filament = parse_loaded_filament_color(body); }
                catch (const std::exception&) { result.error = FilamentColorError::InvalidResponse; }
            }
            callback(std::move(result));
        }).perform();
}

}
