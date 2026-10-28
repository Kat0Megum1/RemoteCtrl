#pragma once
#include <map>
#include "LockDialog.h"
#include "Packet.h"
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
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		CCommand* thiz = (CCommand*)arg;
		if (status > 0)
		{
			int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
			if (ret != 0) {
				TRACE("command failed %d ret=%d\r\n", status, ret);
			}
		}
		else {
			MessageBox(NULL, _T("cannot accept client"), _T("init failed"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	typedef int (CCommand::* CMDFUNC)(std::list<CPacket>& lstPacket, CPacket& inPacket);
	std::map<int, CMDFUNC> m_mapFunction;
	CLockDialog dlg;
	unsigned threadid;
protected:
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
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
		lstPacket.push_back(packet);

		return 0;
	}

	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		if (_chdir(inPacket.strData.c_str()) != 0)
		{
			FILEINFO finfo;
			finfo.IsInvalid = TRUE;
			finfo.HasNext = FALSE;
			CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
			lstPacket.push_back(packet);
			OutputDebugString(_T("no permission"));
			return -2;
		}
		_finddata_t fdata;
		intptr_t hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1)
		{
			OutputDebugString(_T("no file"));
			FILEINFO finfo;
			finfo.IsInvalid = TRUE;
			finfo.HasNext = FALSE;
			CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
			lstPacket.push_back(packet);
			return -3;
		}
		int count = 0;
		do
		{
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
			// szFileName 只有 256，_finddata_t::name 更长，按 strlen 直拷会越界
			size_t nName = strlen(fdata.name);
			if (nName >= sizeof(finfo.szFileName))
				nName = sizeof(finfo.szFileName) - 1;
			memcpy(finfo.szFileName, fdata.name, nName);
			CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
			lstPacket.push_back(packet);
			count++;
		} while (!_findnext(hfind, &fdata));
		_findclose(hfind);
		TRACE("server:count=%d\r\n", count);
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
		lstPacket.push_back(packet);
		return 0;
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		CPacket packet(3, NULL, 0);
		lstPacket.push_back(packet);
		return 0;
	}

	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		long long data = 0;
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0)
		{
			// 失败也发 8 字节 0。长度为 0 的包客户端会当成“长度还没到”直接丢掉，下载框就停住
			CPacket packet(4, (BYTE*)&data, 8);
			lstPacket.push_back(packet);
			return -1;
		}
		if (pFile != NULL)
		{
			_fseeki64(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			if (data < 0)
				data = 0;
			CPacket head(4, (BYTE*)&data, 8);
			lstPacket.push_back(head);
			_fseeki64(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do
			{
				rlen = fread(buffer, 1, 1024, pFile);
				if (rlen == 0)
					break;
				CPacket packet(4, (BYTE*)buffer, rlen);
				lstPacket.push_back(packet);
			} while (rlen >= 1024);
			fclose(pFile);
		}
		else
		{
			CPacket packet(4, (BYTE*)&data, 8);
			lstPacket.push_back(packet);
		}
		return 0;
	}

	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
		DWORD nFlags = 0;
		switch (mouse.nButton)
		{
		case 0://���
			nFlags = 1;
			break;
		case 1://�Ҽ�
			nFlags = 2;
			break;
		case 2://�м�
			nFlags = 4;
			break;
		case 3://û�а������ƶ���
			nFlags = 8;
			break;
		}
		SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		switch (mouse.nAction)
		{
		case 0://����
			nFlags |= 0x10;
			break;
		case 1://˫��
			nFlags |= 0x20;
			break;
		case 2://����
			nFlags |= 0x40;
			break;
		case 3://�ſ�
			nFlags |= 0x80;
			break;
		default:
			// 4�����ƶ������Ӷ���λ������ nFlags==8
			break;
		}
		switch (nFlags)
		{
		case 0x21://���˫��
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11://�������
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41://�������
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81://����ſ�
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22://�Ҽ�˫��
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12://�Ҽ�����
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42://�Ҽ�����
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82://�Ҽ��ſ�
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24://�м�˫��
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14://�м�����
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44://�м�����
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84://�м��ſ�
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08://����������ƶ����������� SetCursorPos ���ã�
			break;
		}
		CPacket packet(5, NULL, 0);
		lstPacket.push_back(packet);

		return 0;
	}

	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		CImage screen;
		HDC hScreen = ::GetDC(NULL);
		if (hScreen == NULL)
			return -1;
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		// 32 位图的 alpha 是 0，存成 PNG 再画出来是全透明。24 位没有透明通道
		if (nBitPerPixel != 24)
			nBitPerPixel = 24;
		if (!screen.Create(nWidth, nHeight, nBitPerPixel))
		{
			::ReleaseDC(NULL, hScreen);
			return -1;
		}
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
		// GetDC 期间位图被选进内存 DC，不先 ReleaseDC，Save 会拿到空图
		screen.ReleaseDC();
		::ReleaseDC(NULL, hScreen);

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL)
			return -1;
		IStream* pStream = NULL;
		HRESULT result = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (result != S_OK || pStream == NULL)
		{
			if (pStream != NULL)
				pStream->Release();
			else
				GlobalFree(hMem);
			return -1;
		}
		if (FAILED(screen.Save(pStream, Gdiplus::ImageFormatPNG)))
		{
			pStream->Release();
			return -1;
		}
		LARGE_INTEGER bg = { 0 };
		ULARGE_INTEGER nSize = { 0 };
		pStream->Seek(bg, STREAM_SEEK_END, &nSize);
		pStream->Seek(bg, STREAM_SEEK_SET, NULL);
		PBYTE pData = (PBYTE)GlobalLock(hMem);
		if (pData != NULL && nSize.QuadPart > 0)
		{
			CPacket packet(6, pData, (size_t)nSize.QuadPart);
			lstPacket.push_back(packet);
			GlobalUnlock(hMem);
		}
		// fDeleteOnRelease 为 TRUE 时，Release 会释放 hMem，不能再 GlobalFree
		pStream->Release();
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

	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
		{
			_beginthreadex(NULL, 0, threadLockDlg, this, 0, &threadid);
		}
		CPacket packet(7, NULL, 0);
		lstPacket.push_back(packet);
		return 0;
	};

	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		PostThreadMessage(threadid, WM_KEYDOWN, VK_ESCAPE, 0);
		CPacket packet(8, NULL, 0);
		lstPacket.push_back(packet);
		return 0;
	};

	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		DeleteFile(strPath.c_str());
		CPacket packet(9, NULL, 0);
		lstPacket.push_back(packet);
		return 0;
	}

	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		CPacket packet(1981, NULL, 0);
		lstPacket.push_back(packet);
		return 0;
	}

};



