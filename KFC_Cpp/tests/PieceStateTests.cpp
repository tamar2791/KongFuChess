#include <doctest/doctest.h>

#include "../src/Board.hpp"
#include "../src/Physics.hpp"
#include "../src/State.hpp"
#include "../src/Piece.hpp"
#include "../src/Moves.hpp"
#include "../src/Graphics.hpp"
#include "../src/img/MockImg.hpp"

#include <memory>
#include <unordered_map>
#include <vector>
#include <filesystem>
using namespace std;

// ---------------------------------------------------------------------------
// Helpers (mirror ones already used in PhysicsStateTests)
// ---------------------------------------------------------------------------
static Board make_board(int cells=8, int px=32) {
    return Board(px, px, cells, cells, make_shared<MockImg>());
}

static shared_ptr<Graphics> dummy_gfx() {
    return shared_ptr<Graphics>(new Graphics("", {32,32}, make_shared<MockImgFactory>()));
}

static PiecePtr make_piece(const string& id, const pair<int,int>& cell, Board& board) {
    auto idle_phys = make_shared<IdlePhysics>(board);
    auto move_phys = make_shared<MovePhysics>(board, 1.0); // 1 cell/s
    auto jump_phys = make_shared<JumpPhysics>(board, 0.1); // 100 ms

    auto gfx = dummy_gfx();

    auto idle = make_shared<State>(nullptr, gfx, idle_phys);
    auto move = make_shared<State>(nullptr, gfx, move_phys);
    auto jump = make_shared<State>(nullptr, gfx, jump_phys);

    idle->name = "idle";
    move->name = "move";
    jump->name = "jump";

    idle->set_transition("move", move);
    idle->set_transition("jump", jump);
    move->set_transition("done", idle);
    jump->set_transition("done", idle);

    auto piece = make_shared<Piece>(id, idle);

    // initialise physics position directly via reset
    idle->reset(Command{0, piece->id, "idle", {cell}});

    return piece;
}

// ---------------------------------------------------------------------------
// Tests mirroring test_piece_state_game.py (subset not requiring Game class)
// ---------------------------------------------------------------------------

TEST_CASE("Piece state transitions via commands") {
    Board board = make_board();
    auto piece = make_piece("QW", {4,4}, board);

    CHECK(piece->state->name == "idle");
    CHECK(piece->current_cell() == pair<int,int>{4,4});

    // Move command
    Cell2Pieces map;
    map[piece->current_cell()].push_back(piece);

    piece->on_command(Command{100, piece->id, "move", {{4,4},{4,5}}}, map);
    CHECK(piece->state->name == "move");

    // After move completes (>1s)
    piece->update(1200,map);
    CHECK(piece->state->name == "idle");

    // Jump command
    Cell2Pieces emptyMap;
    piece->on_command(Command{1300, piece->id, "jump", {{4,4}}}, emptyMap);
    CHECK(piece->state->name == "jump");

    // Jump finishes (>100ms)
    piece->update(1500,emptyMap);
    CHECK(piece->state->name == "idle");
}

TEST_CASE("Piece movement blocker flag") {
    Board board = make_board();
    auto piece = make_piece("QW", {4,4}, board);

    CHECK(piece->is_movement_blocker());

    Cell2Pieces map;
    map[piece->current_cell()].push_back(piece);
    piece->on_command(Command{0, piece->id, "move", {{4,4},{4,5}}}, map);
    CHECK_FALSE(piece->is_movement_blocker());
}

TEST_CASE("Invalid command keeps state unchanged") {
    Board board = make_board();
    auto piece = make_piece("QW", {4,4}, board);
    auto state_before = piece->state;
    Cell2Pieces emptyMap2;
    piece->on_command(Command{0, piece->id, "invalid", {}}, emptyMap2);
    CHECK(piece->state == state_before);
} 