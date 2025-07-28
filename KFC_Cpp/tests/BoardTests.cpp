#include <doctest/doctest.h>
#include <memory>

#include "../src/Board.hpp"
#include "../src/img/MockImg.hpp"

TEST_CASE("Board cell conversions roundtrip") {
    ImgPtr blankImg = std::make_shared<MockImg>(); // empty image – not loaded

    Board board(/*cell_H_pix*/2,
                /*cell_W_pix*/2,
                /*W_cells*/4,
                /*H_cells*/4,
                blankImg);

    std::pair<int,int> cell = {2, 1};
    auto metres = board.cell_to_m(cell);
    auto pix    = board.m_to_pix(metres);
    auto back_cell = board.m_to_cell(metres);

    CHECK(cell == back_cell);
    CHECK(pix  == std::pair<int,int>{1 * board.cell_W_pix, 2 * board.cell_H_pix});

    // Ensure show() does not crash when no image is loaded
    CHECK_NOTHROW(board.show());
} 