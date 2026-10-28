#include "pch.h"
#include "ClientController.h"
#include "resource.h"

CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

CClientController::CClientController()
	: m_watchDlg(&m_remoteDlg)
	, m_statusDlg(&m_remoteDlg)
	, m_isClosed(true)
	, m_hThreadWatch(INVALID_HANDLE_VALUE)
	, m_pDownFile(NULL)
{
}

CClientController::~CClientController()
{
	if (m_hThreadWatch != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(m_hThreadWatch, 100);
		m_hThreadWatch = INVALID_HANDLE_VALUE;
	}
}

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL)
	{
		m_instance = new CClientController();
	}
	return m_instance;
}

void CClientController::releaseInstance()
{
	if (m_instance != NULL)
	{
		delete m_instance;
		m_instance = NULL;
	}
}

int CClientController::InitController()
{
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	m_statusDlg.ShowWindow(SW_HIDE);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return (int)m_remoteDlg.DoModal();
}

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	m_remoteDlg.UpdateData(TRUE);
	CClientSocket::getInstance()->UpdateAddress(
		m_remoteDlg.m_server_address,
		atoi((LPCTSTR)m_remoteDlg.m_nPort));
	return CClientSocket::getInstance()->SendPacket(
		hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
}

int CClientController::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	m_remoteDlg.UpdateData(TRUE);
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(
		m_remoteDlg.m_server_address,
		atoi((LPCTSTR)m_remoteDlg.m_nPort));
	if (!ret)
	{
		AfxMessageBox(_T("network init failed"));
		return -1;
	}
	CPacket packet(nCmd, pData, nLength);
	ret = pClient->Send(packet);
	TRACE("Send ret %d\r\n", ret);
	int cmd = pClient->DealCommand();
	TRACE("ack:%d\r\n", cmd);
	if (bAutoClose)
	{
		pClient->CloseSocket();
	}
	return cmd;
}

void CClientController::DownloadEnd()
{
	if (m_pDownFile != NULL)
	{
		fclose(m_pDownFile);
		m_pDownFile = NULL;
	}
	m_remoteDlg.ResetDownloadProgress();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("download complete"), _T("done"));
}

void CClientController::DownloadAbort()
{
	if (m_pDownFile != NULL)
	{
		fclose(m_pDownFile);
		m_pDownFile = NULL;
	}
	m_remoteDlg.ResetDownloadProgress();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("download failed"), _T("failed"));
}

int CClientController::DownFile(CString strPath)
{
	CString strName = strPath;
	int pos = strName.ReverseFind(_T('\\'));
	if (pos < 0) pos = strName.ReverseFind(_T('/'));
	if (pos >= 0) strName = strName.Mid(pos + 1);

	CFileDialog dlg(FALSE, NULL, strName,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
	if (dlg.DoModal() != IDOK)
		return 0;

	m_strRemote = strPath;
	m_strLocal = dlg.GetPathName();

	FILE* pFile = NULL;
	if (fopen_s(&pFile, (LPCSTR)m_strLocal, "wb+") != 0 || pFile == NULL)
	{
		AfxMessageBox(_T("cannot create local file"));
		return -1;
	}

	m_remoteDlg.BeginWaitCursor();
	m_statusDlg.m_info.SetWindowText(_T("command is running"));
	m_statusDlg.ShowWindow(SW_SHOW);
	m_statusDlg.CenterWindow(&m_remoteDlg);
	m_statusDlg.SetActiveWindow();

	m_pDownFile = pFile;
	m_remoteDlg.ResetDownloadProgress();
	// 和截图、列目录一样，只投进网络线程，避免两套代码同时用一个套接字
	if (!SendCommandPacket(m_remoteDlg.GetSafeHwnd(), 4, false,
		(BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile))
	{
		DownloadAbort();
		return -1;
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	m_watchDlg.SetImageStatus(false);
	m_remoteDlg.UpdateData(TRUE);
	CClientSocket::getInstance()->UpdateAddress(
		m_remoteDlg.m_server_address,
		atoi((LPCTSTR)m_remoteDlg.m_nPort));
	m_hThreadWatch = (HANDLE)_beginthread(
		&CClientController::threadWatchScreenEntry, 0, this);
	m_watchDlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);
	m_hThreadWatch = INVALID_HANDLE_VALUE;
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	while (!m_isClosed)
	{
		// 上一张还没画完就先不发，避免网络线程里堆一串截图
		if (m_watchDlg.isFull() == false)
		{
			m_watchDlg.SetImageStatus(true);
			bool ret = CClientSocket::getInstance()->SendPacket(
				m_watchDlg.GetSafeHwnd(), CPacket(6, NULL, 0), true);
			if (!ret)
			{
				m_watchDlg.SetImageStatus(false);
				Sleep(200);
			}
		}
		else
		{
			Sleep(1);
		}
	}
	TRACE("watch thread end, m_isClosed=%d\r\n", m_isClosed);
}

void CClientController::threadWatchScreenEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}
