#!/usr/bin/env python3
""" A test for exif rotation/flip support"""
from pathlib import Path
from typing import List, Optional, Tuple

from lib import TestSuite

with TestSuite() as t:
    b = t.text2img("b")
    data: List[Tuple[Path, int, bool, Optional[str]]] = []
    for angle in range(0, 360, 90):
        for flip in (True, False):
            new = t.exif_rotate(b, angle, flip)
            if new:
                letter = None
                if angle == 0 and not flip:
                    letter = "b"
                elif angle == 0 and flip:
                    letter = "d"
                elif angle == 180 and not flip:
                    letter = "q"
                elif angle == 180 and flip:
                    letter = "P"
                if letter:
                    real = t.ocr(new)
                    assert real == letter, f"rotation of b by {angle}° and flip:{flip} yields {real} and not {letter}"
                data.append((new, angle, flip, letter))

    t.start()

    for (f, angle, flip, letter) in data:
        print(
            f"adding slide with a b rotated by {angle} flipped: {flip} (should look like a {letter})"
        )
        t.add_slide(f)
        t.add_slide(f)
        if flip:
            t.flip()
        t.rotate(-angle)
    video = t.export()
    time = 0.5
    t.assert_should_save()
    t.save_as(t.temp / "exif.img")
    for (f, angle, flip, letter) in data:
        print(
            f"{time} s: expecting a b rotated by {angle} flipped: {flip} (should look like a {letter})"
        )
        if letter:
            assert t.ocr(t.frame_at(video, time)) == letter
        time += 1
        assert t.ocr(t.frame_at(video, time)) == "b"
        time += 1
    t.quit()
