
// RemoteClientDlg.cpp: ʵ���ļ�
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif




class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX); 

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
	, m_bDirLoading(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_TEST, &CRemoteClientDlg::OnBnClickedButtonTest)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_DIR, &CRemoteClientDlg::OnTvnSelchangedTreeDir)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILE, &CRemoteClientDlg::OnLvnItemchangedListFile)
	ON_BN_CLICKED(IDC_BUTTON_FILEINFO, &CRemoteClientDlg::OnBnClickedButtonFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE_DIR, &CRemoteClientDlg::OnTvnItemexpandingTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg ��Ϣ��������

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵������ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ����Ӷ���ĳ�ʼ������
	UpdateData();
	m_server_address = 0x7F000001;
	m_nPort = _T("8888");
	UpdateData(FALSE);
	// ״̬������ CClientController::InitController ����
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի���������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedButtonTest()
{
	// 只负责投递。连没连上，要等 WM_SEND_PACK_ACK
	if (!CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981))
	{
		AfxMessageBox(_T("post command failed"));
	}
}

void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: �ڴ����ӿؼ�֪ͨ�����������
	*pResult = 0;
}

void CRemoteClientDlg::OnTvnSelchangedTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: �ڴ����ӿؼ�֪ͨ�����������
	*pResult = 0;
}

void CRemoteClientDlg::OnLvnItemchangedListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: �ڴ����ӿؼ�֪ͨ�����������
	*pResult = 0;
}

void CRemoteClientDlg::OnBnClickedButtonFileinfo()
{
	if (!CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0))
	{
		AfxMessageBox(_T("post command failed"));
		return;
	}
	return;
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2)
	{
		m_bDirLoading = false;
		if (CClientController::getInstance()->IsDownloading())
			CClientController::getInstance()->DownloadAbort();
		else
			AfxMessageBox(_T("send failed"));
		return 0;
	}
	if (lParam == 1 || wParam == NULL)
	{
		m_bDirLoading = false;
		if (CClientController::getInstance()->IsDownloading())
			CClientController::getInstance()->DownloadAbort();
		return 0;
	}
	CPacket* pPacket = (CPacket*)wParam;
	CPacket head = *pPacket;
	delete pPacket;
	switch (head.sCmd)
	{
	case 1:
	{
		std::string drivers = head.strData;
		std::string dr;
		m_Tree.DeleteAllItems();
		for (size_t i = 0; i < drivers.size(); i++)
		{
			if (drivers[i] == ',')
			{
				dr += ':';
				HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
				m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
				dr.clear();
				continue;
			}
			dr += drivers[i];
		}
		if (!dr.empty())
		{
			dr += ':';
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
		}
		break;
	}
	case 1981:
		MessageBox(_T("connect ok"), _T("ok"), MB_ICONINFORMATION);
		break;
	case 2:
		if (head.strData.size() >= sizeof(FILEINFO))
			UpdateFileInfo(*(PFILEINFO)head.strData.c_str(), (HTREEITEM)lParam);
		break;
	case 3:
		MessageBox(_T("open file ok"), _T("done"), MB_ICONINFORMATION);
		break;
	case 4:
		UpdateDownloadFile(head.strData, (FILE*)lParam);
		break;
	case 9:
		LoadFileCurrent();
		break;
	default:
		break;
	}
	return 0;
}

void CRemoteClientDlg::LoadFileCurrent()
{
	RequestDir(m_Tree.GetSelectedItem());
}

void CRemoteClientDlg::RequestDir(HTREEITEM hTree)
{
	if (hTree == NULL || m_bDirLoading)
		return;
	// 先删掉旧子节点和占位空项，再请求。不删的话，删除文件后刷新会把同一层文件夹再插一遍
	DeleteTreeChildrenItem(hTree);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTree);
	m_bDirLoading = true;
	if (!CClientController::getInstance()->SendCommandPacket(
		GetSafeHwnd(), 2, false,
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength(),
		(WPARAM)hTree))
	{
		m_bDirLoading = false;
	}
}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent)
{
	if (finfo.HasNext == FALSE || finfo.IsInvalid)
	{
		m_bDirLoading = false;
		if (hParent != NULL && finfo.HasNext == FALSE)
			m_Tree.Expand(hParent, TVE_EXPAND);
		return;
	}
	if (finfo.IsDirectory)
	{
		if (CString(finfo.szFileName) == "." || CString(finfo.szFileName) == "..")
			return;
		HTREEITEM hTemp = m_Tree.InsertItem(finfo.szFileName, hParent, TVI_LAST);
		m_Tree.InsertItem("", hTemp, TVI_LAST);
	}
	else
	{
		m_List.InsertItem(0, finfo.szFileName);
	}
}

