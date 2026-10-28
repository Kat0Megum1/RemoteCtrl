// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include <list>
#include <atlimage.h>
#include "LockDialog.h"
#include "EdoyunTool.h"
#include <cstdio>
#include <conio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "Command.h"
#include "CEdoyunQueue.h"
#include "EdoyunServer.h"
#include <string>
#include <cstring>


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

// 替换 #define INVOKE_PATH
CString GetInvokePath()
{
	TCHAR buf[MAX_PATH] = _T("");
	GetEnvironmentVariable(_T("APPDATA"), buf, MAX_PATH);   // 当前用户的 AppData\Roaming
	CString str = buf;
	str += _T("\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe");
	return str;
}

bool ChooseAutoInvoke(const CString& strPath) {
	if (PathFileExists(strPath)) {          // 启动文件夹里已经有一份，直接算成功
		return true;
	}
	CString strInfo = _T("该程序只允许用于合法的用途！\n");
	strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
	strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
	strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
	strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES) {
		if (!CEdoyunTool::WriteStartupDir(strPath)) {
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (ret == IDCANCEL) {
		return false;                        // 取消 = 退出程序
	}
	return true;                             // 否 = 只运行这一次
}

void initsock()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}

void clearsock()
{
	WSACleanup();
}

static bool SamePeer(const sockaddr_in& a, const sockaddr_in& b)
{
	return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

class UdpHoleServer
{
public:
	UdpHoleServer()
	{
		m_sock = INVALID_SOCKET;
		m_introduced = false;
	}
	~UdpHoleServer()
	{
		if (m_sock != INVALID_SOCKET)
			closesocket(m_sock);
	}
	bool Start()
	{
		m_sock = socket(PF_INET, SOCK_DGRAM, 0);
		if (m_sock == INVALID_SOCKET)
		{
			printf("udp_server socket error %d\r\n", WSAGetLastError());
			return false;
		}
		sockaddr_in server = {};
		server.sin_family = AF_INET;
		server.sin_port = htons(20000);
		server.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(m_sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
		{
			printf("udp_server bind error %d\r\n", WSAGetLastError());
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		printf("udp server 0.0.0.0:20000\r\n");
		return true;
	}
	void Run()
	{
		while (!_kbhit())
		{
			if (!WaitPacket())
				continue;
			sockaddr_in peer = {};
			if (!Receive(peer))
				break;
			Remember(peer);
			Introduce();
		}
	}
private:
	bool WaitPacket()
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_sock, &fds);
		timeval wait = {};
		wait.tv_usec = 200000;
		return select(0, &fds, NULL, NULL, &wait) > 0;
	}
	bool Receive(sockaddr_in& peer)
	{
		int peerLen = sizeof(peer);
		char buf[4096];
		int ret = recvfrom(m_sock, buf, sizeof(buf), 0, (sockaddr*)&peer, &peerLen);
		if (ret <= 0)
		{
			printf("recvfrom error %d\r\n", WSAGetLastError());
			return false;
		}
		printf("from %s:%d\r\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
		return true;
	}
	void Remember(const sockaddr_in& peer)
	{
		for (std::list<sockaddr_in>::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
		{
			if (SamePeer(*it, peer))
				return;
		}
		m_clients.push_back(peer);
	}
	void Introduce()
	{
		if (m_introduced || m_clients.size() < 2)
			return;
		std::list<sockaddr_in>::iterator it = m_clients.begin();
		sockaddr_in first = *it;
		++it;
		sockaddr_in second = *it;
		sendto(m_sock, (char*)&second, sizeof(second), 0, (sockaddr*)&first, sizeof(first));
		sendto(m_sock, (char*)&first, sizeof(first), 0, (sockaddr*)&second, sizeof(second));
		m_introduced = true;
		printf("introduced\r\n");
	}
	SOCKET m_sock;
	std::list<sockaddr_in> m_clients;
	bool m_introduced;
};

void udp_server()
{
	UdpHoleServer server;
	if (server.Start())
		server.Run();
}

void udp_client(bool ishost)
{
	Sleep(2000);
	sockaddr_in server = {};
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("udp_client socket error %d\r\n", WSAGetLastError());
		return;
	}
	const char* name = ishost ? "host" : "slave";
	int ret = sendto(sock, name, (int)strlen(name), 0, (sockaddr*)&server, sizeof(server));
	printf("%s sendto ret = %d\r\n", name, ret);
	if (ret > 0)
	{
		char reply[4096];
		sockaddr_in from = {};
		int fromLen = sizeof(from);
		ret = recvfrom(sock, reply, sizeof(reply), 0, (sockaddr*)&from, &fromLen);
		if (ret == sizeof(sockaddr_in))
		{
			sockaddr_in peer = {};
			memcpy(&peer, reply, sizeof(peer));
			printf("%s peer %s:%d\r\n", name, inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
			const char* ping = "ping";
			sendto(sock, ping, 4, 0, (sockaddr*)&peer, sizeof(peer));
			fromLen = sizeof(from);
			ret = recvfrom(sock, reply, sizeof(reply) - 1, 0, (sockaddr*)&from, &fromLen);
			if (ret > 0)
			{
				reply[ret] = '\0';
				printf("%s got %s from %s:%d\r\n", name, reply, inet_ntoa(from.sin_addr), ntohs(from.sin_port));
			}
		}
	}
	closesocket(sock);
}

static bool LaunchUdpRole(const char* exe, const char* role)
{
	std::string cmd = "\"";
	cmd += exe;
	cmd += "\" udp ";
	cmd += role;
	std::vector<char> buf(cmd.begin(), cmd.end());
	buf.push_back('\0');
	STARTUPINFOA si = {};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};
	if (!CreateProcessA(NULL, buf.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		return false;
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return true;
}

int main(int argc, char* argv[])
{
	if (!CEdoyunTool::Init())
		return 1;
	if (argc >= 2 && strcmp(argv[1], "udp") == 0)
	{
		initsock();
		if (argc == 2)
		{
			LaunchUdpRole(argv[0], "host");
			LaunchUdpRole(argv[0], "slave");
			udp_server();
		}
		else if (strcmp(argv[2], "host") == 0)
			udp_client(true);
		else
			udp_client(false);
		clearsock();
		return 0;
	}
	EdoyunServer server;
	if (!server.StartServer())
		return 1;
	Sleep(INFINITE);
	return 0;
}
