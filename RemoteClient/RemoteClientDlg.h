
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"

#define WM_SEND_PACKET (WM_USER+1)


// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
	// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	void LoadFileCurrent();
	void LoadFileInfo();
	void RequestDir(HTREEITEM hTree);
	void UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent);
	void UpdateDownloadFile(const std::string& strData, FILE* pFile);
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);

	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	void ResetDownloadProgress();
	afx_msg void OnBnClickedButtonTest();
	DWORD m_server_address;
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	CString m_nPort;
	afx_msg void OnTvnSelchangedTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnItemchangedListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTvnItemexpandingTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	bool m_bDirLoading;
};
