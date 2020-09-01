#pragma once

class CROITestPropertyPage : public CPropertyPage
{
	int m_invertal;
	HANDLE m_hThread;
public:
	CROITestPropertyPage();

	int GetInvertal() const;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPERTY_ROI_TEST };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnROITestOne(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnROITestFinished(WPARAM wp, LPARAM lp);
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnEnChangeEditInterval();
	DECLARE_MESSAGE_MAP()
};
