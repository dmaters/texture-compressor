#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <span>
#include <utility>

struct ChannelLayout {
	uint8_t bits;
	uint8_t offset;
};

template <typename T>
concept ContainerType = std::is_integral_v<T>;

template <
	ContainerType Container,
	typename ChannelType,
	ChannelLayout... Channels>
union ColorFormatPacked;

template <typename ChannelType, std::size_t ChannelCount, std::size_t BlockSize>
struct ColorFormat {
	ChannelType data[ChannelCount][BlockSize];

	constexpr ColorFormat(
		const std::array<ColorFormat<ChannelType, ChannelCount, 1>, BlockSize>&
			block
	) {
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				data[c][b] = block[b][c][0];
	}
	constexpr ColorFormat(ChannelType value = ChannelType {}) {
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++) data[c][b] = value;
	}

	static constexpr size_t channels() { return ChannelCount; }
	static constexpr size_t size() { return BlockSize; }

	constexpr std::span<ChannelType> operator[](std::size_t i) {
		return std::span<ChannelType>(data[i]);
	}

	constexpr const std::span<const ChannelType> operator[](
		std::size_t i
	) const {
		return std::span<const ChannelType>(data[i]);
	}

	static constexpr ColorFormat lerp(
		const ColorFormat& c1, const ColorFormat& c2, ChannelType v
	) {
		if constexpr (std::is_integral_v<ChannelType>) {
			ColorFormat res;
			for (int c = 0; c < ChannelCount; c++) {
				for (int b = 0; b < BlockSize; b++) {
					res[c][b] =
						c1[c][b] + ((((c2[c][b] - c1[c][b]) * v +
					                  (1 << sizeof(ChannelType) * 8) / 2)) >>
					                (sizeof(ChannelType) * 8));
				}
			}

			return res;

		} else
			return c1 + ((c2 - c1) * v);
	}

	template <typename ResultType, std::size_t OtherBlockSize>
	static constexpr std::array<ResultType, BlockSize> dot(
		const ColorFormat& c1,
		const ColorFormat<ChannelType, ChannelCount, OtherBlockSize>& c2
	) {
		std::array<ResultType, BlockSize> res = {};
		for (int c = 0; c < ChannelCount; c++)
			for (int b = 0; b < BlockSize; b++)
				res[b] += c1[c][b] * c2[c][OtherBlockSize == 1 ? 0 : b] *
				          (c < 3 ? 1 : 0);
		return res;
	}

	template <std::size_t OtherBlockSize>
	constexpr ColorFormat operator+(
		const ColorFormat<ChannelType, ChannelCount, OtherBlockSize>& o
	) const {
		ColorFormat res;
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				res[c][b] = data[c][b] + o[c][OtherBlockSize == 1 ? 0 : b];
		return res;
	}

	template <std::size_t OtherBlockSize>
	constexpr ColorFormat operator-(
		const ColorFormat<ChannelType, ChannelCount, OtherBlockSize>& o
	) const {
		ColorFormat res;
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				res[c][b] = data[c][b] - o[c][OtherBlockSize == 1 ? 0 : b];
		return res;
	}

	template <std::size_t OtherBlockSize>
	constexpr ColorFormat operator*(
		const ColorFormat<ChannelType, ChannelCount, OtherBlockSize>& o
	) const {
		ColorFormat res;
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				res[c][b] = data[c][b] * o[c][OtherBlockSize == 1 ? 0 : b];
		return res;
	}

	template <std::size_t OtherBlockSize>
	constexpr ColorFormat operator/(
		const ColorFormat<ChannelType, ChannelCount, OtherBlockSize>& o
	) const {
		ColorFormat res;
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				res[c][b] = data[c][b] / o[c][OtherBlockSize == 1 ? 0 : b];
		return res;
	}

	constexpr ColorFormat operator+(ChannelType s) const {
		ColorFormat res;
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				res[c][b] = data[c][b] + s;
		return res;
	}
	constexpr ColorFormat operator-(ChannelType s) const {
		ColorFormat res;
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				res[c][b] = data[c][b] - s;
		return res;
	}
	constexpr ColorFormat operator*(ChannelType s) const {
		ColorFormat res;
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				res[c][b] = data[c][b] * s;
		return res;
	}
	constexpr ColorFormat operator/(ChannelType s) const {
		ColorFormat res;
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				res[c][b] = data[c][b] / s;
		return res;
	}

	template <typename ResultType>
	constexpr std::array<ResultType, BlockSize> lengthSquared() {
		std::array<ResultType, BlockSize> res = {};
		for (std::size_t c = 0; c < ChannelCount; c++)
			for (std::size_t b = 0; b < BlockSize; b++)
				res[b] += data[c][b] * data[c][b] * (c < 3 ? 1 : 0);

		return res;
	}

	template <typename OtherChannelType, size_t OtherChannelCount>
	operator ColorFormat<
		OtherChannelType,
		OtherChannelCount,
		BlockSize>() const {
		ColorFormat<OtherChannelType, OtherChannelCount, BlockSize> res;

		for (int c = 0; c < std::min(ChannelCount, OtherChannelCount); c++) {
			for (int b = 0; b < BlockSize; b++) {
				ChannelType value = data[c][b];
				OtherChannelType otherValue;
				if constexpr (std::floating_point<ChannelType> &&
				              std::integral<OtherChannelType>)
					otherValue = static_cast<OtherChannelType>(
						value * static_cast<OtherChannelType>(
									(1 << (sizeof(OtherChannelType) * 8)) - 1
								)
					);
				else if constexpr (std::integral<ChannelType> &&
				                   std::floating_point<OtherChannelType>)
					otherValue = static_cast<OtherChannelType>(
						value / static_cast<OtherChannelType>(
									(1 << (sizeof(ChannelType) * 8)) - 1
								)
					);
				else
					otherValue = static_cast<OtherChannelType>(value);

				res.data[c][b] = otherValue;
			}
		}

		return res;
	}

	template <
		ContainerType Container,
		typename OtherChannelType,
		ChannelLayout... Channels>
	operator ColorFormatPacked<
		Container,
		OtherChannelType,
		Channels...>() const {
		ColorFormatPacked<Container, OtherChannelType, Channels...> res;

		for (int c = 0; c < std::min(sizeof...(Channels), ChannelCount); c++) {
			ChannelType value = data[c][0];
			OtherChannelType otherValue;
			if constexpr (std::floating_point<ChannelType> &&
			              std::integral<OtherChannelType>)
				otherValue = static_cast<OtherChannelType>(
					value * static_cast<OtherChannelType>(
								(1 << (sizeof(OtherChannelType) * 8)) - 1
							)
				);
			else if constexpr (std::integral<ChannelType> &&
			                   std::floating_point<OtherChannelType>)
				otherValue = static_cast<OtherChannelType>(
					value / static_cast<OtherChannelType>(
								(1 << (sizeof(ChannelType) * 8)) - 1
							)
				);
			else
				otherValue = static_cast<OtherChannelType>(value);
			res[c] = otherValue;
		}
		return res;
	}

	constexpr std::span<const ChannelType> r() const { return data[0]; }
	constexpr std::span<const ChannelType> g() const { return data[1]; }
	constexpr std::span<const ChannelType> b() const { return data[2]; }
	constexpr std::span<const ChannelType> a() const { return data[3]; }
};

