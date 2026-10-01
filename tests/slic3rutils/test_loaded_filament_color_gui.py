"""Drive the real sidebar through native wx events using isolated local settings."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

import pytest
from test_loaded_filament_color_http import server

GUI = os.environ.get("SLICER_COLOR_GUI")
ROOT = Path("/tmp/prusadev-native-20261001/build")


@pytest.mark.parametrize("mode,endpoint", [("apply", "red"), ("black", "valid"),
    ("unknown", "unknown"), ("error", "unsupported"), ("stale", "stale_delay")])
def test_native_sync_button(server, mode, endpoint):
    assert GUI, "Set SLICER_COLOR_GUI to the compiled native harness"
    base, seen = server
    isolation = os.environ.get("SLICER_DENY_KEYCHAIN_DYLIB")
    assert isolation and Path(isolation).is_file(), "Verified keychain isolation is required"
    directory = Path(tempfile.mkdtemp(prefix="slicer-gui-test-", dir=ROOT))
    try:
        (directory / "printer").mkdir()
        profile = directory / "printer" / "Virtual MK4.ini"
        profile.write_text("printer_technology = FFF\nprinter_model = MK4\n"
            "nozzle_diameter = 0.4\nextruder_colour = #0000FF\n"
            "printer_settings_id = Virtual MK4\n")
        original = profile.read_bytes()
        (directory / "PrusaSlicer.ini").write_text("version = 2.9.6\n"
            "translation_language = fr_FR\npreset_update = 0\n"
            "version_check_url = http://127.0.0.1:1/\nnotify_release = none\n"
            "version_system_info_sent = 2.9.6\nauth_login_dialog_confirmed = 1\n"
            "show_hints = 0\nconnect_polling = 0\nsingle_instance = 0\nno_defaults = 0\n"
            "default_action_on_dirty_project = 0\ndefault_action_on_close_application = discard\n"
            "[presets]\nprinter = Virtual MK4\n")
        env = {**os.environ, "SLICER_GUI_TEST_HOST": base + "/" + endpoint,
               "SLICER_GUI_TEST_MODE": mode, "DYLD_INSERT_LIBRARIES": isolation,
               "NO_PROXY": "127.0.0.1,localhost"}
        request_seen = directory / "request-seen"
        env["SLICER_GUI_TEST_REQUEST_SEEN"] = str(request_seen)
        stop = threading.Event()
        def signal_received():
            while not stop.wait(0.02):
                if seen:
                    request_seen.touch()
                    return
        signal_thread = threading.Thread(target=signal_received, daemon=True)
        signal_thread.start()
        process = subprocess.run([GUI, "--datadir", str(directory)], env=env,
            capture_output=True, text=True, timeout=45)
        stop.set()
        signal_thread.join()
        artifacts = Path("/tmp/prusadev-native-20261001/artifacts")
        (artifacts / ("slicer-gui-" + mode + ".log")).write_text(process.stdout + process.stderr)
        assert "TEST_KEYCHAIN_BLOCKED" in process.stderr
        assert process.returncode == 0, process.stdout + process.stderr
        results = [json.loads(line) for line in process.stdout.splitlines() if line.startswith('{"detail"')]
        assert results and results[-1]["ok"]
        assert profile.read_bytes() == original, "No automatic saved-preset rewrite"
        assert len(seen) == 1
        assert seen[0][0] == "/" + endpoint + "/api/v1/filaments"
    finally:
        if "stop" in locals():
            stop.set()
            signal_thread.join()
        shutil.rmtree(directory)
