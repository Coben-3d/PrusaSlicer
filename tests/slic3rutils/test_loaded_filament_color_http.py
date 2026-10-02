"""Exercise the compiled PrusaLink client using synthetic, loopback-only hosts.

Set SLICER_COLOR_CLIENT to the loaded_filament_color_client executable.
No discovery, real printer, existing settings, password store or cloud calls.
"""
import hashlib
import json
import os
import re
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import pytest

CLIENT = os.environ.get("SLICER_COLOR_CLIENT")
KEY = "012345678912345"


def declaration(color="#000000"):
    return {"schema_version": 1, "slots": [{"slot": 0, "material": "PLA",
        "color": color, "source": "user_declared"}]}


@pytest.fixture
def server():
    seen = []

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def do_GET(self):
            seen.append((self.path, self.headers.get("X-Api-Key"), self.headers.get("Authorization")))
            mode = self.path.split("/")[1]
            assert self.path.endswith("/api/v1/filaments") or mode == "leaked"
            status = 200
            body = declaration()
            if mode == "digest":
                auth = self.headers.get("Authorization", "")
                if not auth:
                    self.send_response(401)
                    self.send_header("WWW-Authenticate", 'Digest realm="local-test", nonce="test-nonce", qop="auth", algorithm=MD5')
                    self.send_header("Content-Length", "0")
                    self.end_headers()
                    return
                fields = {}
                for key, quoted, token in re.findall(r'(\w+)=(?:"([^"]*)"|([^, ]+))', auth):
                    fields[key] = quoted or token
                md5 = lambda text: hashlib.md5(text.encode()).hexdigest()
                expected = md5(":".join([md5("test-user:local-test:test-password"), "test-nonce",
                    fields["nc"], fields["cnonce"], "auth", md5("GET:" + self.path)]))
                if fields.get("username") != "test-user" or fields.get("response") != expected:
                    status = 401
                assert self.headers.get("X-Api-Key") is None
            elif mode not in ("auth", "leaked"):
                assert self.headers.get("X-Api-Key") == KEY
            if mode == "auth": status = 401
            elif mode == "forbidden": status = 403
            elif mode == "unsupported": status = 404
            elif mode == "white": body = declaration("#ffffff")
            elif mode == "red": body = declaration("#FF0000")
            elif mode == "unknown": body = declaration(None)
            elif mode == "material_none":
                body = declaration("#FF0000")
                body["slots"][0]["material"] = None
            elif mode == "material_missing":
                body = declaration("#FF0000")
                body["slots"][0]["material"] = "NO_MATCH"
            elif mode == "future": body["schema_version"] = 2
            elif mode == "multi": body["slots"].append(body["slots"][0])
            elif mode == "malformed": body = "<html>failure</html>"
            elif mode == "oversize": body = "X" * 4097
            elif mode == "stale_delay":
                time.sleep(2)
                body = declaration("#FF0000")
            elif mode == "delayed": time.sleep(12)
            elif mode == "cancel": time.sleep(4)
            elif mode == "redirect":
                self.send_response(302)
                self.send_header("Location", f"http://127.0.0.1:{self.server.server_port}/leaked/api/v1/filaments")
                self.send_header("Content-Length", "0")
                self.end_headers()
                return
            data = body.encode() if isinstance(body, str) else json.dumps(body).encode()
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            try:
                self.wfile.write(data)
            except (BrokenPipeError, ConnectionResetError):
                pass

    httpd = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=httpd.serve_forever, daemon=True)
    thread.start()
    yield f"http://127.0.0.1:{httpd.server_port}", seen
    httpd.shutdown()
    httpd.server_close()
    thread.join()


def request(base, endpoint, auth="key"):
    assert CLIENT, "Set SLICER_COLOR_CLIENT to the compiled C++ harness"
    result = subprocess.run([CLIENT, base + "/" + endpoint, auth], capture_output=True,
                            text=True, timeout=15, check=True)
    # Debug level 5 must still not expose even these fake secrets.
    for secret in (KEY, "test-password", "test-user", "X-Api-Key:", "Authorization:"):
        assert secret not in result.stderr
    return json.loads(result.stdout)


@pytest.mark.parametrize("endpoint,color", [("valid", "#000000"), ("white", "#FFFFFF"),
                                            ("red", "#FF0000"), ("unknown", None)])
def test_valid_colors(server, endpoint, color):
    base, seen = server
    assert request(base, endpoint) == {"error": 0, "color": color, "material": "PLA"}
    assert len(seen) == 1


@pytest.mark.parametrize("endpoint,error", [("auth", 1), ("forbidden", 1), ("unsupported", 2),
    ("malformed", 4), ("future", 4), ("multi", 4), ("redirect", 4), ("oversize", 3)])
def test_failure_preserves_color(server, endpoint, error):
    base, seen = server
    assert request(base, endpoint) == {"error": error, "color": None, "material": None}
    assert len(seen) == 1  # Including 302: no second request or credential forwarding.


def test_existing_digest_auth(server):
    base, seen = server
    assert request(base, "digest", "digest")["color"] == "#000000"
    assert len(seen) == 2


def test_total_timeout(server):
    base, _ = server
    start = time.monotonic()
    assert request(base, "delayed")["error"] == 3
    assert time.monotonic() - start < 10


def test_cancel_suppresses_callback(server):
    base, _ = server
    assert request(base, "cancel", "cancel") == {"cancelled": True}


def test_offline():
    server = ThreadingHTTPServer(("127.0.0.1", 0), BaseHTTPRequestHandler)
    port = server.server_port
    server.server_close()
    assert request(f"http://127.0.0.1:{port}", "valid")["error"] == 3
