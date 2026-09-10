# ZenOS Priority Model

## Design decision

ZenOS intentionally supports **32 real task priorities: 0..31**.

- `0` is reserved/normalized by the task-creation API.
- `1..31` are the usable scheduler priority levels.
- Higher numeric values have higher scheduling priority.
- Priorities `32..255` are invalid and must be rejected rather than silently entering the scheduler.

This is a deliberate RAM-minimization choice. ZenOS does not allocate a 256-entry priority bitmap or 256 queue heads.

## Scheduler representation

The scheduler uses the existing fixed structures:

```cpp
uint32_t os_ready_bitmap;
TCB* volatile os_pq_head[32];
```

On a 32-bit MCU this is 4 + (32 × 4) = **132 bytes** of persistent priority-queue storage, excluding the TCBs themselves.

Each priority has exactly one bitmap bit and one queue head. Therefore priority `2` and priority `3` are completely independent scheduler levels.

## Why not 256 priorities?

A direct 256-level implementation would require at minimum:

- 256 bitmap bits = 32 bytes
- 256 queue-head pointers = 1024 bytes on a 32-bit MCU
- total = **1056 bytes** before any additional bookkeeping

Compared with the current 132-byte representation, that is about **924 bytes more RAM** (about 8× the current priority storage).

That increase conflicts with ZenOS's low-RAM design goal and is not justified by the current API requirements.

## Complexity

Ready-level selection uses the 32-bit bitmap and therefore scans a fixed 32 priority levels. Queue operations retain the existing linked-list behavior. The scheduler should be described as deterministic over a fixed 32-priority bitmap, not as an unqualified claim of constant time for every task-selection path.

## Priority inheritance / ceiling

Priority inheritance and priority-ceiling logic continue to use the task's actual priority value. The valid active-priority range is the same 32-level scheduler range, so a boosted priority must remain within `1..31`.

## API validation

Task creation must reject values above `31` before inserting the task into the ready queue. This prevents an invalid priority from being silently lost by the scheduler.

## Examples

| Priority | Meaning |
|---:|---|
| 0 | Reserved / normalized by API policy |
| 1 | Valid |
| 2 | Valid, distinct from 3 |
| 3 | Valid, distinct from 2 |
| 30 | Valid |
| 31 | Highest valid numeric priority |
| 32 | Invalid |
| 255 | Invalid |

This model is intentionally simple: **32 priorities, one bit per priority, no priority banding, no extra scheduler RAM.**
