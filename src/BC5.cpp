#include "BC5.hpp"

BC5Block BC5Block::encode(const std::array<RG8, 16>& values) {
	R8Block c1, c2;

	for (int i = 0; i < 16; i++) {
		c1[0][i] = values[i][0][0];
		c2[0][i] = values[i][1][0];
	}

	BC5Block block;

	block.channel1 = BC4Block::encode(c1);
	block.channel2 = BC4Block::encode(c2);

	return block;
}

std::array<RG8, 16> BC5Block::decode(const BC5Block& block) {
	std::array<R8, 16> channel1 = BC4Block::decode(block.channel1);
	std::array<R8, 16> channel2 = BC4Block::decode(block.channel2);

	std::array<RG8, 16> values;
	for (int i = 0; i < 16; i++) {
		values[i][0][0] = channel1[i][0][0];
		values[i][1][0] = channel2[i][0][0];
	}

	return values;
}
