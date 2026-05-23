#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>

#include <format>

#include "texture_compressor/compression.hpp"
#include "texture_compressor/decompression.hpp"
#include "texture_compressor/utils.hpp"

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::printf("File name missing, pass it as argument");
		return -1;
	}

	int x, y, c;
	void* image = stbi_load(argv[1], &x, &y, &c, 4);
	std::size_t imageSize = x * y * 4;

	void* baseImage = malloc(imageSize);
	memcpy(baseImage, image, imageSize);

	std::size_t width, height;
	width = static_cast<std::size_t>(x);
	height = static_cast<std::size_t>(y);

	std::size_t compressedSize = texture_compressor::query_size(
		width, height, texture_compressor::Format::BC1_ALPHA
	);

	void* output = malloc(compressedSize);

	texture_compressor::compress(
		x, y, texture_compressor::Format::BC1_ALPHA, image, output
	);

	void* decompressedImage = malloc(imageSize);

	texture_compressor::decompress(
		x, y, texture_compressor::Format::BC1_ALPHA, output, decompressedImage
	);

	stbi_write_png(
		"final.png", x, y, 4, static_cast<char*>(decompressedImage), 0
	);

	double squaredError = 0;
	for (int i = 0; i < imageSize; i++) {
		double diff =
			static_cast<double>(static_cast<char*>(baseImage)[i]) -
			static_cast<double>(static_cast<char*>(decompressedImage)[i]);
		squaredError += diff * diff;
	}
	squaredError /= imageSize;

	std::printf("Squared Error: %.6f \n", squaredError);
	double psnr = 10 * log10(pow(255, 2) / squaredError);
	std::printf("PSNR Error: %.2fdb \n", psnr);

	return 0;
}
