# CORRECT — repeat-mistake classes

| Rule | Banned shape | Fix level | Evidence (x2+) |
|---|---|---|---|
| CORRECT-E1 | Per-system `float timer/timerReset` + `timer-=dt` in `should_run`, checked before decrement, acting on last call's input | Types (`RepeatGate`) | e576e22, eb3e966, 34cdb44 (rate/debounce retunes), f574e11 (move after force-drop), 0446f4d+b86d734 (lock gating) — Move/Rotate/Fall/ForceDrop each hand-rolled it |
| CORRECT-E2 | `#include "raylib.h"` outside `rl.h`; `-Werror` defined in `NOFLAGS` but unused by `all` | Lint/CI (`make check`, `$(NOFLAGS)`) | 2d81df7/PR#1 "avoid raylib namespace nonsense" yet `systems.h` included it directly; c0b58b3+07b8a97 warnings fixed twice while the recipe never enabled `-Werror` |

Why: E1 — four copies of one timing state machine, divergently ordered; one `RepeatGate{tick: decrement-then-test}`, callers sample input first, `Fall.period=TR` keeps line-clear speed-up. E2 — the wrapper and the flag already existed; enforcing their use is a lint, not a redesign.
Commits (local, one per class): `7589e04` E2, `5c2d781` E1.
Proof: `python3 scripts/check_correct.py src` passes HEAD, exits 1 (E1+E2) on pre-fix HEAD tree (verified). Behaviour: gate order/sample-first is the documented intentional change from the stale-input original.
Honest limit: full compile is broken at unmodified HEAD too (`afterhours::input` in `main.cpp:11`, vendored-plugin drift) — verified by stash; not a class in this repo's own code, not fixed here.
