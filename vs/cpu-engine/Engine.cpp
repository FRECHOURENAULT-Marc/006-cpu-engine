#include "stdafx.h"

Engine* Engine::s_pEngine;

Engine::Engine()
{
	s_pEngine = this;
	m_hInstance = nullptr;
	m_hWnd = nullptr;
	m_threads = nullptr;

#ifdef GPU_PRESENT
	m_pD2DFactory = nullptr;
	m_pRenderTarget = nullptr;
	m_pBitmap = nullptr;
#else
	m_hDC = nullptr;
	m_bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bi.bmiHeader.biWidth = 0;
	m_bi.bmiHeader.biHeight = 0;
	m_bi.bmiHeader.biPlanes = 1;
	m_bi.bmiHeader.biBitCount = 32;
	m_bi.bmiHeader.biCompression = BI_RGB;
#endif
}

Engine::~Engine()
{
	assert( m_hInstance==nullptr );
}

Engine* Engine::Instance()
{
	return s_pEngine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::Initialize(HINSTANCE hInstance, int renderWidth, int renderHeight)
{
	if ( m_hInstance )
		return;

	// Device
	m_hInstance = hInstance;
	m_windowWidth = renderWidth;
	m_windowHeight = renderHeight;
	m_renderWidth = renderWidth;
	m_renderHeight = renderHeight;
	m_renderPixelCount = m_renderWidth * m_renderHeight;
	m_renderWidthHalf = m_renderWidth * 0.5f;
	m_renderHeightHalf = m_renderHeight * 0.5f;
	m_colorBuffer.resize(m_renderPixelCount);
	m_depthBuffer.resize(m_renderPixelCount);

	// Window
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "RETRO_ENGINE";
	RegisterClass(&wc);
	RECT rect = { 0, 0, m_windowWidth, m_windowHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	m_hWnd = CreateWindow("RETRO_ENGINE", "RETRO ENGINE", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, rect.right-rect.left, rect.bottom-rect.top, nullptr, nullptr, hInstance, nullptr);
	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

	// Surface
#ifdef GPU_PRESENT
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	D2D1_SIZE_U size = D2D1::SizeU(m_windowWidth, m_windowHeight);
	m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hWnd, size), &m_pRenderTarget);
	D2D1_SIZE_U renderSize = D2D1::SizeU(m_renderWidth, m_renderHeight);
	D2D1_BITMAP_PROPERTIES props;
	props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
	props.dpiX = 96.0f;
	props.dpiY = 96.0f;
	m_pRenderTarget->CreateBitmap(renderSize, props, &m_pBitmap);
#else
	m_bi.bmiHeader.biWidth = m_renderWidth;
	m_bi.bmiHeader.biHeight = -m_renderHeight;
	m_hDC = GetDC(m_hWnd);
	SetStretchBltMode(m_hDC, COLORONCOLOR);
#endif

	// Colors
	m_sky = true;
	m_clearColor = ToColor(32, 32, 64);
	m_groundColor = ToColor(32, 64, 32);
	m_skyColor = ToColor(32, 32, 64);

	// Light
	m_lightDir = { 0.5f, -1.0f, 0.5f };
	XMStoreFloat3(&m_lightDir, DirectX::XMVector3Normalize(XMLoadFloat3(&m_lightDir)));
	m_ambientLight = 0.2f;

	// Entity
	m_entityCount = 0;
	m_bornEntityCount = 0;
	m_deadEntityCount = 0;

	// Tile
	m_threadCount = std::max(1u, std::thread::hardware_concurrency());
#ifdef _DEBUG
	m_threadCount = 1;
#endif
	m_tileColCount = static_cast<unsigned int>(std::ceil(std::sqrt(m_threadCount)));
	m_tileRowCount = (m_threadCount + m_tileColCount - 1) / m_tileColCount;
	m_tileCount = m_tileColCount * m_tileRowCount;
	m_tileWidth = m_renderWidth / m_tileColCount;
	m_tileHeight = m_renderHeight / m_tileRowCount;
	int missingWidth = m_renderWidth - (m_tileWidth*m_tileColCount);
	int missingHeight = m_renderHeight - (m_tileHeight*m_tileRowCount);
	for ( int row=0 ; row<m_tileRowCount ; row++ )
	{
		for ( int col=0 ; col<m_tileColCount ; col++ )
		{
			TILE tile;
			tile.row = row;
			tile.col = col;
			tile.index = (int)m_tiles.size();
			tile.left = col * m_tileWidth;
			tile.top = row * m_tileHeight;
			tile.right = (col+1) * m_tileWidth;
			tile.bottom = (row+1) * m_tileHeight;
			if ( row==m_tileRowCount-1 )
				tile.bottom += missingHeight;
			if ( col==m_tileColCount-1 )
				tile.right += missingWidth;
			m_tiles.push_back(tile);
		}
	}

	// Thread
	m_threads = new ThreadJob[m_threadCount];
	for ( int i=0 ; i<m_threadCount ; i++ )
	{
		ThreadJob& thread = m_threads[i];
		thread.m_count = m_tileCount;
		thread.m_hEventStart = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		thread.m_hEventEnd = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	// Window
	ShowWindow(m_hWnd, SW_SHOW);
}

void Engine::Uninitialize()
{
	if ( m_hInstance==nullptr )
		return;

	// Thread
	for ( int i=0 ; i<m_threadCount ; i++ )
	{
		ThreadJob& thread = m_threads[i];
		CloseHandle(thread.m_hEventStart);
		CloseHandle(thread.m_hEventEnd);
	}
	delete [] m_threads;
	m_threads = nullptr;

	// Surface
#ifdef GPU_PRESENT
	RELPTR(m_pBitmap);
	RELPTR(m_pRenderTarget);
	RELPTR(m_pD2DFactory);
#else
	if ( m_hDC )
	{
		ReleaseDC(m_hWnd, m_hDC);
		m_hDC = nullptr;
	}
#endif

	// Window
	if ( m_hWnd )
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
	UnregisterClass("RETRO_ENGINE", m_hInstance);
	m_hInstance = nullptr;
}

void Engine::Run()
{
	// Camera
	FixWindow();
	FixProjection();

	// Start
	m_systime = timeGetTime();
	m_time = 0.0f;
	m_dt = 0.0f;
	m_fpsTime = 0;
	m_fpsCount = 0;
	m_fps = 0;
	OnStart();

	// Thread
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].Run();

	// Loop
	MSG msg = {};
	while ( msg.message!=WM_QUIT )
	{
		// Windows API
		while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
		{
			if ( msg.message==WM_QUIT )
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Time
		if ( Time()==false )
			continue;

		// Update
		Update();

		// Render
		Render();
	}

	// Thread
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].m_quitRequest = true;
	for ( int i=0 ; i<m_threadCount ; i++ )
		SetEvent(m_threads[i].m_hEventStart);
	for ( int i=0 ; i<m_threadCount ; i++ )
		m_threads[i].Wait();

	// End
	OnExit();
}

