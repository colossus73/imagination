#!/usr/bin/env python3
""" A test that imagination does not crash when files disappear"""
# from pathlib import Path
# from typing import List, Optional, Tuple

from lib import TestSuite, ExportFailed

with TestSuite() as t:
    a, b, c = map(t.text2img, "abc")

    t.start()

    for f in (a, b, c):
        t.add_slide(f)
    b.unlink()
    try:
        video = t.export()
    except ExportFailed as e:
        assert str(b) in str(e)
    else:
        raise RuntimeError("export should have failed")
    t.assert_should_save()
    t.save_as(t.temp / "badfile.img")
    t.quit()
