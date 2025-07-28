#pragma once

#include "State.hpp"
#include "Command.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include "Common.hpp"
#include "Piece.hpp"
#include <utility>  // בשביל std::pair


class Piece;
typedef std::shared_ptr<Piece> PiecePtr;

class Piece {
public:
	Piece(std::string id, std::shared_ptr<State> init_state)
		: id(id), state(init_state) {}

	std::string id;
	std::shared_ptr<State> state;

	using Cell = std::pair<int, int>;
	using Cell2Pieces = std::unordered_map<Cell, std::vector<PiecePtr>, PairHash>;

	void on_command(const Command& cmd, Cell2Pieces&) {
		state = state->on_command(cmd);
	}

	void reset(int start_ms) {
		auto cell = this->current_cell();
		state->reset(Command{ start_ms,id,"idle",{cell} });
	}

	void update(int now_ms) {
		state = state->update(now_ms);
	}

	bool is_movement_blocker() const { return state->physics->is_movement_blocker(); }
	void draw_on_board(Board& board, int now_ms) const {
		int x=state->physics->get_pos_pix().first;
		int y=state->physics->get_pos_pix().second;
		ImgPtr sprite= state->graphics->get_img();
		sprite->draw_on(*board.img, x, y);
	}
	Cell current_cell() const { return state->physics->get_curr_cell(); }
};
