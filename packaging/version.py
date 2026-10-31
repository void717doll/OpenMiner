#!/usr/bin/env python3
import os
import re
from pathlib import Path


def game_version():
    root = Path(__file__).resolve().parent.parent
    header = (root / "source/common/core/EngineConfig.hpp").read_text()
    parts = [re.search(r"VERSION_" + name + r"\s*=\s*(\d+)", header).group(1)
             for name in ("MAJOR", "MINOR", "PATCH")]
    suffix = re.search(r'VERSION_SUFFIX\s*=\s*"([^"]*)"', header).group(1)
    return ".".join(parts) + ("-" + suffix if suffix else "")


if __name__ == "__main__":
    version = game_version()
    if os.environ.get("GITHUB_REF_TYPE") == "tag":
        if os.environ.get("GITHUB_REF_NAME") != "v" + version:
            raise SystemExit("release tag must match v" + version)
    print(version)
