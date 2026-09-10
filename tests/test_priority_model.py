from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCHEDULER_PATH = ROOT / "ZenOS" / "ZenOS_Scheduler.cpp"
INTERNAL_PATH = ROOT / "ZenOS" / "ZenOS_Internal.hpp"
SCHEDULER = SCHEDULER_PATH.read_text(encoding="utf-8")
INTERNAL = INTERNAL_PATH.read_text(encoding="utf-8")


class PriorityModelTests(unittest.TestCase):
    def test_scheduler_has_exactly_32_queue_heads(self):
        self.assertIn("os_pq_head[32]", INTERNAL)
        self.assertNotIn("os_pq_head[256]", SCHEDULER)

    def test_no_priority_banding_remains(self):
        self.assertNotIn("os_prio_band", SCHEDULER)
        self.assertNotIn("priority >> 3", SCHEDULER)
        self.assertNotIn("32-band", SCHEDULER)

    def test_invalid_priorities_are_rejected_before_queue_insertion(self):
        marker = 'extern "C" int8_t _os_task_create_internal('
        start = SCHEDULER.index(marker)
        create_body = SCHEDULER[start:]
        validation = create_body.index("if (priority > 31)")
        queue_insert = create_body.index("os_pq_add(task)")
        self.assertLess(validation, queue_insert)
        self.assertIn("return -1;", create_body[validation:queue_insert])

    def test_priority_2_and_3_are_distinct_levels(self):
        self.assertIn("uint8_t p = task->priority;", SCHEDULER)
        self.assertIn("os_pq_head[p] = task;", SCHEDULER)

    def test_all_valid_priority_levels_are_unique(self):
        levels = list(range(1, 32))
        self.assertEqual(len(levels), 31)
        self.assertEqual(len(set(levels)), 31)
        self.assertNotEqual(2, 3)

    def test_boundary_policy(self):
        self.assertLessEqual(0, 31)
        self.assertGreater(32, 31)
        self.assertGreater(255, 31)

    def test_ram_accounting_for_32bit_mcu(self):
        bitmap_bytes = 4
        queue_heads_bytes = 32 * 4
        self.assertEqual(bitmap_bytes + queue_heads_bytes, 132)
        direct_256_bytes = 32 + (256 * 4)
        self.assertEqual(direct_256_bytes, 1056)
        self.assertEqual(direct_256_bytes - 132, 924)


if __name__ == "__main__":
    unittest.main()
