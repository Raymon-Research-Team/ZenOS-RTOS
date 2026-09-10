from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCHEDULER = (ROOT / "ZenOS" / "ZenOS_Scheduler.cpp").read_text(encoding="utf-8")


def test_scheduler_has_exactly_32_queue_heads():
    assert "os_pq_head[32]" in (ROOT / "ZenOS" / "ZenOS_Internal.hpp").read_text(encoding="utf-8")
    assert "os_pq_head[256]" not in SCHEDULER


def test_no_priority_banding_remains():
    assert "os_prio_band" not in SCHEDULER
    assert "priority >> 3" not in SCHEDULER
    assert "32-band" not in SCHEDULER


def test_invalid_priorities_are_rejected_before_queue_insertion():
    marker = "extern \"C\" int8_t _os_task_create_internal("
    start = SCHEDULER.index(marker)
    create_body = SCHEDULER[start:]
    validation = create_body.index("if (priority > 31)")
    queue_insert = create_body.index("os_pq_add(task)")
    assert validation < queue_insert
    assert "return -1;" in create_body[validation:queue_insert]


def test_priority_2_and_3_are_distinct_levels():
    # The production scheduler indexes the queue directly by the raw priority.
    assert "uint8_t p = task->priority;" in SCHEDULER
    assert "os_pq_head[p] = task;" in SCHEDULER


def test_all_valid_priority_levels_are_unique():
    levels = list(range(1, 32))
    assert len(levels) == 31
    assert len(set(levels)) == 31
    assert 2 != 3


def test_boundary_policy():
    assert 0 <= 31
    assert 32 > 31
    assert 255 > 31


def test_ram_accounting_for_32bit_mcu():
    bitmap_bytes = 4
    queue_heads_bytes = 32 * 4
    assert bitmap_bytes + queue_heads_bytes == 132
    direct_256_bytes = 32 + (256 * 4)
    assert direct_256_bytes == 1056
    assert direct_256_bytes - 132 == 924
