#pragma once

namespace png_lib
{
	byte* parse_png_rgba(const byte* data, size_t len, int* w, int* h);
}
