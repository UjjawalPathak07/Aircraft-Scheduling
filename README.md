# Aircraft-Scheduling

A C++ aircraft runway management and conflict resolution system. The program schedules flight requests, prioritizes emergency and low-fuel aircraft, detects runway overlap conflicts, and reassigns flights using a minimum separation window.

## Features

- Multi-level priority handling: Scheduled (normal), Low fuel (expedited), and Emergency (preemption).
- Priority queue implemented with a binary max-heap.
- Temporal conflict detection using half-open time interval overlap checks.
- Automatic slot reallocation with a minimum separation buffer.
- Visual terminal output displaying queue status and the final runway schedule.

## System Architecture

```
Aircraft Request Module (Validation & Ingestion)
                      |
                      v
Priority Queue Manager (Binary Max-Heap)
                      |
                      v
Conflict Detection Engine (Interval Overlap Checks)
                      |
                      v
Slot Reallocation Engine (Separation Buffer Nudge)
                      |
                      v
Schedule Reporter (Formatted Timeline Output)
```

## Modules and Algorithms

| Module | Purpose | Approach |
| --- | --- | --- |
| Request Ingestion | Input validation and request creation | Struct-based records, input bounds checking |
| Priority Queue | Dispatching flights by urgency | Binary max-heap (O(log N) insertion and extraction) |
| Conflict Detection | Finding runway overlap between requests | Half-open interval intersection test |
| Slot Allocation | Resolving overlaps and ensuring safe separation | Greedy forward adjustment to the next free slot |
| Reporting | Displaying timeline and assigned slots | Formatted console table |

## Sample Output

```text
Queued AIC101 (ID 1): priority Scheduled, requested start 09:00, duration 20 minutes.
Queued EMER01 (ID 2): priority Emergency, requested start 09:05, duration 15 minutes.
Queued IGO204 (ID 3): priority Low fuel,  requested start 09:10, duration 15 minutes.

[ALLOCATION] Emergency EMER01 granted preemptive slot: 09:05 - 09:20.
[CONFLICT]   AIC101 conflicts with EMER01 on Runway 1.
[RESOLVED]   AIC101 reassigned to next separation window: 09:25 - 09:45.

------------------------------------------------------------
ID   CALLSIGN   PRIORITY    ASSIGNED RUNWAY   TIMELINE
------------------------------------------------------------
2    EMER01     Emergency   Runway 01         09:05 - 09:20
3    IGO204     Low fuel    Runway 01         09:25 - 09:40
1    AIC101     Scheduled   Runway 01         09:45 - 10:05
------------------------------------------------------------
```
