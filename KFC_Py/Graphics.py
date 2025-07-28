import pathlib
from dataclasses import dataclass, field
from typing import List, Dict, Tuple, Optional
from img import Img
import copy
from Command import Command

import logging

logger = logging.getLogger(__name__)


class Graphics:
    def __init__(self,
                 sprites_folder: pathlib.Path,
                 cell_size: tuple[int, int],
                 img_loader,
                 loop: bool = True,
                 fps: float = 6.0):

        # injectable image loader for tests (defaults to Img().read)
        self._img_loader = img_loader

        self.frames: list[Img] = self._load_sprites(sprites_folder, cell_size)
        self.loop, self.fps = loop, fps
        self.start_ms = 0
        self.cur_frame = 0
        self.frame_duration_ms = 1000 / fps
        logger.debug(f"[LOAD] Graphics from: {sprites_folder}")

    def copy(self):
        # shallow copy is enough: frames list is immutable PNGs
        return copy.copy(self)

    def _load_sprites(self, folder, cell_size):
        frames = []
        for p in sorted(folder.glob("*.png")):
            frames.append(self._img_loader(p, cell_size, keep_aspect=True))
        if not frames:
            raise ValueError(f"No frames found in {folder}")

        return frames

    def reset(self, cmd: Command):
        self.start_ms = cmd.timestamp
        self.cur_frame = 0

    def update(self, now_ms: int):
        elapsed = now_ms - self.start_ms
        frames_passed = int(elapsed / self.frame_duration_ms)
        if self.loop:
            self.cur_frame = frames_passed % len(self.frames)
        else:
            self.cur_frame = min(frames_passed, len(self.frames) - 1)

    def get_img(self) -> Img:
        if not self.frames:
            raise ValueError("No frames loaded for animation.")
        if self.cur_frame >= len(self.frames):
            raise ValueError("Frame index out of range")
        return self.frames[self.cur_frame]
