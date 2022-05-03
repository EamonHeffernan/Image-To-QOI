#include <stdio.h>
#include <stdbool.h>

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
	unsigned int width;
	unsigned int height;
	char *fileLocation;
	// InputType inputType;
	struct Pixel *pixels;
};

struct OutputImage
{
	unsigned int width;
	unsigned int height;
	char *fileLocation;
	char *data;
	unsigned int dataSize;
};

bool matchingPixels(struct Pixel *p1, struct Pixel *p2)
{
	return p1->r == p2->r && p1->g == p2->g && p1->b == p2->b && p1->a == p2->a;
};

int getQOIHash(struct Pixel *p)
{
	return (p->r * 3 + p->g * 5 + p->b * 7 + p->a * 11) % 64;
};

int convertToQOI(struct InputImage *inputImage, struct OutputImage *outputImage)
{
	// There are 14 bytes in the header and 8 in the the footer.
	// 5 bytes is the largest possible size of one pixel.
	// Therefore 5 * the number of pixels + 22 is the maximum size of the array.
	outputImage->data = malloc(5 * inputImage->height * inputImage->width + 22);

	struct Pixel *runningArray = malloc(sizeof(struct Pixel) * 64);

	struct Pixel prevPixel;

	prevPixel.r = 0x00;
	prevPixel.g = 0x00;
	prevPixel.b = 0x00;
	prevPixel.a = 0xFF;

	// 14 Byte Header
	// 4 Bytes
	outputImage->data[0] = 'q';
	outputImage->data[1] = 'o';
	outputImage->data[2] = 'i';
	outputImage->data[3] = 'f';
	// 4 Bytes
	outputImage->data[4] = inputImage->width;
	// 4 Bytes
	outputImage->data[8] = inputImage->height;
	outputImage->data[12] = 0x04;
	outputImage->data[13] = 0x00;

	int dataIndex = 14;
	unsigned char run = 0;

	for (int pixel = 0; pixel < inputImage->height * inputImage->width; pixel++)
	{
		struct Pixel currentPixel = inputImage->pixels[pixel];
		// If run > 0 and the current pixel is not the same as the previous pixel, write the run.
		// If All the same then increment run ALSO HANDLE CASE IF RUN > 62
		if (matchingPixels(&currentPixel, &prevPixel))
		{
			if (run == 62)
			{
				// Run is max
				// Save Run
				outputImage->data[dataIndex] = 0b11000000 | run - 1;
				dataIndex++;
				run = 1;
			}
			else
			{
				run++;
			}
			// Can continue because changing the prev pixel & array do not need to be changed
			continue;
		}

		if (run > 0)
		{
			// Save Run
			outputImage->data[dataIndex] = 0b11000000 | run - 1;
			dataIndex++;
			run = 0;
		}
		unsigned int QOIHash = getQOIHash(&currentPixel);
		// If in array then OP_INDEX
		if (matchingPixels(&currentPixel, &runningArray[QOIHash]))
		{
			// OP_INDEX
			outputImage->data[dataIndex] = 0b00000000 | QOIHash;
			dataIndex++;
		}
		else if (currentPixel.a == prevPixel.a)
		{
			// Try OP_DIFF

			// Try OP_LUMA

			// Otherwise OP_RGB
			outputImage->data[dataIndex] = 0xFE;
			outputImage->data[dataIndex + 1] = currentPixel.r;
			outputImage->data[dataIndex + 2] = currentPixel.g;
			outputImage->data[dataIndex + 3] = currentPixel.b;
			dataIndex += 4;
		}
		else
		{
			// OP_RGBA
			outputImage->data[dataIndex] = 0xFF;
			outputImage->data[dataIndex + 1] = currentPixel.r;
			outputImage->data[dataIndex + 2] = currentPixel.g;
			outputImage->data[dataIndex + 3] = currentPixel.b;
			outputImage->data[dataIndex + 4] = currentPixel.a;
			dataIndex += 5;
		}

		// Save current pixel
		prevPixel = currentPixel;
		runningArray[QOIHash] = currentPixel;
	}

	// 8 Byte footer (7 0x00s followed by a 0x01)
	for (int i = 0; i < 7; i++)
	{
		outputImage->data[dataIndex] = 0x00;
		dataIndex++;
	}
	outputImage->data[dataIndex] = 0x01;

	outputImage->dataSize = dataIndex + 1;

	return 0;
}

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