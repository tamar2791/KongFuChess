#include "Moves.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
Moves::Moves(const std::string& moves_file_path, std::pair<int, int> dims) : dims(dims) {
    std::ifstream file(moves_file_path);
    if (!file.good() || !file.is_open()) {
        // Missing moves file is allowed - leave moves empty
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || starts_with(line, "#")) {
            continue;
        }

        // Parse "dr,dc:tag" format, handle comments
        size_t comment_pos = line.find("#");
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        size_t colon_pos = line.find(":");
        std::string move_part;
        std::string tag = "";

        if (colon_pos != std::string::npos) {
            move_part = trim(line.substr(0, colon_pos));
            tag = trim(line.substr(colon_pos + 1));
        } else {
            move_part = trim(line);
        }

        // Parse dr,dc
        size_t comma_pos = move_part.find(",");
        if (comma_pos != std::string::npos) {
            int dr = std::stoi(trim(move_part.substr(0, comma_pos)));
            int dc = std::stoi(trim(move_part.substr(comma_pos + 1)));
            
            moves[{dr, dc}] = tag;
            
            // Also populate rel_moves for backward compatibility
            int tag_int = BOTH;
            if (tag == "capture") tag_int = CAPTURE;
            else if (tag == "non_capture") tag_int = NON_CAPTURE;
            rel_moves.emplace_back(dr, dc, tag_int);
        }
    }
}

// ---------------------------------------------------------------------------
std::vector<std::tuple<int, int, int>> Moves::load_moves(const std::string& fp) {
    std::vector<std::tuple<int, int, int>> moves_list;
    std::ifstream file(fp);
    
    if (!file.is_open()) {
        return moves_list;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (!trimmed.empty() && !starts_with(trimmed, "#")) {
            moves_list.push_back(parse(line));
        }
    }
    
    return moves_list;
}

// ---------------------------------------------------------------------------
std::tuple<int, int, int> Moves::parse(const std::string& s) {
    if (s.find(":") == std::string::npos) {
        throw std::invalid_argument("Unsupported move syntax: '" + trim(s) + "' – ':' required");
    }

    size_t colon_pos = s.find(":");
    std::string coords = s.substr(0, colon_pos);
    std::string tag_str = s.substr(colon_pos + 1);

    // Parse coordinates
    size_t comma_pos = coords.find(",");
    int dr = std::stoi(trim(coords.substr(0, comma_pos)));
    int dc = std::stoi(trim(coords.substr(comma_pos + 1)));

    // Parse tag
    tag_str = trim(tag_str);
    int tag;
    if (tag_str == "capture") {
        tag = CAPTURE;
    } else if (tag_str == "non_capture") {
        tag = NON_CAPTURE;
    } else {
        tag = BOTH; // can both capture and not capture
    }

    return std::make_tuple(dr, dc, tag);
}

// ---------------------------------------------------------------------------
bool Moves::is_dst_cell_valid(int dr, int dc, 
                              const std::vector<PiecePtr>* dst_pieces,
                              char my_color,
                              const bool* dst_has_piece) const {
    
    // Handle synthesized placeholder when dst_has_piece is provided but dst_pieces is null
    std::vector<PiecePtr> dummy_pieces;
    if (dst_has_piece != nullptr && dst_pieces == nullptr) {
        if (*dst_has_piece) {
            // Create dummy piece like Python - we'll create a piece with dummy ID
            // Since we can't create a real Piece without State, we'll handle this differently
            // For now, we'll just treat it as having a piece
            bool has_piece = true;
            return is_dst_cell_valid(dr, dc, has_piece);
        }
    }

    // Check if move exists
    auto move_it = moves.find({dr, dc});
    if (move_it == moves.end()) {
        return false; // unknown relative move
    }

    const std::string& move_tag = move_it->second;
    
    // No tag = can both capture/non-capture
    if (move_tag.empty()) {
        return true;
    }

    if (move_tag == "capture") {
        if (dst_pieces == nullptr) return false;
        // Check if there are enemy pieces (different color)
        for (const auto& piece : *dst_pieces) {
            if (piece->id.length() >= 2 && piece->id[1] != my_color) {
                return true;
            }
        }
        return false;
    }

    if (move_tag == "non_capture") {
        return dst_pieces == nullptr;
    }

    return false; // Invalid tag
}

// ---------------------------------------------------------------------------
bool Moves::is_dst_cell_valid(int dr, int dc, bool dst_has_piece) const {
    // Backward compatibility version
    for (const auto& mv : rel_moves) {
        if (mv.dr == dr && mv.dc == dc) {
            if (mv.tag == BOTH) return true;
            if (mv.tag == NON_CAPTURE) return !dst_has_piece;
            if (mv.tag == CAPTURE) return dst_has_piece;
            return false;
        }
    }
    return false; // not found
}

