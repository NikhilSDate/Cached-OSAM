#pragma once

#include <cstddef>
#include <array>

constexpr std::size_t block_size = 16;
enum class block_type {
  INNER,
  DATA,
};
using block = std::array<int, block_size>;
