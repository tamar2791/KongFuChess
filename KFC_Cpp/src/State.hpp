#pragma once
#include "Common.hpp" // עבור PairHash

#include "Moves.hpp"
#include "Graphics.hpp"
#include "Physics.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <cctype>
#include <utility>
#include <vector>
#include <iostream>

class State : public std::enable_shared_from_this<State> {
public:
    State(std::shared_ptr<Moves> moves,
          std::shared_ptr<Graphics> graphics,
          std::shared_ptr<BasePhysics> physics)
        : moves(moves), graphics(graphics), physics(physics) {}

    std::shared_ptr<Moves>    moves;
    std::shared_ptr<Graphics> graphics;
    std::shared_ptr<BasePhysics> physics;

    // Keep strong references so target states are not destroyed while only reachable via this map
    std::unordered_map<std::string, std::shared_ptr<State>> transitions;
    std::string name;

    void set_transition(const std::string& event, const std::shared_ptr<State>& target) { transitions[event] = target; }

    void reset(const Command& cmd) {
        graphics->reset(cmd);
        physics->reset(cmd);
    }
    std::shared_ptr<State> on_command(const Command& cmd,Cell2Pieces& c) {
        std::string key = cmd.type;
        for(auto& ch : key) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        
        auto it = transitions.find(key);
        if(it != transitions.end()) {
            auto next = it->second;
            if(next && key == "move" && cmd.params.size() >= 2) {
                // Validate move using Moves class
                if(!moves) {
                    next->reset(cmd);
                    return next;
                }
                std::unordered_set<std::pair<int,int>, PairHash> occupied_cells;
                for(const auto& kv : c) {
                    if(!kv.second.empty()) {
                        occupied_cells.insert(kv.first);
                    }
                }
                if(moves->is_valid(cmd.params[0], cmd.params[1], occupied_cells, physics->is_need_clear_path())) {
                    next->reset(cmd);
                    return next;
                }
                // Invalid move - stay in current state
                return shared_from_this();
            } else if(next) {
                next->reset(cmd);
                return next;
            }
        }
        return shared_from_this();
    }

    std::shared_ptr<State> update(int now_ms, Cell2Pieces& c) {
        auto internal = physics->update(now_ms);
        if(internal) {
            return on_command(*internal,c);
        }
        graphics->update(now_ms); // keep graphics in sync when no state change
        return shared_from_this();
    }

    bool can_be_captured() const { return physics->can_be_captured(); }
    bool can_capture()    const { return physics->can_capture(); }
}; 