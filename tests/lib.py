#!/usr/bin/env python3
""" Provides a TestSuite class to easily run imagination tests. """

import os
import shutil
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory
from time import sleep, time
from typing import Optional

from dogtail.procedural import type
from dogtail.tree import SearchError, root
from dogtail.utils import run


class ExportFailed(RuntimeError):
    pass


def nonce(*args) -> int:
    """ Returns a (time based) nonce """
    return abs(hash((time(), args)))


class TestSuite:
    """ Provides handy test utilities.

    self.imagination is the root object of a running imagination process.
    It is populated by self.start() and reset to None after self.quit().

    self.temp is a temporary directory.

    """

    temp: Path

    def __init__(self):
        self.imagination = None
        self.tempdir_object = None
        self.temp = None
        os.environ["LC_ALL"] = "C"

    def __enter__(self):
        self.tempdir_object = TemporaryDirectory()
        obj = self.tempdir_object.__enter__()
        self.temp = Path(obj)
        self.temp.resolve()
        return self

    def __exit__(self, *args, **kwargs):
        if self.tempdir_object:
            self.tempdir_object.__exit__(*args, **kwargs)
            self.temp = None
            self.tempdir_object = None

    def add_slide(self, filename: Path):
        """ Add a new slide """
        self.menu("Slideshow", "Import pictures")
        self.open_file(filename)

    def _save(self):
        """ Click on save """
        self.imagination.child(roleName="tool bar").child(
            description="Save the slideshow"
        ).button("").click()

    def save(self):
        """ Save, and assert that no save as file chooser pops up """
        self._save()
        try:
            self.imagination.child(roleName="file chooser", retry=False)
        except SearchError:
            return
        raise (RuntimeError("Saved but a file chooser popped up"))

    def save_as(self, filename: Path):
        """ Save as"""
        d = filename.parent
        d.resolve()
        assert filename.suffix == ".img"
        path = str(d / filename.stem)
        self.menu("Slideshow", "Save As")
        filechooser = self.imagination.child(roleName="file chooser")
        button = filechooser.childNamed("Save")
        type(str(path))
        button.click()

    def open_file(self, filename: Path):
        """ When a file chooser is open, open the following path """
        filename.resolve()
        filechooser = self.imagination.child(roleName="file chooser")
        whatever = filechooser.child(description="Open your personal folder")
        whatever.keyCombo("<Control>L")
        text = filechooser.child(roleName="text")
        assert text.visible
        text.click()
        text.typeText(str(filename))
        button = filechooser.childNamed("Open")
        button.click()
        if not button.dead:
            # need to lose the focus first, but not always...
            button.click()
        assert button.dead

    def exif_rotate(self, filename: Path, rotation: int, flip: bool) -> Optional[Path]:
        """ Returns the filename of the same file but with the exif tag refering to the
        same rotation"""
        new = self.temp / (str(nonce((filename, rotation, flip))) + ".jpg")
        rotation = (rotation % 360 + 360) % 360
        if rotation == 0:
            tag = 2 if flip else 1
        elif rotation == 90:
            tag = 8
            if flip:
                return None
        elif rotation == 180:
            tag = 4 if flip else 3
        elif rotation == 270:
            tag = 6
            if flip:
                return None
        else:
            raise ValueError(f"Cannot rotate by a non multiple of 90 amount {rotation}")
        shutil.copy(filename, new)
        subprocess.run(["exiftool", f"-Orientation={tag}", "-n", str(new)])
        new.resolve()
        return new

    def text2img(self, text: str) -> Path:
        """ Creates an image with the text in question on it. """
        filename = self.temp / (str(nonce(text)) + ".jpg")
        subprocess.run(["convert", "-size", "400x600", "label:" + text, str(filename)])
        filename.resolve()
        return filename

    def menu(self, root: str, item: str):
        """ Clicks on the specified menu and submenu """
        menu = self.imagination.menu(root)
        menu.click()
        sleep(0.1)
        menu.menuItem(item).click()

    def quit(self):
        """ Quits.

        Fails if there is a "did you mean to quit without saving" dialog.
        """
        self.menu("Slideshow", "Quit")
        sleep(0.1)
        assert self.imagination.dead
        self.imagination = None

    def open_slideshow(self, filename: Path):
        """ Opens the specified slideshow """
        self.menu("Slideshow", "Open")
        self.open_file(filename)

    def start(self, slideshow=None):
        """ start imagination, returns its root dogtail object """
        app = os.environ.get(
            "IMAGINATION", str(Path(__file__).parent.parent / "src" / "imagination")
        )
        cmd = app
        if slideshow:
            cmd += " " + str(slideshow)
        print("launching", cmd)
        run(cmd, timeout=4)
        self.imagination = root.application("imagination")

    def frame_at(self, video: Path, seconds: float) -> Path:
        """ Extracts one frame of the video, whose path is returned

        seconds is the time of the frame. The output format is jpg.
        """
        out = self.temp / (str(nonce(video, seconds)) + ".jpg")
        subprocess.run(
            ["ffmpeg", "-i", str(video), "-ss", str(seconds), "-vframes", "1", str(out)]
        )
        out.resolve()
        return out

    def ocr(self, image: Path) -> str:
        """ Returns the text on the image in argument.

        Assumes that there is only one line of text.
        """
        out = self.temp / str(nonce(image))
        intermediate = self.temp / (str(nonce(image)) + ".jpg")
        subprocess.run(
            [
                "convert",
                str(image),
                "-auto-orient",
                "-fuzz",
                "1%",
                "-trim",
                str(intermediate),
            ]
        )
        subprocess.run(["tesseract", "-psm", "7", str(intermediate), str(out)])
        with open(f"{out}.txt", "r") as f:
            txt = f.read().strip()
            print(f"ocr={txt}")
            return txt

    def n_slides(self) -> int:
        """ Returns the current number of slides """
        label = self.imagination.child(description="Total number of slides")
        n = int(label.name)
        print("n_slides =", n)
        return n

    def export(self) -> Path:
        """ Export the slideshow to a file whose path is returned """
        out = self.temp / "export.vob"
        self.menu("Slideshow", "Export")
        self.imagination.child("Export Settings").child(roleName="text").click()
        type(str(out))
        self.imagination.child(roleName="dialog").button("OK").click()
        pause = self.imagination.child("Exporting the slideshow").child("Pause")
        while pause.showing:
            sleep(0.3)
        status = (
            self.imagination.child("Exporting the slideshow")
            .child(description="Status of export")
            .text
        )
        self.imagination.child("Exporting the slideshow").button("Close").click()
        if "failed" in status.lower():
            raise ExportFailed(status)
        out.resolve()
        return out

    def choose_slide(self, i: int):
        """ Choose slide index """
        entry = self.imagination.child(description="Current slide number")
        entry.typeText(str(i) + "\n")

    def set_transition_type(self, category: str, name: str):
        """ Set the transition type of the current slide """
        combo = self.imagination.child("Slide Settings").child(
            description="Transition type"
        )
        combo.click()
        menu = combo.menu(category)
        menu.click()
        menu.menuItem(name).click()

    def assert_should_save(self):
        """ Checks that a warning dialog fires when trying to quit without saving """
        main = self.imagination.child(roleName="frame")
        #      never saved                         or saved once but unsaved changes
        assert main.name.startswith("Imagination") or main.name.startswith("*")
        self.menu("Slideshow", "Quit")
        assert not self.imagination.dead
        alert = self.imagination.child(roleName="alert")
        alert.button("Cancel").click()

    def rotate(self, angle: int):
        """ Rotate the current image by $angle degrees in trigonometric direction. $angle must be a multiple of 90. """
        assert angle % 90 == 0
        angle = (angle % 360 + 360) % 360
        angle = angle // 90
        if angle == 3:
            self.menu("Slide", "Rotate clockwise")
        else:
            for i in range(angle):
                self.menu("Slide", "Rotate counter-clockwise")

    def flip(self):
        """ Flip the current image horizontally """
        self.imagination.child(
            description="Flip horizontally the selected slides"
        ).child(roleName="push button").click()
