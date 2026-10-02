"""Real native Slicer sidebar, official INDX profiles, eight synthetic tools."""
import configparser
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import pytest
from test_loaded_filament_color_http import server

ROOT = Path("/tmp/prusadev-native-20261001")
GUI = os.environ.get("SLICER_COLOR_GUI")
OFFICIAL = Path(os.environ.get("SLICER_INDX_TEST_VENDOR", ""))

@pytest.mark.parametrize("mode", ["indx_apply", "indx_shuffled", "indx_partial", "indx_keep",
    "indx_dirty_cancel", "indx_wrong", "indx_bad", "indx_mapping", "indx_stale"])
def test_indx_native_sidebar(server, mode):
    assert GUI and OFFICIAL.is_file()
    isolation = os.environ["SLICER_DENY_KEYCHAIN_DYLIB"]
    assert Path(isolation).is_file()
    directory = Path(tempfile.mkdtemp(prefix="slicer-gui-test-indx-", dir=ROOT / "build"))
    stop = threading.Event()
    try:
        (directory / "printer").mkdir(); (directory / "vendor").mkdir()
        shutil.copyfile(OFFICIAL, directory / "vendor/PrusaResearch.ini")
        vendor = configparser.ConfigParser(interpolation=None, strict=False)
        vendor.optionxform = str; vendor.read(OFFICIAL)
        def flatten(name):
            section = vendor["printer:" + name]; values = {}
            for parent in section.get("inherits", "").split(";"):
                if parent.strip(): values.update(flatten(parent.strip()))
            values.update({k:v for k,v in section.items() if k != "inherits"})
            return values
        parent = "Prusa CORE One INDX 8T HF0.4 nozzle"
        values = flatten(parent)
        values.update(inherits=parent, printer_settings_id="Virtual INDX", extruder_colour=";".join(["#0000FF"] * 8))
        profile = directory / "printer/Virtual INDX.ini"
        profile.write_text("".join(k + " = " + v + "\n" for k,v in values.items()))
        original = profile.read_bytes()
        ini = "version = 2.9.6\ntranslation_language = fr_FR\npreset_update = 0\nversion_check_url = http://127.0.0.1:1/\nnotify_release = none\nversion_system_info_sent = 2.9.6\nauth_login_dialog_confirmed = 1\nshow_hints = 0\nconnect_polling = 0\nsingle_instance = 0\nno_defaults = 0\ndefault_action_on_dirty_project = 0\ndefault_action_on_close_application = discard\n[presets]\nprinter = Virtual INDX\nprint = 0.20mm Balanced ★ @COREONEINDX HF0.4\n"
        for i in range(8): ini += ("filament" + ("_" + str(i) if i else "")) + " = Generic PETG @COREONEINDX HF0.4\n"
        ini += "[vendor:PrusaResearch]\nmodel:COREONE_INDX8T = HF0.4\n[filaments]\nGeneric PLA @COREONEINDX HF0.4 = 1\nGeneric PETG @COREONEINDX HF0.4 = 1\n"
        (directory / "PrusaSlicer.ini").write_text(ini)
        base, seen = server
        env = {**os.environ, "SLICER_GUI_TEST_HOST":base + "/" + mode,
            "SLICER_GUI_TEST_MODE":mode, "DYLD_INSERT_LIBRARIES":isolation, "NO_PROXY":"127.0.0.1,localhost"}
        request_seen = directory / "request-seen"; env["SLICER_GUI_TEST_REQUEST_SEEN"] = str(request_seen)
        def signal():
            while not stop.wait(0.02):
                if seen: request_seen.touch(); return
        thread = threading.Thread(target=signal, daemon=True); thread.start()
        result = subprocess.run([GUI, "--datadir",str(directory)],env=env,capture_output=True,text=True,timeout=50)
        stop.set(); thread.join()
        (ROOT / "artifacts" / ("slicer-gui-"+mode+".log")).write_text(result.stdout+result.stderr)
        assert "TEST_KEYCHAIN_BLOCKED" in result.stderr
        assert result.returncode == 0, result.stdout+result.stderr
        messages = [json.loads(l) for l in result.stdout.splitlines() if l.startswith('{"detail"')]
        assert messages and messages[-1]["ok"]
        assert profile.read_bytes() == original
        assert len(seen) == 1
    finally:
        stop.set()
        if "thread" in locals(): thread.join()
        shutil.rmtree(directory)
