// CBrowseDlg.h : header file
//

#if !defined(AFX_CBROWSEDLG_H__5121F5E6_5324_11D2_93E1_000000000000__INCLUDED_)
#define AFX_CBROWSEDLG_H__5121F5E6_5324_11D2_93E1_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "TabMessages.h"
#include "TabTests.h"
#include "TabDOM.h"

/////////////////////////////////////////////////////////////////////////////
// CBrowseDlg dialog

class CBrowseDlg : public CDialog
{
// Construction
public:
	CControlSiteInstance *m_pControlSite;
	CLSID m_clsid;

	CBrowseDlg(CWnd* pParent = NULL);	// standard constructor

	static CBrowseDlg *m_pBrowseDlg;

	HRESULT CreateWebBrowser();
	HRESULT DestroyWebBrowser();
	HRESULT GetWebBrowser(IWebBrowser **pWebBrowser);

	void RunTestSet(TestSet *pTestSet);
	TestResult RunTest(Test *pTest);
	void UpdateTest(HTREEITEM hItem, TestResult nResult);
	void UpdateTestSet(HTREEITEM hItem);
	void OutputString(const TCHAR *szMessage, ...);

// Dialog Data
	//{{AFX_DATA(CBrowseDlg)
	enum { IDD = IDD_CBROWSE_DIALOG };
	CComboBox	m_cmbURLs;
	BOOL	m_bNewWindow;
	//}}AFX_DATA

	CPropertySheet m_dlgPropSheet;
	CTabMessages m_TabMessages;
	CTabTests    m_TabTests;
	CTabDOM      m_TabDOM;
	CImageList m_cImageList;

	void OnRefreshDOM();
	void OnRunTest();
	void PopulateTests();
	
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	CString m_szTestURL;
	CString m_szTestCGI;
	DWORD m_dwCookie;

	// Generated message map functions
	//{{AFX_MSG(CBrowseDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnGo();
	afx_msg void OnBackward();
	afx_msg void OnForward();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CBROWSEDLG_H__5121F5E6_5324_11D2_93E1_000000000000__INCLUDED_)
