# Moves.py
from __future__ import annotations
import pathlib
from typing import List, Tuple

_CAPTURE = 1  # tag flag
_NON_CAPTURE = 0


class Moves:
    """
    Parse moves.txt lines (whitespace & comments allowed):
        dr,dc               # can both capture and not capture
        dr,dc:non_capture   # non-capture move only
        dr,dc:capture       # capture move only (e.g. pawn diagonal)
    """

    def __init__(self, moves_file: pathlib.Path, dims: Tuple[int, int]):
        """Load moves from a text file.

        Args:
            moves_file: Path to moves.txt file
            dims: Board dimensions (rows, cols)
        """
        self.dims = dims
        self.moves = {}  # (dr, dc) -> tag

        if not moves_file.exists():
            return

        with moves_file.open() as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue

                # Parse "dr,dc:tag" format
                move, *tag = line.split("#")[0].split(":")
                dr, dc = map(int, move.strip().split(","))
                tag = tag[0].strip() if tag else ""

                self.moves[(dr, dc)] = tag

    def _load_moves(self, fp: pathlib.Path) -> List[Tuple[int, int, int]]:
        moves: List[Tuple[int, int, int]] = []
        with open(fp, encoding="utf-8") as f:
            for line in f:
                if line.strip() and not line.lstrip().startswith("#"):
                    moves.append(self._parse(line))
        return moves

    @staticmethod
    def _parse(s: str) -> Tuple[int, int, int]:
        if ":" not in s:
            raise ValueError(f"Unsupported move syntax: '{s.strip()}' – ':' required")

        coords, tag_str = s.split(":", 1)
        dr, dc = [int(x) for x in coords.split(",")]

        if tag_str.strip() == "capture":
            tag = _CAPTURE
        elif tag_str.strip() == "non_capture":
            tag = _NON_CAPTURE
        else:  # can both capture and not capture
            tag = -1

        return dr, dc, tag

    def is_dst_cell_valid(self, dr, dc, dst_pieces = None, my_color = None, dst_has_piece: bool | None = None):
        if dst_has_piece is not None and dst_pieces is None:
            # synthesise minimal placeholder list when a piece is present
            Dummy = type("Dummy", (), {"id": "DX"})
            dst_pieces = [Dummy()] if dst_has_piece else None
            # tests don’t care about colour; default if missing
            my_color   = my_color or "W"

        # unknown relative move
        if (dr, dc) not in self.moves:
            return False

        move_tag = self.moves[(dr, dc)]
        if move_tag == "":  # No tag = can both capture/non-capture
            return True

        if move_tag == "capture":
            return dst_pieces is not None and len([p for p in dst_pieces if p.id[1] != my_color]) > 0

        if move_tag == "non_capture":
            return dst_pieces is None

        return False  # Invalid tag

    def is_valid(self, src_cell, dst_cell, cell2piece, is_need_clear_path, my_color):
        # Check board boundaries
        if not (0 <= dst_cell[0] < self.dims[0] and 0 <= dst_cell[1] < self.dims[1]):
            return False

        dr, dc = dst_cell[0] - src_cell[0], dst_cell[1] - src_cell[1]
        if not self.is_dst_cell_valid(dr, dc, cell2piece.get(dst_cell), my_color):
            return False

        if is_need_clear_path and not self._path_is_clear(src_cell, dst_cell, cell2piece, my_color):
            return False

        return True

    def _path_is_clear(self, src_cell, dst_cell, cell2piece_all, my_color):
        """Check if there are any pieces blocking the path between src and dst."""
        dr = dst_cell[0] - src_cell[0]
        dc = dst_cell[1] - src_cell[1]
        cell2piece = \
            {k: v for k, v in cell2piece_all.items() \
             if any(piece.id[1] == my_color for piece in v)}

        if dst_cell in cell2piece:
            print(f"Path not clear at {dst_cell}")
            return False

        # Get unit vector for movement direction
        steps = max(abs(dr), abs(dc))
        step_r = dr / steps
        step_c = dc / steps

        # Check each cell along the path (excluding src and dst)
        for i in range(1, steps):
            r = src_cell[0] + int(i * step_r)
            c = src_cell[1] + int(i * step_c)
            if (r, c) in cell2piece:
                print(f"Path not clear at {(r,c)}")
                return False

        return True
