#pragma once
#include "Common.hpp"

#include <opencv2/opencv.hpp>
#include "Board.hpp"
#include "PieceFactory.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
#include <unordered_set>
#include "img/Img.hpp"
#include "img/ImgFactory.hpp"
#include <fstream>
#include <sstream>
#include "GraphicsFactory.hpp"
#include <chrono>
#include <thread>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <atomic>
#include <mutex>
#include "KeyboardProcessor.hpp"
#include "KeyboardProducer.hpp"
#include <utility> // בשביל std::pair

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "Filesystem support not found"
#endif

class InvalidBoard : public std::runtime_error
{
public:
    explicit InvalidBoard(const std::string &msg) : std::runtime_error(msg) {}
};

struct PlayerData
{
    int player;
    std::shared_ptr<KeyboardProcessor> kp;
    std::shared_ptr<std::pair<int, int>> last_cursor;

    PlayerData(int p, std::shared_ptr<KeyboardProcessor> k, std::pair<int, int> *l)
        : player(p), kp(k), last_cursor(l) {}
};

class Game
{
public:
    Game(std::vector<PiecePtr> pcs, Board board);
    std::atomic<bool> running{true};
    std::mutex input_mutex;
    // --- main public API ---
    int game_time_ms() const;
    Board clone_board() const;

    // Mirror Python run() behaviour
    void run(int num_iterations = -1, bool is_with_graphics = true);

    std::vector<PiecePtr> pieces;
    Board board;
    std::shared_ptr<KeyboardProcessor> kp1;
    std::shared_ptr<KeyboardProcessor> kp2;

    std::shared_ptr<KeyboardProducer> kb_prod_1;
    std::shared_ptr<KeyboardProducer> kb_prod_2;

    std::pair<int, int> last_cursor1 = {-1, -1};
    std::pair<int, int> last_cursor2 = {-1, -1};
    // helper for tests to inject commands
    void enqueue_command(const Command &cmd);

private:
    // --- helpers mirroring Python implementation ---
    void start_user_input_thread(); // no-op stub for now
    void run_game_loop(int num_iterations, bool is_with_graphics);
    void update_cell2piece_map();
    void process_input(const Command &cmd);
    void resolve_collisions();
    void announce_win() const;

    void validate();
    bool is_win() const;
    void _draw();
    void _show() const;

    std::unordered_map<std::string, PiecePtr> piece_by_id;
    // Map from board cell to list of occupying pieces
    std::unordered_map<std::pair<int, int>, std::vector<PiecePtr>, PairHash> pos;
    std::vector<Command> user_input_queue;

    std::chrono::steady_clock::time_point start_tp;
};

// ---------------- Implementation inline --------------------
inline Game::Game(std::vector<PiecePtr> pcs, Board board)
    : pieces(pcs), board(board)
{
    validate();
    for (const auto &p : pieces)
        piece_by_id[p->id] = p;
    start_tp = std::chrono::steady_clock::now();
}

inline int Game::game_time_ms() const
{
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - start_tp)
                                .count());
}

inline Board Game::clone_board() const
{
    return board.clone();
}

inline void Game::run(int num_iterations, bool is_with_graphics)
{
    start_user_input_thread();
    int start_ms = game_time_ms();
    for (auto &p : pieces)
        p->reset(start_ms);

    run_game_loop(num_iterations, is_with_graphics);

    announce_win();
    int x = 0;
    //std::cin >> x;
}

