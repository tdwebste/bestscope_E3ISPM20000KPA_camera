#pragma once

class COpenCloseTestPropertyPage : public CPropertyPage
{
	bool m_bStarting;
	int m_totalCount;
	int m_count;
	HANDLE m_hThread;
public:
	COpenCloseTestPropertyPage();

	int GetTotalCount() const;
	bool IsStarting() const;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPERTY_OPEN_CLOSE_TEST };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEditOpenCloseCnt();
	afx_msg void OnBnClickedButtonOpenCloseTestStart();
	afx_msg LRESULT OnOpenCloseTestCountUpdate(WPARAM wp, LPARAM lp);
	DECLARE_MESSAGE_MAP()
private:
	void UpdateHint();
};
