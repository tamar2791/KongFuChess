#pragma once

#include <utility>
#include <cstddef>
#include <memory>
#include <optional>

struct PairHash {
    size_t operator()(const std::pair<int,int>& p) const noexcept {
        return static_cast<size_t>(p.first) * 31u + static_cast<size_t>(p.second);
    }
}; 