// WARNING: The following is an overengineered class just for fun and
// limit-test, good code standards might be broken
template <
	ContainerType Container,
	typename ChannelType,
	ChannelLayout... Channels>
union ColorFormatPacked {
	template <std::size_t ChannelIndex>
	static constexpr void _setValue(Container& data, ChannelType value) {
		constexpr ChannelLayout Layout =
			std::get<ChannelIndex>(std::make_tuple(Channels...));

		Container mask = (1 << (Layout.bits + 1)) - 1;
		mask <<= Layout.offset;
		data &= ~mask;

		Container bitValue = 0;
		memcpy(&bitValue, &value, sizeof(ChannelType));

		bitValue >>= sizeof(ChannelType) * 8 - Layout.bits;
		bitValue <<= Layout.offset;

		data |= bitValue;
		return;
	}

	template <std::size_t ChannelIndex>
	static constexpr ChannelType _getValue(const Container data) {
		constexpr ChannelLayout Layout =
			std::get<ChannelIndex>(std::make_tuple(Channels...));

		ChannelType value = 0;
		Container bitValue = data;
		bitValue >>= Layout.offset;
		bitValue = bitValue & ((1 << (Layout.bits + 1)) - 1);
		uint8_t offset = (sizeof(ChannelType) * 8 - Layout.bits);

		bitValue = bitValue << offset | bitValue >> (Layout.bits - offset);

		memcpy(&value, &bitValue, sizeof(ChannelType));

		return value;
	}

	//---- PUBLIC ----

	Container data {};
	ColorFormatPacked() : data() {}
	ColorFormatPacked(ChannelType value) {
		([&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((_setValue<Is>(data, value)), ...);
		})(std::make_index_sequence<sizeof...(Channels)> {});
	}
	ColorFormatPacked(std::array<ChannelType, sizeof...(Channels)> values) {
		([&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((_setValue<Is>(data, values[Is])), ...);
		})(std::make_index_sequence<values.size()> {});
	};

	ColorFormatPacked& operator=(std::initializer_list<ChannelType> values) {
		([&]<std::size_t... Is>() {
			((_setValue<Is>(*(values.begin() + Is))), ...);
		})(std::make_index_sequence<values.size()> {});

		return *this;
	};

