#include <doctest/doctest.h>
#include "../src/Game.hpp"
#include "../src/img/MockImg.hpp"
#include <algorithm>

// Fixed test cases with correct piece positions
TEST_CASE("Pawn move and capture gameplay") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces/", imgFactory);

    // helper lambdas to find piece by cell
    auto find_piece_cell = [&](std::pair<int,int> cell){
        for(const auto & p : game.pieces) if(p->current_cell()==cell) return p; return PiecePtr();};

    // Based on your log: PW pieces are at row 6, PB pieces are at row 1
    auto pw = find_piece_cell({6,6}); // White pawn at (6,6) - column 6, row 6
    auto pb = find_piece_cell({1,1}); // Black pawn at (1,1) - column 1, row 1
    REQUIRE(pw);
    REQUIRE(pb);

    int now = game.game_time_ms();
    // Move white pawn from (6,6) to (4,6) - 2 squares forward
    game.enqueue_command(Command{now, pw->id, "move", {{6,6},{4,6}}});
    // Move black pawn from (1,1) to (3,1) - 2 squares forward  
    game.enqueue_command(Command{now, pb->id, "move", {{1,1},{3,1}}});

    game.run(250, false);

    CHECK(pw->current_cell() == std::pair<int,int>{4,6});
    CHECK(pb->current_cell() == std::pair<int,int>{3,1});

    // enqueue capture - white pawn captures black pawn
    now = game.game_time_ms();
    game.enqueue_command(Command{now, pw->id, "move", {{4,6},{3,1}}});

    game.run(250, false);

    CHECK(pw->current_cell() == std::pair<int,int>{3,1});
    // black pawn should be removed
    CHECK(std::find(game.pieces.begin(), game.pieces.end(), pb) == game.pieces.end());
}

// ---------------------------------------------------------------------------
TEST_CASE("Rook blocked by own pawn") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces/", imgFactory);

    auto find_piece = [&](std::pair<int,int> cell){ for(auto &p:game.pieces) if(p->current_cell()==cell) return p; return PiecePtr(); };
    auto rook = find_piece({7,0});
    std::cout<<"---------PIECE: "<<rook<<std::endl;
    REQUIRE(rook);

    int now = game.game_time_ms();
    game.enqueue_command(Command{now, rook->id, "move", {{7,0},{5,0}}});

    game.run(200,false);
    
    CHECK(rook->current_cell() == std::pair<int,int>{7,0});
}
TEST_CASE("Rook blocked by own pawn") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces/", imgFactory);

    auto find_piece = [&](std::pair<int,int> cell){ 
        for(auto &p:game.pieces) if(p->current_cell()==cell) return p; 
        return PiecePtr(); 
    };
    
    // Based on your log: RB_(7,0) - Black rook at (7,0)
    auto rook = find_piece({7,0}); // Black rook
    REQUIRE(rook);

    int now = game.game_time_ms();
    // Try to move rook from (7,0) to (5,0) - should be blocked by pawn at (7,1)
    game.enqueue_command(Command{now, rook->id, "move", {{7,0},{5,0}}});

    game.run(200,false);
    
    // Rook should stay in original position due to blocking
    CHECK(rook->current_cell() == std::pair<int,int>{7,0});
}

// ---------------------------------------------------------------------------
TEST_CASE("Illegal bishop vertical move rejected") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces/", imgFactory);

    auto find_piece = [&](std::pair<int,int> cell){ 
        for(auto &p:game.pieces) if(p->current_cell()==cell) return p; 
        return PiecePtr(); 
    };
    
    // Based on your log: BW_(2,7) - White bishop at (2,7)
    auto bishop = find_piece({2,7});
    REQUIRE(bishop);
    
    int now = game.game_time_ms();
    // Try illegal vertical move - bishops can only move diagonally
    game.enqueue_command(Command{now,bishop->id,"move", {{2,7},{2,5}}});
    game.run(200,false);
    
    // Bishop should stay in original position
    CHECK(bishop->current_cell() == std::pair<int,int>{2,7});
}

// ---------------------------------------------------------------------------
TEST_CASE("Knight jumps over friendly pieces") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces/", imgFactory);

    auto find_piece = [&](std::pair<int,int> cell){ 
        for(auto &p:game.pieces) if(p->current_cell()==cell) return p; 
        return PiecePtr(); 
    };
    
    // Based on your log: NW_(6,7) - White knight at (6,7)
    auto knight = find_piece({6,7});
    REQUIRE(knight);
    
    int now = game.game_time_ms();
    // Knight L-shaped move: (6,7) to (5,5) or (4,6) etc.
    game.enqueue_command(Command{now, knight->id, "move", {{6,7},{5,5}}});
    game.run(250,false);
    
    CHECK(knight->current_cell() == std::pair<int,int>{5,5});
}

// ---------------------------------------------------------------------------
TEST_CASE("Knight captures pawn after sequence") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces/", imgFactory);
    
    auto find_piece = [&](std::pair<int,int> cell){ 
        for(auto &p:game.pieces) if(p->current_cell()==cell) return p; 
        return PiecePtr(); 
    };
    
    // Based on your log: PB_(3,1) - Black pawn, NW_(6,7) - White knight
    auto bp = find_piece({3,1}); // Black pawn
    auto wn = find_piece({6,7}); // White knight
    REQUIRE(bp); 
    REQUIRE(wn);

    int now = game.game_time_ms();
    // 1. Black pawn advance from (3,1) to (3,3)
    game.enqueue_command(Command{now, bp->id, "move", {{3,1},{3,3}}});
    game.run(200,false);

    // 2. White knight move from (6,7) to (5,5)
    now = game.game_time_ms();
    game.enqueue_command(Command{now, wn->id, "move", {{6,7},{5,5}}});
    game.run(200,false);

    // 3. Knight captures pawn at (3,3)
    now = game.game_time_ms();
    game.enqueue_command(Command{now, wn->id, "move", {{5,5},{3,3}}});
    game.run(250,false);

    CHECK(wn->current_cell() == std::pair<int,int>{3,3});
    CHECK(std::find(game.pieces.begin(), game.pieces.end(), bp) == game.pieces.end());
}

// ---------------------------------------------------------------------------
TEST_CASE("Pawn double step only on first move") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces/", imgFactory);
    
    auto find_piece = [&](std::pair<int,int> cell){ 
        for(auto &p:game.pieces) if(p->current_cell()==cell) return p; 
        return PiecePtr(); 
    };
    
    // Based on your log: PW_(4,6) - White pawn at column 4, row 6
    auto pawn = find_piece({4,6});
    REQUIRE(pawn);
    
    int now = game.game_time_ms();
    // First double move from (4,6) to (4,4) - 2 squares forward
    game.enqueue_command(Command{now,pawn->id,"move", {{4,6},{4,4}}});
    game.run(200,false);
    CHECK(pawn->current_cell() == std::pair<int,int>{4,4});
    
    // Try illegal double move backwards (4,4) to (4,6)
    now = game.game_time_ms();
    game.enqueue_command(Command{now,pawn->id,"move", {{4,4},{4,6}}});
    game.run(200,false);
    
    // Should stay at (4,4) - move rejected
    CHECK(pawn->current_cell() == std::pair<int,int>{4,4});
}