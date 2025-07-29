#pragma once

#include "Piece.hpp"
#include "PhysicsFactory.hpp"
#include "GraphicsFactory.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <optional>
#include "nlohmann/json.hpp"
#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <iostream>

#include "Moves.hpp"
#include "Board.hpp"
#include "Command.hpp"

class PieceFactory
{
public:
    PieceFactory(Board &board,
                 const std::string &pieces_root,
                 const GraphicsFactory &gfx_factory)
        : board(board), pieces_root(pieces_root), gfx_factory(gfx_factory) {}

    // Direct translation of PieceFactory.create_piece from Python
    PiecePtr create_piece(const std::string &type_name,
                          const std::pair<int, int> &cell)
    {

        fs::path piece_dir = fs::path(pieces_root) / type_name;
        auto idle_state = build_state_machine(piece_dir);
        if (!idle_state)
        {
            throw std::runtime_error("Failed to build state machine for piece type: " + type_name);
        }

        // Mimic Python id format: <type>_(r,c)
        std::string id = type_name + "_(" + std::to_string(cell.first) + "," + std::to_string(cell.second) + ")";

        auto piece = std::make_shared<Piece>(id, idle_state);
        idle_state->reset(Command{0, id, "idle", {cell}});
        return piece;
    }

private:
    // ────────────────────────────────────────────────────────────────────
    using GlobalTrans = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

    static GlobalTrans load_master_csv(const fs::path &states_root)
    {
        GlobalTrans out;
        fs::path csv_path = states_root / "transitions.csv";
        if (!fs::exists(csv_path))
            return out;
        std::ifstream in(csv_path);
        if (!in)
            return out;
        std::string line;
        // expecting CSV header: from_state,event,to_state
        std::getline(in, line); // skip header
        while (std::getline(in, line))
        {
            std::stringstream ss(line);
            std::string frm, ev, nxt;
            if (std::getline(ss, frm, ',') && std::getline(ss, ev, ',') && std::getline(ss, nxt, ','))
            {
                out[frm][ev] = nxt;
            }
        }
        return out;
    }

    std::shared_ptr<State> build_state_machine(const fs::path &piece_dir)
    {
        fs::path states_root = piece_dir / "states";
        if (!fs::exists(states_root) || !fs::is_directory(states_root))
        {
            throw std::runtime_error("Missing states directory: " + states_root.string());
        }
        
        GlobalTrans global_trans = load_master_csv(states_root);
        
        std::unordered_map<std::string, std::shared_ptr<State>> states;
        
        std::pair<int, int> board_size = {board.W_cells, board.H_cells};
        std::pair<int, int> cell_px = {board.cell_W_pix, board.cell_H_pix};
        
        // iterate over each subdirectory in states_root
        for (const auto &entry : fs::directory_iterator(states_root))
        {
            if (!entry.is_directory())
            continue;
            std::string name = entry.path().filename().string();
            fs::path cfg_path = entry.path() / "config.json";
            nlohmann::json cfg;
            if (fs::exists(cfg_path))
            {
                std::ifstream f(cfg_path);
                try
                {
                    f >> cfg;
                }
                catch (const std::exception &)
                {
                    // ignore invalid json – default cfg
                }
            }
            
            // Moves
            fs::path moves_path = entry.path() / "moves.txt";
            std::shared_ptr<Moves> moves_ptr;
            if (fs::exists(moves_path))
            {
                moves_ptr = std::make_shared<Moves>(moves_path.string(), board_size);
            }
            
            // Graphics
            nlohmann::json gfx_cfg = cfg.contains("graphics") ? cfg["graphics"] : nlohmann::json{};
            auto graphics = gfx_factory.load((entry.path() / "sprites").string(), gfx_cfg, cell_px);
            
            // Physics
            nlohmann::json phys_cfg = cfg.contains("physics") ? cfg["physics"] : nlohmann::json{};
            PhysicsFactory phys_factory(board);
            auto physics = phys_factory.create({0, 0}, name, phys_cfg);
            // Note: need_clear_path flag not implemented in C++ physics yet
            auto st = std::make_shared<State>(moves_ptr, graphics, physics);
            st->name = name;
            states[name] = st;
        }

        // apply global transitions overrides
        for (const auto &[frm, ev_map] : global_trans)
        {
            auto src_it = states.find(frm);
            if (src_it == states.end())
                continue;
            auto src = src_it->second;
            for (const auto &[ev, nxt] : ev_map)
            {
                auto dst_it = states.find(nxt);
                if (dst_it == states.end())
                    continue;
                src->set_transition(ev, dst_it->second);
            }
        }

        // ensure idle exists
        auto idle_it = states.find("idle");
        if (idle_it == states.end())
        {
            throw std::runtime_error("State machine missing 'idle' state in " + piece_dir.string());
        }
        return idle_it->second;
    }

private:
    Board &board;
    std::string pieces_root;
    const GraphicsFactory &gfx_factory;
};