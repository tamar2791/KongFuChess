#pragma once

#include "Graphics.hpp"
#include <memory>
#include <string>
#include "img/ImgFactory.hpp"
#include "nlohmann/json_fwd.hpp"
#include <utility>  // בשביל std::pair


// Simple GraphicsFactory that forwards an image loader placeholder to
// Graphics.  In this head-less C++ port, the img_loader is unused but the
// factory mirrors the Python API expected by the unit tests.
class GraphicsFactory
{
public:
    explicit GraphicsFactory(ImgFactoryPtr factory_ptr = nullptr)
        : img_factory(factory_ptr) {}

    std::shared_ptr<Graphics> load(const std::string &sprites_dir,
                                   const nlohmann::json & /*cfg*/, // ignored
                                   std::pair<int, int> cell_size) const
    {
        // For now, create a Graphics object with blank frames produced by img_factory
        auto gfx = std::make_shared<Graphics>(sprites_dir, cell_size, img_factory, /*loop*/ true, /*fps*/ 6.0);
        (void)cell_size; // unused for now
        return gfx;
    }

private:
    ImgFactoryPtr img_factory;
};