import pathlib

from Graphics import Graphics
from img import Img
from mock_img import MockImg


class ImgFactory:
    def __call__(self, *args, **kwargs):
        # f = img_factory()
        # img = f(path, size, keep_aspect)
        # Accept both positional and keyword keep_aspect
        path = args[0]
        size = args[1]
        keep_aspect = kwargs.get("keep_aspect", args[2] if len(args) >= 3 else False)
        return Img().read(path, size, keep_aspect)

class MockImgFactory(ImgFactory):
    def __call__(self, *args, **kwargs):
        path = args[0]
        size = args[1]
        keep_aspect = kwargs.get("keep_aspect", args[2] if len(args) >= 3 else False)
        return MockImg().read(path, size, keep_aspect)


class GraphicsFactory:

    def __init__(self, img_factory):
        # callable path, cell_size, keep_aspect -> Img
        self._img_factory = img_factory

    def load(self,
             sprites_dir: pathlib.Path,
             cfg: dict,
             cell_size: tuple[int, int]) -> Graphics:
        return Graphics(
            sprites_folder=sprites_dir,
            cell_size=cell_size,
            img_loader=self._img_factory,
            loop=cfg.get("is_loop", True),
            fps=cfg.get("frames_per_sec", 6.0)
        )
