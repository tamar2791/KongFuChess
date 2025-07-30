#pragma once

#include "Common.hpp"
#include "State.hpp"
#include "Command.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <utility>  // בשביל std::pair


class Piece;
typedef std::shared_ptr<Piece> PiecePtr;

class Piece {
public:
	Piece(std::string id, std::shared_ptr<State> init_state)
		: id(id), state(init_state) {}

	std::string id;
	std::shared_ptr<State> state;

	void on_command(const Command& cmd, Cell2Pieces& c) {
		state = state->on_command(cmd,c);
	}

	void reset(int start_ms) {
		// Extract position from piece ID (e.g., "PW_(6,6)" -> {6,6})
		std::string id_str = id;
		size_t pos = id_str.find("_(");
		if (pos != std::string::npos) {
			std::string coords = id_str.substr(pos + 2);
			size_t comma = coords.find(',');
			if (comma != std::string::npos) {
				int x = std::stoi(coords.substr(0, comma));
				int y = std::stoi(coords.substr(comma + 1, coords.find(')') - comma - 1));
				state->reset(Command{ start_ms, id, "idle", {{x, y}} });
				return;
			}
		}
		// Fallback
		state->reset(Command{ start_ms, id, "idle", {{0, 0}} });
	}

	void update(int now_ms,Cell2Pieces& c) {
		state = state->update(now_ms,c);
	}

	bool is_movement_blocker() const { return state->physics->is_movement_blocker(); }
	void draw_on_board(Board& board, int now_ms) const {
		state->graphics->update(now_ms);
		int x=state->physics->get_pos_pix().first;
		int y=state->physics->get_pos_pix().second;
		ImgPtr sprite= state->graphics->get_img();
		sprite->draw_on(*board.img, x, y);
	}
	Cell current_cell() const { return state->physics->get_curr_cell(); }
};
