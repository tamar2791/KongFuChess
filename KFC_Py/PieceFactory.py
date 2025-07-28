# PieceFactory.py
from __future__ import annotations
import csv, json, pathlib
from plistlib import InvalidFileException
from typing import Dict, Tuple

from Board import Board
from Command import Command
from GraphicsFactory import GraphicsFactory
from Moves import Moves
from PhysicsFactory import PhysicsFactory
from Piece import Piece
from State import State


class PieceFactory:
    def __init__(self,
                 board: Board,
                 pieces_root,
                 graphics_factory=None,
                 physics_factory=None):

        self.board = board
        self.graphics_factory = graphics_factory or GraphicsFactory()
        self.physics_factory = physics_factory or PhysicsFactory(board)
        self._pieces_root = pieces_root

    # ──────────────────────────────────────────────────────────────
    @staticmethod
    def _load_master_csv(pieces_root: pathlib.Path) -> dict[str, dict[str, str]]:
        _global_trans: dict[str, dict[str, str]] = {}
        csv_path = pieces_root / "transitions.csv"
        if not csv_path.exists():
            return _global_trans

        with csv_path.open(newline="", encoding="utf-8") as f:
            rdr = csv.DictReader(f)
            for row in rdr:
                frm, ev, nxt = row["from_state"], row["event"], row["to_state"]
                _global_trans.setdefault(frm, {})[ev] = nxt

        return _global_trans

    # ──────────────────────────────────────────────────────────────
    def _build_state_machine(self, piece_dir: pathlib.Path) -> State:
        board_size = (self.board.W_cells, self.board.H_cells)
        cell_px = (self.board.cell_W_pix, self.board.cell_H_pix)
        _global_trans = self._load_master_csv(piece_dir / "states")

        states: Dict[str, State] = {}

        # There is no longer a piece-wide fall-back. Each state must provide its own
        # `moves.txt`; if it does not, the state will have *no* legal moves.
        # ── load every <piece>/states/<state>/ ───────────────────
        for state_dir in (piece_dir / "states").iterdir():
            if not state_dir.is_dir():
                continue
            name = state_dir.name

            cfg_path = state_dir / "config.json"
            cfg = json.loads(cfg_path.read_text()) if cfg_path.exists() else {}

            moves_path = state_dir / "moves.txt"
            moves = Moves(moves_path, board_size) if moves_path.exists() else None
            graphics = self.graphics_factory.load(state_dir / "sprites",
                                                  cfg.get("graphics", {}), cell_px)
            physics_cfg = cfg.get("physics", {})
            physics = self.physics_factory.create((0, 0), name, physics_cfg)
            physics.do_i_need_clear_path = cfg.get("need_clear_path", True)

            st = State(moves, graphics, physics)
            st.name = name
            states[name] = st

        # apply master CSV overrides
        for frm, ev_map in _global_trans.items():
            src = states.get(frm)
            if not src:
                continue
            for ev, nxt in ev_map.items():
                dst = states.get(nxt)
                if not dst:
                    continue

                src.set_transition(ev, dst)

        # always start at idle
        return states.get("idle")

    # ──────────────────────────────────────────────────────────────
    def create_piece(self, p_type: str, cell: Tuple[int, int]) -> Piece:
        p_dir = self._pieces_root / p_type
        state = self._build_state_machine(p_dir)

        piece = Piece(f"{p_type}_{cell}", state)
        piece.state.reset(Command(0, piece.id, "idle", [cell]))

        return piece
