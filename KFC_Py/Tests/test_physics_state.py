import pathlib
import numpy as np

from Board import Board
from Command import Command
from img import Img
from Physics import IdlePhysics, MovePhysics, JumpPhysics, RestPhysics
from State import State
from Piece import Piece
# Adapt graphics builder to utilise mock image loader
from Graphics import Graphics
from GraphicsFactory import MockImgFactory

PIECES_ROOT = pathlib.Path(__file__).parent.parent.parent / "pieces"
SPRITES_DIR = PIECES_ROOT / "BB" / "states" / "idle" / "sprites"

# ---------------------------------------------------------------------------
#                           HELPER BUILDERS
# ---------------------------------------------------------------------------


def _blank_img(w: int = 8, h: int = 8):
    img_path = PIECES_ROOT / "BB" / "states" / "idle" / "sprites" / "1.png"
    return Img().read(img_path, (w, h), keep_aspect=False)


def _board(cells: int = 8):
    cell_px = 1  # keep images tiny – we only test logic
    return Board(cell_px, cell_px, cells, cells, _blank_img(cells, cells))


def _graphics():
    gfx = Graphics(sprites_folder=SPRITES_DIR, cell_size=(1, 1), loop=False, fps=1.0, img_loader=MockImgFactory())
    # substitute minimal frame list
    from mock_img import MockImg
    gfx.frames = [MockImg()]
    return gfx


# ---------------------------------------------------------------------------
#                              PHYSICS TESTS
# ---------------------------------------------------------------------------


def test_idle_physics_properties():
    board = _board()
    phys = IdlePhysics(board)
    cmd = Command(0, "P", "idle", [(2, 3)])
    phys.reset(cmd)

    assert phys.get_curr_cell() == (2, 3)
    assert phys.update(100) is None
    assert not phys.can_capture()
    assert phys.is_movement_blocker()


def test_move_physics_full_cycle():
    board = _board()
    phys = MovePhysics(board, param=1.0)  # 1 cell per second

    cmd = Command(0, "P", "move", [(0, 0), (0, 2)])
    phys.reset(cmd)

    # halfway (t=1.0 s) → still moving
    assert phys.update(1000) is None

    # slightly past expected arrival (t≈2.1 s) → should produce *done*
    done = phys.update(2100)
    assert done and done.type == "done"

    # After completion the current cell equals the destination
    phys.update(2200)
    assert phys.get_curr_cell() == (0, 2)


def test_jump_and_rest_physics():
    board = _board()

    jump = JumpPhysics(board, param=0.05)  # 50 ms airborne – keeps tests fast
    rest = RestPhysics(board, param=0.05)

    start = Command(0, "J", "jump", [(1, 1)])
    jump.reset(start)
    rest.reset(start)

    # Before duration – both still in progress
    assert jump.update(20) is None
    assert rest.update(20) is None

    # After duration – both return *done*
    assert jump.update(100).type == "done"
    assert rest.update(100).type == "done"

    assert not jump.can_be_captured()  # invulnerable while jumping
    assert not rest.can_capture()      # resting cannot capture
    assert rest.is_movement_blocker()

# ---------------------------------------------------------------------------
#                          STATE TRANSITION TESTS
# ---------------------------------------------------------------------------


def test_state_transitions_via_internal_done():
    board = _board()

    # Create physics objects with *very* short durations so we need minimal waits
    idle_phys = IdlePhysics(board)
    jump_phys = JumpPhysics(board, param=0.01)

    gfx_idle = _graphics()
    gfx_jump = _graphics()

    idle = State(moves=None, graphics=gfx_idle, physics=idle_phys)
    jump = State(moves=None, graphics=gfx_jump, physics=jump_phys)
    idle.name = "idle"; jump.name = "jump"

    idle.set_transition("jump", jump)
    jump.set_transition("done", idle)

    piece = Piece("PX", idle)

    # Issue *jump* command – should enter jump state immediately
    piece.on_command(Command(0, piece.id, "jump", [(0, 0)]), {})
    assert piece.state is jump

    # Advance time until JumpPhysics finishes → state machine auto-returns to idle
    piece.update(20)
    assert piece.state is idle 