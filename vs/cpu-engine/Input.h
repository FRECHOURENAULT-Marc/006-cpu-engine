#pragma once

class cpu_input
{
public:
	enum
	{
		_NONE,
		_DOWN,
		_UP,
		_PUSH,
	};

public:
	cpu_input();
	virtual ~cpu_input();

	bool IsKey(int key);
	bool IsKeyDown(int key);
	bool IsKeyUp(int key);
	void Update();

protected:
	byte m_keys[256];
};
