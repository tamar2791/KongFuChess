#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <stdexcept>
#include <iostream>
#include "Common.hpp"
#include "Piece.hpp"

// Constants for move tags
const int CAPTURE = 1;
const int NON_CAPTURE = 0;
const int BOTH = -1;

class Moves {
public:
    // Move structure: dr, dc, tag where tag: -1 both; 0 non-capture; 1 capture
    struct RelMove { 
        int dr; 
        int dc; 
        int tag; 
        
        RelMove(int dr, int dc, int tag) : dr(dr), dc(dc), tag(tag) {}
    };

    // Constructor matching Python's __init__
    Moves(const std::string& moves_file_path, std::pair<int, int> dims);

    // Load moves from file (equivalent to Python's _load_moves)
    std::vector<std::tuple<int, int, int>> load_moves(const std::string& fp);

    // Parse a single line (equivalent to Python's _parse static method)
    static std::tuple<int, int, int> parse(const std::string& s);

    // Enhanced version matching Python's is_dst_cell_valid
    bool is_dst_cell_valid(int dr, int dc, 
                          const std::vector<PiecePtr>* dst_pieces = nullptr,
                          char my_color = 'W',
                          const bool* dst_has_piece = nullptr) const;

    // Simple version for backward compatibility
    bool is_dst_cell_valid(int dr, int dc, bool dst_has_piece) const;

    // Enhanced is_valid matching Python version
    bool is_valid(const std::pair<int, int>& src_cell,
                  const std::pair<int, int>& dst_cell,
                  const std::unordered_map<std::pair<int, int>, std::vector<PiecePtr>, PairHash>& cell2piece,
                  bool is_need_clear_path,
                  char my_color) const;

    // Simple version for backward compatibility
    bool is_valid(const std::pair<int, int>& src_cell,
                  const std::pair<int, int>& dst_cell,
                  const std::unordered_set<std::pair<int, int>, PairHash>& cell_with_piece) const;

private:
    std::pair<int, int> dims;
    std::unordered_map<std::pair<int, int>, std::string, PairHash> moves; // (dr, dc) -> tag_string
    
    // For backward compatibility
    std::vector<RelMove> rel_moves;

    // Enhanced path checking matching Python version
    bool path_is_clear(const std::pair<int, int>& src_cell,
                      const std::pair<int, int>& dst_cell,
                      const std::unordered_map<std::pair<int, int>, std::vector<PiecePtr>, PairHash>& cell2piece_all,
                      char my_color) const;

    // Simple path checking for backward compatibility
    bool path_is_clear(const std::pair<int, int>& src_cell,
                      const std::pair<int, int>& dst_cell,
                      const std::unordered_set<std::pair<int, int>, PairHash>& cell_with_piece) const;

    // Helper function to trim whitespace
    static std::string trim(const std::string& str);
    
    // Helper function to check if string starts with another string
    static bool starts_with(const std::string& str, const std::string& prefix);
};