#pragma once

class CResTestPropertyPage : public CPropertyPage
{
	bool m_bStarting;
	int m_totalCount;
	int m_count;
	HANDLE m_hThread;
public:
	CResTestPropertyPage();

	int GetTotalCount() const;
	bool IsStarting() const;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPERTY_RESOLUTION_TEST };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnResTestCountUpdate(WPARAM wp, LPARAM lp);
	afx_msg void OnEnChangeEditResolutionTestCount();
	afx_msg void OnBnClickedButtonResolutionTestStart();
	DECLARE_MESSAGE_MAP()
private:
	void UpdateHint();
};
