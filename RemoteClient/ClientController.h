#pragma once
#include "RemoteClientDlg.h"
#include "CWatchDialog.h"
#include "StatusDlg.h"

// 业务逻辑和流程随时可能改变，因此单独放在控制层

class CClientController
{
public:
	static CClientController* getInstance();

	// 初始化控制层（创建状态窗等）
	int InitController();
	// 启动主界面
	int Invoke(CWnd*& pMainWnd);

	// 下载结束 UI（隐藏状态窗 / 提示）
	void DownloadEnd();
	void DownloadAbort();
	bool IsDownloading() const { return m_pDownFile != NULL; }
	// 下载文件（另存为 + 发命令 + 写本地）
	int DownFile(CString strPath);
	// 打开监视对话框，并启动截屏请求线程
	void StartWatchScreen();

	// 旧的同步收发。界面不要再调用，否则会和网络线程抢同一个套接字
	int SendCommandPacket(int nCmd, bool bAutoClose = TRUE,
		BYTE* pData = NULL, size_t nLength = 0);
	// 异步发送：网络线程收发，结果用 WM_SEND_PACK_ACK 回到 hWnd
	bool SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose = true,
		BYTE* pData = NULL, size_t nLength = 0, WPARAM wParam = 0);

	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}
	CPacket& GetPacket() {
		return CClientSocket::getInstance()->GetPacket();
	}

	CRemoteClientDlg& GetRemoteDlg() { return m_remoteDlg; }
	CWatchDialog& GetWatchDlg() { return m_watchDlg; }
	CStatusDlg& GetStatusDlg() { return m_statusDlg; }

protected:
	CClientController();
	~CClientController();

	static void releaseInstance();
	void threadWatchScreen();
	static void threadWatchScreenEntry(void* arg);

private:
	// 声明顺序：先主对话框，再依赖它作父窗口的子对话框
	CRemoteClientDlg m_remoteDlg;
	CWatchDialog m_watchDlg;
	CStatusDlg m_statusDlg;

	bool m_isClosed;
	HANDLE m_hThreadWatch;
	CString m_strRemote;
	CString m_strLocal;
	FILE* m_pDownFile;

	static CClientController* m_instance;

	class CHelper
	{
	public:
		~CHelper() { CClientController::releaseInstance(); }
	};
	static CHelper m_helper;
};
