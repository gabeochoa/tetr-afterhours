#!/usr/bin/env python3
"""CORRECT guards (see CORRECT.md)."""
import pathlib, re, sys
src = pathlib.Path(sys.argv[1] if len(sys.argv)>1 else "src")
bad=[]
systems = (src/"systems.h").read_text(errors="ignore") if (src/"systems.h").exists() else ""
# E1: no hand-rolled timers outside RepeatGate
body = systems.split("struct RepeatGate",1)[-1].split("};",1)[-1] if "struct RepeatGate" in systems else systems
if re.search(r"float timer\s*;\s*float timerReset", body):
    bad.append("CORRECT-E1 systems.h: hand-rolled float timer/timerReset outside RepeatGate (use RepeatGate::tick)")
if "timer -= dt" in body:
    bad.append("CORRECT-E1 systems.h: 'timer -= dt' outside RepeatGate")
# E2: raylib only via rl.h; makefile must actually use -Werror flags
for f in list(src.rglob("*.h"))+list(src.rglob("*.cpp")):
    if f.name=="rl.h": continue
    if re.search(r'#include\s*[<"]raylib\.h[>"]', f.read_text(errors="ignore")):
        bad.append(f"CORRECT-E2 {f}: includes raylib.h directly, bypassing rl.h namespace wrapper (include rl.h)")
mk = pathlib.Path("makefile")
if mk.exists() and "-Werror" in mk.read_text() and "$(NOFLAGS)" not in mk.read_text().split("all:")[-1].split("\n\n")[0]:
    # simple: compile recipe for all must reference NOFLAGS
    if not re.search(r"all:.*?NOFLAGS", mk.read_text(), re.S):
        bad.append("CORRECT-E2 makefile: -Werror lives in NOFLAGS but the 'all' recipe never uses $(NOFLAGS) (warnings silently return)")
print("\n".join(bad) if bad else "check_correct: OK")
sys.exit(1 if bad else 0)
