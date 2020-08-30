#pragma once

class CSizeDlg : public CDialog
{
public:
	CSizeDlg(CWnd* pParent = NULL);
	virtual ~CSizeDlg();

	enum { IDD = IDD_SIZEDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
public:
	unsigned m_nSize;
};
