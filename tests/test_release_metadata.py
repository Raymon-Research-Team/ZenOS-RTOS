import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ReleaseMetadataTests(unittest.TestCase):
    def test_public_version_is_101(self):
        hpp = (ROOT / "ZenOS" / "ZenOS.hpp").read_text(encoding="utf-8")
        self.assertRegex(hpp, r"#define OS_VERSION_MAJOR\s+1")
        self.assertRegex(hpp, r"#define OS_VERSION_MINOR\s+0")
        self.assertRegex(hpp, r"#define OS_VERSION_PATCH\s+1")

    def test_no_stale_version_in_core_headers(self):
        for path in [
            ROOT / "ZenOS" / "ZenOS.hpp",
            ROOT / "ZenOS" / "ZenOS_Internal.hpp",
            ROOT / "ZenOS" / "ZenOS.cpp",
        ]:
            text = path.read_text(encoding="utf-8")
            self.assertNotIn("@version 1.0.0", text, str(path))

    def test_priority_bounds_match_public_config(self):
        config = (ROOT / "ZenOS" / "ZenOS_Config.hpp").read_text(encoding="utf-8")
        public = (ROOT / "ZenOS" / "ZenOS.hpp").read_text(encoding="utf-8")
        self.assertRegex(config, r"OS_KERNEL_MAX_PRIORITIES")
        self.assertRegex(public, r"OS_PRIORITY_BITMAP_WORDS")
        self.assertIn("OS_KERNEL_MAX_PRIORITIES + 31U", public)


if __name__ == "__main__":
    unittest.main()
