#include <doctest/doctest.h>
#include <fstream>
#include <sstream>
#include <filesystem>

TEST_CASE("NW piece need_clear_path flag") {
    namespace fs = std::filesystem;
    fs::path cfg = fs::path("../../") / "pieces" / "NW" / "states" / "idle" / "config.json";
    std::ifstream in(cfg);
    REQUIRE_MESSAGE(in, "config.json must exist");
    std::stringstream buffer; buffer << in.rdbuf();
    std::string txt = buffer.str();
    CHECK(txt.find("\"need_clear_path\"") != std::string::npos);
    CHECK(txt.find("false") != std::string::npos);
} 