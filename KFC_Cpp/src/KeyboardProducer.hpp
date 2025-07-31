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
#include <utility> // בשביל std::pair
#include <opencv2/opencv.hpp>
#include "Game.hpp"
#include "Piece.hpp"

typedef std::shared_ptr<Piece> PiecePtr;

class KeyboardProducer
{
private:
    std::vector<Command> &user_input_queue; // reference לתור הפקודות של המשחק
    std::mutex &input_mutex;                // reference למutex של המשחק
    KeyboardProcessor &processor;           // reference לprocessor
    int player;                             // מספר השחקן (1 או 2)
    std::string selected_id;                // ID של הכלי הנבחר
    std::pair<int, int> selected_cell;      // התא שנבחר
    std::thread worker_thread;              // thread עבור keyboard polling
    std::atomic<bool> running;              // האם הthread רץ

    // משתנים עבור דמוי keyboard events (לטסטים)
    std::queue<std::string> simulated_keys;
    std::mutex sim_mutex;

    // שיתוף בין ה-producers
    std::shared_ptr<KeyboardProducer> other_player_producer;

    // reference לרשימת הכלים
    std::vector<PiecePtr> *pieces_ref;

public:
    // Constructor שמתאים למה שמשתמש במחלקת Game
    KeyboardProducer(std::vector<Command> &queue, std::mutex &mutex,
                     KeyboardProcessor &proc, int player_num)
        : user_input_queue(queue), input_mutex(mutex), processor(proc),
          player(player_num), selected_id(""), selected_cell{-1, -1}, running(false), pieces_ref(nullptr) {}

    ~KeyboardProducer()
    {
        stop();
    }

    void start()
    {
        if (!running.load())
        {
            running = true;
            worker_thread = std::thread(&KeyboardProducer::run, this);
        }
    }

    void stop()
    {
        running = false;
        if (worker_thread.joinable())
        {
            worker_thread.join();
        }
    }

    // פונקציה לקבלת מיקום הסמן האפקטיבי
    std::pair<int, int> get_effective_cursor()
    {
        // כל השחקנים משתמשים בסמן רגיל
        return processor.get_cursor();
    }

    // פונקציה ציבורית לטיפול באירוע מקלדת - תוכל לקרוא לה מבחוץ
    void handle_key_event(const std::string &key)
    {
        std::string action = processor.process_key(key);

        // רק select ו-jump מעניינים אותנו כאן
        if (action != "select" && action != "jump")
        {
            return;
        }

        auto cell = get_effective_cursor();

        if (action == "select")
        {
            handle_select_action(cell);
        }
        else if (action == "jump")
        {
            handle_jump_action(cell);
        }
    }

    // פונקציה לדמוי מקשים (לטסטים)
    void simulate_key(const std::string &key)
    {
        std::lock_guard<std::mutex> lock(sim_mutex);
        simulated_keys.push(key);
    }

    // פונקציה להגדרת השחקן השני
    void set_other_player(std::shared_ptr<KeyboardProducer> other)
    {
        this->other_player_producer = other;
    }

    // פונקציה להגדרת reference לרשימת הכלים
    void set_pieces_reference(std::vector<PiecePtr> *pieces)
    {
        this->pieces_ref = pieces;
    }

    // פונקציה ציבורית לטיפול במקשים
    void handle_key_event_internal(const std::string &key)
    {
        std::cout << "[DEBUG] Player " << player << " processing key: " << key << std::endl;
        std::string action = processor.process_key(key);
        std::cout << "[DEBUG] Player " << player << " action: " << action << std::endl;

        auto cell = get_effective_cursor();

        if (action != "select" && action != "jump")
        {
            if (action == "up" || action == "down" || action == "left" || action == "right")
            {
                std::cout << "[DEBUG] Player " << player << " cursor moved to: (" << cell.first << "," << cell.second << ")" << std::endl;
            }
            return;
        }
        std::cout << "[DEBUG] Player " << player << " cursor at: (" << cell.first << "," << cell.second << ")" << std::endl;

        if (action == "select")
        {
            handle_select_action(cell);
        }
        else if (action == "jump")
        {
            handle_jump_action(cell);
        }
    }

