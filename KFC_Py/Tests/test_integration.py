import pathlib, queue, numpy as np

from Board import Board
from Command import Command
from Game import Game
from KeyboardInput import KeyboardProcessor, KeyboardProducer
from Piece import Piece
from State import State
from Physics import IdlePhysics, JumpPhysics
from Graphics import Graphics
from GraphicsFactory import MockImgFactory
from mock_img import MockImg


# ---------------------------------------------------------------------------
# Helper utilities – these rely **only on production code**, no test doubles
# ---------------------------------------------------------------------------

PIECES_ROOT = pathlib.Path(__file__).parent.parent.parent / "pieces"
BOARD_CSV   = PIECES_ROOT / "board.csv"
SPRITES_DIR = PIECES_ROOT / "BB" / "states" / "idle" / "sprites"


def _make_board(cell_px: int = 32):
    """Create an 8×8 board with a blank background."""
    W_cells = H_cells = 8
    img = MockImg()
    return Board(cell_px, cell_px, W_cells, H_cells, img)


def _build_graphics(cell_px: int = 32):
    """Return a Graphics instance with a single blank sprite."""
    return Graphics(sprites_folder=SPRITES_DIR,
                    cell_size=(cell_px, cell_px),
                    loop=False,
                    fps=1.0,
                    img_loader=MockImgFactory())


def _simple_piece(code: str, cell: tuple[int, int], board: Board) -> Piece:
    """Create a *minimal* piece with an *idle* and *jump* state.

    The goal is *not* to replicate the full asset pipeline, merely to
    exercise the real production classes end-to-end without any mocks.
    """
    cell_px = board.cell_W_pix

    # physics -------------------------------------------------------------
    idle_phys = IdlePhysics(board)
    jump_phys = JumpPhysics(board, param=0.1)  # quick jump – 100 ms

    # graphics ------------------------------------------------------------
    gfx_idle = _build_graphics(cell_px)
    gfx_jump = _build_graphics(cell_px)

    # states --------------------------------------------------------------
    idle_state = State(moves=None, graphics=gfx_idle, physics=idle_phys)
    jump_state = State(moves=None, graphics=gfx_jump, physics=jump_phys)
    idle_state.name = "idle"
    jump_state.name = "jump"

    # wiring --------------------------------------------------------------
    idle_state.set_transition("jump", jump_state)
    jump_state.set_transition("done", idle_state)

    # initialise ----------------------------------------------------------
    piece = Piece(f"{code}_{cell}", idle_state)
    # ensure underlying physics has a start_ms & correct cell
    idle_state.reset(Command(0, piece.id, "idle", [cell]))
    return piece


def _load_pieces_and_board():
    """Parse *board.csv* and build the initial *Game* state."""
    board = _make_board()
    pieces: list[Piece] = []

    with BOARD_CSV.open(encoding="utf-8") as f:
        for row_idx, line in enumerate(f.readlines()):
            for col_idx, cell_code in enumerate(line.strip().split(",")):
                if not cell_code:
                    continue
                pieces.append(_simple_piece(cell_code, (row_idx, col_idx), board))

    game = Game(pieces, board)
    return game


# ---------------------------------------------------------------------------
#                               TESTS
# ---------------------------------------------------------------------------

def test_game_initialises_from_csv():
    """The game builds successfully from the shipped *board.csv*."""
    game = _load_pieces_and_board()
    assert isinstance(game, Game)
    assert len(game.pieces) == 32  # 16 per side in standard chess

def test_win_condition_detects_missing_king():
    """Removing a king from *Game.pieces* triggers win detection."""
    game = _load_pieces_and_board()

    # drop Black king and check _is_win()
    gone = [p for p in game.pieces if p.id.startswith("KB_")][0]
    game.pieces.remove(gone)
    assert game._is_win() 