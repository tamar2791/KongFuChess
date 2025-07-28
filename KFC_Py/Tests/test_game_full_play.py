import pathlib, time

from GraphicsFactory import MockImgFactory
from Game import Game
from Command import Command
from GameFactory import create_game

import numpy as np

import pytest

PIECES_ROOT = pathlib.Path(__file__).parent.parent / "pieces"
BOARD_CSV = PIECES_ROOT / "board.csv"


# ---------------------------------------------------------------------------
#                          GAMEPLAY TESTS
# ---------------------------------------------------------------------------




def test_gameplay_pawn_move_and_capture():
    game = create_game("../../pieces", MockImgFactory())
    game._time_factor = 1_000_000_000
    game._update_cell2piece_map()
    pw = game.pos[(6, 0)][0]
    pb = game.pos[(1, 1)][0]
    game.user_input_queue.put(Command(game.game_time_ms(), pw.id, "move", [(6, 0), (4, 0)]))
    game.user_input_queue.put(Command(game.game_time_ms(), pb.id, "move", [(1, 1), (3, 1)]))
    time.sleep(0.5)
    game.run(num_iterations=100, is_with_graphics=False)
    assert pw.current_cell() == (4, 0)
    assert pb.current_cell() == (3, 1)
    time.sleep(0.5)
    game._run_game_loop(num_iterations=100, is_with_graphics=False)
    game.user_input_queue.put(Command(game.game_time_ms(), pw.id, "move", [(4, 0), (3, 1)]))
    time.sleep(0.5)
    game._run_game_loop(num_iterations=100, is_with_graphics=False)
    assert pw.current_cell() == (3, 1)
    assert pw in game.pieces
    assert pb not in game.pieces


# ---------------------------------------------------------------------------
#                          ADDITIONAL GAMEPLAY SCENARIO TESTS
# ---------------------------------------------------------------------------


def test_piece_blocked_by_own_color():
    """A rook cannot move through a friendly pawn that blocks its path."""
    game = create_game("../../pieces", MockImgFactory())
    game._time_factor = 1_000_000_000  # speed-up time for fast tests
    game._update_cell2piece_map()

    rook = game.pos[(7, 0)][0]  # White rook initially at a1

    # Attempt to move the rook two squares forward but the white pawn on (6,0) blocks it
    game.user_input_queue.put(Command(game.game_time_ms(), rook.id, "move", [(7, 0), (5, 0)]))
    time.sleep(0.2)
    game._run_game_loop(num_iterations=50, is_with_graphics=False)

    # Rook must stay in place because the path is blocked by its own pawn
    assert rook.current_cell() == (7, 0)


def test_illegal_move_rejected():
    """A bishop attempting a vertical move (illegal) should be rejected."""
    game = create_game("../../pieces", MockImgFactory())
    game._time_factor = 1_000_000_000
    game._update_cell2piece_map()

    bishop = game.pos[(7, 2)][0]  # White bishop on c1

    # Vertical move is illegal for a bishop
    game.user_input_queue.put(Command(game.game_time_ms(), bishop.id, "move", [(7, 2), (5, 2)]))
    time.sleep(0.2)
    game._run_game_loop(num_iterations=50, is_with_graphics=False)

    assert bishop.current_cell() == (7, 2)


def test_knight_jumps_over_friendly_pieces():
    """A knight should be able to jump over friendly pieces."""
    game = create_game("../../pieces", MockImgFactory())
    game._time_factor = 1_000_000_000
    game._update_cell2piece_map()

    knight = game.pos[(7, 1)][0]  # White knight on b1

    # Knight jumps to c3 over own pawns
    game.user_input_queue.put(Command(game.game_time_ms(), knight.id, "move", [(7, 1), (5, 2)]))
    time.sleep(0.2)
    game._run_game_loop(num_iterations=100, is_with_graphics=False)

    assert knight.current_cell() == (5, 2)


def test_piece_capture():
    """Knight captures an opposing pawn after a sequence of moves."""
    game = create_game("../../pieces", MockImgFactory())
    game._time_factor = 1_000_000_000
    game._update_cell2piece_map()

    # 1. Advance the black pawn from d7 to d5.
    bp = game.pos[(1, 3)][0]  # Black pawn on d7
    game.user_input_queue.put(Command(game.game_time_ms(), bp.id, "move", [(1, 3), (3, 3)]))
    game._run_game_loop(num_iterations=100, is_with_graphics=False)

    # 2. Move white knight (b1) to c3.
    wn = game.pos[(7, 1)][0]
    game.user_input_queue.put(Command(game.game_time_ms(), wn.id, "move", [(7, 1), (5, 2)]))
    game._run_game_loop(num_iterations=100, is_with_graphics=False)

    # 3. Knight captures the pawn on d5 (3,3).
    game.user_input_queue.put(Command(game.game_time_ms(), wn.id, "move", [(5, 2), (3, 3)]))
    game._run_game_loop(num_iterations=150, is_with_graphics=False)

    assert wn.current_cell() == (3, 3)
    assert bp not in game.pieces  # Pawn was captured


def test_pawn_double_step_only_first_move():
    """Pawn may move two squares only on its initial move; afterwards only one square."""
    game = create_game("../../pieces", MockImgFactory())
    game._time_factor = 1_000_000_000
    game._update_cell2piece_map()

    pawn = game.pos[(6, 4)][0]  # White pawn on e2

    # First move: two-square advance e2→e4.
    game.user_input_queue.put(Command(game.game_time_ms(), pawn.id, "move", [(6, 4), (4, 4)]))
    game._run_game_loop(num_iterations=150, is_with_graphics=False)
    assert pawn.current_cell() == (4, 4)

    # Second move attempt: try another two-square advance e4→e2 (illegal).
    game.user_input_queue.put(Command(game.game_time_ms(), pawn.id, "move", [(4, 4), (2, 4)]))
    game._run_game_loop(num_iterations=100, is_with_graphics=False)

    # Pawn should remain on e4 after the illegal two-square attempt.
    assert pawn.current_cell() == (4, 4)

