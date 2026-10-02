#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>
#include "slic3r/Utils/OctoPrint.hpp"
#include "libslic3r/Utils.hpp"

using namespace Slic3r;

int main(int argc, char** argv)
{
    // Only loopback virtual hosts and synthetic credentials are permitted by this harness.
    if (argc < 2 || std::string(argv[1]).find("http://127.0.0.1:") != 0) return 2;
    auto config = DynamicPrintConfig::full_print_config();
    config.set_key_value("print_host", new ConfigOptionString(argv[1]));
    config.set_key_value("printhost_apikey", new ConfigOptionString("012345678912345"));
    config.set_key_value("printhost_user", new ConfigOptionString("test-user"));
    config.set_key_value("printhost_password", new ConfigOptionString("test-password"));
    config.set_key_value("printhost_cafile", new ConfigOptionString(""));
    config.set_key_value("printhost_ssl_ignore_revoke", new ConfigOptionBool(false));
    const std::string mode = argc > 2 ? argv[2] : "key";
    config.set_key_value("printhost_authorization_type",
                        new ConfigOptionEnum<AuthorizationType>(mode == "digest" ? atUserPassword : atKeyPassword));
    set_logging_level(5); // Sensitive request must still never log headers or credentials.
    auto cancelled = std::make_shared<std::atomic_bool>(false);
    auto promise = std::make_shared<std::promise<FilamentColorResult>>();
    auto future = promise->get_future();
    PrusaLink host(&config);
    auto request = host.get_loaded_filament_color([promise](FilamentColorResult result) {
        promise->set_value(std::move(result));
    }, cancelled);
    if (mode == "cancel") {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cancelled->store(true);
        if (future.wait_for(std::chrono::seconds(2)) != std::future_status::timeout) return 3;
        // Let the aborted worker exit while the promise remains in scope.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "{\"cancelled\":true}\n";
        return 0;
    }
    if (future.wait_for(std::chrono::seconds(10)) != std::future_status::ready) return 4;
    const auto result = future.get();
    nlohmann::json out = {{"error", int(result.error)},
        {"color", result.filament.color ? nlohmann::json(*result.filament.color) : nlohmann::json(nullptr)},
        {"material", result.filament.material ? nlohmann::json(*result.filament.material) : nlohmann::json(nullptr)}};
    if (result.error == FilamentColorError::None && result.declarations.schema_version == 2) {
        out = {{"error", 0}, {"schema_version", 2}, {"slots", nlohmann::json::array()}};
        for (const auto& slot : result.declarations.slots)
            out["slots"].push_back({{"slot", slot.slot}, {"enabled", slot.enabled}, {"loaded", slot.loaded},
                {"material", slot.filament.material ? nlohmann::json(*slot.filament.material) : nlohmann::json(nullptr)},
                {"color", slot.filament.color ? nlohmann::json(*slot.filament.color) : nlohmann::json(nullptr)}});
    }
    std::cout << out.dump() << '\n';
    return 0;
}
