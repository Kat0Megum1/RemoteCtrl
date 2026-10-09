// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialogEx)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_WATCH, pParent)
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
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序

CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint point, bool isScreen)
{
	CRect rect;
	m_picture.GetWindowRect(&rect);
	if (!isScreen)
	{
		ClientToScreen(&point);
	}
	int nWidth = rect.Width();
	int nHeight = rect.Height();
	if (nWidth <= 0 || nHeight <= 0 || m_nObjWidth <= 0 || m_nObjHeight <= 0)
	{
		return CPoint(0, 0);
	}
	int x = (point.x - rect.left) * m_nObjWidth / nWidth;
	int y = (point.y - rect.top) * m_nObjHeight / nHeight;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= m_nObjWidth) x = m_nObjWidth - 1;
	if (y >= m_nObjHeight) y = m_nObjHeight - 1;
	return CPoint(x, y);
}

void CWatchDialog::SendRemoteMouse(WORD nButton, WORD nAction, CPoint point, bool isScreen)
{
	if ((m_nObjWidth == -1) || (m_nObjHeight == -1))
	{
		return;
	}
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
	event.ptXY = UserPoint2RemoteScreenPoint(point, isScreen);
	event.nButton = nButton;
	event.nAction = nAction;
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	// wParam 低位：是否自动关闭套接字；其余位：命令号 5
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM)&event);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetTimer(0, 11, NULL);
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
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull())
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			if (m_nObjWidth == -1)
			{
				m_nObjWidth = pParent->getImage().GetWidth();
			}
			if (m_nObjHeight == -1)
			{
				m_nObjHeight = pParent->getImage().GetHeight();
			}
			pParent->getImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->getImage().Destroy();
			pParent->SetImageStatus();
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
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 7 << 1 | 1);
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 8 << 1 | 1);
}