inline void Game::start_user_input_thread()
{
    std::unordered_map<std::string, std::string> p1_map = {
        {"up", "up"}, {"down", "down"}, {"left", "left"}, {"right", "right"}, {"enter", "select"}, {"+", "jump"}};

    // player 2 keymap
    std::unordered_map<std::string, std::string> p2_map = {
        {"w", "up"}, {"s", "down"}, {"a", "left"}, {"d", "right"}, {"f", "select"}, {"g", "jump"}};
    kp1 = std::make_shared<KeyboardProcessor>(board.H_cells, board.W_cells, p1_map);
    kp2 = std::make_shared<KeyboardProcessor>(board.H_cells, board.W_cells, p2_map);

    // start the keyboard producers (with player number)
    kb_prod_1 = std::make_shared<KeyboardProducer>(
        user_input_queue, input_mutex, *kp1, 1);
    kb_prod_2 = std::make_shared<KeyboardProducer>(
        user_input_queue, input_mutex, *kp2, 2);

    kb_prod_1->start(); // או אפשר להריץ ב־ctor אם זה אוטומטי
    kb_prod_2->start();
}

inline void Game::run_game_loop(int num_iterations, bool is_with_graphics)
{
    int it_counter = 0;
    while (!is_win())
    {
        int now = game_time_ms();
        for (auto &p : pieces)
            p->update(now,pos);

        update_cell2piece_map();

        while (!user_input_queue.empty())
        {
            auto cmd = user_input_queue.back();
            user_input_queue.pop_back();
            process_input(cmd);
        }

        if (is_with_graphics)
        {
            board.show();
        }

        resolve_collisions();

        if (num_iterations >= 0)
        {
            ++it_counter;
            if (it_counter >= num_iterations)
                return;
        }
        /* idle to mimic frame pacing */
        //std::cout << "DEBUG" << std::endl;
    }
    if (is_with_graphics)
    {
        board.show();
    }
}

inline void Game::update_cell2piece_map()
{
    pos.clear();
    for (const auto &p : pieces)
    {
        pos[p->current_cell()].push_back(p);
    }
}

inline void Game::process_input(const Command &cmd)
{
    auto it = piece_by_id.find(cmd.piece_id);
    if (it == piece_by_id.end())
        return;
    it->second->on_command(cmd, pos);
}

inline void Game::resolve_collisions()
{
    update_cell2piece_map();
    for (const auto &kv : pos)
    {
        const auto &plist = kv.second;
        if (plist.size() < 2)
            continue;
        auto winner = *std::max_element(plist.begin(), plist.end(),
                                        [](const PiecePtr &a, const PiecePtr &b)
                                        {
                                            return a->state->physics->get_start_ms() < b->state->physics->get_start_ms();
                                        });
        for (const auto &p : plist)
        {
            if (p == winner)
                continue;
            if (p->state->can_be_captured())
            {
                pieces.erase(std::remove(pieces.begin(), pieces.end(), p), pieces.end());
            }
        }
    }
}

inline void Game::announce_win() const
{
    bool black_alive = std::any_of(pieces.begin(), pieces.end(), [](const PiecePtr &p)
                                   { return p->id.rfind("KB", 0) == 0; });
}

inline bool Game::is_win() const
{
    int kings = 0;
    for (const auto &p : pieces)
    {
        if (p->id.rfind("KW", 0) == 0 || p->id.rfind("KB", 0) == 0)
            ++kings;
    }
    return kings < 2;
}

inline void Game::validate()
{
    bool has_KW = false, has_KB = false;
    std::unordered_set<std::pair<int, int>, PairHash> seen;
    for (const auto &p : pieces)
    {
        if (p->id.rfind("KW", 0) == 0)
            has_KW = true;
        if (p->id.rfind("KB", 0) == 0)
            has_KB = true;
        auto cell = p->current_cell();
        if (!seen.insert(cell).second)
            throw InvalidBoard("Duplicate cell");
    }
    if (!has_KW || !has_KB)
        throw InvalidBoard("Missing kings");
}

inline void Game::enqueue_command(const Command &cmd)
{
    user_input_queue.push_back(cmd);
}

