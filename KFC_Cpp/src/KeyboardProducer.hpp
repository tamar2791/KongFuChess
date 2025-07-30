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
#include <utility>
#include <opencv2/opencv.hpp>
#include "Game.hpp"
#include "Piece.hpp"

// אלטרנטיבה - שימוש בספריית conio.h (Windows) או termios.h (Linux)
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

typedef std::shared_ptr<Piece> PiecePtr;

class KeyboardProducer {
private:
    std::vector<Command>& user_input_queue;
    std::mutex& input_mutex;
    KeyboardProcessor& processor;
    int player;
    std::string selected_id;
    std::pair<int, int> selected_cell;
    std::thread worker_thread;
    std::atomic<bool> running;
    
    // משתנים עבור דמוי keyboard events
    std::queue<std::string> simulated_keys;
    std::mutex sim_mutex;
    
    // חלון OpenCV לקליטת מקשים
    std::string window_name;

public:
    KeyboardProducer(std::vector<Command>& queue, std::mutex& mutex, 
                    KeyboardProcessor& proc, int player_num)
        : user_input_queue(queue), input_mutex(mutex), processor(proc), 
          player(player_num), selected_id(""), selected_cell{-1, -1}, running(false) {
        
        window_name = "Player " + std::to_string(player) + " Input";
    }

    ~KeyboardProducer() {
        stop();
        cv::destroyWindow(window_name);
    }

    void start() {
        if (!running.load()) {
            running = true;
            
            // צור חלון OpenCV לקליטת מקשים
            cv::Mat dummy_img = cv::Mat::zeros(100, 300, CV_8UC3);
            cv::putText(dummy_img, "Player " + std::to_string(player) + " - Press keys here", 
                       cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255));
            cv::imshow(window_name, dummy_img);
            
            worker_thread = std::thread(&KeyboardProducer::run, this);
        }
    }

    void stop() {
        running = false;
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }

    void handle_key_event(const std::string& key) {
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

    void simulate_key(const std::string& key) {
        std::lock_guard<std::mutex> lock(sim_mutex);
        simulated_keys.push(key);
    }

private:
    void run() {
        std::cout << "[INFO] KeyboardProducer started for player " << player << std::endl;
        std::cout << "[INFO] Focus on window '" << window_name << "' and press keys" << std::endl;
        
        while (running.load()) {
            process_simulated_keys();
            
            // שיטה 1: OpenCV (דורש חלון פתוח)
            process_opencv_input();
            
            // שיטה 2: קלט ישירות מהטרמינל (חלופה)
            // process_console_input();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // הגדל מעט את ה-delay
        }
    }

    void process_opencv_input() {
        // וודא שהחלון עדיין קיים
        if (cv::getWindowProperty(window_name, cv::WND_PROP_VISIBLE) < 1) {
            return;
        }
        
        int key = cv::waitKey(1) & 0xFF;
        if (key != 255 && key != -1) { // בדוק גם -1
            std::string key_str = convert_opencv_key_to_string(key);
            std::cout << "[DEBUG] Raw key: " << key << " -> converted: '" << key_str << "'" << std::endl;
            
            if (!key_str.empty()) {
                handle_key_event_internal(key_str);
            }
        }
    }

    // שיטה חלופית לקלט ללא OpenCV
    void process_console_input() {
#ifdef _WIN32
        if (_kbhit()) {
            int key = _getch();
            std::string key_str = convert_console_key_to_string(key);
            if (!key_str.empty()) {
                handle_key_event_internal(key_str);
            }
        }
#else
        // Linux implementation
        int key = get_char_non_blocking();
        if (key != -1) {
            std::string key_str = convert_console_key_to_string(key);
            if (!key_str.empty()) {
                handle_key_event_internal(key_str);
            }
        }
#endif
    }

#ifndef _WIN32
    int get_char_non_blocking() {
        struct termios oldt, newt;
        int ch;
        int oldf;

        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

        ch = getchar();

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);

        return ch;
    }
