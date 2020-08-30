#include "stdafx.h"
#include "demodshow.h"
#include "SizeDlg.h"

CSizeDlg::CSizeDlg(CWnd* pParent /*=NULL*/)
: CDialog(CSizeDlg::IDD, pParent), m_nSize(100)
{
}

CSizeDlg::~CSizeDlg()
{
}

void CSizeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_nSize);
}

BEGIN_MESSAGE_MAP(CSizeDlg, CDialog)
END_MESSAGE_MAP()
