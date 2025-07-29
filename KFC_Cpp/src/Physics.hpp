#pragma once

#include "Board.hpp"
#include "Command.hpp"
#include "Common.hpp"
#include <cmath>
#include <memory>
#include <iostream>

class BasePhysics
{
public:
    explicit BasePhysics(const Board &board, double param = 2.0)
        : board(board), param(param) {}

    virtual ~BasePhysics() = default;

    virtual void reset(const Command &cmd) = 0;
    // Update physics state. Return a Command if one is produced, otherwise nullptr
    virtual std::shared_ptr<Command> update(int now_ms) = 0;

    std::pair<double, double> get_pos_m() const { return curr_pos_m; }
    std::pair<int, int> get_pos_pix() const { return board.m_to_pix(curr_pos_m); }
    std::pair<int, int> get_curr_cell() const { return board.m_to_cell(curr_pos_m); }

    int get_start_ms() const { return start_ms; }

    virtual bool can_be_captured() const { return true; }
    virtual bool can_capture() const { return true; }
    virtual bool is_movement_blocker() const { return false; }

protected:
    const Board &board;
    double param = 2.0;

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
        // קודם נטפל בפרמטרים
        if (cmd.params.empty())
        {
            start_cell = end_cell = {0, 0};
        }
        else
        {
            auto param = cmd.params[0];
            start_cell = end_cell = param;
        }
        if (cmd.type == "done")
        {
            end_cell = start_cell;
        }
        else if (cmd.params.empty())
        {
            start_cell = end_cell = {0, 0};
        }
        else
        {
            auto param = cmd.params[0];
            if (param.first >= 0 && param.first < 8 &&
                param.second >= 0 && param.second < 8)
            {
                start_cell = end_cell = param;
            }
            else
            {
                start_cell = end_cell = {0, 0}; // ערך ברירת מחדל
            }
        }
        curr_pos_m = board.cell_to_m(start_cell);
        // std::cout << "Reset: start_cell = (" << start_cell.first << "," << start_cell.second << ")" << std::endl;
        // std::cout << "Reset: curr_pos_m = (" << curr_pos_m.first << "," << curr_pos_m.second << ")" << std::endl;
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
        start_cell = cmd.params[0];
        end_cell = cmd.params[1];
        curr_pos_m = board.cell_to_m(start_cell);
        start_ms = cmd.timestamp;

        std::pair<double, double> start_pos = board.cell_to_m(start_cell);
        std::pair<double, double> end_pos = board.cell_to_m(end_cell);
        movement_vec = {end_pos.first - start_pos.first, end_pos.second - start_pos.second};
        movement_len = std::hypot(movement_vec.first, movement_vec.second);
        double speed_m_s = param; // 1 cell == 1m with default cell_size_m
        duration_s = movement_len / speed_m_s;
    }

    std::shared_ptr<Command> update(int now_ms) override
    {
        double seconds = (now_ms - start_ms) / 1000.0;
        if (seconds >= duration_s)
        {
            curr_pos_m = board.cell_to_m(end_cell);
            return std::make_shared<Command>(Command{now_ms, "", "done", {}});
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
        start_cell = end_cell = cmd.params[0];
        curr_pos_m = board.cell_to_m(start_cell);
        start_ms = cmd.timestamp;
    }

    std::shared_ptr<Command> update(int now_ms) override
    {
        double seconds = (now_ms - start_ms) / 1000.0;
        if (seconds >= param)
        {
            return std::make_shared<Command>(Command{now_ms, "", "done", {}});
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