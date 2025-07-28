#include <doctest/doctest.h>
#include "../src/Game.hpp"
#include "../src/img/MockImg.hpp"
#include <algorithm>

TEST_CASE("Pawn move and capture gameplay") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    // note: relative path from tests executable – align with existing tests
    Game game = create_game("../../pieces", imgFactory);

    // accelerate physics: we cannot change timeFactor, so rely on sufficient iterations

    // helper lambdas to find piece by cell
    auto find_piece_cell = [&](std::pair<int,int> cell){
        for(const auto & p : game.pieces) if(p->current_cell()==cell) return p; return PiecePtr();};

    auto pw = find_piece_cell({6,0});
    auto pb = find_piece_cell({1,1});
    REQUIRE(pw);
    REQUIRE(pb);

    int now = game.game_time_ms();
    game.enqueue_command(Command{now, pw->id, "move", {{6,0},{4,0}}});
    game.enqueue_command(Command{now, pb->id, "move", {{1,1},{3,1}}});

    game.run(250, false);

    CHECK(pw->current_cell() == std::pair<int,int>{4,0});
    CHECK(pb->current_cell() == std::pair<int,int>{3,1});

    // enqueue capture
    now = game.game_time_ms();
    game.enqueue_command(Command{now, pw->id, "move", {{4,0},{3,1}}});

    game.run(250, false);

    CHECK(pw->current_cell() == std::pair<int,int>{3,1});
    // pawn black should be removed
    CHECK(std::find(game.pieces.begin(), game.pieces.end(), pb) == game.pieces.end());
}

// ---------------------------------------------------------------------------
TEST_CASE("Rook blocked by own pawn") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces", imgFactory);

    auto find_piece = [&](std::pair<int,int> cell){ for(auto &p:game.pieces) if(p->current_cell()==cell) return p; return PiecePtr(); };

    auto rook = find_piece({7,0});
    REQUIRE(rook);

    int now = game.game_time_ms();
    game.enqueue_command(Command{now, rook->id, "move", {{7,0},{5,0}}});

    game.run(200,false);

    CHECK(rook->current_cell() == std::pair<int,int>{7,0});
}

// ---------------------------------------------------------------------------
TEST_CASE("Illegal bishop vertical move rejected") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces", imgFactory);

    auto find_piece = [&](std::pair<int,int> cell){ for(auto &p:game.pieces) if(p->current_cell()==cell) return p; return PiecePtr(); };
    auto bishop = find_piece({7,2});
    REQUIRE(bishop);
    int now = game.game_time_ms();
    game.enqueue_command(Command{now,bishop->id,"move", {{7,2},{5,2}}});
    game.run(200,false);
    CHECK(bishop->current_cell() == std::pair<int,int>{7,2});
}

// ---------------------------------------------------------------------------
TEST_CASE("Knight jumps over friendly pieces") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces", imgFactory);

    auto find_piece = [&](std::pair<int,int> cell){ for(auto &p:game.pieces) if(p->current_cell()==cell) return p; return PiecePtr(); };
    auto knight = find_piece({7,1});
    REQUIRE(knight);
    int now = game.game_time_ms();
    game.enqueue_command(Command{now, knight->id, "move", {{7,1},{5,2}}});
    game.run(250,false);
    CHECK(knight->current_cell() == std::pair<int,int>{5,2});
}

// ---------------------------------------------------------------------------
TEST_CASE("Knight captures pawn after sequence") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces", imgFactory);
    auto find_piece = [&](std::pair<int,int> cell){ for(auto &p:game.pieces) if(p->current_cell()==cell) return p; return PiecePtr(); };
    auto bp = find_piece({1,3});
    auto wn = find_piece({7,1});
    REQUIRE(bp); REQUIRE(wn);

    int now = game.game_time_ms();
    // 1. black pawn advance d7->d5 (1,3)->(3,3)
    game.enqueue_command(Command{now, bp->id, "move", {{1,3},{3,3}}});
    game.run(200,false);

    // 2. white knight b1->c3 (7,1)->(5,2)
    now = game.game_time_ms();
    game.enqueue_command(Command{now, wn->id, "move", {{7,1},{5,2}}});
    game.run(200,false);

    // 3. knight captures pawn c3->d5 (5,2)->(3,3)
    now = game.game_time_ms();
    game.enqueue_command(Command{now, wn->id, "move", {{5,2},{3,3}}});
    game.run(250,false);

    CHECK(wn->current_cell() == std::pair<int,int>{3,3});
    CHECK(std::find(game.pieces.begin(), game.pieces.end(), bp) == game.pieces.end());
}

// ---------------------------------------------------------------------------
TEST_CASE("Pawn double step only on first move") {
    auto imgFactory = std::make_shared<MockImgFactory>();
    Game game = create_game("../../pieces", imgFactory);
    auto find_piece = [&](std::pair<int,int> cell){ for(auto &p:game.pieces) if(p->current_cell()==cell) return p; return PiecePtr(); };
    auto pawn = find_piece({6,4});
    REQUIRE(pawn);
    int now = game.game_time_ms();
    // first double move e2->e4
    game.enqueue_command(Command{now,pawn->id,"move", {{6,4},{4,4}}});
    game.run(200,false);
    CHECK(pawn->current_cell() == std::pair<int,int>{4,4});
    // illegal second double move e4->e2 (backwards two squares)
    now = game.game_time_ms();
    game.enqueue_command(Command{now,pawn->id,"move", {{4,4},{2,4}}});
    game.run(200,false);
    CHECK(pawn->current_cell() == std::pair<int,int>{4,4});
} 