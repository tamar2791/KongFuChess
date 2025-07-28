#pragma once

#include "img/ImgFactory.hpp"
#include "Command.hpp"
#include <vector>
#include <string>
#include <utility>  // בשביל std::pair


class Graphics {
public:
	Graphics(const std::string& sprites_folder,
		std::pair<int, int> cell_size,
		ImgFactoryPtr img_factory,
		bool loop = true,
		double fps = 6.0);

	void reset(const Command& cmd);
	void update(int now_ms);
	const ImgPtr get_img() const;

	// Test helpers ---------------------------------------------------------
	size_t current_frame() const { return cur_frame; }
	void set_frames(const std::vector<ImgPtr>& new_frames) { frames = new_frames; }

private:
	std::vector<ImgPtr> frames;
	bool loop{ true };
	double fps{ 6.0 };
	int start_ms{ 0 };
	size_t cur_frame{ 0 };
	double frame_duration_ms{ 0 };
};