import pathlib, numpy as np
import pytest
from Board import Board
from mock_img import MockImg
from PhysicsFactory import PhysicsFactory
from Physics import IdlePhysics, MovePhysics, JumpPhysics, RestPhysics
from PieceFactory import PieceFactory
from GraphicsFactory import GraphicsFactory, MockImgFactory


# ---------------------------------------------------------------------------
#                               HELPERS
# ---------------------------------------------------------------------------

def _board():
    return Board(cell_H_pix=32, cell_W_pix=32, W_cells=8, H_cells=8, img=MockImg())


ROOT_DIR = pathlib.Path(__file__).parent.parent.parent
PIECES_DIR = ROOT_DIR / "pieces"


# ---------------------------------------------------------------------------
#                          PHYSICS FACTORY TESTS
# ---------------------------------------------------------------------------

def test_physics_factory_creates_correct_subclasses():
    board = _board()
    pf = PhysicsFactory(board)

    idle = pf.create((0, 0), "idle", {})
    assert isinstance(idle, IdlePhysics)

    move_cfg = {"speed_m_per_sec": 2.0}
    move = pf.create((0, 0), "move", move_cfg)
    assert isinstance(move, MovePhysics)
    assert move._speed_m_s == 2.0

    jump = pf.create((0, 0), "jump", {})
    assert isinstance(jump, JumpPhysics)

    rest_cfg = {"duration_ms": 500}
    rest = pf.create((0, 0), "long_rest", rest_cfg)
    assert isinstance(rest, RestPhysics)
    # duration stored in seconds
    assert rest.duration_s == 0.5


# ---------------------------------------------------------------------------
#                          PIECE FACTORY TESTS
# ---------------------------------------------------------------------------

def test_piece_factory_generates_all_pieces():
    board = _board()
    gfx_factory = GraphicsFactory(MockImgFactory())
    p_factory = PieceFactory(board, pieces_root=PIECES_DIR, graphics_factory=gfx_factory)
    i = 0
    j = 0
    piece_ids = set()
    piece_types = ["B", "K", "P", "Q", "R"]
    piece_colors = ["W", "B"]
    num_pieces_created = 0
    for piece_type in piece_types:
        for piece_color in piece_colors:
            piece_type_name = piece_type + piece_color
            loc = (i, j)
            curr_p = p_factory.create_piece(piece_type_name, loc)
            assert curr_p.id.startswith(piece_type_name + "_")
            assert curr_p.current_cell() == loc
            piece_ids.add(curr_p.id)
            num_pieces_created += 1
            i += 1
            if i >= board.W_cells:
                i = 0
                j += 1
    assert len(piece_ids) == num_pieces_created