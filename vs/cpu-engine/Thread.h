#pragma once

class Thread
{
public:
	using _METHOD = std::function<void()>;

public:
	Thread();
	virtual ~Thread();

	bool Run();
	bool Run(const _METHOD& callback);
	void Wait();
	void Stop();
	void PostQuit();

	virtual void OnPrepare() {}
	virtual void OnStart() {}
	void SetCallback(const _METHOD& callback) { m_callback = callback; }
	bool HasCallback() { return m_callback!=nullptr; }
	void OnCallback() { m_callback(); }

	DWORD GetParentID() const { return m_idThreadParent; }
	DWORD GetID() const { return m_idThread; }
	bool IsRunning() const { return m_idThread!=0; }
	bool IsStopping() const { return m_stopping; }

private:
	void Reset();

protected:
	static DWORD WINAPI ThreadProc(void* pParam);

protected:
	HANDLE m_hThread;
	DWORD m_idThread;
	DWORD m_idThreadParent;
	bool m_stopping;
	_METHOD m_callback;
};
