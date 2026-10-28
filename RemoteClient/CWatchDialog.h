#pragma once
#include "afxdialogex.h"


// CWatchDialog 对话框

class CWatchDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif

protected:
	CImage m_image;
	bool m_isFull;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	bool isFull()const
	{
		return m_isFull;
	}
	CImage& getImage()
	{
		return m_image;
	}
	void SetImageStatus(bool isFull = false)
	{
		m_isFull = isFull;
	}

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam);
	CStatic m_picture;

	// 监视画面上的点 → 被控端屏幕坐标。
	// isScreen 为 false：point 是对话框客户区坐标，要先变到屏幕坐标。
	// isScreen 为 true：point 已经是屏幕坐标（PreTranslateMessage 的 pMsg->pt），不能再转一次。
	// 点不在画面控件客户区里时返回 false。
	bool UserPoint2RemoteScreenPoint(CPoint point, bool isScreen, CPoint& remote);
	void SendRemoteMouse(WORD nButton, WORD nAction, CPoint point, bool isScreen);

	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedBtnLock();
	afx_msg void OnBnClickedBtnUnlock();
	virtual void OnOK();

private:
	int m_nObjWidth;
	int m_nObjHeight;
	ULONGLONG m_nLastMoveTick;
};
