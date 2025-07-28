from dataclasses import dataclass, field
from typing import List, Dict, Tuple, Optional

@dataclass
class Command:
    timestamp: int          # ms since game start
    piece_id: str
    type: str               # "move" | "jump" | …
    params: List            # payload (e.g. ["e2", "e4"])

    def __str__(self) -> str:
        return f"Command(timestamp={self.timestamp}, piece_id={self.piece_id}, type={self.type}, params={self.params})"
    
    def __repr__(self) -> str:
        return self.__str__()
