import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONFIG = (ROOT / "ZenOS" / "ZenOS_Priority_Config.hpp").read_text(encoding="utf-8")
INTERNAL = (ROOT / "ZenOS" / "ZenOS_Internal.hpp").read_text(encoding="utf-8")
SCHEDULER = (ROOT / "ZenOS" / "ZenOS_Scheduler.cpp").read_text(encoding="utf-8")


class PriorityModelTests(unittest.TestCase):
    def test_default_is_32_and_range_is_2_to_256(self):
        self.assertIn("#define OS_MAX_PRIORITIES 32", CONFIG)
        self.assertIn("OS_MAX_PRIORITIES < 2", CONFIG)
        self.assertIn("OS_MAX_PRIORITIES > 256", CONFIG)

    def test_bitmap_dimensions_are_derived_from_priority_count(self):
        self.assertIn("OS_PRIORITY_BITMAP_WORDS ((OS_MAX_PRIORITIES + 31U) / 32U)", CONFIG)
        self.assertIn("OS_PRIORITY_EXTRA_COUNT", CONFIG)
        self.assertIn("OS_PRIORITY_EXTRA_WORDS", CONFIG)

    def test_no_priority_banding(self):
        self.assertNotIn("priority >> 3", SCHEDULER)
        self.assertNotIn("os_prio_band", SCHEDULER)
        self.assertNotIn("32-band", SCHEDULER)

    def test_priority_2_and_3_are_distinct_levels(self):
        self.assertIn("os_pq_head_for(p)", SCHEDULER)
        self.assertIn("os_pq_head_ext[OS_PRIORITY_EXTRA_COUNT]", INTERNAL)
        self.assertIn("return prio > 0 && prio < OS_MAX_PRIORITIES;", SCHEDULER)

    def test_creation_validates_configured_priority_before_queue_insertion(self):
        marker = 'extern "C" int8_t _os_task_create_internal('
        create_body = SCHEDULER[SCHEDULER.index(marker):]
        validation = create_body.index("if (!os_priority_valid(priority)) return -1;")
        queue_insert = create_body.index("os_pq_add(task);")
        self.assertLess(validation, queue_insert)

    def test_scheduler_reset_covers_extended_state(self):
        self.assertIn('extern "C" void os_priority_queues_init(void)', SCHEDULER)
        self.assertIn("os_ready_bitmap_ext[i] = 0;", SCHEDULER)
        self.assertIn("os_rr_skip_ext[i] = 0;", SCHEDULER)
        self.assertIn("os_pq_head_ext[i] = nullptr;", SCHEDULER)
        self.assertIn("if (task_count == 0) os_priority_queues_init();", SCHEDULER)
        self.assertIn("task == &idle_tcb && task->priority == 0 && !os_started", SCHEDULER)

    def test_supported_ranges(self):
        for count in (32, 64, 128, 256):
            valid = list(range(1, count))
            self.assertEqual(valid[0], 1)
            self.assertEqual(valid[-1], count - 1)
            self.assertEqual(len(valid), count - 1)
            self.assertEqual(len(set(valid)), count - 1)

    def test_ram_model_for_32bit_target(self):
        self.assertEqual(4 + 32 * 4, 132)
        for count, expected in ((64, 264), (128, 528), (256, 1056)):
            bitmap_bytes = ((count + 31) // 32) * 4
            queue_head_bytes = count * 4
            self.assertEqual(bitmap_bytes + queue_head_bytes, expected)


if __name__ == "__main__":
    unittest.main()
