#pragma once
#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	static CServerSocket* getInstance()
	{
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	bool InitSocket()
	{
		if (m_sock == -1)return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(8888);
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		{
			return false;
		}
		if (listen(m_sock, 1) == -1)
		{
			return false;
		}
		return true;
	}

	bool AcceptClient()
	{
		sockaddr_in client_addr;
		int cli_sz = sizeof(client_addr);
		SOCKET m_client = accept(m_sock, (sockaddr*)&client_addr, &cli_sz);
		if (m_client == -1)return false;
		return true;
	}

	int DealCommand()
	{
		if (m_client == -1)return false;
		char buffer[1024] = "";
		while (true)
		{
			int len = recv(m_sock, buffer, sizeof(buffer), 0);
			if (len <= 0)
			{
				return -1;
			}
		}
	}

	bool Send(const char* pData, int nSize)
	{
		if (m_client == -1)return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
private:
	SOCKET m_sock;
	SOCKET m_client;

	CServerSocket& operator=(const CServerSocket&)
	{
		
	};
	CServerSocket(const CServerSocket&ss)
	{
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	};
	CServerSocket() 
	{
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ����������������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock= socket(PF_INET, SOCK_STREAM, 0);
	};
	~CServerSocket() 
	{
		closesocket(m_sock);
		WSACleanup();
	};
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}

	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	static CServerSocket* m_instance;

	class CHelper
	{
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

extern CServerSocket server;

