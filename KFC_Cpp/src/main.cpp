#include <iostream>
#include "Game.hpp"
#include "img/OpenCvImg.hpp"
#include <memory>

int main()
{
	std::cout << "Starting KFC_Cpp Game..." << std::endl;
	std::cout << "\n=== הוראות משחק ===" << std::endl;
	std::cout << "שחקן 1: I(מעלה) K(מטה) J(שמאל) L(ימין) ENTER(בחירה) +(קפיצה)" << std::endl;
	std::cout << "שחקן 2: W(מעלה) S(מטה) A(שמאל) D(ימין) F(בחירה) G(קפיצה)" << std::endl;
	std::cout << "==================\n" << std::endl;
	auto img_factory = std::make_shared<OpenCvImgFactory>();
	std::string pieces_root = "../../pieces/"; // project root containing assets
	auto game = create_game(pieces_root, img_factory);

	game.run();

}