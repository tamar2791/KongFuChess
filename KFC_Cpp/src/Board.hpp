#pragma once

#include "img/Img.hpp"
#include <utility>


class Board {
public:
    int cell_H_pix=0;
    int cell_W_pix=0;
    int W_cells=0;
    int H_cells=0;

    ImgPtr img;              // board image
    double cell_H_m=0;
    double cell_W_m=0;

public:
    Board(int cell_H_pix,
          int cell_W_pix,
          int W_cells,
          int H_cells,
          const ImgPtr& image,
          double cell_H_m = 1.0f,
          double cell_W_m = 1.0f);

    Board(const Board&) = default;
    Board(Board&&) noexcept = default;
    Board& operator=(const Board&) = default;
    Board& operator=(Board&&) noexcept = default;
    ~Board() = default;

    Board clone() const;                 // Deep-copy of image holder
    void show() const;                   // Show only if an image is loaded

    // Coordinate conversions -------------------------------------------------
    std::pair<int, int> m_to_cell(const std::pair<double, double>& pos_m) const;
    std::pair<double, double> cell_to_m(const std::pair<int, int>& cell) const;
    std::pair<int, int> m_to_pix(const std::pair<double, double>& pos_m) const;

}; 
