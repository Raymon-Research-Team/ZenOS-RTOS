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
        readme = (ROOT / "README.md").read_text(encoding="utf-8")
        guide = (ROOT / "docs/guide-html/guide.html").read_text(encoding="utf-8")
        self.assertIn("donatr.ee/raymon-research-team", readme)
        self.assertIn("donatr.ee/raymon-research-team", guide)
        self.assertTrue((ROOT / "docs/guide-html/donate-qr.png").is_file())

    def test_site_keeps_current_implementation_page(self):
        text = (ROOT / "docs/guide-html/current.html").read_text(encoding="utf-8")
        self.assertIn("Current Implementation", text)
        self.assertIn("1.0.1", text)


if __name__ == "__main__":
    unittest.main()
