#pragma once

#include <memory>
#include <string>
#include <utility>
#include "Img.hpp"

class ImgFactory {
public:
    virtual ~ImgFactory() = default;

    virtual ImgPtr load(const std::string& path,
                                      const std::pair<int,int>& size = {0,0}) = 0;

    virtual ImgPtr create_blank(int width, int height) const = 0;
};
typedef std::shared_ptr<ImgFactory> ImgFactoryPtr;