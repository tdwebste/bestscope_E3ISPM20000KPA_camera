#pragma once

class CSnapTestPropertyPage : public CPropertyPage
{
	HANDLE m_hThread;
	bool m_bStarting;
	int m_totalCount;
	int m_count;
public:
	CSnapTestPropertyPage();

	int GetTotalCount() const;
	bool IsStarting() const;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPERTY_SNAP_TEST };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	afx_msg LRESULT OnSnapCountUpdate(WPARAM wp, LPARAM lp);

	DECLARE_MESSAGE_MAP()

private:
	void UpdateHint();

public:
	afx_msg void OnEnChangeEditSnapCount();
	afx_msg void OnBnClickedButtonStart();
	virtual BOOL OnInitDialog();
};
