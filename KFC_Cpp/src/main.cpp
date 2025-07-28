#include <iostream>
#include "Game.hpp"
#include "img/OpenCvImg.hpp"
#include <memory>

int main() {
	auto img_factory = std::make_shared<OpenCvImgFactory>();
	std::string pieces_root = "../../pieces/";  // project root containing assets
	auto game = create_game(pieces_root, img_factory);
	game.run();
} 