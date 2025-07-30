#include "Moves.hpp"

#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------------
Moves::Moves(const std::string& txt_path, std::pair<int,int> board_dims)
    : W(board_dims.first), H(board_dims.second) {
    std::ifstream in(txt_path);
    if(!in) {
        // Missing moves file is allowed (state may have no legal moves)
        // Leave rel_moves empty – all validations will fail accordingly.
        return;
    }

    std::string line;
    while(std::getline(in, line)) {
        // Trim leading/trailing whitespace
        auto start = line.find_first_not_of(" \t\r\n");
        if(start == std::string::npos) continue; // empty line
        if(line.substr(start,1) == "#") continue; // comment
        rel_moves.push_back(parse_line(line));
    }
}

// ---------------------------------------------------------------------------
Moves::RelMove Moves::parse_line(const std::string& s) {
    auto pos = s.find(':');
    std::string coords = s.substr(0, pos);
    std::string tag_str = s.substr(pos + 1);

    if(pos == std::string::npos) {
        coords = s;
        tag_str = "";
    }
    else
    {
        coords = s.substr(0, pos);
        tag_str = s.substr(pos + 1);
    }
    
    

    int dr, dc;
    char comma;
    std::stringstream ss(coords);
    ss >> dr >> comma >> dc;

    int tag;
    // trim tag_str
    auto first_non = tag_str.find_first_not_of(" \t\r\n");
    if(first_non == std::string::npos) {
        tag = -1; // can both
    } else {
        std::string tag_trim = tag_str.substr(first_non);
        if(tag_trim.rfind("capture",0) == 0) tag = 1; // startswith capture
        else if(tag_trim.rfind("non_capture",0) == 0) tag = 0;
        else tag = -1;
    }
    return {dr, dc, tag};
}

// ---------------------------------------------------------------------------
bool Moves::is_dst_cell_valid(int dr, int dc, bool dst_has_piece) const {
    std::cout << "[MOVES] Looking for move (" << dr << "," << dc << ") in " << rel_moves.size() << " available moves:" << std::endl;
    for(const auto& mv : rel_moves) {
        std::cout << "[MOVES]   Available: (" << mv.dr << "," << mv.dc << ") tag=" << mv.tag << std::endl;
        if(mv.dr == dr && mv.dc == dc) {
            std::cout << "[MOVES]   FOUND MATCH! tag=" << mv.tag << ", dst_has_piece=" << dst_has_piece << std::endl;
            if(mv.tag == -1) return true;
            if(mv.tag == 0)  return !dst_has_piece;
            if(mv.tag == 1)  return dst_has_piece;
            return false;
        }
    }
    std::cout << "[MOVES] Move not found in available moves!" << std::endl;
    return false; // not found
}

bool Moves::is_valid(const std::pair<int,int>& src_cell,
                     const std::pair<int,int>& dst_cell,
                     const std::unordered_set<std::pair<int,int>, PairHash>& cell_with_piece,
                     bool need_clear_path) const {
    int dr = dst_cell.first - src_cell.first;
    int dc = dst_cell.second - src_cell.second;
    bool dst_has_piece = cell_with_piece.count(dst_cell) > 0;
    
    std::cout << "[MOVES] Checking move delta: (" << dr << "," << dc << "), dst_has_piece: " << dst_has_piece << std::endl;
    
    if(!is_dst_cell_valid(dr, dc, dst_has_piece)) {
        std::cout << "[MOVES] Move failed: dst_cell_valid check" << std::endl;
        return false;
    }
    if(need_clear_path && !path_is_clear(src_cell, dst_cell, cell_with_piece)) {
        std::cout << "[MOVES] Move failed: path not clear" << std::endl;
        return false;
    }
    // board bounds
    if(dst_cell.first < 0 || dst_cell.first >= H || dst_cell.second < 0 || dst_cell.second >= W) {
        std::cout << "[MOVES] Move failed: out of bounds" << std::endl;
        return false;
    }
    std::cout << "[MOVES] Move is VALID!" << std::endl;
    return true;
}

bool Moves::path_is_clear(const std::pair<int,int>& src_cell,
                          const std::pair<int,int>& dst_cell,
                          const std::unordered_set<std::pair<int,int>, PairHash>& cell_with_piece) const {
    int dr = dst_cell.first - src_cell.first;
    int dc = dst_cell.second - src_cell.second;
    if(std::abs(dr) <= 1 && std::abs(dc) <= 1) return true;
    int steps = std::max(std::abs(dr), std::abs(dc));
    double step_r = static_cast<double>(dr) / steps;
    double step_c = static_cast<double>(dc) / steps;
    for(int i=1; i<steps; ++i) {
        int r = src_cell.first + static_cast<int>(std::round(i * step_r));
        int c = src_cell.second + static_cast<int>(std::round(i * step_c));
        if(cell_with_piece.count({r,c})) return false;
    }
    return true;
} 