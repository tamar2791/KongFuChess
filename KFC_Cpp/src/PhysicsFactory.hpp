#pragma once

#include "Physics.hpp"
#include <memory>
#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// PhysicsFactory – helper that instantiates physics subclasses from a string
// identifier and optional JSON-like configuration, mirroring the Python
// PhysicsFactory used in tests.
// ---------------------------------------------------------------------------
class PhysicsFactory
{
public:
    explicit PhysicsFactory(const Board &board) : board(board) {}

    std::shared_ptr<BasePhysics> create(const std::pair<int, int> & /*start_cell*/,
                                        const std::string &name,
                                        const nlohmann::json &cfg) const
    {
        nlohmann::json physics_cfg;
        if (!cfg.is_null() && cfg.contains("physics"))
        {
            physics_cfg = cfg["physics"];
        }
        else
        {
            physics_cfg = nlohmann::json{}; // JSON ריק
        }
        std::string key = to_lower(name);
        if (key == "idle")
        {
            return std::make_shared<IdlePhysics>(board);
        }
        if (key == "move")
        {
            double speed = physics_cfg.value("speed_m_per_sec", 1.0);
            return std::make_shared<MovePhysics>(board, speed);
        }
        if (key == "jump")
        {
            double duration_ms = physics_cfg.value("duration_ms", 50.0);
            return std::make_shared<JumpPhysics>(board, duration_ms / 1000.0);
        }
        if (key.find("rest") != std::string::npos)
        {
            double duration_ms = physics_cfg.value("duration_ms", 500.0);
            return std::make_shared<RestPhysics>(board, duration_ms / 1000.0);
        }
        // fallback idle
        return std::make_shared<IdlePhysics>(board);
    }

private:
    const Board &board;
    static std::string to_lower(std::string s)
    {
        for (char &c : s)
            c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        return s;
    }
};