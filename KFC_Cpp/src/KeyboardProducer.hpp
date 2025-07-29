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
#include <queue>
#include <utility>  // בשביל std::pair
#include <opencv2/opencv.hpp>
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
        while (running.load()) {
            // בדוק אם יש מקשים מדומים
            process_simulated_keys();
            
            // קלט מקלדת אמיתי באמצעות OpenCV
            int key = cv::waitKey(1) & 0xFF;
            if (key != 255) { // 255 = no key pressed
                std::string key_str = convert_opencv_key_to_string(key);
                if (!key_str.empty()) {
                    handle_key_event_internal(key_str);
                }
            }
            
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
        
        // הדפסת debug
        if (!action.empty()) {
            std::cout << "[DEBUG] Player " << player << " action: " << action << std::endl;
        }
        
        // טפל בכל הפעולות, לא רק select ו-jump
        if (action.empty()) {
            return;
        }
        
        auto cell = processor.get_cursor();
        
        if (action == "select") {
            handle_select_action(cell);
        } else if (action == "jump") {
            handle_jump_action(cell);
        }
        // תזוזות הסמן מטופלות ב-processor.process_key
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

    std::string convert_opencv_key_to_string(int key) {
        // הדפסת debug לראות אילו מקשים נלחצים
        if (key != 255) {
            std::cout << "[DEBUG] Key pressed: " << key << std::endl;
        }
        
        switch (key) {
            case 27: return "esc";
            case 13: return "enter";
            case 32: return " ";
            case 43: return "+";
            case 45: return "-";
            // חצי כיוון - קודים נכונים של OpenCV
            case 2490368: return "up";    // חץ עליון
            case 2621440: return "down";  // חץ תחתון
            case 2424832: return "left";  // חץ שמאל
            case 2555904: return "right"; // חץ ימין
            // אותיות רגילות
            default:
                if (key >= 'a' && key <= 'z') {
                    return std::string(1, (char)key);
                }
                if (key >= 'A' && key <= 'Z') {
                    return std::string(1, (char)(key + 32)); // המרה לאות קטנה
                }
                return "";
        }
    }
    
    int get_current_time_ms() {
        static auto start_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto duration = now - start_time;
        return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }
};