static long long s_downLength = 0;
static long long s_downIndex = 0;

void CRemoteClientDlg::ResetDownloadProgress()
{
	s_downLength = 0;
	s_downIndex = 0;
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	if (pFile == NULL)
		return;
	if (s_downLength == 0)
	{
		if (strData.size() < sizeof(long long))
			return;
		s_downLength = *(long long*)strData.c_str();
		if (s_downLength == 0)
		{
			AfxMessageBox(_T("file length is zero"));
			CClientController::getInstance()->DownloadAbort();
		}
		return;
	}
	fwrite(strData.c_str(), 1, strData.size(), pFile);
	s_downIndex += (long long)strData.size();
	if (s_downIndex >= s_downLength)
		CClientController::getInstance()->DownloadEnd();
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
		return;
	RequestDir(hTreeSelected);
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do
	{
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + "\\" + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do
	{
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL)m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// 双击会先触发单击。两边都 LoadFileInfo 会把同一目录请求两次，子节点重复
	*pResult = 0;
}

void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}

void CRemoteClientDlg::OnTvnItemexpandingTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;
	if (pNMTreeView->action != TVE_EXPAND)
		return;
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	HTREEITEM hChild = m_Tree.GetChildItem(hItem);
	// 只有占位空项时才请求。已经展开过的文件夹再展开，只显示已有子节点
	if (hChild != NULL && m_Tree.GetNextSiblingItem(hChild) == NULL
		&& m_Tree.GetItemText(hChild).IsEmpty())
	{
		// 这次展开先取消。占位项马上会被删掉，等目录包到齐再 Expand
		*pResult = TRUE;
		RequestDir(hItem);
	}
}

void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0)return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL)
	{
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	if (hSelected == NULL) return;
	int nSelected = m_List.GetSelectionMark();
	if (nSelected < 0) return;
	CString strFile = m_List.GetItemText(nSelected, 0);
	CString strPath = GetPath(hSelected) + strFile;
	CClientController::getInstance()->DownFile(strPath);
}

void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	if (hSelected == NULL)
		return;
	int nSelected = m_List.GetSelectionMark();
	if (nSelected < 0)
		return;
	CString strFile = GetPath(hSelected) + m_List.GetItemText(nSelected, 0);
	CClientController::getInstance()->SendCommandPacket(
		GetSafeHwnd(), 9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
}

void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	if (hSelected == NULL)
		return;
	int nSelected = m_List.GetSelectionMark();
	if (nSelected < 0)
		return;
	CString strFile = GetPath(hSelected) + m_List.GetItemText(nSelected, 0);
	CClientController::getInstance()->SendCommandPacket(
		GetSafeHwnd(), 3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int cmd = (int)(wParam >> 1);
	bool autoClose = (wParam & 1) != 0;
	CClientController* pCtrl = CClientController::getInstance();
	switch (cmd)
	{
	case 4:
	{
		CString strFile = (LPCSTR)lParam;
		pCtrl->SendCommandPacket(GetSafeHwnd(), cmd, autoClose, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
		break;
	}
	case 5:
		pCtrl->SendCommandPacket(GetSafeHwnd(), cmd, autoClose, (BYTE*)lParam, sizeof(MOUSEEV));
		break;
	case 6:
	case 7:
	case 8:
		pCtrl->SendCommandPacket(GetSafeHwnd(), cmd, autoClose);
		break;
	default:
		return -1;
	}
	return 0;
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}

void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ�������Ϣ������������/�����Ĭ��ֵ

	CDialogEx::OnTimer(nIDEvent);
}
