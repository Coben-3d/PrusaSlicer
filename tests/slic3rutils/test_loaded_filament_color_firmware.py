"""Real compiled Slicer client against a running, virtual MK4 firmware.

BUDDY_SOURCE_PATH and SLICER_COLOR_CLIENT must point to local build artifacts.
Records are seeded in virtual EEPROM; this does not validate a completed load.
"""
import asyncio
import json
import os

import pytest
from buddy_integration.actions import utils
from buddy_integration.test_loaded_filament_color import declaration, wait_for_http

CLIENT = os.environ["SLICER_COLOR_CLIENT"]


@pytest.mark.asyncio
@pytest.mark.parametrize("specific_eeprom_variables,expected", [
    (declaration(0x000000), "#000000"),
    (declaration(0xFFFFFF), "#FFFFFF"),
    (declaration(None), None),
])
async def test_slicer_client_reads_virtual_mk4(printer, expected):
    await utils.wait_for_bootstrap(printer)
    await wait_for_http(printer)
    process = await asyncio.create_subprocess_exec(
        CLIENT, f"http://127.0.0.1:{printer.network_proxy_http_port_get()}",
        stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
    out, err = await asyncio.wait_for(process.communicate(), 15)
    assert process.returncode == 0, err.decode()
    assert json.loads(out) == {"error": 0, "color": expected, "material": "PLA"}
    assert b"012345678912345" not in err