void Engine::FixWindow()
{
	RECT rc;
	GetClientRect(m_hWnd, &rc);
	m_windowWidth = rc.right-rc.left;
	m_windowHeight = rc.bottom-rc.top;

#ifdef GPU_PRESENT
	if ( m_pRenderTarget )
		m_pRenderTarget->Resize(D2D1::SizeU(m_windowWidth, m_windowHeight));
#endif
}

void Engine::FixProjection()
{
	const float ratio = float(m_windowWidth) / float(m_windowHeight);
	XMStoreFloat4x4(&m_matProj, DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, ratio, 0.1f, 100.0f));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ENTITY* Engine::CreateEntity()
{
	ENTITY* pEntity = new ENTITY;
	if ( m_bornEntityCount<m_bornEntities.size() )
		m_bornEntities[m_bornEntityCount] = pEntity;
	else
		m_bornEntities.push_back(pEntity);
	m_bornEntityCount++;
	return pEntity;
}

void Engine::ReleaseEntity(ENTITY* pEntity)
{
	if ( pEntity==nullptr || pEntity->dead )
		return;

	pEntity->dead = true;
	if ( m_deadEntityCount<m_deadEntities.size() )
		m_deadEntities[m_deadEntityCount] = pEntity;
	else
		m_deadEntities.push_back(pEntity);
	m_deadEntityCount++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DirectX::XMFLOAT3 Engine::ApplyLighting(DirectX::XMFLOAT3& color, float intensity)
{
	intensity = Clamp(intensity);
	DirectX::XMFLOAT3 trg;
	trg.x = color.x * intensity;
	trg.y = color.y * intensity;
	trg.z = color.z * intensity;
	return trg;
}

ui32 Engine::ToBGR(DirectX::XMFLOAT3& color)
{
	float r = std::max(0.0f, std::min(1.0f, color.x));
	float g = std::max(0.0f, std::min(1.0f, color.y));
	float b = std::max(0.0f, std::min(1.0f, color.z));
	return RGB((int)(b * 255.0f), (int)(g * 255.0f), (int)(r * 255.0f));
}

DirectX::XMFLOAT3 Engine::ToColor(int r, int g, int b)
{
	DirectX::XMFLOAT3 color;
	color.x = r/255.0f;
	color.y = g/255.0f;
	color.z = b/255.0f;
	return color;
}

float Engine::Clamp(float v)
{
	if ( v<0.0f )
		return 0.0f;
	if ( v>1.0f )
		return 1.0f;
	return v;
}

float Engine::Clamp(float v, float min, float max)
{
	if ( v<min )
		return min;
	if ( v>max )
		return max;
	return v;
}

int Engine::Clamp(int v, int min, int max)
{
	if ( v<min )
		return min;
	if ( v>max )
		return max;
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::CreateSpaceship(MESH& mesh)
{
	const float width = 2.0f;
	DirectX::XMFLOAT3 nose = { 0.0f, 0.0f, 1.5f };
	DirectX::XMFLOAT3 rTop = { 0.0f, 0.5f, -1.0f };
	DirectX::XMFLOAT3 rBot = { 0.0f, -0.3f, -1.0f };
	DirectX::XMFLOAT3 wLeft = { -width*0.5f, 0.0f, -1.0f };
	DirectX::XMFLOAT3 wRight = { width*0.5f, 0.0f, -1.0f };

	DirectX::XMFLOAT3 c1 = ToColor(208, 208, 208);
	DirectX::XMFLOAT3 c2 = ToColor(192, 192, 192);
	DirectX::XMFLOAT3 c3 = ToColor(112, 112, 112);
	DirectX::XMFLOAT3 c4 = ToColor(96, 96, 96);
	DirectX::XMFLOAT3 c5 = ToColor(255, 255, 255);
	DirectX::XMFLOAT3 c6 = ToColor(255, 255, 255);

	mesh.AddTriangle(nose, rTop, wLeft, c1);				// Avant Gauche haut
	mesh.AddTriangle(nose, wRight, rTop, c2);				// Avant Droit haut
	mesh.AddTriangle(nose, wLeft, rBot, c3);				// Avant Gauche bas
	mesh.AddTriangle(nose, rBot, wRight, c4);				// Avant Droit bas
	mesh.AddTriangle(wLeft, rTop, rBot, c5);				// Moteur Gauche
	mesh.AddTriangle(wRight, rBot, rTop, c6);				// Moteur Droit
	mesh.Optimize();
}

void Engine::CreateCube(MESH& mesh)
{
	const float s = 0.5f; 
	DirectX::XMFLOAT3 p0 = { -s, -s, -s };					// Avant Bas Gauche
	DirectX::XMFLOAT3 p1 = {  s, -s, -s };					// Avant Bas Droite
	DirectX::XMFLOAT3 p2 = {  s,  s, -s };					// Avant Haut Droite
	DirectX::XMFLOAT3 p3 = { -s,  s, -s };					// Avant Haut Gauche
	DirectX::XMFLOAT3 p4 = { -s, -s,  s };					// Arrière Bas Gauche
	DirectX::XMFLOAT3 p5 = {  s, -s,  s };					// Arrière Bas Droite
	DirectX::XMFLOAT3 p6 = {  s,  s,  s };					// Arrière Haut Droite
	DirectX::XMFLOAT3 p7 = { -s,  s,  s };					// Arrière Haut Gauche
	
	DirectX::XMFLOAT3 c1 = ToColor(255, 255, 255);
	mesh.AddFace(p0, p1, p2, p3, c1);						// Face Avant (Z = -0.5)
	mesh.AddFace(p5, p4, p7, p6, c1);						// Face Arrière (Z = +0.5)
	mesh.AddFace(p1, p5, p6, p2, c1);						// Face Droite (X = +0.5)
	mesh.AddFace(p4, p0, p3, p7, c1);						// Face Gauche (X = -0.5)
	mesh.AddFace(p3, p2, p6, p7, c1);						// Face Haut (Y = +0.5)
	mesh.AddFace(p4, p5, p1, p0, c1);						// Face Bas (Y = -0.5)
	mesh.Optimize();
}

void Engine::CreateCircle(MESH& mesh, float radius, int count)
{
	if ( count<3 )
		return;

	DirectX::XMFLOAT3 c1 = ToColor(255, 255, 255);
	float step = DirectX::XM_2PI / count;
	float angle = 0.0f;
	DirectX::XMFLOAT3 p1, p2, p3;
	p1.x = 0.0f;
	p1.y = 0.0f;
	p1.z = 0.0f;
	p2.y = 0.0f;
	p3.y = 0.0f;
	for ( int i=0 ; i<count ; i++ )
	{
		p2.x = cosf(angle) * radius;
		p2.z = sinf(angle) * radius;
		p3.x = cosf(angle+step) * radius;
		p3.z = sinf(angle+step) * radius;
		mesh.AddTriangle(p1, p2, p3, c1);
		angle += step;
	}
	mesh.Optimize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Engine::Time()
{
	DWORD curtime = timeGetTime();
	DWORD deltatime = curtime - m_systime;
	if ( deltatime<5 )
		return false;

	m_systime = curtime;
	if ( deltatime>30 )
		deltatime = 30;
	m_dt = deltatime/1000.0f;
	m_time += m_dt;

	if ( m_systime-m_fpsTime>=1000 )
	{
		m_fps = m_fpsCount;
		m_fpsCount = 0;
		m_fpsTime = m_systime;
	}
	else
		m_fpsCount++;

	return true;
}

void Engine::Update()
{
	// Physics
	Update_Physics();

	// Callback
	OnUpdate();

	// Purge
	Update_Purge();
}

void Engine::Update_Physics()
{
	for ( int i=0 ; i<m_entityCount ; i++ )
	{
		ENTITY* pEntity = m_entities[i];
		if ( pEntity->dead )
			continue;

		pEntity->lifetime += m_dt;
	}
}

void Engine::Update_Purge()
{
	// Dead
	for ( int i=0 ; i<m_deadEntityCount ; i++ )
	{
		ENTITY* pEntity = m_deadEntities[i];
		if ( pEntity->index==-1 )
		{
			delete pEntity;
			m_deadEntities[i] = nullptr;
			continue;
		}

		if ( pEntity->index<m_entityCount-1 )
		{
			m_entities[pEntity->index] = m_entities[m_entityCount-1];
			m_entities[pEntity->index]->index = pEntity->index;
		}

		if ( pEntity->sortedIndex<m_entityCount-1 )
		{
			m_sortedEntities[pEntity->sortedIndex] = m_sortedEntities[m_entityCount-1];
			m_sortedEntities[pEntity->sortedIndex]->sortedIndex = pEntity->sortedIndex;
		}

		delete pEntity;
		m_deadEntities[i] = nullptr;
		m_entityCount--;
	}
	m_deadEntityCount = 0;

	// Born
	for ( int i=0 ; i<m_bornEntityCount ; i++ )
	{
		ENTITY* pEntity = m_bornEntities[i];
		if ( pEntity->dead )
		{
			delete pEntity;
			m_bornEntities[i] = nullptr;
			continue;
		}

		pEntity->index = m_entityCount;
		pEntity->sortedIndex = pEntity->index;

		if ( pEntity->index<m_entities.size() )
			m_entities[pEntity->index] = pEntity;
		else
			m_entities.push_back(pEntity);

		if ( pEntity->sortedIndex<m_sortedEntities.size() )
			m_sortedEntities[pEntity->sortedIndex] = pEntity;
		else
			m_sortedEntities.push_back(pEntity);

		m_entityCount++;
	}
	m_bornEntityCount = 0;
}

void Engine::Render()
{
	// Prepare
	Render_View();
	Render_Sort();
	Render_Box();
	Render_Tile();

	// Background
	if ( m_sky )
		DrawSky();
	else
		Clear(m_clearColor);

	// Callback
	OnPreRender();

	// Entities
	for ( int i=0 ; i<m_threadCount ; i++ )
		SetEvent(m_threads[i].m_hEventStart);
	for ( int i=0 ; i<m_threadCount ; i++ )
		WaitForSingleObject(m_threads[i].m_hEventEnd, INFINITE);

	// Callback
	OnPostRender();

	// Present
	Present();
}

void Engine::Render_View()
{
	m_camera.UpdateWorld();
	DirectX::XMMATRIX mat = DirectX::XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera.world));
	XMStoreFloat4x4(&m_matView, mat);
	mat *= XMLoadFloat4x4(&m_matProj);
	XMStoreFloat4x4(&m_matViewProj, mat);
}

void Engine::Render_Sort()
{
	std::sort(m_sortedEntities.begin(), m_sortedEntities.begin()+m_entityCount, [](const ENTITY* pA, const ENTITY* pB) { return pA->view.z < pB->view.z; });
	for ( int i=0 ; i<m_entityCount ; i++ )
		m_sortedEntities[i]->sortedIndex = i;
}

void Engine::Render_Box()
{
	float width = (float)m_renderWidth;
	float height = (float)m_renderHeight;

	for ( int iEntity=0 ; iEntity<m_entityCount ; iEntity++ )
	{
		ENTITY* pEntity = m_entities[iEntity];
		if ( pEntity->dead )
			continue;

		// World
		pEntity->transform.UpdateWorld();
		DirectX::XMMATRIX matWorld = DirectX::XMLoadFloat4x4(&pEntity->transform.world);

		// OBB
		pEntity->obb = pEntity->pMesh->aabb;
		pEntity->obb.Transform(matWorld);

		// AABB
		pEntity->aabb = pEntity->obb;

		// Rectangle (screen)
		DirectX::XMMATRIX matWVP = matWorld;
		matWVP *= DirectX::XMLoadFloat4x4(&m_matViewProj);
		pEntity->pMesh->aabb.ToScreen(pEntity->box, matWVP, width, height);

		// View
		DirectX::XMMATRIX matView = DirectX::XMLoadFloat4x4(&m_matView);
		DirectX::XMVECTOR pos = XMLoadFloat3(&pEntity->transform.pos);
		pos = DirectX::XMVector3TransformCoord(pos, matView);
		XMStoreFloat3(&pEntity->view, pos);
	}
}

void Engine::Render_Tile()
{
	// Reset
	m_nextTile = 0;

	// Entities
	for ( int iEntity=0 ; iEntity<m_entityCount ; iEntity++ )
	{
		ENTITY* pEntity = m_entities[iEntity];
		if ( pEntity->dead )
			continue;

		int minX = Clamp(int(pEntity->box.min.x) / m_tileWidth, 0, m_tileColCount-1);
		int maxX = Clamp(int(pEntity->box.max.x) / m_tileWidth, 0, m_tileColCount-1);
		int minY = Clamp(int(pEntity->box.min.y) / m_tileHeight, 0, m_tileRowCount-1);
		int maxY = Clamp(int(pEntity->box.max.y) / m_tileHeight, 0, m_tileRowCount-1);

		pEntity->tile = 0;
		for ( int y=minY ; y<=maxY ; y++ )
		{
			for ( int x=minX ; x<=maxX ; x++ )
			{
				int index = y*m_tileColCount + x;
				pEntity->tile |= 1 << index;
			}
		}
	}
}

void Engine::Render_Entity(int iTile)
{
	TILE& tile = m_tiles[iTile];
	for ( int iEntity=0 ; iEntity<m_entityCount ; iEntity++ )
	{
		ENTITY* pEntity = m_sortedEntities[iEntity];
		if ( pEntity->dead )
			continue;
	
		bool entityHasTile = (pEntity->tile>>tile.index) & 1 ? true : false;
		if ( entityHasTile==false )
			continue;

		Draw(pEntity, tile);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Engine::Present()
{
#ifdef GPU_PRESENT
	if ( m_pRenderTarget==nullptr || m_pBitmap==nullptr )
		return;

	m_pRenderTarget->BeginDraw();
	m_pBitmap->CopyFromMemory(NULL, m_colorBuffer.data(), m_renderWidth * 4);
	D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, (float)m_windowWidth, (float)m_windowHeight);
	m_pRenderTarget->DrawBitmap(m_pBitmap, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
	//m_pRenderTarget->DrawBitmap(m_pBitmap, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
	HRESULT hr = m_pRenderTarget->EndDraw();
	if ( hr==D2DERR_RECREATE_TARGET )
	{
		m_pBitmap->Release();
		m_pBitmap = nullptr;
		m_pRenderTarget->Release();
		m_pRenderTarget = nullptr;
		// Rappeler Initialize() ou recréer les ressources...
	}
#else
	if ( m_windowWidth==m_renderWidth && m_windowHeight==m_renderHeight )
		SetDIBitsToDevice(m_hDC, 0, 0, m_renderWidth, m_renderHeight, 0, 0, 0, m_renderHeight, m_colorBuffer.data(), &m_bi, DIB_RGB_COLORS);
	else
		StretchDIBits(m_hDC, 0, 0, m_windowWidth, m_windowHeight, 0, 0, m_renderWidth, m_renderHeight, m_colorBuffer.data(), &m_bi, DIB_RGB_COLORS, SRCCOPY);
#endif
}

void Engine::Clear(DirectX::XMFLOAT3& color)
{
	ui32 bgr = ToBGR(color);
	std::fill(m_colorBuffer.begin(), m_colorBuffer.end(), bgr);
	std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 1.0f);
}

void Engine::DrawSky()
{
	ui32 gCol = ToBGR(m_groundColor);
	ui32 sCol = ToBGR(m_skyColor);

	DirectX::XMMATRIX matView = XMLoadFloat4x4(&m_matView);
	DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR viewUp = DirectX::XMVector3TransformNormal(worldUp, matView);

	float nx = DirectX::XMVectorGetX(viewUp);
	float ny = DirectX::XMVectorGetY(viewUp);
	float nz = DirectX::XMVectorGetZ(viewUp);

	float p11 = m_matProj._11; // Cotan(FovX/2) / Aspect
	float p22 = m_matProj._22; // Cotan(FovY/2)

	float a = nx / (p11 * m_renderWidthHalf);
	float b = -ny / (p22 * m_renderHeightHalf); // Le signe '-' compense l'axe Y inversé de l'écran
	float c = nz - (nx / p11) + (ny / p22);

	bool rightSideIsSky = a > 0;
	
	uint32_t colLeft  = rightSideIsSky ? gCol : sCol;
	uint32_t colRight = rightSideIsSky ? sCol : gCol;

	std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 1.0f);
	if ( abs(a)<0.000001f )
	{
		for ( int y=0 ; y<m_renderHeight ; y++ )
		{
			float val = b * (float)y + c;
			uint32_t col = val>0.0f ? sCol : gCol;
			std::fill(m_colorBuffer.begin() + (y * m_renderWidth),  m_colorBuffer.begin() + ((y + 1) * m_renderWidth),  col);
		}
	}
	else
	{
		float invA = -1.0f / a;
		for ( int y=0 ; y<m_renderHeight ; y++ )
		{
			float fSplitX = (b * (float)y + c) * invA;
			int splitX;
			if ( fSplitX<0.0f )
				splitX = 0;
			else if ( fSplitX>(float)m_renderWidth)
				splitX = m_renderWidth;
			else
				splitX = (int)fSplitX;

			uint32_t* rowPtr = m_colorBuffer.data() + (y * m_renderWidth);
			if ( splitX>0 )
				std::fill(rowPtr, rowPtr + splitX, colLeft);
			if ( splitX<m_renderWidth )
				std::fill(rowPtr + splitX, rowPtr + m_renderWidth, colRight);
		}
	}
}

void Engine::DrawLine(int x0, int y0, float z0, int x1, int y1, float z1, DirectX::XMFLOAT3& color)
{
	ui32 bgr = ToBGR(color);

	int dx = abs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;

	float dist = (float)std::max(dx, abs(dy));
	if ( dist==0.0f )
		return;

	float zStep = (z1 - z0) / dist;
	float currentZ = z0;

	while ( true )
	{
		if ( x0>=0 && x0<m_renderWidth && y0>=0 && y0<m_renderHeight )
		{
			int index = y0 * m_renderWidth + x0;
			if ( currentZ<m_depthBuffer[index] )
			{
				m_colorBuffer[index] = bgr;
				m_depthBuffer[index] = currentZ;
			}
		}

		if ( x0==x1 && y0==y1 )
			break;

		int e2 = 2 * err;
		if ( e2>=dy )
		{
			err += dy;
			x0 += sx;
		}
		if ( e2<=dx )
		{
			err += dx;
			y0 += sy;
		}

		currentZ += zStep;
	}
}

void Engine::FillTriangle(DirectX::XMFLOAT3& v1, DirectX::XMFLOAT3& v2, DirectX::XMFLOAT3& v3, DirectX::XMFLOAT3& c1, DirectX::XMFLOAT3& c2, DirectX::XMFLOAT3& c3, TILE& tile)
{
	const float x1 = v1.x, y1 = v1.y, z1 = v1.z;
	const float x2 = v2.x, y2 = v2.y, z2 = v2.z;
	const float x3 = v3.x, y3 = v3.y, z3 = v3.z;

	int minX = (int)floor(std::min(std::min(x1, x2), x3));
	int maxX = (int)ceil(std::max(std::max(x1, x2), x3));
	int minY = (int)floor(std::min(std::min(y1, y2), y3));
	int maxY = (int)ceil(std::max(std::max(y1, y2), y3));

	if ( minX<0 )
		minX = 0;
	if ( minY<0 )
		minY = 0;
	if ( maxX>m_renderWidth )
		maxX = m_renderWidth;
	if ( maxY>m_renderHeight )
		maxY = m_renderHeight;

	if ( minX<tile.left )
		minX = tile.left;
	if ( maxX>tile.right )
		maxX = tile.right;
	if ( minY<tile.top )
		minY = tile.top;
	if ( maxY>tile.bottom )
		maxY = tile.bottom;

	if ( minX>=maxX || minY>=maxY )
		return;

	float a12 = y1 - y2;
	float b12 = x2 - x1;
	float c12 = x1 * y2 - x2 * y1;
	float a23 = y2 - y3;
	float b23 = x3 - x2;
	float c23 = x2 * y3 - x3 * y2;
	float a31 = y3 - y1;
	float b31 = x1 - x3;
	float c31 = x3 * y1 - x1 * y3;
	float area = a12 * x3 + b12 * y3 + c12;
	if ( fabsf(area)<1e-6f )
		return;

	float invArea = 1.0f / area;
	bool areaPositive = area>0.0f;
	float startX = (float)minX + 0.5f;
	float startY = (float)minY + 0.5f;
	float e12_row = a12 * startX + b12 * startY + c12;
	float e23_row = a23 * startX + b23 * startY + c23;
	float e31_row = a31 * startX + b31 * startY + c31;
	const float dE12dx = a12;
	const float dE12dy = b12;
	const float dE23dx = a23;
	const float dE23dy = b23;
	const float dE31dx = a31;
	const float dE31dy = b31;

	for ( int y=minY ; y<maxY ; ++y )
	{
		float e12 = e12_row;
		float e23 = e23_row;
		float e31 = e31_row;
		for ( int x=minX ; x<maxX ; ++x )
		{
			if ( areaPositive )
			{
				if ( e12<0.0f || e23<0.0f || e31<0.0f )
				{
					e12 += dE12dx;
					e23 += dE23dx;
					e31 += dE31dx;
					continue;
				}
			}
			else
			{
				if ( e12>0.0f || e23>0.0f || e31>0.0f )
				{
					e12 += dE12dx;
					e23 += dE23dx;
					e31 += dE31dx;
					continue;
				}
			}

			float w1 = e23 * invArea;
			float w2 = e31 * invArea;
			float w3 = e12 * invArea;
			float z = z1 * w1 + z2 * w2 + z3 * w3;
			int index = y * m_renderWidth + x;
			if ( z>=m_depthBuffer[index] )
			{
				e12 += dE12dx;
				e23 += dE23dx;
				e31 += dE31dx;
				continue;
			}
			m_depthBuffer[index] = z;

			float r = Clamp(c1.x * w1 + c2.x * w2 + c3.x * w3);
			float g = Clamp(c1.y * w1 + c2.y * w2 + c3.y * w3);
			float b = Clamp(c1.z * w1 + c2.z * w2 + c3.z * w3);
			m_colorBuffer[index] = RGB((int)(b*255.0f), (int)(g*255.0f), (int)(r*255.0f));

			e12 += dE12dx;
			e23 += dE23dx;
			e31 += dE31dx;
		}

		e12_row += dE12dy;
		e23_row += dE23dy;
		e31_row += dE31dy;
	}
}

void Engine::Draw(ENTITY* pEntity, TILE& tile)
{
	if ( pEntity==nullptr || pEntity->pMesh==nullptr )
		return;

	DirectX::XMMATRIX matWorld = DirectX::XMLoadFloat4x4(&pEntity->transform.world);
	DirectX::XMMATRIX matView = DirectX::XMLoadFloat4x4(&m_matView);
	DirectX::XMMATRIX matViewProj = DirectX::XMLoadFloat4x4(&m_matViewProj);
	DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&m_lightDir);

	for ( const TRIANGLE& t : pEntity->pMesh->triangles )
	{
		// World space
		DirectX::XMVECTOR vWorld[3];
		for ( int i=0 ; i<3 ; i++ )
		{
			DirectX::XMVECTOR loc = DirectX::XMLoadFloat3(&t.v[i].pos);
			loc = DirectX::XMVectorSetW(loc, 1.0f);
			vWorld[i] = DirectX::XMVector3Transform(loc, matWorld);
		}

		//DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&m_cam.pos);
		//DirectX::XMVECTOR e0 = DirectX::XMVectorSubtract(vWorld[1], vWorld[0]);
		//DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract(vWorld[2], vWorld[0]);
		//DirectX::XMVECTOR faceNormal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(e0, e1));
		//DirectX::XMVECTOR toCam = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(camPos, vWorld[0]));
		//float ndotv = DirectX::XMVectorGetX(DirectX::XMVector3Dot(faceNormal, toCam));
		//if ( ndotv<=0.001f )
		//	continue;

		// Screen space
		DirectX::XMFLOAT3 vScreen[3];
		bool safe = true;
		for ( int i=0 ; i<3 ; i++ )
		{
			DirectX::XMVECTOR clip = DirectX::XMVector3Transform(vWorld[i], matViewProj);
			DirectX::XMFLOAT4 out;
			DirectX::XMStoreFloat4(&out, clip);
			if ( out.w<=0.0f )
			{
				safe = false;
				break;
			}
			float invW = 1.0f / out.w;
			float ndcX = out.x * invW;   // [-1,1]
			float ndcY = out.y * invW;   // [-1,1]
			float ndcZ = out.z * invW;   // [0,1] avec XMMatrixPerspectiveFovLH
			ndcZ = Clamp(ndcZ);
			vScreen[i].x = (ndcX + 1.0f) * m_renderWidthHalf;
			vScreen[i].y = (1.0f - ndcY) * m_renderHeightHalf;
			vScreen[i].z = ndcZ;         // profondeur normalisée 0..1
		}
		if ( safe==false )
			continue;

		// Culling
		float area = (vScreen[2].x - vScreen[0].x) * (vScreen[1].y - vScreen[0].y) - (vScreen[2].y - vScreen[0].y) * (vScreen[1].x - vScreen[0].x);
		if ( area<=0.0f )
			continue;

		// Color
		DirectX::XMFLOAT3 vertColors[3];
		for ( int i=0 ; i<3 ; i++ )
		{
			DirectX::XMVECTOR localNormal = DirectX::XMLoadFloat3(&t.v[i].normal);
			DirectX::XMVECTOR worldNormal = DirectX::XMVector3TransformNormal(localNormal, matWorld);
			worldNormal = DirectX::XMVector3Normalize(worldNormal);
			float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(worldNormal, DirectX::XMVectorNegate(lightDir)));
			float intensity = std::max(0.0f, dot) + m_ambientLight;
			DirectX::XMFLOAT3 finalColor;
			finalColor.x = t.v[i].color.x * pEntity->material.x * intensity;
			finalColor.y = t.v[i].color.y * pEntity->material.y * intensity;
			finalColor.z = t.v[i].color.z * pEntity->material.z * intensity;
			vertColors[i] = finalColor;
		}

		// Draw
		FillTriangle(vScreen[0], vScreen[1], vScreen[2], vertColors[0], vertColors[1], vertColors[2], tile);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT Engine::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Engine* pEngine = (Engine*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if ( pEngine )
		return pEngine->OnWindowProc(hWnd, message, wParam, lParam);

	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT Engine::OnWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch ( message )
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		FixWindow();
		FixProjection();
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ThreadJob::OnStart()
{
	Engine* pEngine = Engine::Instance();
	m_quitRequest = false;

	// Thread
	while ( true )
	{
		// Waiting next job
		WaitForSingleObject(m_hEventStart, INFINITE);
		if ( m_quitRequest )
			break;

		// Job
		while ( true )
		{
			int index = pEngine->m_nextTile.fetch_add(1, std::memory_order_relaxed);
			if ( index>=m_count )
				break;

			pEngine->Render_Entity(index);
		}

		// End
		SetEvent(m_hEventEnd);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VERTEX::VERTEX()
{
	Identity();
}

void VERTEX::Identity()
{
	pos = { 0.0f, 0.0f, 0.0f };
	color = { 1.0f, 1.0f, 1.0f };
	normal = { 0.0f, 1.0f, 0.0f };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TRIANGLE::TRIANGLE()
{
	Identity();
}

void TRIANGLE::Identity()
{
	v[0].Identity();
	v[1].Identity();
	v[2].Identity();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RECTANGLE::RECTANGLE()
{
	Zero();
}

void RECTANGLE::Zero()
{
	min = { 0.0f, 0.0f };
	max = { 0.0f, 0.0f };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AABB::AABB()
{
	Zero();
}

AABB& AABB::operator=(const OBB& obb)
{
	min = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
	max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for ( int i=0 ; i<8 ; ++i )
	{
		const auto& p = obb.pts[i];

		if ( p.x<min.x )
			min.x = p.x;
		if ( p.y<min.y )
			min.y = p.y;
		if ( p.z<min.z )
			min.z = p.z;

		if ( p.x>max.x )
			max.x = p.x;
		if ( p.y>max.y )
			max.y = p.y;
		if ( p.z>max.z )
			max.z = p.z;
	}
	return *this;
}

void AABB::Zero()
{
	min = { 0.0f, 0.0f, 0.0f };
	max = { 0.0f, 0.0f, 0.0f };
}

bool XM_CALLCONV AABB::ToScreen(RECTANGLE& out, DirectX::FXMMATRIX wvp, float renderWidth, float renderHeight)
{
	const float renderX = 0.0f;
	const float renderY = 0.0f;
	float minX =  FLT_MAX, minY =  FLT_MAX;
	float maxX = -FLT_MAX, maxY = -FLT_MAX;

	const float xmin = min.x;
	const float ymin = min.y;
	const float zmin = min.z;
	const float xmax = max.x;
	const float ymax = max.y;
	const float zmax = max.z;

	const DirectX::XMVECTOR pts[8] =
	{
		DirectX::XMVectorSet(xmin, ymin, zmin, 1.0f),
		DirectX::XMVectorSet(xmax, ymin, zmin, 1.0f),
		DirectX::XMVectorSet(xmax, ymax, zmin, 1.0f),
		DirectX::XMVectorSet(xmin, ymax, zmin, 1.0f),
		DirectX::XMVectorSet(xmin, ymin, zmax, 1.0f),
		DirectX::XMVectorSet(xmax, ymin, zmax, 1.0f),
		DirectX::XMVectorSet(xmax, ymax, zmax, 1.0f),
		DirectX::XMVectorSet(xmin, ymax, zmax, 1.0f)
	};

	for ( int i=0 ; i<8 ; ++i )
	{
		DirectX::XMVECTOR clip = XMVector4Transform(pts[i], wvp);

		float w = DirectX::XMVectorGetW(clip);
		if ( w<=0.00001f )
			continue; // derrière caméra

		float invW = 1.0f / w;
		float ndcX = DirectX::XMVectorGetX(clip) * invW;
		float ndcY = DirectX::XMVectorGetY(clip) * invW;

		float sx = renderX + (ndcX * 0.5f + 0.5f) * renderWidth;
		float sy = renderY + (-ndcY * 0.5f + 0.5f) * renderHeight;

		minX = std::min(minX, sx);
		minY = std::min(minY, sy);
		maxX = std::max(maxX, sx);
		maxY = std::max(maxY, sy);
	}

	// Outside
	if ( minX>maxX || minY>maxY )
		return false;

	minX = Engine::Clamp(minX, renderX, renderX+renderWidth);
	maxX = Engine::Clamp(maxX, renderX, renderX+renderWidth);
	minY = Engine::Clamp(minY, renderY, renderY+renderHeight);
	maxY = Engine::Clamp(maxY, renderY, renderY+renderHeight);
	out.min = { minX, minY };
	out.max = { maxX, maxY };
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OBB::OBB()
{
	Zero();
}

OBB& OBB::operator=(const AABB& aabb)
{
	// Bas (zmin) puis Haut (zmax), en tournant sur x/y
	const float xmin = aabb.min.x, ymin = aabb.min.y, zmin = aabb.min.z;
	const float xmax = aabb.max.x, ymax = aabb.max.y, zmax = aabb.max.z;
	pts[0] = { xmin, ymin, zmin };
	pts[1] = { xmax, ymin, zmin };
	pts[2] = { xmax, ymax, zmin };
	pts[3] = { xmin, ymax, zmin };
	pts[4] = { xmin, ymin, zmax };
	pts[5] = { xmax, ymin, zmax };
	pts[6] = { xmax, ymax, zmax };
	pts[7] = { xmin, ymax, zmax };
	return *this;
}

void OBB::Zero()
{
	for ( int i=0 ; i<8 ; i++ )
		pts[i] = { 0.0f, 0.0f, 0.0f };
}

void XM_CALLCONV OBB::Transform(DirectX::FXMMATRIX m)
{
	for ( int i=0 ; i<8 ; ++i )
	{
		DirectX::XMVECTOR p = XMLoadFloat3(&pts[i]);
		p = XMVector3TransformCoord(p, m);
		XMStoreFloat3(&pts[i], p);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MESH::MESH()
{
	Clear();
}

void MESH::Clear()
{
	triangles.clear();
	aabb.Zero();
}

void MESH::AddTriangle(DirectX::XMFLOAT3& a, DirectX::XMFLOAT3& b, DirectX::XMFLOAT3& c, DirectX::XMFLOAT3& color)
{
	TRIANGLE t;
	t.v[0].pos = a;
	t.v[0].color = color;
	t.v[1].pos = b;
	t.v[1].color = color;
	t.v[2].pos = c;
	t.v[2].color = color;
	triangles.push_back(t);
}

void MESH::AddFace(DirectX::XMFLOAT3& a, DirectX::XMFLOAT3& b, DirectX::XMFLOAT3& c, DirectX::XMFLOAT3& d, DirectX::XMFLOAT3& color)
{
	AddTriangle(a, c, b, color);
	AddTriangle(a, d, c, color);
}

void MESH::Optimize()
{
	CalculateNormals();
	CalculateBox();
}

void MESH::CalculateNormals()
{
	std::map<DirectX::XMFLOAT3, DirectX::XMVECTOR, VEC3_CMP> normalAccumulator;
	for ( TRIANGLE& t : triangles )
	{
		DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&t.v[0].pos);
		DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&t.v[1].pos);
		DirectX::XMVECTOR p2 = DirectX::XMLoadFloat3(&t.v[2].pos);

		DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(p1, p0);
		DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(p2, p0);
		DirectX::XMVECTOR faceNormal = DirectX::XMVector3Cross(edge1, edge2);
		
		if ( normalAccumulator.count(t.v[0].pos)==0 )
			normalAccumulator[t.v[0].pos] = DirectX::XMVectorZero();
		if ( normalAccumulator.count(t.v[1].pos)==0 )
			normalAccumulator[t.v[1].pos] = DirectX::XMVectorZero();
		if ( normalAccumulator.count(t.v[2].pos)==0 )
			normalAccumulator[t.v[2].pos] = DirectX::XMVectorZero();

		normalAccumulator[t.v[0].pos] = DirectX::XMVectorAdd(normalAccumulator[t.v[0].pos], faceNormal);
		normalAccumulator[t.v[1].pos] = DirectX::XMVectorAdd(normalAccumulator[t.v[1].pos], faceNormal);
		normalAccumulator[t.v[2].pos] = DirectX::XMVectorAdd(normalAccumulator[t.v[2].pos], faceNormal);
	}

	for ( TRIANGLE& t : triangles )
	{
		for ( int i=0 ; i<3 ; i++ )
		{
			DirectX::XMVECTOR sumNormal = normalAccumulator[t.v[i].pos];
			sumNormal = DirectX::XMVector3Normalize(sumNormal);
			DirectX::XMStoreFloat3(&t.v[i].normal, sumNormal);
		}
	}
}

void MESH::CalculateBox()
{
	aabb.min.x = FLT_MAX;
	aabb.min.y = FLT_MAX;
	aabb.min.z = FLT_MAX;
	aabb.max.x = -FLT_MAX;
	aabb.max.y = -FLT_MAX;
	aabb.max.z = -FLT_MAX;

	for ( TRIANGLE& t : triangles )
	{
		for ( int i=0 ; i<3 ; i++ )
		{
			if ( t.v[i].pos.x<aabb.min.x )
				aabb.min.x = t.v[i].pos.x;
			if ( t.v[i].pos.y<aabb.min.y )
				aabb.min.y = t.v[i].pos.y;
			if ( t.v[i].pos.z<aabb.min.z )
				aabb.min.z = t.v[i].pos.z;
				
			if ( t.v[i].pos.x>aabb.max.x )
				aabb.max.x = t.v[i].pos.x;
			if ( t.v[i].pos.y>aabb.max.y )
				aabb.max.y = t.v[i].pos.y;
			if ( t.v[i].pos.z>aabb.max.z )
				aabb.max.z = t.v[i].pos.z;
		}
	}	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TRANSFORM::TRANSFORM()
{
	Identity();
}

void TRANSFORM::Identity()
{
	pos = { 0.0f, 0.0f, 0.0f };
	sca = { 1.0f, 1.0f, 1.0f };
	ResetRotation();
}

void TRANSFORM::UpdateWorld()
{
	DirectX::XMVECTOR s = XMLoadFloat3(&sca);
	DirectX::XMVECTOR p = XMLoadFloat3(&pos);

	DirectX::XMVECTOR sx = DirectX::XMVectorSplatX(s);
	DirectX::XMVECTOR sy = DirectX::XMVectorSplatY(s);
	DirectX::XMVECTOR sz = DirectX::XMVectorSplatZ(s);

	DirectX::XMMATRIX w = XMLoadFloat4x4(&rot);
	w.r[0] = DirectX::XMVectorMultiply(w.r[0], sx);
	w.r[1] = DirectX::XMVectorMultiply(w.r[1], sy);
	w.r[2] = DirectX::XMVectorMultiply(w.r[2], sz);
	w.r[3] = DirectX::XMVectorSetW(p, 1.0f);

	XMStoreFloat4x4(&world, w);
}

void TRANSFORM::SetScaling(float scale)
{
	sca.x = scale;
	sca.y = scale;
	sca.z = scale;
}

void TRANSFORM::SetPosition(float x, float y, float z)
{
	pos.x = x;
	pos.y = y;
	pos.z = z;
}

void TRANSFORM::Move(float dist)
{
	pos.x += dir.x * dist;
	pos.y += dir.y * dist;
	pos.z += dir.z * dist;
}

void TRANSFORM::ResetRotation()
{
	dir = { 0.0f, 0.0f, 1.0f };
	right = { 1.0f, 0.0f, 0.0f };
	up = { 0.0f, 1.0f, 0.0f };
	quat = { 0.0f, 0.0f, 0.0f, 1.0f };
	XMStoreFloat4x4(&rot, DirectX::XMMatrixIdentity());
}

void TRANSFORM::SetRotation(TRANSFORM& transform)
{
	dir = transform.dir;
	right = transform.right;
	up = transform.up;
	quat = transform.quat;
	rot = transform.rot;
}

void TRANSFORM::SetYPR(float yaw, float pitch, float roll)
{
	ResetRotation();
	AddYPR(yaw, pitch, roll);
}

void TRANSFORM::AddYPR(float yaw, float pitch, float roll)
{
	DirectX::XMVECTOR axisDir = XMLoadFloat3(&dir);
	DirectX::XMVECTOR axisRight = XMLoadFloat3(&right);
	DirectX::XMVECTOR axisUp = XMLoadFloat3(&up);

	DirectX::XMVECTOR qRot = XMLoadFloat4(&quat);
	if ( roll )
		qRot = DirectX::XMQuaternionMultiply(qRot, DirectX::XMQuaternionRotationAxis(axisDir, roll));
	if ( pitch )
		qRot = DirectX::XMQuaternionMultiply(qRot, DirectX::XMQuaternionRotationAxis(axisRight, pitch));
	if ( yaw )
		qRot = DirectX::XMQuaternionMultiply(qRot, DirectX::XMQuaternionRotationAxis(axisUp, yaw));

	XMStoreFloat4(&quat, qRot);

	DirectX::XMMATRIX mRot = DirectX::XMMatrixRotationQuaternion(qRot);
	XMStoreFloat4x4(&rot, mRot);

	right.x = rot._11;
	right.y = rot._12;
	right.z = rot._13;
	up.x = rot._21;
	up.y = rot._22;
	up.z = rot._23;
	dir.x = rot._31;
	dir.y = rot._32;
	dir.z = rot._33;
}

void TRANSFORM::LookAt(float x, float y, float z)
{
	DirectX::XMVECTOR eyePos = XMLoadFloat3(&pos);
	DirectX::XMVECTOR focusPos = DirectX::XMVectorSet(x, y, z, 0.0f);
	DirectX::XMVECTOR upDir = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX cam = DirectX::XMMatrixInverse(nullptr, DirectX::XMMatrixLookAtLH(eyePos, focusPos, upDir));
	XMStoreFloat4x4(&rot, cam);
	XMStoreFloat4(&quat, DirectX::XMQuaternionRotationMatrix(cam));

	right.x = rot._11;
	right.y = rot._12;
	right.z = rot._13;
	up.x = rot._21;
	up.y = rot._22;
	up.z = rot._23;
	dir.x = rot._31;
	dir.y = rot._32;
	dir.z = rot._33;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ENTITY::ENTITY()
{
	index = -1;
	sortedIndex = -1;
	dead = false;
	pMesh = nullptr;
	material = Engine::ToColor(255, 255, 255);
	lifetime = 0.0f;
	tile = 0;
}