#endif

    void process_simulated_keys() {
        std::lock_guard<std::mutex> lock(sim_mutex);
        while (!simulated_keys.empty()) {
            std::string key = simulated_keys.front();
            simulated_keys.pop();
            handle_key_event_internal(key);
        }
    }

    void handle_key_event_internal(const std::string& key) {
        std::cout << "[INPUT] Player " << player << " pressed: '" << key << "'" << std::endl;
        
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

    // גרסה מתוקנת של המרת מקשים
    std::string convert_opencv_key_to_string(int key) {
        std::cout << "[DEBUG] Converting OpenCV key: " << key << std::endl;
        
        switch (key) {
            case 27: return "esc";
            case 13: case 10: return "enter";
            case 32: return " ";
            case 43: return "+";
            case 45: return "-";
            
            // מקשי חצים - ערכים נכונים עבור OpenCV
            case 81: case 2490368: return "up";    // חץ עליון
            case 83: case 2621440: return "down";  // חץ תחתון  
            case 82: case 2424832: return "left";  // חץ שמאל
            case 84: case 2555904: return "right"; // חץ ימין
            
            // מקשי פונקציה אפשריים
            case 8: return "backspace";
            case 9: return "tab";
            
            default:
                // אותיות ומספרים רגילים
                if (key >= 'a' && key <= 'z') {
                    return std::string(1, (char)key);
                }
                if (key >= 'A' && key <= 'Z') {
                    return std::string(1, (char)(key + 32)); // המרה לאות קטנה
                }
                if (key >= '0' && key <= '9') {
                    return std::string(1, (char)key);
                }
                
                std::cout << "[WARNING] Unknown key: " << key << std::endl;
                return "";
        }
    }

    // המרה עבור קלט קונסולה
    std::string convert_console_key_to_string(int key) {
        switch (key) {
            case 27: return "esc";
            case 13: case 10: return "enter";
            case 32: return " ";
            case 8: return "backspace";
            case 9: return "tab";
            
            default:
                if (key >= 'a' && key <= 'z') {
                    return std::string(1, (char)key);
                }
                if (key >= 'A' && key <= 'Z') {
                    return std::string(1, (char)(key + 32));
                }
                if (key >= '0' && key <= '9') {
                    return std::string(1, (char)key);
                }
                return "";
        }
    }

    // השאר מהפונקציות כמו שהן...
    PiecePtr find_piece_at(const std::pair<int, int>& cell, const std::vector<PiecePtr>& pieces) {
        for (const auto& piece : pieces) {
            if (piece->current_cell() == cell) {
                if (is_piece_owned_by_player(piece, player)) {
                    return piece;
                }
            }
        }
        return nullptr;
    }

    bool is_piece_owned_by_player(const PiecePtr& piece, int player_num) {
        if (piece->id.length() < 2) return false;
        
        char color = piece->id[1];
        if (player_num == 1 && color == 'W') return true;
        if (player_num == 2 && color == 'B') return true;
        
        return false;
    }

    void handle_select_action(const std::pair<int, int>& cell) {
        if (selected_id.empty()) {
            std::cout << "[KEY] Player " << player << " trying to select at (" 
                     << cell.first << "," << cell.second << ")" << std::endl;
            
            selected_id = "dummy_piece_" + std::to_string(player);
            selected_cell = cell;
            
            std::cout << "[KEY] Player " << player << " selected piece at (" 
                     << cell.first << "," << cell.second << ")" << std::endl;
        }
        else if (cell == selected_cell) {
            std::cout << "[KEY] Player " << player << " deselected piece" << std::endl;
            selected_id = "";
            selected_cell = {-1, -1};
        }
        else {
            create_move_command(selected_cell, cell);
            selected_id = "";
            selected_cell = {-1, -1};
        }
    }

    void handle_jump_action(const std::pair<int, int>& cell) {
        if (!selected_id.empty()) {
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
        static auto start_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto duration = now - start_time;
        return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }
};