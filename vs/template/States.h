#pragma once

#include "Entity.h"

#include <functional>

struct StateEntityGlobal
{;
	void OnEnter(Entity& e, int from);
	void OnExecute(Entity& e);
	void OnExit(Entity& e, int to);
};

