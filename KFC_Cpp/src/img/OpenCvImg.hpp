#pragma once

#include "Img.hpp"
#include "ImgFactory.hpp"
#include <memory>
#include <string>
#include <utility>

class OpenCvImg : public Img {
public:
    OpenCvImg();
    ~OpenCvImg() override;

    void read(const std::string& path,
              const std::pair<int,int>& size = {0,0}) override;
    std::pair<int,int> size() const override;
    
    void draw_on(Img& dst, int x, int y) override;
    void put_text(const std::string& txt, int x, int y, double font_size) override;
    void show() const override;
    ImgPtr clone() const override;

    void create_blank(int width, int height);

    void draw_rect(int x, int y, int width, int height, const std::vector<uint8_t> & color) override;



private:
    struct Impl;
    std::unique_ptr<Impl> impl;
}; 

class OpenCvImgFactory : public ImgFactory {
public:
    ImgPtr create_blank(int width, int height) const override {
        auto img = std::make_shared<OpenCvImg>();
        img->create_blank(width, height);
        return img;
    }

    ImgPtr load(const std::string& path,
                const std::pair<int,int>& size = {0,0}) override {
        auto img = std::make_shared<OpenCvImg>();
        img->read(path, size);
        return img;
    }
};