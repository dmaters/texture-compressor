#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
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

template <typename ChannelType, size_t ChannelCount>
struct ColorFormat {
	ChannelType data[ChannelCount];

	constexpr ColorFormat(ChannelType value = ChannelType {}) {
		for (size_t i = 0; i < ChannelCount; i++) data[i] = value;
	}

	constexpr ChannelType& operator[](size_t i) { return data[i]; }
	constexpr const ChannelType& operator[](size_t i) const { return data[i]; }

	constexpr size_t size() const { return ChannelCount; }

	float lengthSquared() const {
		float length = 0;
		for (int i = 0; i < ChannelCount; i++) {
			ChannelType v = data[i];
			length += static_cast<float>(v * v);
		}
		return length;
	};

	static constexpr ColorFormat lerp(
		const ColorFormat& c1, const ColorFormat& c2, ChannelType v
	) {
		if constexpr (std::is_integral_v<ChannelType>) {
			ColorFormat res;
			for (int i = 0; i < ChannelCount; i++) {
				res[i] = c1[i] + ((((c2[i] - c1[i]) * v +
				                    (1 << sizeof(ChannelType) * 8) / 2)) >>
				                  (sizeof(ChannelType) * 8));
			}

			return res;

		} else
			return c1 + ((c2 - c1) * v);
	}

	template <typename ResultType>
	static constexpr ResultType dot(
		const ColorFormat& c1, const ColorFormat& c2
	) {
		ResultType res = 0;
		for (int i = 0; i < ChannelCount; i++) res += c1[i] * c2[i];
		return res;
	}

	constexpr ColorFormat operator+(const ColorFormat& o) const {
		ColorFormat res;
		for (size_t i = 0; i < ChannelCount; i++) res[i] = data[i] + o[i];
		return res;
	}
	constexpr ColorFormat operator-(const ColorFormat& o) const {
		ColorFormat res;
		for (size_t i = 0; i < ChannelCount; i++) res[i] = data[i] - o[i];
		return res;
	}
	constexpr ColorFormat operator*(const ColorFormat& o) const {
		ColorFormat res;
		for (size_t i = 0; i < ChannelCount; i++) res[i] = data[i] * o[i];
		return res;
	}
	constexpr ColorFormat operator/(const ColorFormat& o) const {
		ColorFormat res;
		for (size_t i = 0; i < ChannelCount; i++) res[i] = data[i] / o[i];
		return res;
	}

	constexpr ColorFormat operator+(ChannelType s) const {
		ColorFormat res;
		for (size_t i = 0; i < ChannelCount; i++) res[i] = data[i] + s;
		return res;
	}
	constexpr ColorFormat operator-(ChannelType s) const {
		ColorFormat res;
		for (size_t i = 0; i < ChannelCount; i++) res[i] = data[i] - s;
		return res;
	}
	constexpr ColorFormat operator*(ChannelType s) const {
		ColorFormat res;
		for (size_t i = 0; i < ChannelCount; i++) res[i] = data[i] * s;
		return res;
	}
	constexpr ColorFormat operator/(ChannelType s) const {
		ColorFormat res;
		for (size_t i = 0; i < ChannelCount; i++) res[i] = data[i] / s;
		return res;
	}

	template <typename OtherChannelType, size_t OtherChannelCount>
	operator ColorFormat<OtherChannelType, OtherChannelCount>() const {
		ColorFormat<OtherChannelType, OtherChannelCount> res;

		for (int c = 0; c < std::min(ChannelCount, OtherChannelCount); c++) {
			ChannelType value = data[c];
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

			res.data[c] = otherValue;
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
			ChannelType value = data[c];
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

	constexpr ChannelType r() const { return data[0]; }
	constexpr ChannelType g() const { return data[1]; }
	constexpr ChannelType b() const { return data[2]; }
	constexpr ChannelType a() const { return data[3]; }
};

// WARNING: The following is an overengineered class just for fun and
// limit-test, good code standards might be broken
template <
	ContainerType Container,
	typename ChannelType,
	ChannelLayout... Channels>
union ColorFormatPacked {
	template <size_t ChannelIndex>
	static constexpr void _setValue(Container& data, ChannelType value) {
		constexpr ChannelLayout Layout =
			std::get<ChannelIndex>(std::make_tuple(Channels...));

		Container mask = (1 << (Layout.bits + 1)) - 1;
		mask <<= Layout.offset;
		data &= ~mask;

		Container bitValue;
		memcpy(&bitValue, &value, sizeof(ChannelType));

		bitValue >>= sizeof(ChannelType) * 8 - Layout.bits;
		bitValue <<= Layout.offset;

		data |= bitValue;
		return;
	}

	template <size_t ChannelIndex>
	static constexpr ChannelType _getValue(const Container data) {
		constexpr ChannelLayout Layout =
			std::get<ChannelIndex>(std::make_tuple(Channels...));

		ChannelType value;
		Container bitValue = data;
		bitValue >>= Layout.offset;
		bitValue = bitValue & ((1 << (Layout.bits + 1)) - 1);

		bitValue = bitValue << (sizeof(ChannelType) * 8 - Layout.bits) |
		           bitValue >> Layout.bits;

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

	template <typename OtherChannelType, size_t ChannelCount>
	operator ColorFormat<OtherChannelType, ChannelCount>() const {
		ColorFormat<OtherChannelType, ChannelCount> otherFormat;

		for (int c = 0; c < std::min(sizeof...(Channels), ChannelCount); c++) {
			otherFormat[c] = static_cast<OtherChannelType>((*this)[c]);
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

	template <size_t I>
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

using RGBA_8 = ColorFormat<uint8_t, 4>;
using RGB_8 = ColorFormat<uint8_t, 3>;

using RGB_565 = ColorFormatPacked<
	uint16_t,
	uint8_t,
	{
		.bits = 5,
		.offset = 0,
	},
	{
		.bits = 6,
		.offset = 5,
	},
	{
		.bits = 5,
		.offset = 11,
	}>;
using RGB_16F = ColorFormat<float, 3>;
using R_8 = ColorFormat<uint8_t, 1>;
