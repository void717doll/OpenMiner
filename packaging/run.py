#!/usr/bin/env python3
import subprocess
import sys
from collections import deque

tail = deque(maxlen=120)
process = subprocess.Popen(sys.argv[1:], stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, text=True,
                           encoding="utf-8", errors="replace")
for line in process.stdout:
    print(line, end="", flush=True)
    tail.append(line)

result = process.wait()
if result:
    # keep the failure visible in public check annotations as well as job logs.
    message = "".join(tail)[-24000:]
    message = message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
    print(f"::error title=package command failed::{message}", flush=True)
raise SystemExit(result)
