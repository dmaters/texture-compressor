#pragma once
// WARNING: The following is an overengineered class just for fun and
// limit-test, good code standards might be broken
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
concept ContainerType = std::is_array_v<T> || std::is_integral_v<T>;

template <
	ContainerType Container,
	typename ChannelType,
	ChannelLayout... Channels>
union ColorFormat {
	//---- PRIVATE ----

	template <typename Operator>
	inline static constexpr ColorFormat _channelOperation(
		const ColorFormat& n1, const ColorFormat& n2, Operator op
	) {
		ColorFormat result;

		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			(
				[&] {
					ChannelType v1 = _getValue<Is>(n1.data);
					ChannelType v2 = _getValue<Is>(n2.data);

					_setValue<Is>(result.data, op(v1, v2));
				}(),
				...
			);
		}(std::make_index_sequence<sizeof...(Channels)> {});

		return result;
	}
	template <typename Operator>
	static constexpr ColorFormat _channelOperation(
		const ColorFormat& n1, ChannelType value, Operator op
	) {
		ColorFormat result;

		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			(
				[&]() {
					ChannelType v = _getValue<Is>(n1.data);

					_setValue<Is>(result.data, op(v, value));
				}(),
				...
			);
		}(std::make_index_sequence<sizeof...(Channels)> {});

		return result;
	}

	template <size_t ChannelIndex>
	static constexpr void _setValue(Container& data, ChannelType value) {
		if constexpr (std::is_array_v<Container>) {
			data[ChannelIndex] = value;
			return;
		} else {
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
	}

	template <size_t ChannelIndex>
	static constexpr ChannelType _getValue(const Container data) {
		if constexpr (std::is_array_v<Container>) {
			return data[ChannelIndex];
		} else {
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
	}

	//---- PUBLIC ----

	Container data {};
	ColorFormat() : data() {}
	ColorFormat(ChannelType value) {
		([&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((_setValue<Is>(data, value)), ...);
		})(std::make_index_sequence<sizeof...(Channels)> {});
	}
	ColorFormat(std::array<ChannelType, sizeof...(Channels)> values) {
		([&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((_setValue<Is>(data, values[Is])), ...);
		})(std::make_index_sequence<values.size()> {});
	};

	ColorFormat operator-(const ColorFormat& n) const {
		return _channelOperation(*this, n, std::minus<ChannelType> {});
	}
	ColorFormat operator+(const ColorFormat& n) const {
		return _channelOperation(*this, n, std::plus<ChannelType> {});
	}
	ColorFormat operator/(const ColorFormat& n) const {
		return _channelOperation(*this, n, std::divides<ChannelType> {});
	}
	ColorFormat operator*(const ColorFormat& n) const {
		return _channelOperation(*this, n, std::multiplies<ChannelType> {});
	}

	ColorFormat operator-(ChannelType v) const {
		return _channelOperation(*this, v, std::minus<ChannelType> {});
	}
	ColorFormat operator+(ChannelType v) const {
		return _channelOperation(*this, v, std::plus<ChannelType> {});
	}
	ColorFormat operator/(ChannelType v) const {
		return _channelOperation(*this, v, std::divides<ChannelType> {});
	}
	ColorFormat operator*(ChannelType v) const {
		return _channelOperation(*this, v, std::multiplies<ChannelType> {});
	}

	ColorFormat& operator=(std::initializer_list<ChannelType> values) {
		([&]<std::size_t... Is>() {
			((_setValue<Is>(*(values.begin() + Is))), ...);
		})(std::make_index_sequence<values.size()> {});

		return *this;
	};

	template <
		typename OtherContainer,
		typename OtherChannelType,
		ChannelLayout... OtherChannels>
	explicit operator ColorFormat<
		OtherContainer,
		OtherChannelType,
		OtherChannels...>() const {
		ColorFormat<OtherContainer, OtherChannelType, OtherChannels...>
			otherFormat;

		if constexpr (std::is_same_v<
						  ColorFormat<Container, ChannelType, Channels...>,
						  ColorFormat<
							  OtherContainer,
							  OtherChannelType,
							  OtherChannels...>>) {
			otherFormat.data = data;
			return otherFormat;
		}

		auto castAndSetValue = [&]<size_t Is>() mutable {
			ChannelType value = _getValue<Is>(data);
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

			otherFormat.template _setValue<Is>(otherFormat.data, otherValue);
		};

		constexpr auto l1 = std::make_tuple(Channels...);
		constexpr auto l2 = std::make_tuple(OtherChannels...);

		constexpr std::size_t size =
			std::min(sizeof...(Channels), sizeof...(OtherChannels));

		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((castAndSetValue.template operator()<Is>()), ...);
		}(std::make_index_sequence<size> {});

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
						constexpr ChannelLayout Layouts[] = { Channels... };
						value = _getValue<Is>(data);
					}
				},
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

using RGBA_8 = ColorFormat<
	uint8_t[4],
	uint8_t,
	{
		.bits = 8,
		.offset = 0,
	},
	{
		.bits = 8,
		.offset = 8,
	},
	{
		.bits = 8,
		.offset = 16,
	},
	{
		.bits = 8,
		.offset = 24,
	}>;
using RGB_565 = ColorFormat<
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
using RGB_16F = ColorFormat<
	float[3],
	float,
	{
		.bits = 32,
		.offset = 0,
	},
	{
		.bits = 32,
		.offset = 32,
	},
	{
		.bits = 32,
		.offset = 64,
	}>;
