#include "pch.h"
#include "EdoyunServer.h"
#include "Command.h"

bool EdoyunServer::CreateSocket()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_sock == INVALID_SOCKET)
		return false;
	int opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	return true;
}

bool EdoyunServer::StartServer()
{
	if (!CreateSocket())
		return false;
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	if (listen(m_sock, 3) == -1)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
	if (m_hIOCP == NULL)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	if (CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0) == NULL)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		CloseHandle(m_hIOCP);
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}

	m_hThread = (HANDLE)_beginthreadex(NULL, 0, threadEntry, this, 0, NULL);
	if (m_hThread == NULL)
		return false;
	return NewAccept();
}

EdoyunServer::~EdoyunServer()
{
	m_stop = true;
	if (m_sock != INVALID_SOCKET)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	if (m_hIOCP != INVALID_HANDLE_VALUE)
		PostQueuedCompletionStatus(m_hIOCP, 0, 0, NULL);
	if (m_hThread != NULL)
	{
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	if (m_hIOCP != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hIOCP);
		m_hIOCP = INVALID_HANDLE_VALUE;
	}
	for (EdoyunClient* client : m_clients)
		delete client;
	m_clients.clear();
}

void EdoyunServer::ReleaseClient(EdoyunClient* client)
{
	if (client == NULL)
		return;
	if (client->m_sock != INVALID_SOCKET)
	{
		closesocket(client->m_sock);
		client->m_sock = INVALID_SOCKET;
	}
	m_clients.remove(client);
	delete client;
}

bool EdoyunServer::NewAccept()
{
	if (m_stop)
		return false;
	EdoyunClient* pClient = new EdoyunClient();
	if (pClient->m_sock == INVALID_SOCKET)
	{
		delete pClient;
		return false;
	}
	if (!AcceptEx(m_sock,
		pClient->m_sock,
		pClient->m_accept.m_buffer.data(),
		0,
		(DWORD)(sizeof(sockaddr_in) + 16),
		(DWORD)(sizeof(sockaddr_in) + 16),
		&pClient->m_received,
		&pClient->m_accept.m_overlapped
	))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			delete pClient;
			return false;
		}
	}
	m_clients.push_back(pClient);
	return true;
}



int EdoyunServer::threadIocp()
{
	DWORD transferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	while (true)
	{
		BOOL ok = GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE);
		if (lpOverlapped == NULL || CompletionKey == 0)
			return -1;
		EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
		pOverlapped->m_server = this;
		EdoyunClient* client = pOverlapped->m_client;
		if (!ok)
		{
			if (pOverlapped->m_operator == EAccept)
				NewAccept();
			ReleaseClient(client);
			continue;
		}
		switch (pOverlapped->m_operator)
		{
		case EAccept:
		{
			if (client == NULL)
			{
				NewAccept();
				break;
			}
			setsockopt(client->m_sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_sock, sizeof(m_sock));
			sockaddr* localAddr = NULL;
			sockaddr* remoteAddr = NULL;
			int localLen = 0;
			int remoteLen = 0;
			GetAcceptExSockaddrs(
				client->m_accept.m_buffer.data(),
				0,
				(DWORD)(sizeof(sockaddr_in) + 16),
				(DWORD)(sizeof(sockaddr_in) + 16),
				&localAddr, &localLen,
				&remoteAddr, &remoteLen);
			if (localAddr != NULL)
				memcpy(&client->m_laddr, localAddr, sizeof(sockaddr_in));
			if (remoteAddr != NULL)
				memcpy(&client->m_raddr, remoteAddr, sizeof(sockaddr_in));
			if (CreateIoCompletionPort((HANDLE)client->m_sock, m_hIOCP, (ULONG_PTR)this, 0) == NULL
				|| !client->PostRecv())
				ReleaseClient(client);
			NewAccept();
			break;
		}
		case ERecv:
		{
			if (client == NULL)
				break;
			if (transferred == 0)
			{
				ReleaseClient(client);
				break;
			}
			client->AppendRecv(client->m_recv.m_buffer.data(), transferred);
			CPacket pack;
			bool gotPacket = false;
			while (client->TakePacket(pack))
			{
				gotPacket = true;
				client->recvPackets.push_back(pack);
				if (pack.sCmd > 0)
				{
					static CCommand cmd;
					cmd.ExcuteCommand(pack.sCmd, client->sendPackets, pack);
				}
			}
			if (!gotPacket)
				client->PostRecv();
			else if (!client->m_sending)
				client->PostSend();
			if (client->m_sock == INVALID_SOCKET)
				ReleaseClient(client);
			break;
		}
		case ESend:
		{
			if (client == NULL)
				break;
			if (transferred == 0)
			{
				client->m_sending = false;
				ReleaseClient(client);
				break;
			}
			client->m_sendOffset += transferred;
			if (!client->sendPackets.empty() && client->m_sendOffset >= (size_t)client->sendPackets.front().Size())
			{
				client->sendPackets.pop_front();
				client->m_sendOffset = 0;
			}
			client->m_sending = false;
			client->PostSend();
			if (client->m_sock == INVALID_SOCKET)
				ReleaseClient(client);
			break;
		}
		}
	}
	return 0;
}

bool EdoyunClient::PostRecv()
{
	if (m_sock == INVALID_SOCKET)
		return false;
	memset(&m_recv.m_overlapped, 0, sizeof(m_recv.m_overlapped));
	m_recv.m_wsabuffer.buf = m_recv.m_buffer.data();
	m_recv.m_wsabuffer.len = (ULONG)m_recv.m_buffer.size();
	m_flags = 0;
	m_received = 0;
	int ret = WSARecv(m_sock, &m_recv.m_wsabuffer, 1, &m_received, &m_flags, &m_recv.m_overlapped, NULL);
	if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		return false;
	return true;
}

void EdoyunClient::AppendRecv(const char* data, DWORD len)
{
	if (m_used + len > m_buffer.size())
		m_buffer.resize(m_used + len);
	memcpy(m_buffer.data() + m_used, data, len);
	m_used += len;
}

bool EdoyunClient::PostSend()
{
	if (m_sock == INVALID_SOCKET)
		return false;
	if (sendPackets.empty())
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_sending = false;
		return true;
	}
	CPacket& pack = sendPackets.front();
	int packetSize = pack.Size();
	if (packetSize <= 0)
	{
		sendPackets.pop_front();
		m_sendOffset = 0;
		return PostSend();
	}
	if (m_sendOffset == 0)
	{
		const char* data = pack.Data();
		m_send.m_buffer.resize((size_t)packetSize);
		memcpy(m_send.m_buffer.data(), data, (size_t)packetSize);
	}
	DWORD len = (DWORD)((size_t)packetSize - m_sendOffset);
	memset(&m_send.m_overlapped, 0, sizeof(m_send.m_overlapped));
	m_send.m_wsabuffer.buf = m_send.m_buffer.data() + m_sendOffset;
	m_send.m_wsabuffer.len = len;
	DWORD sent = 0;
	int ret = WSASend(m_sock, &m_send.m_wsabuffer, 1, &sent, 0, &m_send.m_overlapped, NULL);
	if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_sending = false;
		return false;
	}
	m_sending = true;
	return true;
}

bool EdoyunClient::TakePacket(CPacket& pack)
{
	if (m_used < 8)
		return false;
	size_t nSize = m_used;
	pack = CPacket((BYTE*)m_buffer.data(), nSize);
	if (nSize == 0)
		return false;
	memmove(m_buffer.data(), m_buffer.data() + nSize, m_used - nSize);
	m_used -= nSize;
	return true;
}
