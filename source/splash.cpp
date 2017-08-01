#include <cstdio>
#include <vector>
#include "types.h"
#include "splash.h"
#include "fsutil.h"
#include "lodepng.h"
#include "compress.h"



/*static void rotate(std::vector<u8>& splash, u16 width, u16 height)
{
	u8 *data = splash.data() + sizeof(SplashHeader);
}*/

static void rgba8ToRgb565(std::vector<u8>& splash, u16 width, u16 height, bool swap)
{
	u8 *data = splash.data() + sizeof(SplashHeader);

	for(u32 i = 0; i < width * height * 4; i += 4)
	{
		u8 r = data[i + 0];
		u8 g = data[i + 1];
		u8 b = data[i + 2];
		u8 a = data[i + 3];

		r = 1.0f * r * a / 255.0f;
		g = 1.0f * g * a / 255.0f;
		b = 1.0f * b * a / 255.0f;

		r = r >> 3;
		g = g >> 2;
		b = b >> 3;

		u16 pixel;
		if(swap) pixel = (b << 11) | (g << 5) | r;
		else     pixel = (r << 11) | (g << 5) | b;

		data[i / 2 + 0] = pixel;
		data[i / 2 + 1] = pixel>>8;
	}

	splash.resize(width * height * 2);
}

static void rgba8ToRgb8(std::vector<u8>& splash, u16 width, u16 height, bool swap)
{
	u8 *data = splash.data() + sizeof(SplashHeader);

	for(u32 i = 0; i < width * height * 4; i += 4)
	{
		u8 r = data[i + 0];
		u8 g = data[i + 1];
		u8 b = data[i + 2];
		u8 a = data[i + 3];

		r = 1.0f * r * a / 255.0f;
		g = 1.0f * g * a / 255.0f;
		b = 1.0f * b * a / 255.0f;

		u32 pixel;
		if(swap) pixel = (r << 16) | (g << 8) | b;
		else     pixel = (b << 16) | (g << 8) | r;

		data[i / 4 * 3 + 0] = pixel;
		data[i / 4 * 3 + 1] = pixel>>8;
		data[i / 4 * 3 + 2] = pixel>>16;
	}

	splash.resize(width * height * 3);
}

static void swapRgba(std::vector<u8>& splash, u16 width, u16 height)
{
	u8 *data = splash.data() + sizeof(SplashHeader);

	for(u32 i = 0; i < width * height * 4; i += 4)
	{
		u8 r = data[i + 0];
		u8 g = data[i + 1];
		u8 b = data[i + 2];
		u8 a = data[i + 3];

		data[i + 0] = a;
		data[i + 1] = b;
		data[i + 2] = g;
		data[i + 3] = r;
	}
}

bool pngToSplash(u32 flags, const char *const inFile, const char *const outFile)
{
	std::vector<u8> png = vectorFromFile(inFile);
	if(png.empty()) return false;

	unsigned err;
	std::vector<u8> splash(sizeof(SplashHeader), 0);
	u32 width, height;
	if((err = lodepng::decode(splash, width, height, png)) != 0)
	{
		fprintf(stderr, "Failed to decode png file: %s\n", lodepng_error_text(err));
		return false;
	}
	if(width > 65535u || height > 65535u)
	{
		fprintf(stderr, "Error: Pictures with width/height higher than 65535 are not supported!\n");
		return false;
	}

	SplashHeader header;
	memcpy(&header.magic, "SPLA", 4);
	header.flags = flags;

	/*if(flags & FLAG_ROTATED)
	{
		header.width = height;
		header.height = width;
		rotate(rgba, width, height);
	}
	else
	{*/
		header.width = width & 0xFFFFu;
		header.height = height & 0xFFFFu;
	//}
	memcpy(splash.data(), &header, sizeof(SplashHeader));

	switch(flags & FORMAT_INVALID)
	{
		case FORMAT_RGB565:
			rgba8ToRgb565(splash, width, height, flags & FLAG_SWAPPED);
			break;
		case FORMAT_RGB8:
			rgba8ToRgb8(splash, width, height, flags & FLAG_SWAPPED);
			break;
		case FORMAT_RGBA8:
			if(flags & FLAG_SWAPPED) swapRgba(splash, width, height);
			break;
		default: ;
	}

	if(flags & FLAG_COMPRESSED)
	{
		size_t size = 0;
		void *tmp = lz11_encode(splash.data() + sizeof(SplashHeader), splash.size() - sizeof(SplashHeader), &size);
		if(!tmp || size > splash.size() - sizeof(SplashHeader)) return false;

		memcpy(splash.data() + sizeof(SplashHeader), tmp, size);
		splash.resize(size + sizeof(SplashHeader));

		free(tmp);
	}

	return vectorToFile(splash, outFile);
}
