import pathlib, tempfile
from types import SimpleNamespace

from Board import Board
from Command import Command
from Graphics import Graphics
from GraphicsFactory import MockImgFactory
from Moves import Moves
from mock_img import MockImg


PIECES_ROOT = pathlib.Path(__file__).parent.parent.parent / "pieces"
SPRITES_DIR = PIECES_ROOT / "BB" / "states" / "idle" / "sprites"


def test_graphics_animation_timing():
    gfx = Graphics(
        sprites_folder=SPRITES_DIR,
        cell_size=(32, 32),
        loop=True,
        fps=10.0,
        img_loader=MockImgFactory(),
    )
    gfx.reset(Command(0, "test", "idle", []))

    num_frames = len(gfx.frames)
    frame_ms   = 1000 / gfx.fps
    for i in range(num_frames):
        gfx.update(i * frame_ms + frame_ms / 2)
        assert gfx.cur_frame == i

    gfx.update(num_frames * frame_ms + frame_ms / 2)
    assert gfx.cur_frame == 0


def test_graphics_non_looping():
    gfx = Graphics(
        sprites_folder=SPRITES_DIR,
        cell_size=(32, 32),
        loop=False,
        fps=10.0,
        img_loader=MockImgFactory(),
    )
    gfx.frames = [MockImg() for _ in range(3)]
    gfx.reset(Command(0, "test", "idle", []))

    gfx.update(1000)                # well past the end
    assert gfx.cur_frame == 2       # stuck on last frame


def test_graphics_empty_frames():
    gfx = Graphics(
        sprites_folder=SPRITES_DIR,
        cell_size=(32, 32),
        loop=True,
        fps=10.0,
        img_loader=MockImgFactory(),
    )
    gfx.frames = []                 # no frames loaded
    try:
        gfx.get_img()
        assert False, "Should raise ValueError"
    except ValueError:
        pass


# ---------------------------------------------------------------------------
#                          MOVES TESTS
# ---------------------------------------------------------------------------

def test_moves_parsing_edge_cases():
    """Exercises capture / non‑capture tags and out‑of‑table checks."""
    with tempfile.TemporaryDirectory() as tmp:
        moves_txt = (
            "1,0:capture\n"
            "-1,0:non_capture\n"
            "0,1:\n"
        )
        path = pathlib.Path(tmp) / "moves.txt"
        path.write_text(moves_txt)

        mv = Moves(path, dims=(8, 8))
        fake_piece = SimpleNamespace(id="PY")   # colour “Y”

        # capture‑only move - valid only if dst location has a piece of other color
        assert mv.is_dst_cell_valid(1, 0, [fake_piece], "X")   # piece present
        assert not mv.is_dst_cell_valid(1, 0, None, "X")       # destination empty

        # non‑capture‑only move
        assert mv.is_dst_cell_valid(-1, 0, None, "X")          # destination empty
        assert not mv.is_dst_cell_valid(-1, 0, [fake_piece], "X")  # piece present

        # tag‑less move: always allowed
        assert mv.is_dst_cell_valid(0, 1, None, "X")
        assert mv.is_dst_cell_valid(0, 1, [fake_piece], "X")

        # move not in table
        assert not mv.is_dst_cell_valid(5, 5, None, "X")


def test_moves_board_bounds():
    with tempfile.TemporaryDirectory() as tmp:
        moves_txt = (
            "1,0:\n"
            "-1,0:\n"
            "0,1:\n"
            "0,-1:\n"
        )
        path = pathlib.Path(tmp) / "moves.txt"
        path.write_text(moves_txt)

        mv = Moves(path, dims=(8, 8))
        center = (4, 4)

        # in‑bounds moves
        assert mv.is_valid(center, (3, 4), {}, True, "X")
        assert mv.is_valid(center, (5, 4), {}, True, "X")
        assert mv.is_valid(center, (4, 3), {}, True, "X")
        assert mv.is_valid(center, (4, 5), {}, True, "X")

        # out‑of‑bounds moves
        assert not mv.is_valid((0, 4), (-1, 4), {}, True, "X")
        assert not mv.is_valid((7, 4), (8, 4), {}, True, "X")
        assert not mv.is_valid((4, 0), (4, -1), {}, True, "X")
        assert not mv.is_valid((4, 7), (4, 8), {}, True, "X")
