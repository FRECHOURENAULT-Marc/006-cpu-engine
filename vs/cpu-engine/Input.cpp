#include "stdafx.h"

cpu_input::cpu_input()
{
	memset(m_keys, 0, 256);
}

cpu_input::~cpu_input()
{
}

bool cpu_input::IsKey(int key)
{
	return m_keys[key]==_DOWN || m_keys[key]==_PUSH;
}

bool cpu_input::IsKeyUp(int key)
{
	return m_keys[key]==_UP;
}

bool cpu_input::IsKeyDown(int key)
{
	return m_keys[key]==_DOWN;
}

void cpu_input::Update()
{
	for ( int i=1 ; i<255 ; i++ )
	{
		if ( GetAsyncKeyState(i)<0 )
		{
			switch ( m_keys[i] )
			{
			case _NONE:
			case _UP:
				m_keys[i] = _DOWN;
				break;
			case _DOWN:
				m_keys[i] = _PUSH;
				break;
			}
		}
		else
		{
			switch ( m_keys[i] )
			{
			case _PUSH:
			case _DOWN:
				m_keys[i] = _UP;
				break;
			case _UP:
				m_keys[i] = _NONE;
				break;
			}
		}
	}
}
