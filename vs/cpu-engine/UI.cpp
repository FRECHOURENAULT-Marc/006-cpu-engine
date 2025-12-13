#include "stdafx.h"

TEXTURE::TEXTURE()
{
	rgba = nullptr;
	Close();
}

TEXTURE::~TEXTURE()
{
	Close();
}

bool TEXTURE::Load(const char* path)
{
	Close();

	FILE* file;
	if ( fopen_s(&file, path, "rb") )
		return false;
	fseek(file, 0, SEEK_END);
	int filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	byte* data = new byte[filesize];
	fread(data, 1, filesize, file);
	fclose(file);

	lodepng::State state;
	ui32 pngWidth, pngHeight;
	if ( lodepng_inspect(&pngWidth, &pngHeight, &state, data, filesize) )
	{
		delete [] data;
		return false;
	}

	bool alpha = false;
	if ( state.info_png.color.colortype==LCT_RGBA )
		alpha = true;
	else if ( state.info_png.color.colortype!=LCT_RGB )
	{
		delete [] data;
		return false;
	}

	ui32 error = lodepng_decode32(&rgba, (ui32*)&width, (ui32*)&height, data, filesize);
	delete [] data;
	if ( error || width==0 || height==0 )
	{
		Close();
		return false;
	}
	count = width * height;
	size = count * 4;
	return true;
}

void TEXTURE::Close()
{
	delete [] rgba;
	rgba = nullptr;
	width = 0;
	height = 0;
	count = 0;
	size = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SPRITE::SPRITE()
{
	pTexture = nullptr;
	x = 0;
	y = 0;
	z = 0;
	anchorX = 0;
	anchorY = 0;
	index = -1;
	sortedIndex = -1;
	dead = false;
}

void SPRITE::CenterAnchor()
{
	if ( pTexture )
	{
		anchorX = pTexture->width/2;
		anchorY = pTexture->height/2;
	}
}
