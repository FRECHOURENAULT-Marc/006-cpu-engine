#include "stdafx.h"

cpu_material::cpu_material()
{
#ifdef _DEBUG
	lighting = GOURAUD;
#else
	lighting = LAMBERT;
#endif

	ps = nullptr;
	color = WHITE;
	values = nullptr;
}
