#pragma once
#include <map>
#include "LockDialog.h"
#include "ServerSocket.h"
#include <io.h>
#include <direct.h>
#include <atlimage.h>
#include "resource.h"
#include <list>
class CCommand
{
public:
	CCommand();
	~CCommand();
	int ExcuteCommand(int nCmd);
protected:
	typedef int (CCommand::* CMDFUNC)();
	std::map<int, CMDFUNC> m_mapFunction;
	CLockDialog dlg;
	unsigned threadid;
protected:
	int MakeDriverInfo()
	{
		std::string result;
		DWORD driveBit = GetLogicalDrives();
		for (int i = 0; i < 26; ++i)
		{
			if (driveBit & (1UL << i))
			{
				if (!result.empty())
					result += ',';
				result += 'A' + i;
			}
		}
		CPacket packet(1, (BYTE*)result.c_str(), result.size());
		CServerSocket::getInstance()->Send(packet);

		return 0;
	}

	int MakeDirectoryInfo()
	{
		std::string strPath;
		if (CServerSocket::getInstance()->GetFilePath(strPath) == false)
		{
			OutputDebugString(_T("当前的命令，不是获取文件列表，命令解析错误！"));
			return -1;
		}
		if (_chdir(strPath.c_str()) != 0)
		{
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(packet);
			OutputDebugString(_T("没有权限，访问目录！"));
			return -2;
		}
		_finddata_t fdata;
		intptr_t hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1)
		{
			OutputDebugString(_T("没有找到任何文件！"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(packet);
			return -3;
		}
		do
		{
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(packet);
		} while (!_findnext(hfind, &fdata));
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(packet);
		return 0;
	}

	int RunFile()
	{
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		CPacket packet(3, NULL, 0);
		CServerSocket::getInstance()->Send(packet);
		return 0;
	}

	int DownloadFile()
	{
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		long long data = 0;
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0)
		{
			CPacket packet(4, (BYTE*)&data, 0);
			CServerSocket::getInstance()->Send(packet);
			return -1;
		}
		if (pFile != NULL)
		{
			fseek(pFile, 0, SEEK_END);
			data = ftell(pFile);
			CPacket head(4, (BYTE*)&data, 8);
			CServerSocket::getInstance()->Send(head);
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do
			{
				rlen = fread(buffer, 1, 1024, pFile);
				CPacket packet(4, (BYTE*)buffer, rlen);
				CServerSocket::getInstance()->Send(packet);
			} while (rlen >= 1024);
			fclose(pFile);
		}
		CPacket packet(4, NULL, 0);
		CServerSocket::getInstance()->Send(packet);
		return 0;
	}

	int MouseEvent()
	{
		MOUSEEV mouse;
		if (CServerSocket::getInstance()->GetMouseEvent(mouse))
		{
			DWORD nFlags = 0;
			switch (mouse.nButton)
			{
			case 0://左键
				nFlags = 1;
				break;
			case 1://右键
				nFlags = 2;
				break;
			case 2://中键
				nFlags = 4;
				break;
			case 3://没有按键（移动）
				nFlags = 8;
				break;
			}
			SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
			switch (mouse.nAction)
			{
			case 0://单击
				nFlags |= 0x10;
				break;
			case 1://双击
				nFlags |= 0x20;
				break;
			case 2://按下
				nFlags |= 0x40;
				break;
			case 3://放开
				nFlags |= 0x80;
				break;
			default:
				// 4：纯移动，不加动作位，保持 nFlags==8
				break;
			}
			switch (nFlags)
			{
			case 0x21://左键双击
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x11://左键单击
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x41://左键按下
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x81://左键放开
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x22://右键双击
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x12://右键单击
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x42://右键按下
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x82://右键放开
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x24://中键双击
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x14://中键单击
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x44://中键按下
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x84://中键放开
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x08://单纯的鼠标移动（坐标已由 SetCursorPos 设置）
				break;
			}
			CPacket packet(5, NULL, 0);
			CServerSocket::getInstance()->Send(packet);
		}
		else
		{
			OutputDebugString(_T("获取鼠标操作参数失败！"));
			return -1;
		}
		return 0;
	}

	int SendScreen()
	{
		CImage screen;
		HDC hScreen = ::GetDC(NULL);
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeigth = GetDeviceCaps(hScreen, VERTRES);
		screen.Create(nWidth, nHeigth, nBitPerPixel);
		BitBlt(screen.GetDC(), 0, 0, 2560, 1440, hScreen, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hScreen);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		IStream* pStream = NULL;
		HRESULT result = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (result == S_OK)
		{
			screen.Save(pStream, Gdiplus::ImageFormatPNG);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			PBYTE pData = (PBYTE)GlobalLock(hMem);
			SIZE_T nSize = GlobalSize(hMem);
			CPacket packet(6, pData, nSize);
			CServerSocket::getInstance()->Send(packet);
			GlobalUnlock(hMem);
		}
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();
		return 0;
	}

	static unsigned _stdcall threadLockDlg(void* arg)
	{
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	
	void threadLockDlgMain()
	{
		dlg.Create(IDD_DIALOG_INFO, NULL);
		dlg.ShowWindow(SW_SHOW);
		CRect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		rect.bottom = LONG(rect.bottom * 1.10);
		dlg.MoveWindow(rect);
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText)
		{
			CRect rtText;
			pText->GetWindowRect(rtText);
			int x = (rect.right - rtText.Width()) / 2;
			int y = (rect.bottom - rtText.Height()) / 2;
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		ShowCursor(false);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
		rect.left = 0;
		rect.top = 0;
		rect.right = 1;
		rect.bottom = 1;
		ClipCursor(rect);
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN)
			{
				if (msg.wParam == VK_ESCAPE)
					break;
			}
		}
		ClipCursor(NULL);
		ShowCursor(true);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
		dlg.DestroyWindow();
	}

	int LockMachine()
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
		{
			_beginthreadex(NULL, 0, threadLockDlg, this, 0, &threadid);
		}
		CPacket packet(7, NULL, 0);
		CServerSocket::getInstance()->Send(packet);
		return 0;
	};

	int UnlockMachine()
	{
		PostThreadMessage(threadid, WM_KEYDOWN, VK_ESCAPE, 0);
		CPacket packet(8, NULL, 0);
		CServerSocket::getInstance()->Send(packet);
		return 0;
	};

	int DeleteLocalFile()
	{
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		DeleteFile(strPath.c_str());
		CPacket packet(9, NULL, 0);
		bool ret = CServerSocket::getInstance()->Send(packet);
		TRACE("Send ret=%d \r\n", ret);
		return 0;
	}

	int TestConnect()
	{
		CPacket packet(1981, NULL, 0);
		bool ret = CServerSocket::getInstance()->Send(packet);
		TRACE("Send ret=%d \r\n", ret);
		return 0;
	}

};



