import pathlib, queue
import numpy as np

from Board import Board
from Command import Command
from Game import Game, InvalidBoard
from Piece import Piece
from State import State
from Physics import IdlePhysics, MovePhysics, JumpPhysics
from Graphics import Graphics
from GraphicsFactory import MockImgFactory
from img import Img
from Moves import Moves

ROOT_DIR = pathlib.Path(__file__).parent.parent.parent
PIECES_DIR = ROOT_DIR / "pieces"
MOVES_FILE = PIECES_DIR / "QW" / "states" / "idle" / "moves.txt"

def _blank_img(w: int = 8, h: int = 8):
    img_path = ROOT_DIR / "pieces/board.png"
    return Img().read(img_path, (w, h), keep_aspect=False)


def _board(cells: int = 8):
    cell_px = 32
    return Board(cell_px, cell_px, cells, cells, _blank_img(cells * cell_px, cells * cell_px))


def _graphics():
    sprites_dir = pathlib.Path(__file__).parent.parent.parent / "pieces" / "BB" / "states" / "idle" / "sprites"
    return Graphics(sprites_folder=sprites_dir, cell_size=(32, 32),
                    loop=False, fps=1.0,
                    img_loader=MockImgFactory())


def _make_piece(piece_id: str, cell: tuple[int, int], board: Board) -> Piece:
    """Create a test piece with idle, move and jump states."""
    idle_phys = IdlePhysics(board)
    move_phys = MovePhysics(board, param=1.0)  # 1 cell/sec
    jump_phys = JumpPhysics(board, param=0.1)  # 100ms jump

    gfx = _graphics()

    idle = State(moves=Moves(MOVES_FILE, (8,8)), graphics=gfx, physics=idle_phys)
    move = State(moves=None, graphics=gfx, physics=move_phys)
    jump = State(moves=None, graphics=gfx, physics=jump_phys)

    idle.name = "idle"
    move.name = "move"
    jump.name = "jump"

    idle.set_transition("move", move)
    idle.set_transition("jump", jump)
    move.set_transition("done", idle)
    jump.set_transition("done", idle)

    piece = Piece(piece_id, idle)
    # initialise physics position directly
    idle.reset(Command(0, piece.id, "idle", [cell]))  # set actual position
    return piece


def test_piece_state_transitions():
    """Test piece state transitions via commands."""
    board = _board()
    piece = _make_piece("QW", (4, 4), board)
    
    # Initial state
    assert piece.state.name == "idle"
    assert piece.current_cell() == (4, 4)
    
    # Move command
    cell2piece = {piece.current_cell(): [piece]}
    piece.on_command(Command(100, piece.id, "move", [(4, 4), (4, 5)]), cell2piece)
    assert piece.state.name == "move"
    
    # After move completes (>1s at 1 cell/sec)
    piece.update(1200)
    assert piece.state.name == "idle"
    
    # Jump command
    piece.on_command(Command(1300, piece.id, "jump", [(4, 4)]), {})
    assert piece.state.name == "jump"
    
    # After jump completes (>100ms)
    piece.update(1500)
    assert piece.state.name == "idle"


def test_piece_movement_blocker():
    """Test piece movement blocker flag."""
    board = _board()
    piece = _make_piece("QW", (4, 4), board)
    
    # Idle pieces block movement
    assert piece.is_movement_blocker()
    
    # Moving pieces don't block
    cell2piece = {piece.current_cell(): [piece]}
    piece.on_command(Command(0, piece.id, "move", [(4, 4), (4, 5)]), cell2piece)
    assert not piece.is_movement_blocker()


def test_state_invalid_transitions():
    """Test state machine error handling."""
    board = _board()
    piece = _make_piece("QW", (4, 4), board)
    
    # Unknown command type
    piece.on_command(Command(0, piece.id, "invalid", []), {})
    assert piece.state.name == "idle"  # stays in current state
    
    # Move without parameters
    # piece.on_command(Command(0, piece.id, "move", []), {})
    # assert piece.state.name == "idle"



def test_game_initialization_validation():
    """Test Game initialization and board validation."""
    board = _board()
    
    # Valid game needs both kings
    white_king = _make_piece("KW_1", (7, 4), board)
    black_king = _make_piece("KB_1", (0, 4), board)
    game = Game([white_king, black_king], board)
    assert isinstance(game, Game)
    
    # Missing black king
    try:
        Game([white_king], board)
        assert False, "Should raise InvalidBoard"
    except InvalidBoard:
        pass
    
    # Duplicate positions
    try:
        piece1 = _make_piece("PW_1", (4, 4), board)
        piece2 = _make_piece("PW_2", (4, 4), board)
        Game([white_king, black_king, piece1, piece2], board)
        assert False, "Should raise InvalidBoard"
    except InvalidBoard:
        pass


def test_game_collision_resolution():
    """Test piece capture mechanics."""
    board = _board()
    
    # Setup pieces
    white_king = _make_piece("KW_1", (7, 4), board)
    black_king = _make_piece("KB_1", (0, 4), board)
    pawn1 = _make_piece("PW_1", (4, 4), board)
    pawn2 = _make_piece("PB_1", (4, 4), board)  # Same cell as pawn1
    
    game = Game([white_king, black_king, pawn1, pawn2], board)
    
    # Most recent piece wins collision
    pawn1.state.physics._start_ms = 100  # Earlier arrival
    pawn2.state.physics._start_ms = 200  # Later arrival
    
    game._resolve_collisions()
    assert pawn2 in game.pieces  # Winner
    assert pawn1 not in game.pieces  # Captured


def test_game_keyboard_input():
    """Test keyboard input processing."""
    board = _board()
    
    # Setup minimal game
    white_king = _make_piece("KW_1", (7, 4), board)
    black_king = _make_piece("KB_1", (0, 4), board)
    game = Game([white_king, black_king], board)
    
    # Queue a move command
    cmd = Command(game.game_time_ms(), white_king.id, "move", [(7, 4), (6, 4)])
    game.user_input_queue.put(cmd)
    
    # Process one input cycle
    game._update_cell2piece_map()
    while not game.user_input_queue.empty():
        cmd = game.user_input_queue.get()
        game._process_input(cmd)
    
    # Verify king moved
    assert white_king.state.name == "move" 