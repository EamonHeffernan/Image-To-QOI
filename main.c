#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Pixel
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

struct InputImage
{
	int width;
	int height;
	char *fileLocation;
	// InputType inputType;
	struct Pixel *pixels;
};

int main(void)
{
	char *fileLocation = "./test.png";

	int x, y, n;

	int channels = 4;

	unsigned char *data = stbi_load(fileLocation, &x, &y, &n, channels);

	printf("%d\n", x);
	printf("%d\n", y);
	printf("%d\n", n);

	struct Pixel *pixels = malloc(sizeof(struct Pixel) * x * y);

	for (int i = 0; i < x * y; i++)
	{
		// For each pixel, save each channel
		for (int j = 0; j < channels; j++)
		{
			pixels[i].r = data[i * channels + 0];
			pixels[i].g = data[i * channels + 1];
			pixels[i].b = data[i * channels + 2];
			pixels[i].a = data[i * channels + 3];
		}
	}

	struct InputImage inputImage;

	inputImage.fileLocation = fileLocation;
	inputImage.width = x;
	inputImage.height = y;
	inputImage.pixels = pixels;

	for (int i = 0; i < x * y; i++)
	{
		printf("%d\n", inputImage.pixels[i].r);
		printf("%d\n", inputImage.pixels[i].g);
		printf("%d\n", inputImage.pixels[i].b);
		printf("%d\n", inputImage.pixels[i].a);
	}
	printf("%d\n", inputImage.width);

	stbi_image_free(data);
	return 0;
}