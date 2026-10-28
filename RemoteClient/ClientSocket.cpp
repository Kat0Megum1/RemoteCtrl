#include "pch.h"
#include "ClientSocket.h"

struct PacketData {
	std::string strData;
	UINT nMode;
	WPARAM wParam;
};

unsigned __stdcall CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

void CClientSocket::threadFunc2()
{
	MSG msg;
	// 先取出一条空消息，让本线程建立消息队列，之后 PostThreadMessage 才不会丢
	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
	SetEvent(m_eventInvoke);
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
		if (it != m_mapFunc.end())
		{
			(this->*it->second)(msg.message, msg.wParam, msg.lParam);
		}
	}
}

bool CClientSocket::InitSocket()
{
	return InitSocket(m_nIP, m_nPort);
}

bool CClientSocket::SendPacket(HWND hWnd, CPacket pack, bool isAutoClosed, WPARAM wParam)
{
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	PacketData* pData = new PacketData();
	pData->strData.assign(pack.Data(), pack.Size());
	pData->nMode = nMode;
	pData->wParam = wParam;
	if (PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd) == FALSE)
	{
		delete pData;
		return false;
	}
	return true;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(nMsg);
	PacketData data = *(PacketData*)wParam;
	delete (PacketData*)wParam;
	HWND hWnd = (HWND)lParam;

	if (InitSocket() == false)
	{
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
		return;
	}
	int ret = send(m_sock, data.strData.c_str(), (int)data.strData.size(), 0);
	if (ret <= 0)
	{
		CloseSocket();
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		return;
	}

	size_t index = 0;
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = &strBuffer[0];
	while (m_sock != INVALID_SOCKET)
	{
		int length = recv(m_sock, pBuffer + index, (int)(BUFFER_SIZE - index), 0);
		// length <= 0 表示对方断开或出错。不能把负数加进 index，否则下标会变成极大值
		if (length > 0)
		{
			index += (size_t)length;
		}
		else if (index == 0)
		{
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
			return;
		}

		while (index > 0)
		{
			size_t nLen = index;
			CPacket pack((BYTE*)pBuffer, nLen);
			if (nLen == 0)
				break;
			// 应答包由窗口过程 delete。窗口已经不在时，这里自己删，避免泄漏
			CPacket* pAck = new CPacket(pack);
			if (hWnd != NULL && ::IsWindow(hWnd))
				::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)pAck, data.wParam);
			else
				delete pAck;
			if (data.nMode & CSM_AUTOCLOSE)
			{
				CloseSocket();
				return;
			}
			index -= nLen;
			memmove(pBuffer, pBuffer + nLen, index);
		}
		if (length <= 0)
		{
			CloseSocket();
			if (index > 0)
				::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
			return;
		}
	}
}

CClientSocket* CClientSocket::m_instance = NULL;

CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance();

std::string GetErrorInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}
