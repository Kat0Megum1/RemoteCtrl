#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

#pragma pack(push)
#pragma pack(1)



typedef void(*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket);

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

	bool InitSocket(short port)
	{
		if (m_sock == INVALID_SOCKET)return false;
		//允许端口在TIME_WAIT状态下重新绑定，否则调试重启时bind会失败(10048)
		BOOL bReuse = TRUE;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&bReuse, sizeof(bReuse));
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		{
			TRACE("bind失败，错误码：%d\r\n", WSAGetLastError());
			return false;
		}
		if (listen(m_sock, 1) == SOCKET_ERROR)
		{
			TRACE("listen失败，错误码：%d\r\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool AcceptClient()
	{
		sockaddr_in client_addr;
		int cli_sz = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &cli_sz);
		TRACE("m_client =%d\r\n", m_client);
		if (m_client == -1)return false;
		return true;
	}
#define BUFFER_SIZE 4096
	int DealCommand()
	{
		if (m_client == -1)return false;
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL)
		{
			TRACE("内存不足!\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0)
			{
				delete[] buffer;
				return -1;
			}
			TRACE("recv %d\r\n", len);
			index += len;
			// 解析必须看缓冲区里已经攒下的全部字节，不能只用本次 recv 的长度
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1;
	}

	bool Send(const char* pData, int nSize)
	{
		if (m_client == -1)return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& packet)
	{
		if (m_client == -1)return false;
		return send(m_client, packet.Data(), packet.Size(), 0);
	}

	CPacket& GetPacket()
	{
		return m_packet;
	}

	void CloseClient()
	{
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 8888)
	{
		if (InitSocket(port) == false) return -1;
		std::list<CPacket> lstPackets;
		int count = 0;
		while (true) {
			if (AcceptClient() == false) {
				if (count >= 3) return -2;
				count++;
				continue;
			}
			count = 0;
			int ret = DealCommand();
			if (ret > 0) {
				callback(arg, ret, lstPackets, m_packet);
				while (!lstPackets.empty()) {
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}
		return 0;
	}

private:
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;

	CServerSocket& operator=(const CServerSocket&)
	{

	};
	CServerSocket(const CServerSocket& ss)
	{
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	};
	CServerSocket()
	{
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL,
				_T("无法初始化套接字环境，初始化错误!"),
				_T("初始化失败!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
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