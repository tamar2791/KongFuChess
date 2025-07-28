#include "Graphics.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <filesystem>

Graphics::Graphics(const std::string& sprites_folder,
	std::pair<int, int> cell_size,
	ImgFactoryPtr img_factory,
	bool loop_, double fps_)

	: loop(loop_), fps(fps_), frame_duration_ms(1000.0 / fps_) {

    namespace fs = std::filesystem;
    if(!sprites_folder.empty() && img_factory) {
        std::vector<fs::path> pngs;
        fs::path root(sprites_folder);
        if(fs::exists(root) && fs::is_directory(root)) {
            for(const auto& entry : fs::directory_iterator(root)) {
                if(entry.is_regular_file() && entry.path().extension() == ".png") {
                    pngs.push_back(entry.path());
                }
            }
            std::sort(pngs.begin(), pngs.end());
            for(const auto& p : pngs) {
                auto img_ptr = img_factory->load(p.string(), cell_size);
                if(img_ptr) frames.push_back(img_ptr); // copy into value vector
            }
        }
    }
}

void Graphics::reset(const Command& cmd) {
	start_ms = cmd.timestamp;
	cur_frame = 0;
}

void Graphics::update(int now_ms) {
	int elapsed = now_ms - start_ms;
	int frames_passed = static_cast<int>(elapsed / frame_duration_ms);
	if (loop)
		cur_frame = frames_passed % frames.size();
	else
		cur_frame = std::min<size_t>(frames_passed, frames.size() - 1);
}

const ImgPtr Graphics::get_img() const {
	if (frames.empty()) throw std::runtime_error("Graphics has no frames loaded");
	return frames[cur_frame];
}