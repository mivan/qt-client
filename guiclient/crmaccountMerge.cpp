/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "crmaccountMerge.h"

#include <QMessageBox>
#include <QPixmap>

#include "crmaccountMergePickAccountsPage.h"
#include "crmaccountMergePickDataPage.h"
#include "crmaccountMergePickTaskPage.h"
#include "crmaccountMergePurgePage.h"
#include "crmaccountMergeResultPage.h"

class crmaccountMergePrivate {
  public:
    crmaccountMergePrivate(QWidget *parent = 0) :
      _parent(parent),
      _selectCol(-1)
    {
      _oldpage = crmaccountMerge::Page_PickTask;
    }

    ~crmaccountMergePrivate() {
    }
    
    int         _oldpage;
    QWidget    *_parent;
    int         _selectCol;
};

crmaccountMerge::crmaccountMerge(QWidget* parent, const char* name, Qt::WFlags fl)
    : QWizard(parent, fl)
{
  setupUi(this);
  if (name)
    setObjectName(name);
  _data = new crmaccountMergePrivate(this);
  QPixmap *pixmap = new QPixmap(":/images/icon128x32.png");
  if (pixmap)
  {
#ifdef Q_WS_MAC
  setPixmap(BackgroundPixmap, *pixmap);
#else
  setPixmap(BannerPixmap,     *pixmap);
#endif
  }

  QWizardPage *picktaskpage = new CrmaccountMergePickTaskPage(this);
  QWizardPage *pickacctpage = new CrmaccountMergePickAccountsPage(this);
  QWizardPage *pickdatapage = new CrmaccountMergePickDataPage(this);
  QWizardPage *mergerespage = new CrmaccountMergeResultPage(this);
  QWizardPage *mergeprgpage = new CrmaccountMergePurgePage(this);

  setPage(Page_PickTask,        picktaskpage);
  setPage(Page_PickAccounts,    pickacctpage);
  setPage(Page_PickData,        pickdatapage);
  setPage(Page_Result,          mergerespage);
  setPage(Page_Purge,           mergeprgpage);

  setButtonText(CancelButton, tr("Close"));

  setDefaultProperty("XComboBox", "text", SIGNAL(currentIndexChanged(QString)));

  connect(mergeprgpage, SIGNAL(mergeSetChanged()), picktaskpage, SLOT(sUpdateComboBoxes()));
  connect(mergeprgpage, SIGNAL(mergeSetChanged()), mergerespage, SLOT(clearIfPurged()));
}

crmaccountMerge::~crmaccountMerge()
{
  // no need to delete child widgets, Qt does it all for us
}

void crmaccountMerge::languageChange()
{
  retranslateUi(this);
}
