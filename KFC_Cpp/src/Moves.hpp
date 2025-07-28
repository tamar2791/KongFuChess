#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <utility>
#include "Common.hpp"

class Moves {
public:
    // dr,dc,tag where tag: -1 both; 0 non-capture; 1 capture
    struct RelMove { int dr; int dc; int tag; };

    Moves(const std::string& txt_path, std::pair<int,int> board_dims);

    bool is_dst_cell_valid(int dr, int dc, bool dst_has_piece) const;
    bool is_valid(const std::pair<int,int>& src_cell,
                  const std::pair<int,int>& dst_cell,
                  const std::unordered_set<std::pair<int,int>, PairHash>& cell_with_piece) const;

private:
    std::vector<RelMove> rel_moves;
    int W; int H;

    static RelMove parse_line(const std::string& s);

    bool path_is_clear(const std::pair<int,int>& src_cell,
                       const std::pair<int,int>& dst_cell,
                       const std::unordered_set<std::pair<int,int>, PairHash>& cell_with_piece) const;
}; 