    // פונקציה לטיפול במקשים מ-OpenCV
    void handle_opencv_key(int key)
    {
        std::string key_str = convert_opencv_key_to_string(key);
        if (!key_str.empty())
        {
            std::cout << "[DEBUG] OpenCV key converted: " << key_str << std::endl;
            distribute_key_to_players(key_str);
        }
    }

private:
    void run()
    {
        std::cout << "[DEBUG] KeyboardProducer Player " << player << " started!" << std::endl;
        while (running.load())
        {
            // בדוק אם יש מקשים מדומים
            process_simulated_keys();

            // המקשים מגיעים מ-OpenCV דרך handle_opencv_key

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::cout << "[DEBUG] KeyboardProducer Player " << player << " stopped!" << std::endl;
    }

    void process_simulated_keys()
    {
        std::lock_guard<std::mutex> lock(sim_mutex);
        while (!simulated_keys.empty())
        {
            std::string key = simulated_keys.front();
            simulated_keys.pop();
            // עבד את המקש ללא נעילה (כי כבר יש לנו נעילה)
            handle_key_event_internal(key);
        }
    }

    PiecePtr find_piece_at(const std::pair<int, int> &cell)
    {
        if (!pieces_ref)
            return nullptr;

        // הקורסור נותן (row,col) והכלים עכשיו גם שמורים ב-(row,col)
        std::pair<int, int> piece_coords = cell;

        std::cout << "[DEBUG] Looking for piece at cursor (" << cell.first << "," << cell.second << ") -> piece coords (" << piece_coords.first << "," << piece_coords.second << ")" << std::endl;

        for (const auto &piece : *pieces_ref)
        {
            auto current = piece->current_cell();
            std::cout << "[DEBUG] Checking piece " << piece->id << " at (" << current.first << "," << current.second << ")" << std::endl;
            if (current == piece_coords)
            {
                if (is_piece_owned_by_player(piece, player))
                {
                    return piece;
                }
            }
        }
        return nullptr;
    }

    bool is_piece_owned_by_player(const PiecePtr &piece, int player_num)
    {
        if (piece->id.length() < 2)
            return false;

        char color = piece->id[1];
        if (player_num == 1 && color == 'W')
            return true;
        if (player_num == 2 && color == 'B')
            return true;

        return false;
    }

    void handle_select_action(const std::pair<int, int> &cell)
    {
        if (selected_id.empty())
        {
            // לחיצה ראשונה - נסה לבחור כלי
            std::cout << "[DEBUG] Looking for piece to select at (" << cell.first << "," << cell.second << ")" << std::endl;

            // הדפס את כל הכלים בתא הזה
            if (pieces_ref)
            {
                std::pair<int, int> piece_coords = {cell.second, cell.first};
                for (const auto &piece : *pieces_ref)
                {
                    if (piece->current_cell() == piece_coords)
                    {
                        std::cout << "[DEBUG] Found piece " << piece->id << " at this cell, belongs to player: " << (is_piece_owned_by_player(piece, player) ? "YES" : "NO") << std::endl;
                    }
                }
            }

            PiecePtr piece = find_piece_at(cell);
            if (!piece)
            {
                std::cout << "[WARN] Player " << player << " - No piece at ("
                          << cell.first << "," << cell.second << ")" << std::endl;
                return;
            }

            selected_id = piece->id;
            selected_cell = cell;

            std::cout << "[KEY] Player " << player << " selected " << piece->id << " at ("
                      << cell.first << "," << cell.second << ")" << std::endl;
        }
        else if (cell == selected_cell)
        {
            // לחיצה על אותו תא - בטל בחירה
            std::cout << "[KEY] Player " << player << " deselected piece" << std::endl;
            selected_id = "";
            selected_cell = {-1, -1};
        }
        else
        {
            // לחיצה על תא אחר - צור פקודת מהלך
            std::cout << "[KEY] Player " << player << " moving " << selected_id << " from ("
                      << selected_cell.first << "," << selected_cell.second << ") to ("
                      << cell.first << "," << cell.second << ")" << std::endl;
            create_move_command(selected_cell, cell);

            selected_id = "";
            selected_cell = {-1, -1};
        }
    }

    void handle_jump_action(const std::pair<int, int> &cell)
    {
        if (selected_id.empty())
        {
            // אם אין כלי נבחר, בחר כלי במיקום הנוכחי ובצע קפיצה מיד
            PiecePtr piece = find_piece_at(cell);
            if (!piece)
            {
                std::cout << "[WARN] Player " << player << " - No piece to jump at ("
                          << cell.first << "," << cell.second << ")" << std::endl;
                return;
            }
            
            // בצע קפיצה מיד למיקום הנוכחי
            std::cout << "[KEY] Player " << player << " jumping " << piece->id << " at ("
                      << cell.first << "," << cell.second << ")" << std::endl;
            
            selected_id = piece->id;
            selected_cell = cell;
            create_jump_command(cell, cell); // קפיצה במקום
            
            selected_id = "";
            selected_cell = {-1, -1};
        }
        else
        {
            // יש כלי נבחר - בצע קפיצה למיקום הנוכחי
            std::cout << "[KEY] Player " << player << " jumping " << selected_id << " from ("
                      << selected_cell.first << "," << selected_cell.second << ") to ("
                      << cell.first << "," << cell.second << ")" << std::endl;
            create_jump_command(selected_cell, cell);

            selected_id = "";
            selected_cell = {-1, -1};
        }
    }

    void create_move_command(const std::pair<int, int> &from, const std::pair<int, int> &to)
    {
        // הכלי נמצא בקורדינטות אמיתיות - נשתמש בהן
        PiecePtr piece = nullptr;
        for (const auto &p : *pieces_ref)
        {
            if (p->id == selected_id)
            {
                piece = p;
                break;
            }
        }
        if (!piece)
            return;

        auto piece_pos = piece->current_cell();
        std::pair<int, int> from_coords = piece_pos;
        std::pair<int, int> to_coords = to;

        std::vector<std::pair<int, int>> move_params = {from_coords, to_coords};
        int current_time = get_current_time_ms();
        std::cout << "[DEBUG] Creating move command with timestamp: " << current_time << std::endl;
        Command cmd{
            current_time,
            selected_id,
            "move",
            move_params};

        {
            std::lock_guard<std::mutex> guard(input_mutex);
            user_input_queue.push_back(cmd);
        }

        std::cout << "[INFO] Player " << player << " queued: " << cmd << std::endl;
    }

    void create_jump_command(const std::pair<int, int> &from, const std::pair<int, int> &to)
    {
        std::vector<std::pair<int, int>> jump_params = {from, to};
        Command cmd{
            get_current_time_ms(),
            selected_id,
            "jump",
            jump_params};

        {
            std::lock_guard<std::mutex> guard(input_mutex);
            user_input_queue.push_back(cmd);
        }

        std::cout << "[INFO] Player " << player << " queued jump: " << cmd << std::endl;
    }

    std::string convert_opencv_key_to_string(int key)
    {
        // הדפס את הערך לדיבוג
        if (key != 255)
        {
            std::cout << "[DEBUG] Key pressed: " << key << std::endl;
        }

        switch (key)
        {
        case 27:
            return "esc";
        case 13:
            return "enter";
        case 32:
            return " ";
        case 43:
            return "+";
        case 45:
            return "-";
        // מקשי IJKL כבר מטופלים בחלק הרגיל של האותיות

        case 249:
            return "a"; // ראינו 249 בפלט
        case 227:
            return "s"; // ראינו 227 בפלט
        case 226:
            return "d"; // ראינו 226 בפלט
        case 39:
            return "w"; // ראינו 39 בפלט
        case 235:
            return "f"; // ראינו 235 בפלט
        // מקשי IJKL במצב עברית
        case 239: // י במקום I
            return "i";
        case 236: // כ במקום K  
            return "k";
        case 234: // ל במקום L
            return "l";
        case 231: // ג במקום J
            return "j";
        case 242: // ע במצב עברית (במקום G)
            return "g";
        // אותיות רגילות
        default:
            if (key >= 'a' && key <= 'z')
            {
                return std::string(1, (char)key);
            }
            if (key >= 'A' && key <= 'Z')
            {
                return std::string(1, (char)(key + 32)); // המרה לאות קטנה
            }

            // הדפס מקשים לא מוכרים
            if (key != 255 && key != -1) {
                std::cout << "[DEBUG] Unknown key code: " << key << std::endl;
            }
            return "";
        }
    }

    int get_current_time_ms()
    {
        static auto start_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto duration = now - start_time;
        return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }

    void distribute_key_to_players(const std::string &key)
    {
        // בדוק אילו מקשים שייכים לשחקן 1 (IJKL במקום חיצים)
        if (key == "i" || key == "k" || key == "j" || key == "l" ||
            key == "enter" || key == "+")
        {
            std::cout << "[DEBUG] Key '" << key << "' for Player 1" << std::endl;
            handle_key_event_internal(key);
        }
        // בדוק אילו מקשים שייכים לשחקן 2
        else if (key == "w" || key == "s" || key == "a" || key == "d" ||
                 key == "f" || key == "g")
        {
            std::cout << "[DEBUG] Key '" << key << "' for Player 2" << std::endl;
            if (other_player_producer)
            {
                other_player_producer->handle_key_event_internal(key);
            }
        }
        else
        {
            std::cout << "[DEBUG] Unknown key: " << key << std::endl;
        }
    }
};