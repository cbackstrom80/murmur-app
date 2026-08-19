"""HTTP client for the MURMUR Standalone MCP bridge (127.0.0.1 only).

When MURMUR.app runs in Standalone mode it writes a manifest:
  ~/Library/Application Support/MURMUR/mcp-bridge.json

The MCP server uses this module to push scratch-built patches into the live
app for audition while agents iterate.
"""
from __future__ import annotations

import json
import os
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any


def manifest_paths() -> list[Path]:
    paths: list[Path] = []
    home = Path.home()
    paths.append(home / "Library/Application Support/MURMUR/mcp-bridge.json")
    paths.append(home / "Library/Application Support/Patchwork Eight/mcp-bridge.json")
    if env := os.environ.get("PW8_MCP_BRIDGE_MANIFEST"):
        paths.insert(0, Path(env).expanduser())
    return paths


def read_manifest() -> dict[str, Any] | None:
    for path in manifest_paths():
        if not path.is_file():
            continue
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        if isinstance(data, dict) and data.get("port"):
            data["_manifest_path"] = str(path)
            return data
    return None


def _request(method: str, target: str, body: dict | None = None, timeout: float = 5.0) -> dict[str, Any]:
    manifest = read_manifest()
    if manifest is None:
        return {
            "ok": False,
            "connected": False,
            "error": "standalone_not_running",
            "hint": "Launch MURMUR Standalone (MURMUR.app) — it exposes an MCP bridge on localhost.",
        }

    host = str(manifest.get("host", "127.0.0.1"))
    port = int(manifest["port"])
    url = f"http://{host}:{port}{target}"
    payload = None
    headers = {"Accept": "application/json"}
    if body is not None:
        payload = json.dumps(body).encode("utf-8")
        headers["Content-Type"] = "application/json"

    req = urllib.request.Request(url, data=payload, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            raw = resp.read().decode("utf-8")
            parsed = json.loads(raw) if raw else {}
            if isinstance(parsed, dict):
                parsed.setdefault("ok", resp.status < 400)
                parsed["connected"] = True
                parsed["bridge_port"] = port
                parsed["manifest"] = manifest.get("_manifest_path")
                return parsed
            return {"ok": True, "connected": True, "bridge_port": port, "body": parsed}
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        try:
            parsed = json.loads(detail)
        except json.JSONDecodeError:
            parsed = {"error": detail or exc.reason}
        if isinstance(parsed, dict):
            parsed.setdefault("ok", False)
            parsed["connected"] = True
            parsed["bridge_port"] = port
            parsed["http_status"] = exc.code
            return parsed
        return {"ok": False, "connected": True, "bridge_port": port, "http_status": exc.code}
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError) as exc:
        return {
            "ok": False,
            "connected": False,
            "error": "bridge_unreachable",
            "detail": str(exc),
            "bridge_port": port,
        }


def status() -> dict[str, Any]:
    return _request("GET", "/v1/status")


def load_path(path: str | Path) -> dict[str, Any]:
    resolved = Path(path).expanduser().resolve()
    if not resolved.is_file():
        return {"ok": False, "connected": False, "error": "file_not_found", "path": str(resolved)}
    return _request("POST", "/v1/load_path", {"path": str(resolved)})


def load_patch_dict(patch: dict[str, Any]) -> dict[str, Any]:
    return _request("POST", "/v1/load_json", {"patch": patch})
