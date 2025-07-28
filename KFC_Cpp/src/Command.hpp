#pragma once

#include <string>
#include <vector>
#include <ostream>
#include <utility>  // בשביל std::pair


struct Command {
    int timestamp;                 // ms since game start
    std::string piece_id;          // identifier of the piece (may be empty)
    std::string type;              // e.g., "move", "jump", "done"...
    std::vector<std::pair<int,int>> params;  // payload – board cells etc.

    Command(int ts, std::string pid, std::string t, std::vector<std::pair<int,int>> p)
        : timestamp(ts), piece_id(pid), type(t), params(p) {}

    friend std::ostream& operator<<(std::ostream& os, const Command& cmd) {
        os << "Command(timestamp=" << cmd.timestamp;
        os << ", piece_id=" << cmd.piece_id ;
        os << ", type=" << cmd.type ;
        os << ", params_size=" << cmd.params.size();

        for (const auto& param : cmd.params) {
            os << ", {" << param.first << ":" << param.second << "}";
        }
        
        os << ")";
        return os;
    }
}; 