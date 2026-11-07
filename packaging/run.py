#!/usr/bin/env python3
import subprocess
import sys
import re
from collections import deque

tail = deque(maxlen=120)
errors = deque(maxlen=20)
process = subprocess.Popen(sys.argv[1:], stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, text=True,
                           encoding="utf-8", errors="replace")
for line in process.stdout:
    print(line, end="", flush=True)
    tail.append(line)
    if re.search(r"^FAILED:|CMake Error|fatal error|error (?:C|LNK|MSB)\d+|error:|undefined reference|Undefined symbols", line):
        errors.append(line)

result = process.wait()
if result:
    # keep the failure visible in public check annotations as well as job logs.
    message = "".join(errors)[-6000:] + "".join(tail)[-18000:]
    # github truncates individual check annotations after 4096 characters.
    for start in range(0, len(message), 3000):
        part = message[start:start + 3000]
        part = part.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
        print(f"::error title=package command failed::{part}", flush=True)
raise SystemExit(result)
