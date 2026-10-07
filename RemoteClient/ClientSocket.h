#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>

#pragma pack(push)
#pragma pack(1)

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0)
	{

	};

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else
		{
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	CPacket(const CPacket& packet)
	{
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSum = packet.sSum;
	}

	CPacket& operator=(const CPacket& packet)
	{
		if (this != &packet)
		{
			sHead = packet.sHead;
			nLength = packet.nLength;
			sCmd = packet.sCmd;
			strData = packet.strData;
			sSum = packet.sSum;
		}
		return *this;
	}

	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize)//�����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ�
		{
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize)//��δ��ȫ���յ����ͷ��أ�����ʧ��
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum)
		{
			nSize = i;
			return;
		}
		nSize = 0;
	};

	int Size()
	{
		return nLength + 6;
	}

	const char* Data()
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	~CPacket()
	{

	};
public:
	WORD sHead;//��ͷ
	DWORD nLength;//����
	WORD sCmd;//����
	std::string strData;//����
	WORD sSum;//��У��
	std::string strOut;
};
#pragma pack(pop)

typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//������ƶ���˫��
	WORD nButton;//������Ҽ����м�
	POINT ptXY;//����
}MOUSEEV, * PMOUSEEV;

typedef struct file_info
{
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;
	BOOL IsDirectory;
	BOOL HasNext;
	char szFileName[256];
}FILEINFO, * PFILEINFO;

std::string GetErrorInfo(int wsaErrCode);

class CClientSocket
{
public:
	static CClientSocket* getInstance()
	{
		if (m_instance == NULL)
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}

	bool InitSocket(int nIP,int nPort)
	{
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == INVALID_SOCKET) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(nIP);
		serv_addr.sin_port = htons(nPort);
		if (serv_addr.sin_addr.s_addr == INADDR_NONE)
		{
			AfxMessageBox("ָ指定的IP地址不存在!");
			return false;
		}
		int result = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (result == SOCKET_ERROR)
		{
			int nErr = WSAGetLastError();
			AfxMessageBox("连接失败!");
			TRACE("连接失败:%d %s\r\n", nErr, GetErrorInfo(nErr).c_str());
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
#define BUFFER_SIZE 4096000
	int DealCommand()
	{
		if (m_sock == INVALID_SOCKET)return -1;
		char* buffer = m_buffer.data();
		while (true)
		{
			//先尝试从缓冲区已有数据解析一个完整包
			//成功时 parseSize 为消耗的字节数；为 0 表示包不完整/校验失败
			size_t parseSize = m_bufLen;
			CPacket packet((BYTE*)buffer, parseSize);
			if (parseSize > 0)
			{
				m_packet = packet;
				//移走已消费的字节，剩余的（同一次 recv 粘来的后续包）留下次解析
				m_bufLen -= parseSize;
				memmove(buffer, buffer + parseSize, m_bufLen);
				return m_packet.sCmd;
			}
			//包不完整，继续 recv 追加到缓冲区末尾
			if (m_bufLen >= BUFFER_SIZE)
			{
				m_bufLen = 0;//缓冲满仍解析不出，丢弃脏数据避免死循环
				return -1;
			}
			int len = recv(m_sock, buffer + m_bufLen, (int)(BUFFER_SIZE - m_bufLen), 0);
			if (len <= 0)
			{
				return -1;
			}
			m_bufLen += len;
		}
	}

	bool Send(const char* pData, int nSize)
	{
		if (m_sock == -1)return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& packet)
	{
		if (m_sock == -1)return false;
		return send(m_sock, packet.Data(), packet.Size(), 0);
	}

	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	CPacket& GetPacket()
	{
		return m_packet;
	}

	void CloseSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_bufLen = 0;
	}
private:
	SOCKET m_sock;
	CPacket m_packet;
	std::vector<char>m_buffer;
	size_t m_bufLen = 0;//缓冲区中尚未解析的有效字节数

	CClientSocket& operator=(const CClientSocket&)
	{

	};
	CClientSocket(const CClientSocket& ss)
	{
		m_sock = ss.m_sock;
	};
	CClientSocket()
	{
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL,
				_T("无法初始化套接字环境，初始化错误!"),
				_T("初始化失败!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
	};
	~CClientSocket()
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
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	static CClientSocket* m_instance;

	class CHelper
	{
	public:
		CHelper()
		{
			CClientSocket::getInstance();
		}
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

extern CClientSocket client;

