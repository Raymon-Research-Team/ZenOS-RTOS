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
            "docs/guide-html/guide.html",
            "docs/guide-html/guide-legacy.html",
            "docs/guide-html/current.html",
            "docs/guide-html/donate.html",
            "docs/guide-html/donate-qr.png",
        ]:
            self.assertTrue((ROOT / rel).is_file(), rel)

    def test_donation_content_is_preserved(self):
        readme = (ROOT / "README.md").read_text(encoding="utf-8")
        legacy = (ROOT / "docs/guide-html/guide-legacy.html").read_text(encoding="utf-8")
        donate = (ROOT / "docs/guide-html/donate.html").read_text(encoding="utf-8")
        self.assertIn("donatr.ee/raymon-research-team", readme)
        self.assertIn("donatr.ee/raymon-research-team", legacy)
        self.assertIn("donatr.ee/raymon-research-team", donate)
        self.assertTrue((ROOT / "docs/guide-html/donate-qr.png").is_file())

    def test_site_keeps_current_implementation_page(self):
        text = (ROOT / "docs/guide-html/current.html").read_text(encoding="utf-8")
        self.assertIn("Current Implementation", text)
        self.assertIn("1.0.1", text)

    def test_guide_is_authoritative_for_101(self):
        guide = (ROOT / "docs/guide-html/guide.html").read_text(encoding="utf-8")
        self.assertIn("ZenOS 1.0.1", guide)
        self.assertIn("32 priority slots by default", guide)
        self.assertIn("2 to 256", guide)
        self.assertIn("priority-ceiling", guide)
        self.assertNotIn("O(1)", guide)
        self.assertNotIn("Priority Inheritance", guide)


if __name__ == "__main__":
    unittest.main()
