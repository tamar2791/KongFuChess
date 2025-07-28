#include <doctest/doctest.h>

#include "../src/PhysicsFactory.hpp"
#include "../src/PieceFactory.hpp"
#include "../src/GraphicsFactory.hpp"
#include "../src/Game.hpp"
#include "../src/img/MockImg.hpp"
#include <unordered_set>
using namespace std;

TEST_CASE("PhysicsFactory creates correct subclasses") {
    Board board(32,32,8,8, make_shared<MockImg>());
    PhysicsFactory pf(board);

    auto idle = pf.create({0,0}, "idle", {});
    CHECK(dynamic_cast<IdlePhysics*>(idle.get()) != nullptr);

    nlohmann::json move_cfg;
    move_cfg["physics"] = {{"speed_m_per_sec", 2.0}};
    auto move = pf.create({0,0}, "move", move_cfg);
    auto* mvPtr = dynamic_cast<MovePhysics*>(move.get());
    REQUIRE(mvPtr);
    CHECK(mvPtr->get_speed_m_s() == doctest::Approx(2.0));

    auto jump = pf.create({0,0}, "jump", {});
    CHECK(dynamic_cast<JumpPhysics*>(jump.get()) != nullptr);

    nlohmann::json rest_cfg;
    rest_cfg["physics"] = {{"duration_ms", 500}};
    auto rest = pf.create({0,0}, "long_rest", rest_cfg);
    auto* restPtr = dynamic_cast<RestPhysics*>(rest.get());
    REQUIRE(restPtr);
    CHECK(restPtr->get_duration_s() == doctest::Approx(0.5));
}

TEST_CASE("PieceFactory generates unique piece ids and correct location") {
    Board board(32,32,8,8, make_shared<MockImg>());
    GraphicsFactory gf;
    PieceFactory pf(board, "../../pieces", gf);

    std::unordered_set<std::string> ids;
    for(int i=0;i<10;++i) {
        auto piece = pf.create_piece("PW", {i%8, i/8});
        CHECK(ids.insert(piece->id).second);
        CHECK(piece->current_cell() == std::pair<int,int>{i%8, i/8});
    }
}

TEST_CASE("create_game builds full board of 32 pieces") {
    auto gf = std::make_shared<MockImgFactory>();
    // the relative path is relative to the exe path , not the code path. 
    Game game = create_game("../../pieces", gf);
    CHECK(game.pieces.size() == 32);
} 