	template <
		ContainerType OtherContainer,
		typename OtherChannelType,
		ChannelLayout... OtherChannels>
	operator ColorFormatPacked<
		OtherContainer,
		OtherChannelType,
		OtherChannels...>() const {
		ColorFormatPacked<OtherContainer, OtherChannelType, OtherChannels...>
			otherFormat;

		if constexpr (std::is_same_v<
						  ColorFormatPacked<
							  Container,
							  ChannelType,
							  Channels...>,
						  ColorFormatPacked<
							  OtherContainer,
							  OtherChannelType,
							  OtherChannels...>>) {
			otherFormat.data = data;
			return otherFormat;
		}

		for (int c = 0;
		     c < std::min(sizeof...(Channels), sizeof...(OtherChannels));
		     c++) {
			ChannelType value = (*this)[c];
			OtherChannelType otherValue;
			if constexpr (std::floating_point<ChannelType> &&
			              std::integral<OtherChannelType>)
				otherValue = static_cast<OtherChannelType>(
					value * static_cast<OtherChannelType>(
								(1 << (sizeof(OtherChannelType) * 8)) - 1
							)
				);
			else if constexpr (std::integral<ChannelType> &&
			                   std::floating_point<OtherChannelType>)
				otherValue = static_cast<OtherChannelType>(
					value / static_cast<OtherChannelType>(
								(1 << (sizeof(ChannelType) * 8)) - 1
							)
				);
			else
				otherValue = static_cast<OtherChannelType>(value);

			otherFormat[c] = otherValue;
		}

		return otherFormat;
	}

	template <typename OtherChannelType, std::size_t ChannelCount>
	operator ColorFormat<OtherChannelType, ChannelCount, 1>() const {
		ColorFormat<OtherChannelType, ChannelCount, 1> otherFormat;

		for (int c = 0; c < std::min(sizeof...(Channels), ChannelCount); c++) {
			otherFormat[c][0] = static_cast<OtherChannelType>((*this)[c]);
		}
		return otherFormat;
	}

	struct ChannelProxy {
		Container& data;
		uint16_t channelIndex;
		ChannelProxy& operator=(ChannelType value) {
			[&]<std::size_t... Is>(std::index_sequence<Is...>) {
				(
					[&] {
						if (Is == channelIndex) {
							constexpr ChannelLayout Layouts[] = { Channels... };
							_setValue<Is>(data, value);
						}
					}(),
					...
				);
			}(std::make_index_sequence<sizeof...(Channels)> {});

			return *this;
		}
		operator ChannelType() const {
			ChannelType value;
			std::size_t i = 0;
			[&]<std::size_t... Is>(std::index_sequence<Is...>) {
				(
					[&] {
						if (Is == channelIndex) {
							constexpr ChannelLayout Layouts[] = { Channels... };
							value = _getValue<Is>(data);
						}
					}(),
					...
				);
			}(std::make_index_sequence<sizeof...(Channels)> {});

			return value;
		}
	};

	ChannelProxy operator[](int index) {
		return ChannelProxy { data, static_cast<uint16_t>(index) };
	}
	ChannelType operator[](int index) const {
		ChannelType value;
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			(
				[&] {
					if (Is == index) {
						value = _getValue<Is>(data);
					}
				}(),
				...
			);
		}(std::make_index_sequence<sizeof...(Channels)> {});
		return value;
	}

	template <std::size_t I>
	struct NamedChannelProxy {
		Container data;
		NamedChannelProxy& operator=(ChannelType value) {
			_setValue<I>(data, value);

			return *this;
		};
		operator ChannelType() const { return _getValue<I>(data); }
	};

	NamedChannelProxy<0> r;
	NamedChannelProxy<1> g;
	NamedChannelProxy<2> b;
	NamedChannelProxy<3> a;
};
template <std::size_t BlockSize>
using RGBA8n = ColorFormat<uint8_t, 4, BlockSize>;
using RGBA8 = RGBA8n<1>;
using RGBA8Block = RGBA8n<16>;

template <std::size_t BlockSize>
using RGB8n = ColorFormat<uint8_t, 3, BlockSize>;

using RGB8 = RGB8n<1>;
using RGB8Block = RGB8n<16>;

using RGB565 = ColorFormatPacked<
	uint16_t,
	uint8_t,
	{
		.bits = 5,
		.offset = 11,
	},
	{
		.bits = 6,
		.offset = 5,
	},
	{
		.bits = 5,
		.offset = 0,
	}>;

template <std::size_t BlockSize>
using RG8n = ColorFormat<uint8_t, 2, BlockSize>;

using RG8 = RG8n<1>;
using RG8Block = RG8n<16>;

template <std::size_t BlockSize>
using R8n = ColorFormat<uint8_t, 1, BlockSize>;

using R8 = R8n<1>;
using R8Block = R8n<16>;
