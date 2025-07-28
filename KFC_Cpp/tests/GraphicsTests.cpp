#include <doctest/doctest.h>

#include "../src/Graphics.hpp"
#include "../src/img/MockImg.hpp"
#include "../src/Command.hpp"

#include <vector>
#include <cstddef>
#include <stdexcept>
using namespace std;

static ImgPtr blank_img() {
    return make_shared<MockImg>(pair<int,int>(32,32));
}

TEST_CASE("Graphics animation timing - looping") {
    string sprites_folder = "";
    Graphics gfx(sprites_folder, { 32,32 }, make_shared<MockImgFactory>(), /*loop*/true, /*fps*/10.0);
    // Provide 3 frames
    vector<ImgPtr> frames;
    frames.push_back(blank_img());
    frames.push_back(blank_img());
    frames.push_back(blank_img());
    gfx.set_frames(frames);
    gfx.reset(Command{0,"test","idle",{}});

    const double frame_ms = 1000.0 / 10.0;
    for(size_t i=0;i<frames.size();++i) {
        gfx.update(static_cast<int>(i*frame_ms + frame_ms/2));
        CHECK(gfx.current_frame() == i);
    }
    // Advance beyond last frame - should wrap to 0
    gfx.update(static_cast<int>(frames.size()*frame_ms + frame_ms/2));
    CHECK(gfx.current_frame() == 0);
}

TEST_CASE("Graphics animation - non looping clamps at last frame") {
    Graphics gfx("", {32,32}, make_shared<MockImgFactory>(), /*loop*/false, /*fps*/10.0);
    vector<ImgPtr> frames;
    frames.push_back(blank_img());
    frames.push_back(blank_img());
    frames.push_back(blank_img());
    gfx.set_frames(frames);
    gfx.reset(Command{0,"test","idle",{}});

    gfx.update(1000); // plenty of time - 10 frames passed
    CHECK(gfx.current_frame() == frames.size()-1);
}

TEST_CASE("Graphics get_img throws when no frames") {
    Graphics gfx("", {32,32}, make_shared<MockImgFactory>(), /*loop*/true, 10.0);
    gfx.set_frames({});
    gfx.reset(Command{0,"test","idle",{}});
    CHECK_THROWS_AS(gfx.get_img(), runtime_error);
} 