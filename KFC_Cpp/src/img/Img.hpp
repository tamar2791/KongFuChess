#pragma once

#include <string>
#include <utility>
#include <memory>
#include <vector>
#include <cstdint>

class Img; // forward declaration for smart pointer alias
using ImgPtr = std::shared_ptr<Img>;

class Img {
public:
    virtual ~Img() = default;

    // Default no-op implementations allow direct instantiation for tests
    virtual void read(const std::string& /*path*/, const std::pair<int,int>& /*size*/ = {0,0}) {}
    virtual std::pair<int,int> size() const = 0;
    virtual void draw_on(Img& /*dst*/, int /*x*/, int /*y*/) {}
    virtual void put_text(const std::string& /*txt*/, int /*x*/, int /*y*/, double /*font_size*/) {}
    virtual void show() const {}
    virtual ImgPtr clone() const = 0;

    virtual void draw_rect(int x, int y, int width, int height, const std::vector<uint8_t> & color) = 0;
};