#pragma once

struct cpu_glyph
{
	int x = 0, y = 0, w = 0, h = 0;
	bool valid = false;
};

struct cpu_font
{
	int first;
	int last;
	int count;

	int cellW;
	int cellH;
	int advance;

	int width;
	int height;

	std::vector<byte> rgba;

	cpu_glyph glyph[256];

	cpu_font();
	bool Create(int fontPx = 24, XMFLOAT3 color = WHITE, const char* fontName = "Consolas", int cellW = -1, int cellH = -1, int firstChar = 32, int lastChar = 255);
};
