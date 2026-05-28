#pragma once

#include <array>
#include <cstdint>

template <typename ChannelType, std::size_t ChannelCount>
using ColorFormat = std::array<ChannelType, ChannelCount>;

using RGBA8 = ColorFormat<uint8_t, 4>;
using RGB8 = ColorFormat<uint8_t, 3>;
using RG8 = ColorFormat<uint8_t, 2>;
using R8 = ColorFormat<uint8_t, 1>;

using RGB565 = uint16_t;

template <typename ChannelType, std::size_t ChannelCount>
using ColorBlock = ColorFormat<std::array<ChannelType, 16>, ChannelCount>;

using RGBA8Block = ColorBlock<uint8_t, 4>;
using RGB8Block = ColorBlock<uint8_t, 3>;
using RG8Block = ColorBlock<uint8_t, 2>;
using R8Block = ColorBlock<uint8_t, 1>;
