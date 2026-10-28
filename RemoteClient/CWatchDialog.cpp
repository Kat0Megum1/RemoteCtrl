// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialogEx)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_WATCH, pParent)
	, m_isFull(false)
	, m_nObjWidth(-1)
	, m_nObjHeight(-1)
	, m_nLastMoveTick(0)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialogEx)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CWatchDialog::OnSendPackAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序

bool CWatchDialog::UserPoint2RemoteScreenPoint(CPoint point, bool isScreen, CPoint& remote)
{
	remote = CPoint(0, 0);
	// 还没收到第一张截图时，不知道被控端分辨率，换算没有依据
	if (m_nObjWidth <= 0 || m_nObjHeight <= 0)
		return false;
	// 两条入口的坐标系不一样，这里只补一次，避免屏幕坐标再被当成客户区坐标加一次偏移
	if (!isScreen)
		ClientToScreen(&point);
	// 画面是 StretchBlt 到控件客户区 (0,0) 的。SS_BLACKFRAME 的窗口矩形比客户区大一圈边框，
	// 用窗口矩形做原点，点会整圈偏到边框上。
	m_picture.ScreenToClient(&point);
	CRect client;
	m_picture.GetClientRect(&client);
	int nWidth = client.Width();
	int nHeight = client.Height();
	if (nWidth <= 0 || nHeight <= 0)
		return false;
	// 锁机按钮那一排不在画面上，不能钳到 (0,0)，否则一点按钮就把远端光标甩到左上角
	if (point.x < 0 || point.y < 0 || point.x >= nWidth || point.y >= nHeight)
		return false;
	int x = point.x * m_nObjWidth / nWidth;
	int y = point.y * m_nObjHeight / nHeight;
	if (x >= m_nObjWidth) x = m_nObjWidth - 1;
	if (y >= m_nObjHeight) y = m_nObjHeight - 1;
	remote = CPoint(x, y);
	return true;
}

void CWatchDialog::SendRemoteMouse(WORD nButton, WORD nAction, CPoint point, bool isScreen)
{
	CPoint remote;
	if (!UserPoint2RemoteScreenPoint(point, isScreen, remote))
		return;
	if (nAction == 4)
	{
		ULONGLONG now = GetTickCount64();
		if (now - m_nLastMoveTick < 40)
		{
			return;
		}
		m_nLastMoveTick = now;
	}
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = nButton;
	event.nAction = nAction;
	CClientController::getInstance()->SendCommandPacket(
		GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
}

LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2 || lParam == 1)
	{
		SetImageStatus(false);
		return 0;
	}
	if (wParam == NULL)
	{
		SetImageStatus(false);
		return 0;
	}
	CPacket* pPacket = (CPacket*)wParam;
	CPacket head = *pPacket;
	delete pPacket;
	if (head.sCmd == 6)
	{
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, head.strData.size());
		if (hMem != NULL)
		{
			IStream* pStream = NULL;
			if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
			{
				ULONG length = 0;
				pStream->Write(head.strData.c_str(), (ULONG)head.strData.size(), &length);
				LARGE_INTEGER bg = { 0 };
				pStream->Seek(bg, STREAM_SEEK_SET, NULL);
				if (!m_image.IsNull())
					m_image.Destroy();
				if (SUCCEEDED(m_image.Load(pStream)))
				{
					// 和坐标换算用同一块客户区，点在图上的位置才和发出去的坐标一致
					CRect rect;
					m_picture.GetClientRect(&rect);
					m_nObjWidth = m_image.GetWidth();
					m_nObjHeight = m_image.GetHeight();
					CDC* pDC = m_picture.GetDC();
					if (pDC != NULL)
					{
						m_image.StretchBlt(pDC->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
						m_picture.ReleaseDC(pDC);
					}
					// 不要 Invalidate。SS_BLACKFRAME 重画时会先擦背景，刚贴上的图会被抹掉
					m_image.Destroy();
				}
				pStream->Release();
			}
			else
			{
				GlobalFree(hMem);
			}
		}
	}
	SetImageStatus(false);
	return 0;
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_isFull = false;
	return TRUE;
}

BOOL CWatchDialog::PreTranslateMessage(MSG* pMsg)
{
	// 画面控件会把鼠标消息吃掉，对话框的 ON_WM_* 收不到。
	// 在这里把落在画面上的鼠标消息转成远程命令。
	if (m_picture.GetSafeHwnd() != NULL && pMsg->hwnd == m_picture.GetSafeHwnd())
	{
		CPoint point(pMsg->pt);
		switch (pMsg->message)
		{
		case WM_MOUSEMOVE:
			SendRemoteMouse(3, 4, point, true);
			break;
		case WM_LBUTTONDOWN:
			SendRemoteMouse(0, 2, point, true);
			break;
		case WM_LBUTTONUP:
			SendRemoteMouse(0, 3, point, true);
			break;
		case WM_LBUTTONDBLCLK:
			SendRemoteMouse(0, 1, point, true);
			break;
		case WM_RBUTTONDOWN:
			SendRemoteMouse(1, 2, point, true);
			break;
		case WM_RBUTTONUP:
			SendRemoteMouse(1, 3, point, true);
			break;
		case WM_RBUTTONDBLCLK:
			SendRemoteMouse(1, 1, point, true);
			break;
		case WM_MBUTTONDOWN:
			SendRemoteMouse(2, 2, point, true);
			break;
		case WM_MBUTTONUP:
			SendRemoteMouse(2, 3, point, true);
			break;
		case WM_MBUTTONDBLCLK:
			SendRemoteMouse(2, 1, point, true);
			break;
		default:
			break;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0)
	{
		if (isFull())
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			if (m_nObjWidth == -1)
			{
				m_nObjWidth = getImage().GetWidth();
			}
			if (m_nObjHeight == -1)
			{
				m_nObjHeight = getImage().GetHeight();
			}
			getImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			getImage().Destroy();
			SetImageStatus();
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	SendRemoteMouse(0, 1, point, false);
	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	SendRemoteMouse(0, 2, point, false);
	CDialogEx::OnLButtonDown(nFlags, point);
}

void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	SendRemoteMouse(0, 3, point, false);
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	SendRemoteMouse(1, 1, point, false);
	CDialogEx::OnRButtonDblClk(nFlags, point);
}

void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	SendRemoteMouse(1, 2, point, false);
	CDialogEx::OnRButtonDown(nFlags, point);
}

void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	SendRemoteMouse(1, 3, point, false);
	CDialogEx::OnRButtonUp(nFlags, point);
}

void CWatchDialog::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	SendRemoteMouse(2, 1, point, false);
	CDialogEx::OnMButtonDblClk(nFlags, point);
}

void CWatchDialog::OnMButtonDown(UINT nFlags, CPoint point)
{
	SendRemoteMouse(2, 2, point, false);
	CDialogEx::OnMButtonDown(nFlags, point);
}

void CWatchDialog::OnMButtonUp(UINT nFlags, CPoint point)
{
	SendRemoteMouse(2, 3, point, false);
	CDialogEx::OnMButtonUp(nFlags, point);
}

void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	SendRemoteMouse(3, 4, point, false);
	CDialogEx::OnMouseMove(nFlags, point);
}

void CWatchDialog::OnOK()
{
	// 回车不关闭监视窗口
}

void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7, true);
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8, true);
}
