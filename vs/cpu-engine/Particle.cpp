#include "stdafx.h"

cpu_particle_physics::cpu_particle_physics()
{
	memset(this, 0, sizeof(cpu_particle_physics));
	gy = -9.81f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_particle_data::cpu_particle_data()
{
	memset(this, 0, sizeof(cpu_particle_data));
}

cpu_particle_data::~cpu_particle_data()
{
	Destroy();
}

void cpu_particle_data::Create(int maxP)
{
	Destroy();
	maxCount = maxP;
	px = new float[maxP];
	py = new float[maxP];
	pz = new float[maxP];
	vx = new float[maxP];
	vy = new float[maxP];
	vz = new float[maxP];
	age = new float[maxP];
	duration = new float[maxP];
	seed = new ui32[maxP];
	r = new float[maxP];
	g = new float[maxP];
	b = new float[maxP];
}

void cpu_particle_data::Destroy()
{
	delete [] px;
	delete [] py;
	delete [] pz;
	delete [] vx;
	delete [] vy;
	delete [] vz;
	delete [] age;
	delete [] duration;
	delete [] seed;
	delete [] r;
	delete [] g;
	delete [] b;
	memset(this, 0, sizeof(cpu_particle_data));
}

void cpu_particle_data::Update(float dt, const cpu_particle_physics& phys)
{
	if ( dt<=0.0f || alive<=0 )
		return;

	// Drag stable (approx exp(-k dt) sans expf)
	const float dragFactor = phys.drag>0.0f ? 1.0f/(1.0f+phys.drag*dt) : 1.0f;

	int i = 0;
	while ( i<alive )
	{
		// Age / kill
		age[i] += dt;
		if ( age[i]>=duration[i] )
		{
			const int last = alive - 1;
			px[i] = px[last];
			py[i] = py[last];
			pz[i] = pz[last];
			vx[i] = vx[last];
			vy[i] = vy[last];
			vz[i] = vz[last];
			age[i] = age[last];
			duration[i] = duration[last];
			r[i] = r[last];
			g[i] = g[last];
			b[i] = b[last];
			seed[i] = seed[last];
			--alive;
			continue;
		}

		float x = vx[i] + phys.gx * dt;
		float y = vy[i] + phys.gy * dt;
		float z = vz[i] + phys.gz * dt;

		x *= dragFactor;
		y *= dragFactor;
		z *= dragFactor;

		if ( phys.maxSpeed>0.0f )
		{
			const float v2 = x*x + y*y + z*z;
			const float m2 = phys.maxSpeed * phys.maxSpeed;
			if ( v2>m2 )
			{
				const float invLen = phys.maxSpeed / sqrtf(v2);
				x *= invLen;
				y *= invLen;
				z *= invLen;
			}
		}

		px[i] += x * dt;
		py[i] += y * dt;
		pz[i] += z * dt;
		vx[i] = x;
		vy[i] = y;
		vz[i] = z;

		if ( phys.useBounds )
		{
			if ( px[i]<phys.minX ) { px[i] = phys.minX; vx[i] = -vx[i] * phys.bounce; }
			if ( px[i]>phys.maxX ) { px[i] = phys.maxX; vx[i] = -vx[i] * phys.bounce; }

			if ( py[i]<phys.minY) { py[i] = phys.minY; vy[i] = -vy[i] * phys.bounce; }
			if ( py[i]>phys.maxY) { py[i] = phys.maxY; vy[i] = -vy[i] * phys.bounce; }

			if ( pz[i]<phys.minZ) { pz[i] = phys.minZ; vz[i] = -vz[i] * phys.bounce; }
			if ( pz[i]>phys.maxZ) { pz[i] = phys.maxZ; vz[i] = -vz[i] * phys.bounce; }
		}

		++i;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_particle_emitter::cpu_particle_emitter()
{
	index = -1;
	sortedIndex = -1;
	dead = false;

	rate = 200.0f;
	accum = 0.0f;
	durationMin = 0.5f;
	durationMax = 1.2f;
};

void cpu_particle_emitter::Update(cpu_particle_data& p, float dt, float ox, float oy, float oz, float baseVx, float baseVy, float baseVz, float r, float g, float b)
{
	accum += rate * dt;
	int n = (int)accum;
	accum -= (float)n;

	while ( n-->0 && p.alive<p.maxCount )
	{
		const int i = p.alive++;

		ui32 seed = (ui32)(i * 9781u + 0x9E3779B9u);

		p.px[i] = ox;
		p.py[i] = oy;
		p.pz[i] = oz;
		p.vx[i] = baseVx;
		p.vy[i] = baseVy;
		p.vz[i] = baseVz;

		p.age[i] = 0.0f;

		float rand = Rand01(seed);
		p.duration[i] = durationMin + (durationMax - durationMin) * rand;

		p.r[i] = r;
		p.g[i] = g;
		p.b[i] = b;
		p.seed[i] = seed;
	}
}
