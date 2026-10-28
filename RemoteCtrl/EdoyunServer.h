#pragma once
#include <vector>
#include <map>
#include <list>
#include <MSWSock.h>
#include "Packet.h"
enum EdoyunOperator
{
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class EdoyunServer;
class EdoyunClient;

class EdoyunOverlapped
{
public:
	EdoyunOverlapped()
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_client = NULL;
		m_server = NULL;
	}

	OVERLAPPED m_overlapped;
	DWORD m_operator;
	std::vector<char>m_buffer;
	WSABUF m_wsabuffer;
	EdoyunClient* m_client;
	EdoyunServer* m_server;
};

class AcceptOverlapped :public EdoyunOverlapped
{
public:
	AcceptOverlapped()
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_operator = EAccept;
		m_buffer.resize(1024);
		m_server = NULL;
	}
};

class RecvOverlapped :public EdoyunOverlapped
{
public:
	RecvOverlapped()
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_operator = ERecv;
		m_buffer.resize(4096);
	}
};

class SendOverlapped :public EdoyunOverlapped
{
public:
	SendOverlapped()
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_operator = ESend;
		m_buffer.resize(1024);
	}
};

class EdoyunClient
{
public:
	EdoyunClient()
	{
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		m_received = 0;
		m_flags = 0;
		m_used = 0;
		m_sendOffset = 0;
		m_sending = false;
		m_buffer.resize(4096);
		memset(&m_laddr, 0, sizeof(m_laddr));
		memset(&m_raddr, 0, sizeof(m_raddr));
		m_accept.m_client = this;
		m_recv.m_client = this;
		m_send.m_client = this;
	}
	~EdoyunClient()
	{
		if (m_sock != INVALID_SOCKET)
			closesocket(m_sock);
	}
	bool PostRecv();
	bool PostSend();
	void AppendRecv(const char* data, DWORD len);
	bool TakePacket(CPacket& pack);
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	AcceptOverlapped m_accept;
	RecvOverlapped m_recv;
	SendOverlapped m_send;
	std::vector<char> m_buffer;
	size_t m_used;
	size_t m_sendOffset;
	bool m_sending;
	std::list<CPacket> recvPackets;
	std::list<CPacket> sendPackets;
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
};

class EdoyunServer
{
public:
	EdoyunServer(const std::string& ip = "0.0.0.0", short port = 8888)
	{
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_addr.sin_family = AF_INET;
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
		m_addr.sin_port = htons(port);
		m_hThread = NULL;
		m_stop = false;
	}
	~EdoyunServer();

	bool CreateSocket();
	bool StartServer();
	bool NewAccept();
	int threadIocp();
	static unsigned __stdcall threadEntry(void* arg)
	{
		EdoyunServer* thiz = (EdoyunServer*)arg;
		thiz->threadIocp();
		return 0;
	}
private:
	void ReleaseClient(EdoyunClient* client);
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	HANDLE m_hThread;
	std::list<EdoyunClient*> m_clients;
	bool m_stop;
};
