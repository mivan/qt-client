/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "crmaccountMergeResultPage.h"

#include <QMessageBox>

#include <metasql.h>
#include <mqlutil.h>

#include "crmaccount.h"
#include "crmaccountMerge.h"
#include "errorReporter.h"

class CrmaccountMergeResultPagePrivate
{
  public:
    CrmaccountMergeResultPagePrivate(QWizardPage *parent)
      : _parent(parent)
    {
      _crmaccount = new crmaccount();
      _crmaccount->_save->setVisible(false);
      _crmaccount->_close->setVisible(false);

      QString errmsg;
      bool    ok = false;
      _obsoletemqlstr = MQLUtil::mqlLoad("crmaccountmerge", "wip", errmsg, &ok);
      if (!ok)
        ErrorReporter::error(QtCriticalMsg, _parent,
                             QT_TRANSLATE_NOOP("CrmaccountMergeResultPage",
                                               "Getting Source Accounts"),
                             errmsg, __FILE__, __LINE__);

      ParameterList params;
      params.append("mode", "view");
      _crmaccount->set(params);

      QWidget *sa = _parent->findChild<QWidget*>("_resultScrollAreaContents");
      QLayout *lyt = sa ? sa->layout() : 0;
      if (lyt)
        lyt->addWidget(_crmaccount);
      else
        ErrorReporter::error(QtWarningMsg, _parent,
                             QT_TRANSLATE_NOOP("CrmaccountMergeResultPage",
                                               "Could not draw Account"),
                             QT_TRANSLATE_NOOP("CrmaccountMergeResultPage",
                                "Could not find the portion of the window "
                                "in which to draw the target Account."));
    };

    crmaccount *_crmaccount;
    QString     _obsoletemqlstr;
    QWidget    *_parent;
};

CrmaccountMergeResultPage::CrmaccountMergeResultPage(QWidget *parent)
  : QWizardPage(parent),
    _data(0)
{
  setupUi(this);

  _data = new CrmaccountMergeResultPagePrivate(this);

  _source->addColumn(tr("Account Number"), -1, Qt::AlignLeft, true, "crmacct_number");
  _source->addColumn(tr("Account Name"),   -1, Qt::AlignLeft, true, "crmacct_name");
}

CrmaccountMergeResultPage::~CrmaccountMergeResultPage()
{
  if (_data)
    delete _data;
}

void CrmaccountMergeResultPage::initializePage()
{
  XSqlQuery idq;
  idq.prepare("SELECT crmacct_id"
              "  FROM crmacct"
              " WHERE (crmacct_number=:number);");
  idq.bindValue(":number", field("_completedMerge"));
  idq.exec();
  if (idq.first())
    _data->_crmaccount->setId(idq.value("crmacct_id").toInt());
  else if (ErrorReporter::error(QtCriticalMsg, this,
                                tr("Error Getting Account"),
                                idq, __FILE__, __LINE__))
    return;
  else
  {
    QMessageBox::warning(this, tr("Could Not Find Account"),
                         tr("Could not find the merged Account (%1).")
                         .arg(field("_completedMerge").toString()));
    _data->_crmaccount->setId(-1);
  }

  ParameterList params;
  params.append("destid", _data->_crmaccount->id());

  MetaSQLQuery mql(_data->_obsoletemqlstr);
  XSqlQuery srcq = mql.toQuery(params);
  _source->populate(srcq);
  if (ErrorReporter::error(QtCriticalMsg, this,
                           tr("Error Getting Obsolete Accounts"),
                           srcq, __FILE__, __LINE__))
    return;

  _keep->setChecked(true);
  _undo->setEnabled(_data->_crmaccount->id() > 0);
}

int CrmaccountMergeResultPage::nextId() const
{
  int nextpage = crmaccountMerge::Page_Result;

  if (_keep->isChecked())
    nextpage = crmaccountMerge::Page_Purge;
  // else if (_undo->isChecked()) then handled in validatepage() below
  else
    qWarning("BUG: CrmaccountMergeResultPage couldn't figure out what the "
             "next wizard page should be.");

  return nextpage;
}

bool CrmaccountMergeResultPage::validatePage()
{
  if (_undo->isChecked())
  {
    if (QMessageBox::question(this, tr("Revert?"),
                              tr("<p>Are you sure you want to undo this "
                                 "Account Merge?</p><p>The Accounts "
                                 "will be restored and you will need to "
                                 "start the merge from the beginning.</p>"),
                              QMessageBox::No | QMessageBox::Default,
                              QMessageBox::Yes) == QMessageBox::No)
      return false;

    XSqlQuery undoq;
    undoq.prepare("SELECT undomerge('public', 'crmacct', :id) AS result;");
    undoq.bindValue(":id", _data->_crmaccount->id());
    undoq.exec();
    if (undoq.first())
      qWarning("undomerge(%d) returned %d",
               _data->_crmaccount->id(), undoq.value("result").toInt());
    else if (ErrorReporter::error(QtCriticalMsg, this,
                                  tr("Error Undoing Merge"),
                                  undoq, __FILE__, __LINE__))
      return false;
    omfgThis->sCrmAccountsUpdated(_data->_crmaccount->id());
    wizard()->restart();
  }

  return true;
}

void CrmaccountMergeResultPage::clearIfPurged()
{
  XSqlQuery getq;
  getq.prepare("SELECT EXISTS("
               "  SELECT 1 FROM mrgundo"
               "   WHERE ((mrgundo_base_table='crmacct')"
               "      AND (mrgundo_base_id=:crmacctid))) AS stillthere;");
  getq.bindValue(":crmacctid", _data->_crmaccount->id());
  getq.exec();
  if (getq.first())
  {
    if (! getq.value("stillthere").toBool())
    {
      _data->_crmaccount->setId(-1);
      _source->clear();
      _keep->setChecked(true);
      _undo->setEnabled(false);
    }
  }
  else if (ErrorReporter::error(QtCriticalMsg, this, tr("Checking Undo Status"),
                                getq, __FILE__, __LINE__))
    return;
}
