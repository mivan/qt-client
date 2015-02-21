/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "rejectCodes.h"

#include <QVariant>
#include <QMessageBox>
#include <QMenu>
#include <openreports.h>
#include "rejectCode.h"

rejectCodes::rejectCodes(QWidget* parent, const char* name, Qt::WFlags fl)
  : XWidget(parent, name, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_rjctcode, SIGNAL(valid(bool)), _view, SLOT(setEnabled(bool)));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_view, SIGNAL(clicked()), this, SLOT(sView()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_rjctcode, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));

  if (_privileges->check("MaintainRejectCodes"))
  {
    connect(_rjctcode, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_rjctcode, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_rjctcode, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_rjctcode, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  _rjctcode->addColumn(tr("Code"),        _itemColumn, Qt::AlignLeft, true, "rjctcode_code" );
  _rjctcode->addColumn(tr("Description"), -1,          Qt::AlignLeft, true, "rjctcode_descrip" );
    
  sFillList();
}

rejectCodes::~rejectCodes()
{
  // no need to delete child widgets, Qt does it all for us
}

void rejectCodes::languageChange()
{
  retranslateUi(this);
}

void rejectCodes::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  rejectCode newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void rejectCodes::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("rjctcode_id", _rjctcode->id());

  rejectCode newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void rejectCodes::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("rjctcode_id", _rjctcode->id());

  rejectCode newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void rejectCodes::sDelete()
{
  XSqlQuery rejectDelete;
  rejectDelete.prepare( "SELECT poreject_id "
             "FROM poreject "
             "WHERE (poreject_rjctcode_id=:rjctcode_id) "
             "LIMIT 1;" );
  rejectDelete.bindValue(":rjctcode_id", _rjctcode->id());
  rejectDelete.exec();
  if (rejectDelete.first())
  {
    QMessageBox::critical( this, tr("Cannot Delete Reject Code"),
                           tr( "You may not delete the selected Reject Code as there are Material Reject records that refer it.\n"
                               "You must purge these records before you may delete the selected Reject Code." ) );
    return;
  }

  rejectDelete.prepare( "DELETE FROM rjctcode "
             "WHERE (rjctcode_id=:rjctcode_id);" );
  rejectDelete.bindValue(":rjctcode_id", _rjctcode->id());
  rejectDelete.exec();

  sFillList();
}

void rejectCodes::sFillList()
{
  _rjctcode->populate( "SELECT rjctcode_id, rjctcode_code, rjctcode_descrip "
	               "FROM rjctcode "
	               "ORDER BY rjctcode_code;" );
}

void rejectCodes::sPopulateMenu( QMenu * )
{

}

void rejectCodes::sPrint()
{
  orReport report("RejectCodeMasterList");
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

