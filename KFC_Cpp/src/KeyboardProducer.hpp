#pragma once

#include "KeyboardProcessor.hpp"
#include "Command.hpp"
#include <thread>
#include <vector>
#include <memory>
#include <iostream>
#include <functional>
#include <atomic>
#include <chrono>
#include <mutex>
#include <utility>  // בשביל std::pair
#include "Game.hpp"
#include "Piece.hpp"

typedef std::shared_ptr<Piece> PiecePtr;

class KeyboardProducer {
private:
    std::vector<Command>& user_input_queue;  // reference לתור הפקודות של המשחק
    std::mutex& input_mutex;                 // reference למutex של המשחק
    KeyboardProcessor& processor;            // reference לprocessor
    int player;                             // מספר השחקן (1 או 2)
    std::string selected_id;                // ID של הכלי הנבחר
    std::pair<int, int> selected_cell;      // התא שנבחר
    std::thread worker_thread;              // thread עבור keyboard polling
    std::atomic<bool> running;              // האם הthread רץ
    
    // משתנים עבור דמוי keyboard events (לטסטים)
    std::queue<std::string> simulated_keys;
    std::mutex sim_mutex;

public:
    // Constructor שמתאים למה שמשתמש במחלקת Game
    KeyboardProducer(std::vector<Command>& queue, std::mutex& mutex, 
                    KeyboardProcessor& proc, int player_num)
        : user_input_queue(queue), input_mutex(mutex), processor(proc), 
          player(player_num), selected_id(""), selected_cell{-1, -1}, running(false) {}

    ~KeyboardProducer() {
        stop();
    }

    void start() {
        if (!running.load()) {
            running = true;
            worker_thread = std::thread(&KeyboardProducer::run, this);
        }
    }

    void stop() {
        running = false;
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }

    // פונקציה ציבורית לטיפול באירוע מקלדת - תוכל לקרוא לה מבחוץ
    void handle_key_event(const std::string& key) {
        std::string action = processor.process_key(key);
        
        // רק select ו-jump מעניינים אותנו כאן
        if (action != "select" && action != "jump") {
            return;
        }
        
        auto cell = processor.get_cursor();
        
        if (action == "select") {
            handle_select_action(cell);
        } else if (action == "jump") {
            handle_jump_action(cell);
        }
    }

