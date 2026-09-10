# Configurable Scheduler Priority Model

ZenOS supports a compile-time configurable number of scheduler priority slots through `OS_MAX_PRIORITIES` in `ZenOS/ZenOS_Priority_Config.hpp`.

## Semantics

- Default: `OS_MAX_PRIORITIES = 32`
- Supported configuration: `2..256` slots
- Priority `0` is reserved
- Usable priorities are `1..OS_MAX_PRIORITIES-1`
- `32` slots → priorities `1..31`
- `64` slots → priorities `1..63`
- `128` slots → priorities `1..127`
- `256` slots → priorities `1..255`

Every usable priority is a distinct scheduler level. There is no priority banding or `priority >> 3` mapping.

## RAM model

On a 32-bit target, the priority queue heads require `OS_MAX_PRIORITIES * 4` bytes. The ready bitmap requires `ceil(OS_MAX_PRIORITIES / 32) * 4` bytes.

| Slots | Usable priorities | Queue heads | Bitmap | Base priority storage |
|---:|---:|---:|---:|---:|
| 32 | 1..31 | 128 B | 4 B | 132 B |
| 64 | 1..63 | 256 B | 8 B | 264 B |
| 128 | 1..127 | 512 B | 16 B | 528 B |
| 256 | 1..255 | 1024 B | 32 B | 1056 B |

The default 32-priority build retains the original scalar bitmap and 32 queue-head objects, so configurable-priority support adds no priority-storage RAM in the default configuration.

The scheduler also keeps its existing round-robin throttle state. Its extension bitmap is compiled only for configurations above 32 priorities.

## Validation

Task creation rejects a non-zero priority outside the configured range before inserting the task into a priority queue. Existing behavior that normalizes priority `0` to `1` is preserved.
