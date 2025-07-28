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
      cell_W_m(cell_W_m_) {}

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
    double x_m = pos_m.first;  // מיקום X במטרים
    double y_m = pos_m.second; // מיקום Y במטרים

    // המרה לקואורדינטות תא (col, row) - ללא עיגול
    int col = static_cast<int>(x_m / cell_W_m); // עמודה
    int row = static_cast<int>(y_m / cell_H_m); // שורה
    // std::cout << "pos_m: (" << x_m << ", " << y_m << ")"
    //           << " -> cell: (" << col << ", " << row << ")"
    //           << " [cell_W_m=" << cell_W_m << ", cell_H_m=" << cell_H_m << "]" << std::endl;

    return {col, row}; // מחזיר (עמודה, שורה)
}

std::pair<double, double> Board::cell_to_m(const std::pair<int, int> &cell) const
{
    int col = cell.first;  // עמודה
    int row = cell.second; // שורה

    // המרה למיקום במטרים
    double x_m = col * cell_W_m; // עמודה → x במטרים
    double y_m = row * cell_H_m; // שורה → y במטרים

    return {x_m, y_m};
}

std::pair<int, int> Board::m_to_pix(const std::pair<double, double>& pos_m) const {
    double x_m = pos_m.first;
    double y_m = pos_m.second;
    
    // המרה לפיקסלים - במערכת הפיקסלים: (y_px, x_px) במקום (x_px, y_px)
    int x_px = static_cast<int>(std::round((x_m / cell_W_m) * cell_W_pix));
    int y_px = static_cast<int>(std::round((y_m / cell_H_m) * cell_H_pix));
    
    // החזרה במבנה (y, x) כפי שהטסט מצפה
    return {y_px, x_px};
}