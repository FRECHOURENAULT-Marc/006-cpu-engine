#pragma once

class Entity
{
	cpu_mesh m_mesh;
	cpu_material m_material;

	cpu_fsm<Entity>* m_pFSM;
	cpu_entity* m_pEntity;

	float m_speed = ENTITY_SPEED;
public:
	Entity();
	~Entity();

	void Move(int dirX, int dirY, int dirZ);

	void Update();
};

