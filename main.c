#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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

int withinWrappedRange(int original, int comparison, int minOffset, int maxOffset)
{
	if (comparison >= original - minOffset && comparison <= original + maxOffset)
	{
		return comparison - original;
	}
	if (original - minOffset < 0 && comparison >= 256 + (original - minOffset))
	{
		// Over Wrapped Min
		return comparison - (original + 256);
	}
	if (original + maxOffset > 255 && comparison <= original + maxOffset - 256)
	{
		// Under Wrapped Max
		return comparison - (original - 256);
	}
	return INT_MIN;
}

void writeIntToByteArray(char *bytes, int index, int value)
{
	for (int i = 0; i < 4; i++)
	{
		bytes[index + i] = (value >> ((3 - i) * 8)) & 0xFF;
	}
}

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
	writeIntToByteArray(outputImage->data, 4, inputImage->width);
	// 4 Bytes
	writeIntToByteArray(outputImage->data, 8, inputImage->height);
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
			int dr = withinWrappedRange(prevPixel.r, currentPixel.r, 2, 1);
			int dg = withinWrappedRange(prevPixel.g, currentPixel.g, 2, 1);
			int db = withinWrappedRange(prevPixel.b, currentPixel.b, 2, 1);
			if (dataIndex == 16)
			{
				printf("%d,%d,%d\n", prevPixel.r, prevPixel.g, prevPixel.b);
				printf("%d,%d,%d\n", currentPixel.r, currentPixel.g, currentPixel.b);
				printf("%d,%d,%d\n", dr, dg, db);
			}
			if (dr != INT_MIN && dg != INT_MIN && db != INT_MIN)
			{
				// OP_DIFF
				outputImage->data[dataIndex] = 0b01000000 | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2);
				dataIndex++;
			}
			else
			{
				// Try OP_LUMA
				dg = withinWrappedRange(prevPixel.g, currentPixel.g, 32, 31);
				// Max value is prev + (dg + 7)
				// Min value is prev + (dg - 8)
				dr = withinWrappedRange(prevPixel.r + dg, currentPixel.r, 8, 7);
				db = withinWrappedRange(prevPixel.b + dg, currentPixel.b, 8, 7);

				if (dg != INT_MIN && dr != INT_MIN && db != INT_MIN)
				{
					// OP_LUMA
					outputImage->data[dataIndex] = 0b10000000 | (dg + 32);
					outputImage->data[dataIndex + 1] = (dr + 8) << 4 | (db + 8);
					dataIndex += 2;
				}
				else
				{
					// Otherwise OP_RGB
					outputImage->data[dataIndex] = 0xFE;
					outputImage->data[dataIndex + 1] = currentPixel.r;
					outputImage->data[dataIndex + 2] = currentPixel.g;
					outputImage->data[dataIndex + 3] = currentPixel.b;
					dataIndex += 4;
				}
			}
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
	// If the image ends on a run.
	if (run > 0)
	{
		// Save Run
		outputImage->data[dataIndex] = 0b11000000 | run - 1;
		dataIndex++;
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

	struct OutputImage outputImage;
	convertToQOI(&inputImage, &outputImage);

	FILE *f = fopen("output.qoi", "wb");

	fwrite(outputImage.data, sizeof(char), outputImage.dataSize, f);

	fclose(f);

	stbi_image_free(data);
	return 0;
}