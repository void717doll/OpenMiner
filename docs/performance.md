# frame pacing

retromcp's beta client limits renderer rebuilds and uses the time left in a frame for extra work (`RenderGlobal.updateRenderers` and `EntityRenderer`). oldworld keeps its existing simulation thread and mesh workers, but now spreads backlog across updates instead of draining everything at once.

- chunk streaming has a 3 ms scheduling deadline, examines at most 32 requests, and generates at most one engine chunk per update. neighbor generation resumes on the next update. pending requests are deduplicated; moving to another chunk restarts traversal near the player.
- each world update handles at most 16 lighting/update entries and two processing entries, with separate 2 ms deadlines. pending membership is removed before invoking a chunk so it can schedule another pass. chunk unloading cancels its queued work.
- lighting propagates at most 4,096 nodes per call and schedules another pass when needed. authoritative chunk snapshots cancel superseded client lighting work. save completion drains remaining lighting work before writing cached light values.
- local client messages have a 2 ms / 64-message budget per update. their ordering is preserved.
- the mesh builder uses at most two workers and four outstanding jobs. duplicate requests for a chunk merge into its newest revision. snapshots and gpu uploads are limited to two per update and share a 2 ms deadline. stale results are discarded, including results for removed or replaced chunks. empty meshes clear old geometry. the render thread no longer shrinks completed vertex arrays.

these are scheduling limits, not hard deadlines around individual jobs. generating one terrain column, taking one snapshot, or uploading one mesh can exceed its scheduling allowance. generation still runs on the simulation thread, and the engine retains its existing world-generation algorithms and view distance. unfinished terrain appears gradually rather than blocking the whole backlog. no lighting or chunk records are dropped to meet a budget.

## measured check

a headless executable using the real default mod, seed 1337, the normal server world, and its chunk request queue measured the first twelve streaming updates:

| build | time per update | initialized chunks after twelve updates |
| --- | --- | ---: |
| before | 498.9 to 505.5 ms | 2,485 |
| after | 1.4 to 3.7 ms | 12 |

this compares burst size; it does not compare equal completed work or claim an fps result. the new scheduler intentionally processes less work in each update. regression checks cover backlog completion, deduplication, self-rescheduling, safe removal, and incremental lighting convergence against an unbounded run. a save/reload smoke check also confirmed that pending lighting settles before saving and reloads with identical light bytes. native gpu frame times and the user's hardware still need live gameplay validation.