    // פונקציה לדמוי מקשים (לטסטים)
    void simulate_key(const std::string& key) {
        std::lock_guard<std::mutex> lock(sim_mutex);
        simulated_keys.push(key);
    }

private:
    void run() {
        // זהו thread שמדמה keyboard polling
        // במציאות כאן יהיה integration עם ספריית UI או system hooks
        while (running.load()) {
            // בדוק אם יש מקשים מדומים
            process_simulated_keys();
            
            // במציאות כאן תוסיף:
            // - OpenCV waitKey() polling (הכי פשוט)
            // - SFML event polling (יותר מתקדם)
            // - Win32 keyboard hooks
            // - Linux input device polling
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void process_simulated_keys() {
        std::lock_guard<std::mutex> lock(sim_mutex);
        while (!simulated_keys.empty()) {
            std::string key = simulated_keys.front();
            simulated_keys.pop();
            // עבד את המקש ללא נעילה (כי כבר יש לנו נעילה)
            handle_key_event_internal(key);
        }
    }

    void handle_key_event_internal(const std::string& key) {
        std::string action = processor.process_key(key);
        
        if (action != "select" && action != "jump") {
            return;
        }
        
        auto cell = processor.get_cursor();
        
        if (action == "select") {
            handle_select_action(cell);
        } else if (action == "jump") {
            handle_jump_action(cell);
        }
    }

    PiecePtr find_piece_at(const std::pair<int, int>& cell, const std::vector<PiecePtr>& pieces) {
        for (const auto& piece : pieces) {
            if (piece->current_cell() == cell) {
                // בדוק שזה כלי של השחקן הנכון
                if (is_piece_owned_by_player(piece, player)) {
                    return piece;
                }
            }
        }
        return nullptr;
    }

    bool is_piece_owned_by_player(const PiecePtr& piece, int player_num) {
        // בהתבסס על הקוד שלך, נראה שהמזהה מתחיל ב-K, ואז W או B
        // W = White (שחקן 1), B = Black (שחקן 2)
        if (piece->id.length() < 2) return false;
        
        char color = piece->id[1];  // התו השני במזהה
        if (player_num == 1 && color == 'W') return true;
        if (player_num == 2 && color == 'B') return true;
        
        return false;
    }

    void handle_select_action(const std::pair<int, int>& cell) {
        if (selected_id.empty()) {
            // לחיצה ראשונה - נסה לבחור כלי
            // כאן אנחנו צריכים גישה לרשימת הכלים, אבל אין לנו reference למשחק
            // נניח שנעביר את זה דרך callback או נשמור reference למשחק
            
            std::cout << "[KEY] Player " << player << " trying to select at (" 
                     << cell.first << "," << cell.second << ")" << std::endl;
            
            // כרגע נדמה בחירה - במציאות צריך לבדוק אם יש כלי בתא
            selected_id = "dummy_piece_" + std::to_string(player);
            selected_cell = cell;
            
          std::cout << "[KEY] Player " << player << " selected piece at (" 
                     << cell.first << "," << cell.second << ")" << std::endl;
        }
        else if (cell == selected_cell) {
            // לחיצה על אותו תא - בטל בחירה
            std::cout << "[KEY] Player " << player << " deselected piece" << std::endl;
            selected_id = "";
            selected_cell = {-1, -1};
        }
        else {
            // לחיצה על תא אחר - צור פקודת מהלך
            create_move_command(selected_cell, cell);
            
            selected_id = "";
            selected_cell = {-1, -1};
        }
    }

    void handle_jump_action(const std::pair<int, int>& cell) {
        if (!selected_id.empty()) {
            // יצירת פקודת קפיצה
            create_jump_command(selected_cell, cell);
            
            selected_id = "";
            selected_cell = {-1, -1};
        }
    }

    void create_move_command(const std::pair<int, int>& from, const std::pair<int, int>& to) {
        std::vector<std::pair<int,int>> move_params = {from, to};
        Command cmd{
            get_current_time_ms(),
            selected_id,
            "move",
            move_params
        };
        
        // הוסף לתור בצורה thread-safe
        {
            std::lock_guard<std::mutex> guard(input_mutex);
            user_input_queue.push_back(cmd);
        }
        
        std::cout << "[INFO] Player " << player << " queued: " << cmd << std::endl;
    }

    void create_jump_command(const std::pair<int, int>& from, const std::pair<int, int>& to) {
        std::vector<std::pair<int,int>> jump_params = {from, to};
        Command cmd{
            get_current_time_ms(),
            selected_id,
            "jump",
            jump_params
        };
        
        {
            std::lock_guard<std::mutex> guard(input_mutex);
            user_input_queue.push_back(cmd);
        }
        
        std::cout << "[INFO] Player " << player << " queued jump: " << cmd << std::endl;
    }

    int get_current_time_ms() {
        // זמן מהתחלת התוכנית - מתאים למה שמשתמש במחלקת Game
        static auto start_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto duration = now - start_time;
        return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }
};

// דוגמה לשימוש (בקובץ נפרד או בtests):
/*
void example_usage() {
    // יצירת keymap לשחקן 1
    std::unordered_map<std::string, std::string> p1_keymap = {
        {"up", "up"}, {"down", "down"}, {"left", "left"}, {"right", "right"},
        {"enter", "select"}, {"+", "jump"}
    };
    
    // יצירת processor
    KeyboardProcessor processor(8, 8, p1_keymap);
    
    // יצירת תור פקודות ומutex
    std::vector<Command> command_queue;
    std::mutex queue_mutex;
    
    // יצירת producer
    KeyboardProducer producer(command_queue, queue_mutex, processor, 1);
    
    producer.start();
    
    // דמוי אירועי מקלדת
    producer.simulate_key("enter");  // select
    producer.simulate_key("down");   // move cursor
    producer.simulate_key("enter");  // select again (move)
    
    // המתן קצת שהthread יעבד
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // קריאת פקודות מהתור
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        while (!command_queue.empty()) {
            Command cmd = command_queue.back();
            command_queue.pop_back();
            std::cout << "Processing: " << cmd << std::endl;
        }
    }
    
    producer.stop();
}
*/