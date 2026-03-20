#include <chrono>
#include <vector>

#include "texture_compressor/compression.hpp"
#include "texture_compressor/utils.hpp"
static const uint WIDTH = 4096, HEIGHT = 4096;

std::vector<uint8_t> generateImage() {
	std::vector<uint8_t> image(WIDTH * HEIGHT * 4);
	uint width = WIDTH / 4;
	uint height = HEIGHT / 4;
	// Gradient
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t index = x * 4 + y * width * 4;

			image[index + 0] = static_cast<uint8_t>(x / width * 255.0f);
			image[index + 1] = static_cast<uint8_t>(y / height * 255.0f);
			image[index + 2] =
				static_cast<uint8_t>((x + y / 2) / width * 255.0f);
			image[index + 3] = static_cast<uint8_t>(255);
		}
	}
	// Blocks
	for (int y = 0; y < height; y++) {
		for (int x = width; x < width * 2; x++) {
			uint32_t index = x * 4 + y * width * 4;

			image[index + 0] = (x / 300 % 2) * 255;
			image[index + 1] = (y / 300 % 2) * 255;
			image[index + 2] = 0;
			image[index + 3] = static_cast<uint8_t>(255);
		}
	}

	// Random
	for (int y = height; y < height * 2; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t index = x * 4 + y * width * 4;

			image[index + 0] = rand();
			image[index + 1] = rand();
			image[index + 2] = rand();
			image[index + 3] = static_cast<uint8_t>(255);
		}
	}
	// Set color
	for (int y = height; y < height * 2; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t index = x * 4 + y * width * 4;

			image[index + 0] = 128;
			image[index + 1] = 128;
			image[index + 2] = 128;
			image[index + 3] = static_cast<uint8_t>(255);
		}
	}
	return image;
}

int main() {
	uint32_t size = texture_compressor::query_size(
		WIDTH, HEIGHT, texture_compressor::Format::BC1
	);

	std::vector<uint8_t> baseInput = generateImage();
	std::vector<uint8_t> output = std::vector<uint8_t>(size);

	double averageDuration = 0;

	for (int i = 0; i < 5; i++) {
		std::vector<uint8_t> input = baseInput;
		auto timeStamp = std::chrono::steady_clock::now();

		texture_compressor::compress(
			WIDTH,
			HEIGHT,
			texture_compressor::Format::BC1,
			input.data(),
			output.data()
		);
		auto endStamp = std::chrono::steady_clock::now();
		double sampleDuration =
			std::chrono::duration<double, std::milli>(endStamp - timeStamp)
				.count();

		averageDuration += sampleDuration / 5;

		std::printf("Sample %i: %.3fms\n", i, sampleDuration);
	}

	std::printf("Average time : %.3f ms", averageDuration);

	return 0;
}
