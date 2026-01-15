#pragma once

#ifdef _DEBUG
	#pragma comment(lib, "../x64/Debug/cpu-core.lib")
	#pragma comment(lib, "../x64/Debug/cpu-render.lib")
	#pragma comment(lib, "../x64/debug/cpu-engine.lib")
#else
	#pragma comment(lib, "../x64/Release/cpu-core.lib")
	#pragma comment(lib, "../x64/Release/cpu-render.lib")
	#pragma comment(lib, "../x64/Release/cpu-engine.lib")
#endif

#include "../cpu-engine/cpu-engine.h"

#include "App.h"

#define PORT_DEFAULT 1888

#define BUF_LEN_DEFAULT 1024
#define BUF_TYPE_CREATION "cre"
#define BUF_TYPE_UPDATE "upd"
#define BUF_DATA_NAME "name"
#define BUF_DATA_KEY "key"

#define ENTITY_SPEED 1.0f
