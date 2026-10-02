// Opt-in native GUI harness. Isolated test settings and loopback API only.
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <wx/eventfilter.h>
#include <wx/button.h>
#include <wx/timer.h>
#include <wx/dialog.h>
#include <nlohmann/json.hpp>
#include "PrusaSlicer.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Sidebar.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/I18N.hpp"

using namespace Slic3r;
using namespace Slic3r::GUI;

static wxButton* find_sync_button(wxWindow* parent)
{
    for (wxWindow* child : parent->GetChildren()) {
        if (auto* button = dynamic_cast<wxButton*>(child);
            button && button->GetLabel() == _L("Sync filament from PrusaLink")) return button;
        if (auto* button = find_sync_button(child)) return button;
    }
    return nullptr;
}

class ColorTest : public wxEvtHandler, public wxEventFilter {
    std::unique_ptr<wxTimer> m_timer;
    wxButton* m_button = nullptr;
    DynamicPrintConfig m_before;
    DynamicPrintConfig m_printer_before;
    DynamicPrintConfig m_print_before;
    DynamicPrintConfig m_filament_before;
    std::string m_filament_name_before;
    bool m_dialog_cancelled = false;
    bool m_started = false;
    bool m_stale_edit_done = false;
    std::string m_request_seen;
    std::chrono::steady_clock::time_point m_start;
    std::string m_mode;
    std::string m_host;
public:
    int result = 1;
    ColorTest(std::string host, std::string mode) : m_mode(std::move(mode)), m_host(std::move(host))
    {
        if (const char* seen = std::getenv("SLICER_GUI_TEST_REQUEST_SEEN")) m_request_seen = seen;
        Bind(wxEVT_TIMER, &ColorTest::poll, this);
    }
    int FilterEvent(wxEvent& event) override
    {
        if (m_started && (m_mode == "material_cancel" || m_mode == "material_ambiguous" || m_mode == "indx_dirty_cancel") &&
            event.GetEventType() == wxEVT_INIT_DIALOG) {
            if (auto* dialog = dynamic_cast<wxDialog*>(event.GetEventObject())) {
                m_dialog_cancelled = true;
                wxTheApp->CallAfter([dialog]() { if (dialog->IsModal()) dialog->EndModal(wxID_CANCEL); });
            }
        }
        if (!m_started && event.GetEventType() == wxEVT_IDLE && wxGetApp().initialized()) {
            m_started = true;
            wxTheApp->CallAfter([this]() { start(); });
        }
        return Event_Skip;
    }
    void finish(bool success, const std::string& detail)
    {
        if (m_timer) m_timer->Stop();
        result = success ? 0 : 1;
        std::cout << nlohmann::json({{"ok", success}, {"mode", m_mode}, {"detail", detail}}).dump() << std::endl;
        wxGetApp().mainframe->Close();
    }
    void start()
    {
        auto& app = wxGetApp();
        auto& bundle = *app.preset_bundle;
        auto config = bundle.physical_printers.default_config();
        config.set_key_value("host_type", new ConfigOptionEnum<PrintHostType>(htPrusaLink));
        config.set_key_value("print_host", new ConfigOptionString(m_host));
        config.set_key_value("printhost_apikey", new ConfigOptionString("012345678912345"));
        config.set_key_value("preset_names", new ConfigOptionStrings({bundle.printers.get_selected_preset_name()}));
        bundle.physical_printers.load_printer("", "Virtual MK4", std::move(config), true, false);
        app.sidebar().update_printer_presets_combobox();
        m_button = find_sync_button(&app.sidebar());
        if (!m_button || !m_button->IsEnabled() || !m_button->IsShownOnScreen()) {
            finish(false, "Button missing, hidden or disabled"); return;
        }
        // Initialize the normal background-process temp path before closing an empty model.
        app.plater()->update(static_cast<unsigned int>(Plater::UpdateParams::FORCE_BACKGROUND_PROCESSING_UPDATE));
        if (m_mode == "material_keep" || m_mode == "material_cancel" || m_mode == "indx_keep" || m_mode == "indx_dirty_cancel") {
            auto tuned = *app.get_tab(Preset::TYPE_FILAMENT)->get_config();
            tuned.set_key_value("temperature", new ConfigOptionInts({233}));
            app.get_tab(Preset::TYPE_FILAMENT)->load_config(tuned);
            app.plater()->on_config_change(bundle.full_config());
        }
        if (m_mode == "material_ambiguous") {
            for (auto& preset : bundle.filaments.get_presets())
                if (preset.name.compare(0, 11, "Generic PLA") == 0)
                    bundle.filaments.find_preset(preset.name, false)->is_visible = false;
        }
        m_printer_before = bundle.printers.get_edited_preset().config;
        m_print_before = bundle.prints.get_edited_preset().config;
        m_filament_before = bundle.filaments.get_edited_preset().config;
        m_filament_name_before = bundle.filaments.get_selected_preset_name();
        m_before = bundle.full_config();
        wxCommandEvent click(wxEVT_BUTTON, m_button->GetId());
        click.SetEventObject(m_button);
        m_button->GetEventHandler()->ProcessEvent(click);
        if (m_button->IsEnabled()) { finish(false, "Request did not disable button"); return; }
        m_start = std::chrono::steady_clock::now();
        m_timer = std::make_unique<wxTimer>(this);
        m_timer->Start(100);
    }
    void poll(wxTimerEvent&)
    {
        const auto elapsed = std::chrono::steady_clock::now() - m_start;
        if (elapsed > std::chrono::seconds(12)) { finish(false, "GUI timeout"); return; }
        if ((m_mode == "stale" || m_mode == "material_stale" || m_mode == "indx_stale") && !m_stale_edit_done) {
            // The test server signals receipt, so this exercises a truly late response.
            if (m_request_seen.empty() || !std::filesystem::exists(m_request_seen)) return;
            auto& app = wxGetApp();
            auto updated = *app.get_tab(Preset::TYPE_PRINTER)->get_config();
            updated.set_key_value("extruder_colour", new ConfigOptionStrings(m_mode == "indx_stale" ? std::vector<std::string>{"#0000FF", "#0000FF", "#0000FF", "#00FF00", "#0000FF", "#0000FF", "#0000FF", "#0000FF"} : std::vector<std::string>{"#00FF00"}));
            app.get_tab(Preset::TYPE_PRINTER)->load_config(updated);
            app.plater()->on_config_change(updated);
            m_stale_edit_done = true;
        }
        if ((m_mode == "stale" || m_mode == "material_stale" || m_mode == "indx_stale") && elapsed < std::chrono::seconds(3)) return;
        if (!m_button->IsEnabled()) return;
        for (wxWindow* window : wxTopLevelWindows)
            if (auto* dialog = dynamic_cast<wxDialog*>(window); dialog && dialog->IsModal()) return;
        const auto after = wxGetApp().preset_bundle->full_config();
        const auto diff = after.diff(m_before);
        const auto color = after.option<ConfigOptionStrings>("extruder_colour")->values.front();
        bool success;
        if (m_mode.compare(0, 5, "indx_") == 0) {
            const auto& bundle = *wxGetApp().preset_bundle;
            const auto& colors = after.option<ConfigOptionStrings>("extruder_colour")->values;
            const std::vector<std::string> expected_rgb = {"#000000", "#FFFFFF", "#FF0000", "#00FF00", "#FFFF00", "#00FFFF", "#800080", "#123456"};
            if (m_mode == "indx_wrong" || m_mode == "indx_bad" || m_mode == "indx_mapping" || m_mode == "indx_dirty_cancel") {
                success = after == m_before && bundle.filaments.get_edited_preset().config == m_filament_before;
                if (m_mode == "indx_dirty_cancel") success &= m_dialog_cancelled;
            } else if (m_mode == "indx_stale") {
                success = diff == std::vector<std::string>{"extruder_colour"} && colors.size() == 8 && colors[3] == "#00FF00";
            } else {
                success = colors.size() == 8 && bundle.extruders_filaments.size() == 8 &&
                    bundle.prints.get_edited_preset().config == m_print_before &&
                    bundle.printers.get_edited_preset().config.diff(m_printer_before) == std::vector<std::string>{"extruder_colour"};
                for (size_t i = 0; i < 8 && success; ++i) {
                    const bool kept_color = m_mode == "indx_partial" && (i == 2 || i == 4 || i == 6);
                    const bool petg = i % 2 || (m_mode == "indx_partial" && (i == 2 || i == 4)) || (m_mode == "indx_keep" && i == 0);
                    const auto name = std::string(petg ? "Generic PETG" : "Generic PLA") + " @COREONEINDX HF0.4";
                    const auto* expected = bundle.filaments.find_preset(name, false);
                    const auto selected = bundle.extruders_filaments[i].get_selected_preset_name();
                    success &= expected && selected == name && colors[i] == (kept_color ? "#0000FF" : expected_rgb[i]);
                    const auto* actual_type = after.option<ConfigOptionStrings>("filament_type");
                    success &= actual_type && actual_type->values.at(i) == (petg ? "PETG" : "PLA");
                    const auto* actual_temp = after.option<ConfigOptionInts>("temperature");
                    if (expected && actual_temp)
                        success &= actual_temp->values.at(i) == (m_mode == "indx_keep" && petg ? 233 : expected->config.option<ConfigOptionInts>("temperature")->values.front());
                }
                if (m_mode == "indx_keep") success &= bundle.filaments.get_edited_preset().config == m_filament_before;
            }
        } else if (m_mode.compare(0, 9, "material_") == 0) {
            const auto& bundle = *wxGetApp().preset_bundle;
            if (m_mode == "material_switch" || m_mode == "material_unknown") {
                const auto* expected = bundle.filaments.find_preset("Generic PLA @PGIS", false);
                success = expected && bundle.filaments.get_selected_preset_name() == expected->name &&
                    bundle.extruders_filaments.front().get_selected_preset_name() == expected->name &&
                    bundle.filaments.get_edited_preset().config == expected->config &&
                    bundle.prints.get_edited_preset().config == m_print_before &&
                    bundle.printers.get_edited_preset().config.diff(m_printer_before) ==
                        (m_mode == "material_unknown" ? std::vector<std::string>{} : std::vector<std::string>{"extruder_colour"}) &&
                    color == (m_mode == "material_unknown" ? "#0000FF" : "#FF0000");
            } else if (m_mode == "material_cancel" || m_mode == "material_ambiguous") {
                success = m_dialog_cancelled && after == m_before &&
                    bundle.filaments.get_selected_preset_name() == m_filament_name_before &&
                    bundle.extruders_filaments.front().get_selected_preset_name() == m_filament_name_before;
            } else {
                success = bundle.filaments.get_edited_preset().config == m_filament_before &&
                    bundle.filaments.get_selected_preset_name() == m_filament_name_before &&
                    diff == std::vector<std::string>{"extruder_colour"} &&
                    color == (m_mode == "material_stale" ? "#00FF00" : "#FF0000");
            }
        }
        else if (m_mode == "unknown" || m_mode == "error") success = after == m_before;
        else success = diff == std::vector<std::string>{"extruder_colour"} &&
                       color == (m_mode == "stale" ? "#00FF00" : m_mode == "black" ? "#000000" : "#FF0000");
        finish(success, "Color=" + color + "; changed keys=" + std::to_string(diff.size()));
    }
};

int main(int argc, char** argv)
{
    const char* host = std::getenv("SLICER_GUI_TEST_HOST");
    const char* mode = std::getenv("SLICER_GUI_TEST_MODE");
    if (!host || std::string(host).find("http://127.0.0.1:") != 0) return 2;
    bool isolated = false;
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--datadir" &&
            std::string(argv[i + 1]).find("/tmp/prusadev-native-20261001/build/slicer-gui-test-") == 0) isolated = true;
    if (!isolated) return 2;
    ColorTest filter(host, mode ? mode : "apply");
    wxEvtHandler::AddFilter(&filter);
    const int app_result = CLI::run(argc, argv);
    wxEvtHandler::RemoveFilter(&filter);
    return app_result ? app_result : filter.result;
}
