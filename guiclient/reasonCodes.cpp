/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "reasonCodes.h"

#include <QVariant>
#include <QMessageBox>
#include <QMenu>
#include <openreports.h>
#include "reasonCode.h"

reasonCodes::reasonCodes(QWidget* parent, const char* name, Qt::WFlags fl)
  : XWidget(parent, name, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_rsncode, SIGNAL(valid(bool)), _view, SLOT(setEnabled(bool)));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_view, SIGNAL(clicked()), this, SLOT(sView()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_rsncode, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));

  if (_privileges->check("MaintainReasonCodes"))
  {
    connect(_rsncode, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_rsncode, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_rsncode, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_rsncode, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  _rsncode->addColumn(tr("Code"),        _itemColumn, Qt::AlignLeft, true, "rsncode_code" );
  _rsncode->addColumn(tr("Description"), -1,          Qt::AlignLeft, true, "rsncode_descrip" );
    
  sFillList();
}

reasonCodes::~reasonCodes()
{
  // no need to delete child widgets, Qt does it all for us
}

void reasonCodes::languageChange()
{
  retranslateUi(this);
}

void reasonCodes::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  reasonCode newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void reasonCodes::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("rsncode_id", _rsncode->id());

  reasonCode newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void reasonCodes::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("rsncode_id", _rsncode->id());

  reasonCode newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void reasonCodes::sDelete()
{
  XSqlQuery reasonDelete;
  reasonDelete.prepare( "SELECT cmhead_id "
             "FROM cmhead "
             "WHERE (cmhead_rsncode_id=:rsncode_id) "
             "LIMIT 1;" );
  reasonDelete.bindValue(":rsncode_id", _rsncode->id());
  reasonDelete.exec();
  if (reasonDelete.first())
  {
    QMessageBox::critical( this, tr("Cannot Delete Reason Code"),
                           tr( "You may not delete the selected Reason Code as there are Credit Memo records that refer it.\n"
                               "You must purge these records before you may delete the selected Reason Code." ) );
    return;
  }

  reasonDelete.prepare( "SELECT aropen_id "
             "FROM aropen "
             "WHERE (aropen_rsncode_id=:rsncode_id) "
             "LIMIT 1;" );
  reasonDelete.bindValue(":rsncode_id", _rsncode->id());
  reasonDelete.exec();
  if (reasonDelete.first())
  {
    QMessageBox::critical( this, tr("Cannot Delete Reason Code"),
                           tr( "You may not delete the selected Reason Code as there are A/R Open Item records that refer it.\n"
                               "You must purge these records before you may delete the selected Reason Code." ) );
    return;
  }

  reasonDelete.prepare( "DELETE FROM rsncode "
             "WHERE (rsncode_id=:rsncode_id);" );
  reasonDelete.bindValue(":rsncode_id", _rsncode->id());
  reasonDelete.exec();

  sFillList();
}

void reasonCodes::sFillList()
{
  _rsncode->populate( "SELECT rsncode_id, rsncode_code, rsncode_descrip "
	               "FROM rsncode "
	               "ORDER BY rsncode_code;" );
}

void reasonCodes::sPopulateMenu( QMenu * )
{

}

void reasonCodes::sPrint()
{
  orReport report("ReasonCodeMasterList");
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

