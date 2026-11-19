#!/usr/bin/env python3
"""Material palette authority tests for the userland graphics stack.

LIBGRAPHICS owns ordinary visual/background policy. The legacy kernel window
compatibility path still carries the two active theme palettes, so those fields
remain parity-checked only until that path is removed. Background gradients are
userland-only and are checked against the nxres catalog anchor colors.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TOKENS = (ROOT / "system/include/contract/os1_material_colors.h").read_text()
KERNEL = (ROOT / "kernel/graphics/compositor_style.c").read_text()
USER = (ROOT / "system/lib/extensions/LIBGRAPHICS/theme.c").read_text()
MIRROR = (ROOT / "user/NexsOS1-main/lib/look/include/nxres.h").read_text()

BACKGROUNDS = (
    "BLACK", "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN",
    "WHITE", "GRAY", "BRIGHT_RED", "BRIGHT_GREEN", "BRIGHT_YELLOW",
    "BRIGHT_BLUE", "BRIGHT_MAGENTA", "BRIGHT_CYAN", "BRIGHT_WHITE",
)
THEME_FIELDS = (
    "win_bg", "title_active", "title_inactive",
    "title_text_active", "title_text_inactive", "close_btn",
    "border", "accent",
)


def token(name):
    pattern = r"^#define\s+" + re.escape(name) + r"\s+(0x[0-9A-Fa-f]+)U\s*$"
    match = re.search(pattern, TOKENS, re.M)
    if match is None:
        raise ValueError("missing Material token: " + name)
    return int(match.group(1), 16)


def rgba(expr):
    if expr.startswith("OS1_MATERIAL_"):
        return token(expr)
    return int(expr.rstrip("uU"), 16)


def initializer(source, preset, field):
    pattern = (r"\[" + re.escape(preset) + r"\]\s*=\s*\{[^}]*?\."
               + re.escape(field) +
               r"\s*=\s*(0x[0-9A-Fa-f]+[uU]?|OS1_MATERIAL_[A-Z_]+)")
    match = re.search(pattern, source, re.S)
    if match is None:
        raise ValueError("missing theme field: " + preset + "." + field)
    return rgba(match.group(1))


class MaterialPaletteParity(unittest.TestCase):
    def test_all_light_and_dark_fields_match(self):
        for theme in ("LIGHT", "DARK"):
            for field in THEME_FIELDS:
                with self.subTest(theme=theme, field=field):
                    self.assertEqual(
                        initializer(KERNEL, "THEME_" + theme, field),
                        initializer(USER, "OS1_THEME_" + theme, field))

    def test_all_sixteen_userland_backgrounds_are_complete(self):
        for background in BACKGROUNDS:
            preset = "OS1_BG_" + background
            with self.subTest(background=background):
                anchor = initializer(USER, preset, "bg_color")
                top = initializer(USER, preset, "bg_top")
                bottom = initializer(USER, preset, "bg_bottom")
                self.assertGreaterEqual(anchor, 0xFF000000)
                self.assertGreaterEqual(top, 0xFF000000)
                self.assertGreaterEqual(bottom, 0xFF000000)

    def test_registry_color_mirror_matches_userland_authority(self):
        entries = re.findall(
            r"(0x[0-9A-Fa-f]+[uU]?|OS1_MATERIAL_[A-Z_]+)\s*,\s*"
            r"/\*\s*(BG_[A-Z_]+)\s*\*/", MIRROR)
        found = dict((name, rgba(value)) for value, name in entries)
        self.assertEqual(len(found), len(BACKGROUNDS))
        for background in BACKGROUNDS:
            legacy_name = "BG_" + background
            user_name = "OS1_BG_" + background
            with self.subTest(background=background):
                self.assertEqual(
                    found[legacy_name], initializer(USER, user_name, "bg_color"))

    def test_material_neutrals_are_warm_and_grey_is_light(self):
        self.assertLess(token("OS1_MATERIAL_BG_GRAY") & 255, 255)
        self.assertGreater(token("OS1_MATERIAL_BG_GRAY") & 255, 170)
        self.assertNotEqual(token("OS1_MATERIAL_BG_WHITE"), 0xFFFFFFFF)
        self.assertNotEqual(token("OS1_MATERIAL_BG_BRIGHT_WHITE"),
                            0xFFFFFFFF)


if __name__ == "__main__":
    unittest.main()
