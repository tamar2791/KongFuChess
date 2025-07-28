import pathlib, pytest

# NOTE: These tests reflect the *desired* public API the production code
# will soon provide.  They are expected to FAIL until the implementation
# is completed.

from mock_img import MockImg
from GraphicsFactory import GraphicsFactory, MockImgFactory
from GameFactory import create_game
from Game import Game

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
ROOT_DIR = pathlib.Path(__file__).parent.parent.parent
PIECES_DIR = ROOT_DIR / "pieces"

# ---------------------------------------------------------------------------
# create_game helper expected from GameFactory
# ---------------------------------------------------------------------------

def test_create_game_builds_full_board():
    """create_game() should return a fully-initialised *Game* with 32 pieces."""
    game = create_game(PIECES_DIR, MockImgFactory())

    assert isinstance(game, Game)
    assert len(game.pieces) == 32  # standard chess starting position


def test_graphics_factory_uses_custom_img_factory():
    """GraphicsFactory(img_factory=…) should forward loader to Graphics objects."""

    gf = GraphicsFactory(img_factory=MockImgFactory())

    sprites_dir = PIECES_DIR / "PW" / "states" / "idle" / "sprites"
    gfx = gf.load(sprites_dir, cfg={}, cell_size=(32, 32))

    for frm in gfx.frames:
        assert isinstance(frm, MockImg) 