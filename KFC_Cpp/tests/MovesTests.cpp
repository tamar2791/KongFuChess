#include <doctest/doctest.h>

#include "../src/Moves.hpp"

#include <unordered_set>
#include <fstream>
#include <cstdio>

static std::string create_temp_moves_file(const std::string& content) {
    char filename[L_tmpnam];
    std::tmpnam(filename);
    std::ofstream out(filename);
    out << content;
    out.close();
    return filename;
}

TEST_CASE("Moves parsing and validation") {
    std::string moves_txt = "1,0:capture\n-1,0:non_capture\n0,1:\n";
    std::string path = create_temp_moves_file(moves_txt);

    Moves mv(path, {8,8});

    // dst_has_piece cases
    CHECK(mv.is_dst_cell_valid(1,0,true));
    CHECK_FALSE(mv.is_dst_cell_valid(1,0,false));

    CHECK(mv.is_dst_cell_valid(-1,0,false));
    CHECK_FALSE(mv.is_dst_cell_valid(-1,0,true));

    CHECK(mv.is_dst_cell_valid(0,1,false));
    CHECK(mv.is_dst_cell_valid(0,1,true));

    // Clean temp file
    std::remove(path.c_str());
}

TEST_CASE("Moves board bounds and path clear") {
    std::string txt = "1,0:\n-1,0:\n0,1:\n0,-1:\n";
    std::string path = create_temp_moves_file(txt);
    Moves mv(path,{8,8});

    std::unordered_set<std::pair<int,int>, PairHash> occupied; // empty

    std::pair<int,int> center{4,4};
    CHECK(mv.is_valid(center,{3,4},occupied));
    CHECK(mv.is_valid(center,{5,4},occupied));
    CHECK(mv.is_valid(center,{4,3},occupied));
    CHECK(mv.is_valid(center,{4,5},occupied));

    CHECK_FALSE(mv.is_valid({0,4},{-1,4},occupied));
    CHECK_FALSE(mv.is_valid({7,4},{8,4},occupied));
    CHECK_FALSE(mv.is_valid({4,0},{4,-1},occupied));
    CHECK_FALSE(mv.is_valid({4,7},{4,8},occupied));
    std::remove(path.c_str());
} 