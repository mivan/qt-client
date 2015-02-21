/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "createInvoices.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <openreports.h>

#include "storedProcErrorLookup.h"

createInvoices::createInvoices(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_post, SIGNAL(clicked()), this, SLOT(sPost()));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));

  _customerType->setType(ParameterGroup::CustomerType);
}

createInvoices::~createInvoices()
{
  // no need to delete child widgets, Qt does it all for us
}

void createInvoices::languageChange()
{
  retranslateUi(this);
}

void createInvoices::sPost()
{
  XSqlQuery createPost;
  QString sql("SELECT createInvoices(custtype_id, :consolidate) AS result"
              " FROM custtype" );
  if (_customerType->isSelected())
    sql += " WHERE (custtype_id=:custtype_id)";
  else if (_customerType->isPattern())
    sql += " WHERE (custtype_code ~ :custtype_pattern)";
  createPost.prepare(sql);
  _customerType->bindValue(createPost);
  createPost.bindValue(":consolidate", _consolidate->isChecked());
  createPost.exec();
  if (createPost.first())
  {
    int result = createPost.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("createInvoices", result),
                  __FILE__, __LINE__);
      return;
    }
    omfgThis->sInvoicesUpdated(-1, TRUE);
    omfgThis->sBillingSelectionUpdated(-1, TRUE);
    omfgThis->sSalesOrdersUpdated(-1);
  }
  else if (createPost.lastError().type() != QSqlError::NoError)
  {
    systemError(this, createPost.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  accept();
}

