#include <chrono>
#include <vector>

#include "texture_compressor/compression.hpp"
#include "texture_compressor/decompression.hpp"
#include "texture_compressor/utils.hpp"

static const size_t WIDTH = 4096, HEIGHT = 4096;

std::vector<uint8_t> generateImage(uint8_t channelCount) {
	std::vector<uint8_t> image(WIDTH * HEIGHT * channelCount);
	size_t width = WIDTH / 2;
	size_t height = HEIGHT / 2;
	// Gradient
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t index = x * channelCount + y * WIDTH * channelCount;

			image[index + 0] =
				static_cast<uint8_t>(static_cast<float>(x) / width * 255.0f);
			if (channelCount > 1)
				image[index + 1] = static_cast<uint8_t>(
					static_cast<float>(x) / height * 255.0f
				);
			if (channelCount > 2)
				image[index + 2] = static_cast<uint8_t>(
					((static_cast<float>(x) + static_cast<float>(x)) / 2) /
					width * 255.0f
				);
			if (channelCount > 3) image[index + 3] = static_cast<uint8_t>(255);
		}
	}
	// Blocks
	for (int y = 0; y < height; y++) {
		for (int x = width; x < width * 2; x++) {
			uint32_t index = x * channelCount + y * WIDTH * channelCount;

			image[index + 0] = (x / 11 % 2) * 255;
			if (channelCount > 1) image[index + 1] = (y / 11 % 2) * 255;
			if (channelCount > 2) image[index + 2] = 0;
			if (channelCount > 3) image[index + 3] = static_cast<uint8_t>(255);
		}
	}

	// Random
	for (int y = height; y < height * 2; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t index = x * channelCount + y * WIDTH * channelCount;

			image[index + 0] = rand();
			if (channelCount > 1) image[index + 1] = rand();
			if (channelCount > 2) image[index + 2] = rand();
			if (channelCount > 3) image[index + 3] = static_cast<uint8_t>(255);
		}
	}
	// Set color
	for (int y = height; y < height * 2; y++) {
		for (int x = width; x < width * 2; x++) {
			uint32_t index = x * channelCount + y * WIDTH * channelCount;

			image[index + 0] = 128;
			if (channelCount > 1) image[index + 1] = 128;
			if (channelCount > 2) image[index + 2] = 128;
			if (channelCount > 3) image[index + 3] = static_cast<uint8_t>(255);
		}
	}
	return image;
}

void benchmarkFormat(texture_compressor::Format format) {
	std::size_t compressedSize =
		texture_compressor::query_size(WIDTH, HEIGHT, format);

	uint8_t channels = 0;
	switch (format) {
		case texture_compressor::Format::BC1:
		case texture_compressor::Format::BC1_ALPHA:
			channels = 4;
			break;
		case texture_compressor::Format::BC4:
			channels = 1;
			break;
		case texture_compressor::Format::BC5:
			channels = 2;
			break;
	}

	std::vector<uint8_t> baseInput = generateImage(channels);
	std::vector<uint8_t> compressedOutput =
		std::vector<uint8_t>(compressedSize);

	double averageDuration = 0;

	for (int i = 0; i < 5; i++) {
		std::vector<uint8_t> input = baseInput;
		auto timeStamp = std::chrono::steady_clock::now();

		texture_compressor::compress(
			WIDTH, HEIGHT, format, input.data(), compressedOutput.data()
		);
		auto endStamp = std::chrono::steady_clock::now();
		double sampleDuration =
			std::chrono::duration<double, std::milli>(endStamp - timeStamp)
				.count();

		averageDuration += sampleDuration / 5;

		std::printf("Sample %i: %.2fms\n", i, sampleDuration);
	}

	std::printf("Average time : %.2f ms \n", averageDuration);

	size_t size = WIDTH * HEIGHT * channels;
	std::vector<uint8_t> decompressedOutput = std::vector<uint8_t>(size);

	texture_compressor::decompress(
		WIDTH,
		HEIGHT,
		format,
		compressedOutput.data(),
		decompressedOutput.data()
	);

	double squaredError = 0;
	for (int i = 0; i < size; i++) {
		double diff = static_cast<double>(baseInput[i]) -
		              static_cast<double>(decompressedOutput[i]);
		squaredError += diff * diff;
	}
	squaredError /= size;

	std::printf("Error: %.6f \n", squaredError);
}

int main() {
	printf("BC1 Compression :\n");
	benchmarkFormat(texture_compressor::Format::BC1);
	printf("\nBC1 Compression + ALPHA :\n");
	benchmarkFormat(texture_compressor::Format::BC1_ALPHA);
	printf("\nBC4 Compression :\n");
	benchmarkFormat(texture_compressor::Format::BC4);
	printf("\nBC5 Compression :\n");
	benchmarkFormat(texture_compressor::Format::BC5);

	return 0;
}
