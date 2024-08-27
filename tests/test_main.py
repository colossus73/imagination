#!/usr/bin/env python3
""" A test for opening, saving and exporting basic images """
from lib import TestSuite

with TestSuite() as t:
    t.start()

    t.add_slide(t.text2img("AB"))
    t.assert_should_save()
    t.add_slide(t.text2img("CD"))
    t.choose_slide(2)
    t.set_transition_type("Bar Wipe", "Left to Right")
    slideshow = t.temp / "result.img"
    t.save_as(slideshow)
    assert t.n_slides() == 2
    t.quit()

    t.start()
    t.open_slideshow(slideshow)
    assert t.n_slides() == 2
    video = t.export()
    assert t.ocr(t.frame_at(video, 0.5)) == "AB"
    assert t.ocr(t.frame_at(video, 5.5)) == "CD"
    # just in the middle of the transition
    assert t.ocr(t.frame_at(video, 3)) == "CB"
    t.quit()

    t.start(slideshow)
    assert t.n_slides() == 2
    # check no need to save
    t.quit()

    t.start()
    t.add_slide(t.text2img("N"))
    assert t.n_slides() == 1
    t.assert_should_save()
    slideshow2 = t.temp / "result2.img"
    t.save_as(slideshow2)
    t.menu("Slideshow", "Import slideshow")
    t.open_file(slideshow)
    t.assert_should_save()
    assert t.n_slides() == 3
    t.save()
    t.quit()

    t.start(slideshow2)
    assert t.n_slides() == 3
    video = t.export()
    assert t.ocr(t.frame_at(video, 0.5)) == "N"
    assert t.ocr(t.frame_at(video, 1.5)) == "AB"
    assert t.ocr(t.frame_at(video, 6.5)) == "CD"
    # just in the middle of the transition
    assert t.ocr(t.frame_at(video, 4)) == "CB"
    t.choose_slide(1)
    t.rotate(-90)
    t.assert_should_save()
    slideshow3 = t.temp / "result3.img"
    t.save_as(slideshow3)
    t.add_slide(t.text2img("d"))
    t.flip()
    t.save()
    assert t.n_slides() == 4
    t.open_slideshow(slideshow2)
    # check that save as does not affect the old slideshow
    assert t.n_slides() == 3
    video = t.export()
    assert t.ocr(t.frame_at(video, 0.5)) == "N"
    assert t.ocr(t.frame_at(video, 1.5)) == "AB"
    assert t.ocr(t.frame_at(video, 6.5)) == "CD"
    # just in the middle of the transition
    assert t.ocr(t.frame_at(video, 4)) == "CB"
    t.quit()

    t.start(slideshow3)
    assert t.n_slides() == 4
    video = t.export()
    assert t.ocr(t.frame_at(video, 0.5)) == "Z"
    assert t.ocr(t.frame_at(video, 1.5)) == "AB"
    assert t.ocr(t.frame_at(video, 6.5)) == "CD"
    # just in the middle of the transition
    assert t.ocr(t.frame_at(video, 4)) == "CB"
    assert t.ocr(t.frame_at(video, 7.5)) == "b"
    t.quit()
