#!/usr/bin/env python3
"""Generate Axmol client art assets with the Right Code image API.

This tool is intentionally frontend-only. It creates visual assets under
app/frontend/axmol_client/Content/art and does not define gameplay rules.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Any

API_BASE = "https://www.right.codes/draw"
GENERATIONS_PATH = "/v1/images/generations"
DEFAULT_KEY_ENV = "RIGHT_CODE_IMAGE_API_KEY"


def repo_relative_default_manifest() -> Path:
    return Path(__file__).resolve().parents[1] / "Content" / "art" / "asset_manifest.json"


def load_manifest(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError("manifest root must be an object")
    assets = data.get("assets")
    if not isinstance(assets, list):
        raise ValueError("manifest.assets must be a list")
    return data


def post_json(url: str, payload: dict[str, Any], api_key: str, timeout_seconds: int) -> dict[str, Any]:
    body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    request = urllib.request.Request(
        url,
        data=body,
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json",
        },
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=timeout_seconds) as response:
        return json.loads(response.read().decode("utf-8"))


def download_file(url: str, output_path: Path, timeout_seconds: int) -> None:
    request = urllib.request.Request(url, method="GET")
    with urllib.request.urlopen(request, timeout=timeout_seconds) as response:
        payload = response.read()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(payload)


def extract_first_url(response: dict[str, Any]) -> str:
    data = response.get("data")
    if not isinstance(data, list) or not data:
        raise ValueError("image API response missing data[0].url")
    first = data[0]
    if not isinstance(first, dict) or not isinstance(first.get("url"), str):
        raise ValueError("image API response missing data[0].url")
    return first["url"]


def select_assets(assets: list[dict[str, Any]], only: set[str]) -> list[dict[str, Any]]:
    if not only:
        return assets
    selected = [asset for asset in assets if str(asset.get("id", "")) in only]
    missing = sorted(only - {str(asset.get("id", "")) for asset in selected})
    if missing:
        raise ValueError(f"unknown asset id(s): {', '.join(missing)}")
    return selected


def build_prompt(style_prefix: str, prompt: str) -> str:
    if style_prefix:
        return f"{style_prefix}. {prompt}"
    return prompt


def load_env_file(path: Path) -> None:
    if not path.exists():
        return
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip().strip("'\"")
        if key and key not in os.environ:
            os.environ[key] = value


def default_env_file() -> Path:
    return Path(__file__).resolve().parents[1] / ".env.local"


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Axmol art assets with Right Code image API.")
    parser.add_argument("--manifest", type=Path, default=repo_relative_default_manifest())
    parser.add_argument("--out-dir", type=Path, default=None, help="Defaults to the manifest directory.")
    parser.add_argument("--only", nargs="*", default=[], help="Optional asset ids to generate.")
    parser.add_argument("--dry-run", action="store_true", help="Print planned requests without calling the API.")
    parser.add_argument("--overwrite", action="store_true", help="Regenerate files that already exist.")
    parser.add_argument("--api-base", default=os.environ.get("RIGHT_CODE_IMAGE_API_BASE", API_BASE))
    parser.add_argument("--key-env", default=DEFAULT_KEY_ENV)
    parser.add_argument("--timeout", type=int, default=180)
    parser.add_argument("--sleep", type=float, default=0.5, help="Delay between requests.")
    parser.add_argument("--env-file", type=Path, default=default_env_file(), help="Optional local env file, ignored by git.")
    args = parser.parse_args()

    load_env_file(args.env_file)
    manifest = load_manifest(args.manifest)
    out_dir = args.out_dir or args.manifest.parent
    style_prefix = str(manifest.get("style_prefix", ""))
    default_model = str(manifest.get("default_model", "gpt-image-2"))
    default_size = str(manifest.get("default_size", "1024x1024"))
    assets = select_assets(manifest["assets"], set(args.only))

    api_key = os.environ.get(args.key_env, "")
    if not args.dry_run and not api_key:
        print(f"Missing API key. Export {args.key_env}=sk-... before running.", file=sys.stderr)
        return 2

    endpoint = urllib.parse.urljoin(args.api_base.rstrip("/") + "/", GENERATIONS_PATH.lstrip("/"))
    generated = 0

    for asset in assets:
        asset_id = str(asset.get("id", ""))
        output = asset.get("output")
        prompt = asset.get("prompt")
        if not asset_id or not isinstance(output, str) or not isinstance(prompt, str):
            raise ValueError(f"invalid asset entry: {asset!r}")

        output_path = out_dir / output
        final_prompt = build_prompt(style_prefix, prompt)
        model = str(asset.get("model", default_model))
        size = str(asset.get("size", default_size))

        if output_path.exists() and not args.overwrite:
            print(f"skip existing {asset_id}: {output_path}")
            continue

        payload = {
            "model": model,
            "prompt": final_prompt,
            "image": asset.get("image", []),
            "size": size,
            "response_format": "url",
        }

        if args.dry_run:
            print(json.dumps({"id": asset_id, "output": str(output_path), "request": payload}, ensure_ascii=False, indent=2))
            continue

        print(f"generate {asset_id} -> {output_path}")
        response = post_json(endpoint, payload, api_key, args.timeout)
        image_url = extract_first_url(response)
        download_file(image_url, output_path, args.timeout)
        generated += 1
        if args.sleep > 0:
            time.sleep(args.sleep)

    print(f"done: {generated} generated, {len(assets)} selected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
