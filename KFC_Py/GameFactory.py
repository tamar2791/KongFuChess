import pathlib
from Board import Board
from PieceFactory import PieceFactory
from Game import Game

CELL_PX = 64


def create_game(pieces_root: str | pathlib.Path, img_factory) -> Game:
    """Build a *Game* from the on-disk asset hierarchy rooted at *pieces_root*.

    This reads *board.csv* located inside *pieces_root*, creates a blank board
    (or loads board.png if present), instantiates every piece via PieceFactory
    and returns a ready-to-run *Game* instance.
    """
    pieces_root = pathlib.Path(pieces_root)
    board_csv = pieces_root / "board.csv"
    if not board_csv.exists():
        raise FileNotFoundError(board_csv)

    # Board image: use board.png beside this file if present, else blank RGBA
    board_png = pieces_root / "board.png"
    if not board_png.exists():
        raise FileNotFoundError(board_png)

    loader = img_factory

    board_img = loader(board_png, (CELL_PX*8, CELL_PX*8), keep_aspect=False)

    board = Board(CELL_PX, CELL_PX, 8, 8, board_img)

    from GraphicsFactory import GraphicsFactory
    gfx_factory = GraphicsFactory(img_factory)
    pf = PieceFactory(board, pieces_root, graphics_factory=gfx_factory)

    pieces = []
    with board_csv.open() as f:
        for r, line in enumerate(f):
            for c, code in enumerate(line.strip().split(",")):
                if code:
                    pieces.append(pf.create_piece(code, (r, c)))

    return Game(pieces, board) 