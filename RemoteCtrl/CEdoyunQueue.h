#pragma once
#include "pch.h"
#include <list>
template <class T>
class CEdoyunQueue
{
public:
	enum
	{
		EQNone,
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};
	typedef struct IocpParam
	{
		size_t nOperator;
		T Data;
		HANDLE hEvent;
		IocpParam(int op, const T& data, HANDLE hEve = NULL)
		{
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam()
		{
			nOperator = EQNone;
		}
	}PPARAM;
public:
	CEdoyunQueue()
	{
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL)
		{
			m_hThread = (HANDLE)_beginthreadex(
				NULL,
				0,
				&CEdoyunQueue<T>::threadEntry,
				this,
				0,
				NULL);
		}
	}
	virtual ~CEdoyunQueue()
	{
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
		if (m_hCompletionPort != NULL)
		{
			HANDLE temp = m_hCompletionPort;
			m_hCompletionPort = NULL;
			CloseHandle(temp);
		}
	}
	bool PushBack(const T& data)
	{
		IocpParam* pParam = new IocpParam(EQPush, data);
		if (m_lock)
		{
			delete pParam;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		return ret;
	}
	bool PopFront(T& data)
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam pParam(EQPop, data, hEvent);
		if (m_lock == true)
		{
			if (hEvent)CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)&pParam, NULL);
		if (ret == false)
		{
			CloseHandle(hEvent);
			return false;
		}

		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		CloseHandle(hEvent);
		if (ret)
		{
			data = pParam.Data;
		}
		return true;
	}
	size_t Size()
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam pParam(EQSize, T(), hEvent);
		if (m_lock == true)
		{
			if (hEvent)CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)&pParam, NULL);
		if (ret == false)
		{
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		CloseHandle(hEvent);
		if (ret)
		{
			return pParam.nOperator;
		}
		return -1;
	}
	bool Clear()
	{
		if (m_lock == true)return false;
		IocpParam* pParam = new IocpParam(EQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		return ret;
	}
protected:
	static unsigned __stdcall threadEntry(void* arg)
	{
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
		thiz->threadMain();
		return 0;
	}
	void DealParam(typename CEdoyunQueue<T>::PPARAM* pParam)
	{
		switch (pParam->nOperator)
		{
		case EQPush:
			m_listData.push_back(pParam->Data);
			delete pParam;
			break;
		case EQPop:
			if (m_listData.size() > 0)
			{
				pParam->Data = m_listData.front();
				m_listData.pop_front();
			}
			if (pParam->hEvent != NULL)SetEvent(pParam->hEvent);
			break;
		case EQSize:
			pParam->nOperator = m_listData.size();
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent);
			}
			break;
		case EQClear:
			m_listData.clear();
			delete pParam;
			break;
		}
	}
	virtual void threadMain()
	{
		DWORD dwTransferred;
		PPARAM* pParam = NULL;
		ULONG_PTR CompletionKey;
		OVERLAPPED* pOverLapped = NULL;
		while (GetQueuedCompletionStatus(
			m_hCompletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverLapped,
			INFINITE))
		{
			if (dwTransferred == 0 || CompletionKey == NULL)
			{
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		while (GetQueuedCompletionStatus(
			m_hCompletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverLapped,
			0))
		{
			if (dwTransferred == 0 || CompletionKey == NULL)
			{
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
	}
private:
	std::list<T> m_listData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;
};


