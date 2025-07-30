#pragma once

#include "Common.hpp"
#include "Board.hpp"
#include "Command.hpp"
#include <cmath>
#include <memory>
#include <iostream>
#include <chrono>

class BasePhysics
{
public:
    explicit BasePhysics(const Board &board, double param = 2.0)
        : board(board), param(param), curr_pos_m{0.0, 0.0}, need_clear_path(true) {}

    virtual ~BasePhysics() = default;

    virtual void reset(const Command &cmd) = 0;
    // Update physics state. Return a Command if one is produced, otherwise nullptr
    virtual std::shared_ptr<Command> update(int now_ms) = 0;

    std::pair<double, double> get_pos_m() const { return curr_pos_m; }
    std::pair<int, int> get_pos_pix() const { return board.m_to_pix(curr_pos_m); }
    std::pair<int, int> get_curr_cell() const
    {
        return board.m_to_cell(curr_pos_m);
    }

    int get_start_ms() const { return start_ms; }
    bool is_need_clear_path() const { return need_clear_path; }
    void set_need_clear_path(bool value) { need_clear_path = value; }

    virtual bool can_be_captured() const { return true; }
    virtual bool can_capture() const { return true; }
    virtual bool is_movement_blocker() const { return false; }

protected:
    Board board;
    double param = 2.0;
    bool need_clear_path;

    std::pair<int, int> start_cell;
    std::pair<int, int> end_cell;
    std::pair<double, double> curr_pos_m{0.f, 0.f};
    int start_ms{0};
};

// ---------------------------------------------------------------------------
class IdlePhysics : public BasePhysics
{
public:
    using BasePhysics::BasePhysics;
    void reset(const Command &cmd) override
    {
        std::cout << "[IDLE RESET] Command: " << cmd.type << " with " << cmd.params.size() << " params" << std::endl;
        if (!cmd.params.empty())
        {
            std::cout << "[IDLE RESET] Setting position to: (" << cmd.params[0].first << "," << cmd.params[0].second << ")" << std::endl;
        }

        if (cmd.params.empty())
        {
            std::cout << "[IDLE RESET] No params - keeping current position" << std::endl;
            // השאר הכל כמו שהוא
        }
        else
        {
            start_cell = end_cell = cmd.params[0];
            curr_pos_m = board.cell_to_m(start_cell);
            std::cout << "[IDLE RESET] New curr_pos_m: (" << curr_pos_m.first << "," << curr_pos_m.second << ")" << std::endl;
        }
        start_ms = cmd.timestamp;
    }
    std::shared_ptr<Command> update(int) override { return nullptr; }

    bool can_capture() const override { return false; }
    bool is_movement_blocker() const override { return true; }
};

// ---------------------------------------------------------------------------
class MovePhysics : public BasePhysics
{
public:
    explicit MovePhysics(const Board &board, double speed_cells_per_s)
        : BasePhysics(board, speed_cells_per_s) {}

    void reset(const Command &cmd) override
    {
        std::cout << "[MOVE RESET] Command: " << cmd.type << " with " << cmd.params.size() << " params" << std::endl;
        if (cmd.params.size() >= 2)
        {
            std::cout << "[MOVE RESET] From: (" << cmd.params[0].first << "," << cmd.params[0].second
                      << ") To: (" << cmd.params[1].first << "," << cmd.params[1].second << ")" << std::endl;
        }

        if (cmd.params.size() < 2)
        {
            start_cell = end_cell = {0, 0};
            std::cout << "[MOVE RESET] ERROR: Not enough params, defaulting to (0,0)" << std::endl;
        }
        else
        {
            start_cell = cmd.params[0];
            end_cell = cmd.params[1];
        }
        curr_pos_m = board.cell_to_m(start_cell);
        start_ms = cmd.timestamp;

        std::pair<double, double> start_pos = board.cell_to_m(start_cell);
        std::pair<double, double> end_pos = board.cell_to_m(end_cell);
        movement_vec = {end_pos.first - start_pos.first, end_pos.second - start_pos.second};
        movement_len = std::hypot(movement_vec.first, movement_vec.second);
        double speed_m_s = param;
        duration_s = movement_len / speed_m_s;
    }

    std::shared_ptr<Command> update(int now_ms) override
    {
        double seconds = (now_ms - start_ms) / 1000.0;
        if (seconds >= duration_s)
        {
            curr_pos_m = board.cell_to_m(end_cell);
            start_cell = end_cell;
            std::cout << "[MOVE UPDATE] Movement DONE! Sending done command with end_cell: ("
                      << end_cell.first << "," << end_cell.second << ")" << std::endl;

            return std::make_shared<Command>(Command{now_ms, "", "done", {end_cell}});
        }
        double ratio = seconds / duration_s;
        curr_pos_m = {board.cell_to_m(start_cell).first + movement_vec.first * ratio,
                      board.cell_to_m(start_cell).second + movement_vec.second * ratio};
        return nullptr;
    }

private:
    std::pair<double, double> movement_vec{0.f, 0.f};
    double movement_len{0};
    double duration_s{1.0};

public:
    double get_speed_m_s() const { return param; }
    double get_duration_s() const { return duration_s; }
}; // end MovePhysics

// ---------------------------------------------------------------------------
class StaticTemporaryPhysics : public BasePhysics
{
public:
    using BasePhysics::BasePhysics;
    double get_duration_s() const { return param; }

    void reset(const Command &cmd) override
    {
        if (cmd.params.empty())
        {
            // אל תשנה את המיקום הנוכחי אם אין פרמטרים
            // start_cell = end_cell = {0, 0};
        }
        else
        {
            start_cell = end_cell = cmd.params[0];
            curr_pos_m = board.cell_to_m(start_cell);
        }
        start_ms = cmd.timestamp;
    }

    std::shared_ptr<Command> update(int now_ms) override
    {
        double seconds = (now_ms - start_ms) / 1000.0;
        if (seconds >= param)
        {
            return std::make_shared<Command>(Command{now_ms, "", "done", {end_cell}});
        }
        return nullptr;
    }

    bool is_movement_blocker() const override { return true; }
};

class JumpPhysics : public StaticTemporaryPhysics
{
public:
    using StaticTemporaryPhysics::StaticTemporaryPhysics;
    bool can_be_captured() const override { return false; }
};

class RestPhysics : public StaticTemporaryPhysics
{
public:
    using StaticTemporaryPhysics::StaticTemporaryPhysics;
    bool can_capture() const override { return false; }
};