// ---------------------------------------------------------------------------
bool Moves::is_valid(const std::pair<int, int>& src_cell,
                     const std::pair<int, int>& dst_cell,
                     const std::unordered_map<std::pair<int, int>, std::vector<PiecePtr>, PairHash>& cell2piece,
                     bool is_need_clear_path,
                     char my_color) const {
    
    // Check board boundaries first (like Python)
    if (dst_cell.first < 0 || dst_cell.first >= dims.first || 
        dst_cell.second < 0 || dst_cell.second >= dims.second) {
        return false;
    }

    int dr = dst_cell.first - src_cell.first;
    int dc = dst_cell.second - src_cell.second;
    
    // Get destination pieces
    const std::vector<PiecePtr>* dst_pieces = nullptr;
    auto dst_it = cell2piece.find(dst_cell);
    if (dst_it != cell2piece.end()) {
        dst_pieces = &dst_it->second;
    }
    
    if (!is_dst_cell_valid(dr, dc, dst_pieces, my_color)) {
        return false;
    }

    if (is_need_clear_path && !path_is_clear(src_cell, dst_cell, cell2piece, my_color)) {
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
bool Moves::is_valid(const std::pair<int, int>& src_cell,
                     const std::pair<int, int>& dst_cell,
                     const std::unordered_set<std::pair<int, int>, PairHash>& cell_with_piece) const {
    
    // Backward compatibility version
    int dr = dst_cell.first - src_cell.first;
    int dc = dst_cell.second - src_cell.second;
    bool dst_has_piece = cell_with_piece.count(dst_cell) > 0;
    
    if (!is_dst_cell_valid(dr, dc, dst_has_piece)) return false;
    if (!path_is_clear(src_cell, dst_cell, cell_with_piece)) return false;
    
    // Board bounds check
    if (dst_cell.first < 0 || dst_cell.first >= dims.first || 
        dst_cell.second < 0 || dst_cell.second >= dims.second) return false;
    
    return true;
}

// ---------------------------------------------------------------------------
bool Moves::path_is_clear(const std::pair<int, int>& src_cell,
                         const std::pair<int, int>& dst_cell,
                         const std::unordered_map<std::pair<int, int>, std::vector<PiecePtr>, PairHash>& cell2piece_all,
                         char my_color) const {
    
    int dr = dst_cell.first - src_cell.first;
    int dc = dst_cell.second - src_cell.second;
    
    // Create filtered map with only my pieces (like Python)
    std::unordered_map<std::pair<int, int>, std::vector<PiecePtr>, PairHash> cell2piece;
    for (const auto& entry : cell2piece_all) {
        std::vector<PiecePtr> my_pieces;
        for (const auto& piece : entry.second) {
            if (piece->id.length() >= 2 && piece->id[1] == my_color) {
                my_pieces.push_back(piece);
            }
        }
        if (!my_pieces.empty()) {
            cell2piece[entry.first] = my_pieces;
        }
    }

    // Check if destination has my pieces
    if (cell2piece.find(dst_cell) != cell2piece.end()) {
        std::cout << "Path not clear at (" << dst_cell.first << "," << dst_cell.second << ")" << std::endl;
        return false;
    }

    // Get unit vector for movement direction
    int steps = std::max(std::abs(dr), std::abs(dc));
    if (steps == 0) return true;
    
    double step_r = static_cast<double>(dr) / steps;
    double step_c = static_cast<double>(dc) / steps;

    // Check each cell along the path (excluding src and dst)
    for (int i = 1; i < steps; ++i) {
        int r = src_cell.first + static_cast<int>(i * step_r);
        int c = src_cell.second + static_cast<int>(i * step_c);
        std::pair<int, int> cell = {r, c};
        
        if (cell2piece.find(cell) != cell2piece.end()) {
            std::cout << "Path not clear at (" << r << "," << c << ")" << std::endl;
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
bool Moves::path_is_clear(const std::pair<int, int>& src_cell,
                         const std::pair<int, int>& dst_cell,
                         const std::unordered_set<std::pair<int, int>, PairHash>& cell_with_piece) const {
    
    // Backward compatibility version
    int dr = dst_cell.first - src_cell.first;
    int dc = dst_cell.second - src_cell.second;
    
    if (std::abs(dr) <= 1 && std::abs(dc) <= 1) return true;
    
    int steps = std::max(std::abs(dr), std::abs(dc));
    double step_r = static_cast<double>(dr) / steps;
    double step_c = static_cast<double>(dc) / steps;
    
    for (int i = 1; i < steps; ++i) {
        int r = src_cell.first + static_cast<int>(std::round(i * step_r));
        int c = src_cell.second + static_cast<int>(std::round(i * step_c));
        if (cell_with_piece.count({r, c})) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
std::string Moves::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// ---------------------------------------------------------------------------
bool Moves::starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}