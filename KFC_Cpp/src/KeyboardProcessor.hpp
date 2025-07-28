#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include <utility>  // בשביל std::pair


class KeyboardProcessor {
public:
    KeyboardProcessor(int rows=0, int cols=0,
                      std::unordered_map<std::string,std::string> keymap={})
        : rows(rows), cols(cols), keymap(keymap) {
        cursor[0] = 0; cursor[1] = 0;
    }

    // Return action string when special key, otherwise returns action mapping (up/down etc) or nullopt
    std::string process_key(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = keymap.find(key);
        if(it == keymap.end()) return "";
        const std::string& action = it->second;
        if(action == "up") {
            if(cursor[0] > 0) cursor[0] -= 1;
        } else if(action == "down") {
            if(cursor[0] < rows-1) cursor[0] += 1;
        } else if(action == "left") {
            if(cursor[1] > 0) cursor[1] -= 1;
        } else if(action == "right") {
            if(cursor[1] < cols-1) cursor[1] += 1;
        }
        // Return the action (choose, jump or move directions) so tests can assert.
        return action;
    };

    std::pair<int,int> get_cursor() const {
        std::lock_guard<std::mutex> lock(mtx);
        return {cursor[0], cursor[1]};
    }

private:
    int rows;
    int cols;
    int cursor[2];
    std::unordered_map<std::string,std::string> keymap;
    mutable std::mutex mtx;
}; 