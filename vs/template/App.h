#pragma once

class Entity;

class App
{
public:
	App();
	virtual ~App();

	static App& GetInstance() { return *s_pApp; }

	static Entity* CreateEntity();

	virtual void OnStart();
	virtual void OnUpdate();
	virtual void OnExit();
	virtual void OnRender(int pass);

	static void MyPixelShader(cpu_ps_io& io);

protected:
	inline static App* s_pApp = nullptr;

	std::vector<Entity*> m_Entites;

	friend class Server;
	friend class Client;
};

//SERVER APP

class ServerApp : public App {
public:
	ServerApp() : App() {};
	virtual ~ServerApp();

	void OnStart();
	void OnUpdate();
	void OnExit();
	void OnRender(int pass);

private:

	friend class Server;
};

//CLIENT APP

class ClientApp : public App {
public:
	ClientApp() : App() {};
	virtual ~ClientApp();

	void OnStart();
	void OnUpdate();
	void OnExit();
	void OnRender(int pass);

private:

	friend class Client;
};