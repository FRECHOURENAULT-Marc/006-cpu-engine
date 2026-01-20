#include "pch.h"
#include "States.h"

void StateEntityGlobal::OnEnter(Entity& e, int from)
{
	//
}

void StateEntityGlobal::OnExecute(Entity& e)
{
	e.Update();
}

void StateEntityGlobal::OnExit(Entity& e, int to)
{
	//
}
