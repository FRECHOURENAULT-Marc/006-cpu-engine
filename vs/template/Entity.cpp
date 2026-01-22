#include "pch.h"
#include "Entity.h"

#include "States.h"

Entity::Entity()
{
	m_mesh.CreateSphere(0.5f);
	m_material.color = cpu::ToColor(255, 0, 0);

	m_pEntity = cpuEngine.CreateEntity();
	m_pEntity->pMesh = &m_mesh;
	m_pEntity->pMaterial = &m_material;

	m_pFSM = cpuEngine.CreateFSM(this);
	m_pFSM->SetGlobal<StateEntityGlobal>();
}

Entity::~Entity()
{
	m_pEntity = cpuEngine.Release(m_pEntity);
}

void Entity::SetPos(XMFLOAT3 pos)
{
	m_pEntity->transform.pos = pos;
}

void Entity::Move(int x, int y, int z)
{
	m_pEntity->transform.dir.x = x;
	m_pEntity->transform.dir.y = y;
	m_pEntity->transform.dir.z = z;
	float dt = cpuTime.delta;
	m_pEntity->transform.Move(dt * m_speed);
	m_pEntity->transform.dir.x = 0;
	m_pEntity->transform.dir.y = 0;
	m_pEntity->transform.dir.z = 0;
}

void Entity::Update() 
{
	float dt = cpuTime.delta;
}