// ---------------------------------------------------------------------------
// Helper to read board.csv and create a full game
// ---------------------------------------------------------------------------
inline Game create_game(const std::string &pieces_root,
                        const ImgFactoryPtr &img_factory)
{
    //std::cout << "=== DEBUG: Creating game ===" << std::endl;
    //std::cout << "Pieces root: " << pieces_root << std::endl;
    GraphicsFactory gfx_factory(img_factory);
    fs::path root = fs::path(pieces_root);
    fs::path board_csv = root / "board.csv";
    // בדיקת קיום קבצים
    //std::cout << "Board CSV path: " << board_csv.string() << std::endl;
    //std::cout << "Board CSV exists: " << fs::exists(board_csv) << std::endl;

    std::ifstream in(board_csv);
    if (!in)
    {
        std::cerr << "Error: Cannot open board.csv at " << board_csv << std::endl;
        throw std::runtime_error("Cannot open board.csv");
    }
    fs::path board_png = root / "board.png";
    //std::cout << "Board PNG path: " << board_png.string() << std::endl;
    //std::cout << "Board PNG exists: " << fs::exists(board_png) << std::endl;

    auto board_img = img_factory->load(board_png.string());
    Board board(32, 32, 8, 8, board_img);

    PieceFactory pf(board, pieces_root, gfx_factory);
    std::vector<PiecePtr> out;

    std::string line;
    int row = 0;
    //std::cout << "=== Reading board.csv ===" << std::endl;
    while (std::getline(in, line))
    {
        //std::cout << "Line " << row << ": '" << line << "'" << std::endl;

        std::stringstream ss(line);
        std::string cell;
        int col = 0;
        while (std::getline(ss, cell, ','))
        {
            //std::cout << "Line " << row << ": '" << line << "'" << std::endl;
            // הסרת רווחים מיותרים
            cell.erase(0, cell.find_first_not_of(" \t\r\n"));
            cell.erase(cell.find_last_not_of(" \t\r\n") + 1);

            if (!cell.empty())
            {
                //std::cout << "  Creating piece: '" << cell << "' at (" << col << "," << row << ")" << std::endl;
                try {
                    auto piece = pf.create_piece(cell, {col, row});
                    if (piece) {
                        //std::cout << "    SUCCESS: Created piece with ID: " << piece->id << std::endl;
                        out.push_back(piece);
                    } else {
                        std::cout << "    ERROR: create_piece returned nullptr for '" << cell << "'" << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "    EXCEPTION: " << e.what() << std::endl;
                }
            }
            ++col;
        }
        ++row;
    }
    return Game(out, board);
}
inline void Game::_draw()
{
    Board display_board = clone_board();
    for (const auto &p : pieces)
    {
        p->draw_on_board(display_board, game_time_ms());
        if (kp1 && kp2)
        {
            std::vector<PlayerData> players = {
                PlayerData{1, kp1, &last_cursor1},
                PlayerData{2, kp2, &last_cursor2}};

            for (const auto &data : players)
            {
                int player = data.player;
                std::shared_ptr<KeyboardProcessor> kp = data.kp;
                std::shared_ptr<std::pair<int, int>> last = data.last_cursor;

                // קבלת מיקום הסמן
                std::pair<int, int> pos = kp->get_cursor();
                int r = pos.first;
                int c = pos.second;

                // חישוב גבולות המלבן
                int y1 = r * board.cell_H_pix;
                int x1 = c * board.cell_W_pix;
                int y2 = y1 + board.cell_H_pix - 1;
                int x2 = x1 + board.cell_W_pix - 1;

                // צבע לפי שחקן
                std::vector<uint8_t> color = (player == 1) ? std::vector<uint8_t>{0, 255, 0} : // ירוק לשחקן 1
                                                 std::vector<uint8_t>{0, 0, 255};              // כחול לשחקן 2 (במקום אדום)

                // חישוב רוחב וגובה
                int width = x2 - x1 + 1;
                int height = y2 - y1 + 1;

                // ציור המלבן
                board.img->draw_rect(x1, y1, width, height, color);
                // הדפסת לוג רק אם הסמן השתנה
                if (*last != pos)
                {
                    // logger.debug("Marker P%d moved to (%d, %d)", player, r, c);
                    *last = pos;
                }
            }
        }
    }
    display_board.show();
}
inline void Game::_show() const
{
    board.show();
}