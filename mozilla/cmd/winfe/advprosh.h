/* Insert copyright and license here 1996 */

#ifndef ADVPROPSHEET_H
#define ADVPROPSHEET_H

#include "property.h"
#include "msg_filt.h"

#define WM_ADVANCED_OPTIONS_DONE (WM_USER + 1555)
#define WM_EDIT_CUSTOM_DONE (WM_USER + 1556)

class CAdvancedOptionsPropertySheet;

class CSearchOptionsPage : public CNetscapePropertyPage
{
public:
  CSearchOptionsPage(CWnd *pParent, UINT nID);
  ~CSearchOptionsPage();

  virtual BOOL OnInitDialog();
  virtual BOOL OnSetActive();
  virtual BOOL OnKillActive( );
  virtual void OnOK();
  virtual void OnCancel();

  //{{AFX_DATA(CSearchOptionsPage)
	enum { IDD = IDD_ADVANCED_SEARCH };
	BOOL	m_bIncludeSubfolders;
	int		m_iSearchArea;
	//}}AFX_DATA

  //{{AFX_VIRTUAL(CSearchOptionsPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected: 
	CWnd * m_pParent;
  BOOL m_bHasBeenViewed;
  BOOL m_bOffline;


  DECLARE_MESSAGE_MAP()
};

class CCustomHeadersPage : public CNetscapePropertyPage
{
public:
  CCustomHeadersPage(CWnd *pParent, UINT nID);
  ~CCustomHeadersPage();

  virtual BOOL OnInitDialog();
  virtual BOOL OnSetActive();
  virtual BOOL OnKillActive();
  virtual void OnAddHeader();
	virtual void OnRemoveHeader();
	virtual void OnReplaceHeader();
  virtual void OnChangeEditHeader();
  virtual void OnSelChangeHeaderList();
  virtual void OnOK();
	virtual void OnCancel();

  void updateUI();
  void enableDlgItem(int iId, BOOL bEnable);

  //{{AFX_DATA(CCustomHeadersPage)
	enum { IDD = IDD_HEADER_LIST };
	//}}AFX_DATA
  //{{AFX_VIRTUAL(CCustomHeadersPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected: 
	CWnd * m_pParent;
  BOOL m_bHasBeenViewed;

  DECLARE_MESSAGE_MAP()
};

class CAdvancedOptionsPropertySheet : public CNetscapePropertySheet
{
public:

  CAdvancedOptionsPropertySheet(CWnd *pParent, const char* pName, DWORD dwPagesBitFlag = 0L);
  ~CAdvancedOptionsPropertySheet();
	
	CNetscapePropertyPage * GetCurrentPage();

  virtual void OnHelp();

protected:
	CWnd * m_pParent;
  int m_iReturnCode;
  DWORD m_dwPagesBitFlag;
	
  CSearchOptionsPage * m_OptionsPage;
  CCustomHeadersPage * m_HeadersPage;
  
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  virtual void OnNcDestroy();
  virtual void OnClose();
  virtual BOOL PreTranslateMessage(MSG * pMsg);

#ifdef _WIN32
	virtual BOOL OnInitDialog();
#else
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
#endif

  virtual void OnDestroy();

  DECLARE_MESSAGE_MAP()
};

// Advanced Options pages (bitflags)
#define AOP_NOPAGES           0x00000000
#define AOP_SEARCH_OPTIONS    0x00000001
#define AOP_CUSTOM_HEADERS    0x00000002

#endif // ADVPROPSHEET_H