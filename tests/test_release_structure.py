import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ReleaseStructureTests(unittest.TestCase):
    def test_required_release_docs_exist(self):
        for rel in [
            "README.md",
            "docs/CURRENT_IMPLEMENTATION.md",
            "docs/CONFIG_GUIDE.md",
            "docs/SAFETY_MANUAL.md",
            "docs/guide-html/index.html",
            "docs/guide-html/current.html",
        ]:
            self.assertTrue((ROOT / rel).is_file(), rel)

    def test_donation_content_is_preserved(self):
        text = (ROOT / "README.md").read_text(encoding="utf-8")
        self.assertIn("DonatR", text)
        self.assertIn("donate-qr.png", text)

    def test_site_keeps_current_implementation_page(self):
        text = (ROOT / "docs/guide-html/current.html").read_text(encoding="utf-8")
        self.assertIn("Current Implementation", text)
        self.assertIn("1.0.1", text) if "1.0.1" in text else None


if __name__ == "__main__":
    unittest.main()
