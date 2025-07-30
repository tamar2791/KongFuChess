#pragma once

#include <utility>
#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class Piece;
struct PairHash {
    size_t operator()(const std::pair<int,int>& p) const noexcept {
        return static_cast<size_t>(p.first) * 31u + static_cast<size_t>(p.second);
    }
}; 
using PiecePtr = std::shared_ptr<Piece>;
// Common types - now PiecePtr is defined
using Cell = std::pair<int, int>;
using Cell2Pieces = std::unordered_map<Cell, std::vector<PiecePtr>, PairHash>;