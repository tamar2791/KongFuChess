from __future__ import annotations

from typing import Tuple, Optional
from abc import ABC, abstractmethod
import math, logging

from Command import Command
from Board import Board
import numpy as np

logger = logging.getLogger(__name__)


class BasePhysics(ABC):  # Interface/base class

    def __init__(self, board: Board, param: float = 1.0):
        self.board = board

        self._start_cell = None
        self._end_cell = None
        self._curr_pos_m = None

        self.param = param
        self._start_ms = 0
        self.do_i_need_clear_path = True



    # ------------------------------------------------------------------
    @abstractmethod
    def reset(self, cmd: Command): ...

    @abstractmethod
    def update(self, now_ms: int) -> Optional[Command]: ...

    # ------------------------------------------------------------------
    # Utilities common to all subclasses
    # ---------------- public helpers ------------------------------------
    def get_pos_m(self) -> Tuple[float, float]:
        """Current position in metres."""
        return self._curr_pos_m

    def get_pos_pix(self) -> Tuple[int, int]:
        """Current position converted to pixels."""
        return self.board.m_to_pix(self._curr_pos_m)

    def get_curr_cell(self) -> Tuple[int, int]:
        """Return current board cell `(row, col)` derived from position."""
        return self.board.m_to_cell(self._curr_pos_m)

    def get_start_ms(self) -> int:
        return self._start_ms

    def can_be_captured(self) -> bool: return True

    def can_capture(self) -> bool:     return True

    def is_movement_blocker(self) -> bool: return False

    def is_need_clear_path(self) -> bool:
        return self.do_i_need_clear_path


class IdlePhysics(BasePhysics):

    def reset(self, cmd: Command):
        self._end_cell = self._start_cell = cmd.params[0]
        self._curr_pos_m = self.board.cell_to_m(self._start_cell)
        self._start_ms = cmd.timestamp

    def update(self, now_ms: int):
        return None

    def can_capture(self) -> bool:
        return False

    def is_movement_blocker(self) -> bool:
        return True


class MovePhysics(BasePhysics):

    def __init__(self, board: Board, param: float = 1.0):
        super().__init__(board, param)
        self._speed_m_s = param
        if self._speed_m_s == 0:
            raise ValueError("_speed_m_s is 0")
        if self._speed_m_s < 0:
            self._speed_m_s = abs(self._speed_m_s)

    def reset(self, cmd: Command):
        self._start_cell = cmd.params[0]
        self._end_cell = cmd.params[1]
        self._curr_pos_m = self.board.cell_to_m(self._start_cell)
        self._start_ms = cmd.timestamp
        start_pos = np.array(self.board.cell_to_m(self._start_cell))
        end_pos = np.array(self.board.cell_to_m(self._end_cell))
        self._movement_vector = end_pos - start_pos
        self._movement_vector_length = math.hypot(*self._movement_vector)
        self._movement_vector = self._movement_vector / self._movement_vector_length
        self._duration_s = self._movement_vector_length / self._speed_m_s

    def update(self, now_ms: int):
        seconds_passed = (now_ms - self._start_ms) / 1000
        self._curr_pos_m = np.array(
            self.board.cell_to_m(self._start_cell)) + self._movement_vector * seconds_passed * self._speed_m_s

        if seconds_passed >= self._duration_s:
            return Command(now_ms, None, "done", [self._end_cell])

        return None

    def get_pos_m(self):
        return self._curr_pos_m

    def get_pos_pix(self):
        return super().get_pos_pix()


class StaticTemporaryPhysics(BasePhysics):
    def __init__(self, board: Board, param: float = 1.0):
        super().__init__(board, param)
        self.duration_s = param

    def reset(self, cmd: Command):
        self._end_cell = self._start_cell = cmd.params[0]
        self._curr_pos_m = self.board.cell_to_m(self._start_cell)
        self._start_ms = cmd.timestamp

    def update(self, now_ms: int):
        seconds_passed = (now_ms - self._start_ms) / 1000
        if seconds_passed >= self.duration_s:
            return Command(now_ms, None, "done", [self._end_cell])

        return None


class JumpPhysics(StaticTemporaryPhysics):
    def reset(self, cmd: Command):
        """Teleport the piece to its destination cell and start the cooldown timer.

        The *jump* command is expected to contain two cells in `cmd.params` –
        the source cell (where the jump starts) and the destination cell
        (where the piece should land).  Some callers/tests, however, might
        still send only a single cell.  We therefore handle both cases.

        Because a jump in KungFu-Chess is meant to be instantaneous from the
        point of view of board occupancy (the piece disappears from the
        source square and immediately shows up on the destination square),
        we update the internal position right away to the *destination* cell.
        """

        # Default behaviour – if only one coordinate is supplied behave the
        # same as the parent implementation (static on that square).
        if len(cmd.params) == 1:
            super().reset(cmd)
            return

        # When two coordinates are provided treat it as a true jump
        self._start_cell, self._end_cell = cmd.params[0], cmd.params[1]

        # Land instantly on the destination square.
        self._curr_pos_m = self.board.cell_to_m(self._end_cell)
        self._start_ms = cmd.timestamp

        # Note: `update()` from StaticTemporaryPhysics will use
        # `self.duration_s` to emit the "done" event after the cooldown
        # period, allowing the state-machine to transition afterwards.

    def can_be_captured(self) -> bool:
        return False


class RestPhysics(StaticTemporaryPhysics):
    def can_capture(self) -> bool: return False

    def is_movement_blocker(self) -> bool: return True
