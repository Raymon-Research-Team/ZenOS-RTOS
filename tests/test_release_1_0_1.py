import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class Release101ConsistencyTests(unittest.TestCase):
    def test_version_is_101(self):
        version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
        self.assertEqual(version, "1.0.1")
        h = read("ZenOS/ZenOS.hpp")
        self.assertIn("OS_VERSION_MAJOR           1", h)
        self.assertIn("OS_VERSION_MINOR           0", h)
        self.assertIn("OS_VERSION_PATCH           1", h)

    def test_priority_contract(self):
        config = read("ZenOS/ZenOS_Config.hpp")
        scheduler = read("ZenOS/ZenOS_Scheduler.cpp")
        self.assertIn("OS_KERNEL_MAX_PRIORITIES 32", config)
        self.assertIn("prio > 0 && prio < OS_KERNEL_MAX_PRIORITIES", scheduler)
        self.assertIn("if (priority == 0) priority = 1;", scheduler)

    def test_current_reference_does_not_claim_fixed_16_tasks(self):
        doc = read("docs/CURRENT_IMPLEMENTATION.md")
        self.assertNotIn("`OS_KERNEL_MAX_TASKS` defaults to **16**", doc)

    def test_public_docs_do_not_use_removed_expected_error_macro(self):
        paths = [
            "README.md",
            "docs/README_FA.md",
            "docs/API_TUTORIAL.md",
            "docs/API_TUTORIAL_FA.md",
            "docs/guide-html/guide.html",
        ]
        for path in paths:
            self.assertNotIn("OS_ERROR_EXPECTED", read(path), path)
            self.assertNotIn("os_error_expect_begin() / os_error_expect_end()", read(path), path)

    def test_site_keeps_current_implementation_page(self):
        self.assertTrue((ROOT / "docs/guide-html/current.html").exists())

    def test_donation_page_is_preserved(self):
        readme = read("README.md")
        legacy = read("docs/guide-html/guide-legacy.html")
        donate = read("docs/guide-html/donate.html")
        self.assertIn("donatr.ee/raymon-research-team", readme)
        self.assertIn("donatr.ee/raymon-research-team", legacy)
        self.assertIn("donatr.ee/raymon-research-team", donate)
        self.assertTrue((ROOT / "docs/guide-html/donate-qr.png").exists())

    def test_authoritative_guide_facts(self):
        guide = read("docs/guide-html/guide.html")
        self.assertIn("ZenOS 1.0.1", guide)
        self.assertIn("Default priority slots", guide)
        self.assertIn("Configurable from 2 to 256", guide)
        self.assertIn("priority-ceiling", guide)
        self.assertIn("not be described as priority-count-independent O(1)", guide)
        self.assertNotIn("Priority Inheritance", guide)
        self.assertNotIn("32-level priority", guide)


if __name__ == "__main__":
    unittest.main()

# Release 1.0.1 final validation trigger.
