#include "Board.hpp"
#include <iostream>
#include <cmath> // std::round

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
Board::Board(int cell_H_pix_,
             int cell_W_pix_,
             int W_cells_,
             int H_cells_,
             const ImgPtr &image,
             double cell_H_m_,
             double cell_W_m_)
    : cell_H_pix(cell_H_pix_),
      cell_W_pix(cell_W_pix_),
      W_cells(W_cells_),
      H_cells(H_cells_),
      img(image),
      cell_H_m(cell_H_m_),
      cell_W_m(cell_W_m_)
{
    // std::cout << "Board created: " << W_cells_ << "x" << H_cells_
    //           << " cell_dims=" << cell_W_m_ << "x" << cell_H_m_ << std::endl;
}
Board::Board()
    : cell_H_pix(64),
      cell_W_pix(64),
      W_cells(8),
      H_cells(8),
      img(nullptr),
      cell_H_m(1.0),
      cell_W_m(1.0)
{
    //std::cout << "Board created with default values" << std::endl;
}
// ---------------------------------------------------------------------------
Board Board::clone() const
{
    ImgPtr new_img = img->clone();
    return Board(cell_H_pix, cell_W_pix, W_cells, H_cells, new_img, cell_H_m, cell_W_m);
}

// ---------------------------------------------------------------------------
void Board::show() const
{
    img->show();
}

// ---------------------------------------------------------------------------
std::pair<int, int> Board::m_to_cell(const std::pair<double, double> &pos_m) const
{
    if (W_cells <= 0 || H_cells <= 0)
    {
        // std::cerr << "ERROR: Board not properly initialized! "
        //           << "W_cells=" << W_cells << ", H_cells=" << H_cells << std::endl;
        //throw std::runtime_error("Board not properly initialized");
    }
    double x_m = pos_m.first;  // מיקום X במטרים
    double y_m = pos_m.second; // מיקום Y במטרים

    // בדיקה אם cell_W_m או cell_H_m הם אפס (חלוקה באפס!)
    if (cell_W_m <= 0 || cell_H_m <= 0 ||
        cell_W_m > 1e10 || cell_H_m > 1e10)
    { // בדיקה לערכי זבל
        // std::cerr << "ERROR: Invalid cell dimensions! "
        //           << "cell_W_m=" << cell_W_m << ", cell_H_m=" << cell_H_m << std::endl;
        //throw std::runtime_error("Invalid cell dimensions");
    }

    // המרה לקואורדינטות תא (col, row) - ללא עיגול
    int col = static_cast<int>(x_m / cell_W_m); // עמודה
    int row = static_cast<int>(y_m / cell_H_m); // שורה

    // DEBUG: הדפס את הערכים לפני החזרה
    // std::cout << "pos_m: (" << x_m << ", " << y_m << ")"
    //           << " -> cell: (" << col << ", " << row << ")"
    //           << " [cell_W_m=" << cell_W_m << ", cell_H_m=" << cell_H_m << "]"
    //           << " [board_size: " << W_cells << "x" << H_cells << "]" << std::endl;

    // בדיקת גבולות מלאה
    if (col < 0 || col >= W_cells || row < 0 || row >= H_cells)
    {
        // std::cerr << "ERROR: Cell coordinates out of bounds! "
        //           << "pos_m(" << x_m << "," << y_m << ") -> "
        //           << "cell(" << col << "," << row << ") "
        //           << "board_size(" << W_cells << "x" << H_cells << ")" << std::endl;

        // זרוק חריגה אם הקואורדינטות לא חוקיות
        //throw std::out_of_range("Cell coordinates out of bounds");
    }

    return {row, col}; // מחזיר (שורה, עמודה) - תיקון הבעיה!
}

std::pair<double, double> Board::cell_to_m(const std::pair<int, int> &cell) const
{
    int row = cell.first;  // שורה
    int col = cell.second; // עמודה

    // המרה למיקום במטרים
    double x_m = col * cell_W_m; // עמודה → x במטרים
    double y_m = row * cell_H_m; // שורה → y במטרים

    //std::cout << "[DEBUG] cell_to_m: cell(" << row << "," << col << ") -> pos_m(" << x_m << "," << y_m << ")" << std::endl;
    return {x_m, y_m};
}

std::pair<int, int> Board::m_to_pix(const std::pair<double, double> &pos_m) const
{
    double x_m = pos_m.first;
    double y_m = pos_m.second;

    // המרה לפיקסלים
    int x_px = static_cast<int>(std::round((x_m / cell_W_m) * cell_W_pix));
    int y_px = static_cast<int>(std::round((y_m / cell_H_m) * cell_H_pix));

    //std::cout << "[DEBUG] m_to_pix: pos_m(" << x_m << "," << y_m << ") -> pix(" << x_px << "," << y_px << ")" << std::endl;
    return {x_px, y_px};
}