/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "taxRegistrations.h"

#include <QVariant>
#include <QMessageBox>
#include <QSqlError>
#include <parameter.h>
#include <QWorkspace>
#include "taxRegistration.h"

taxRegistrations::taxRegistrations(QWidget* parent, const char* name, Qt::WFlags fl)
  : XWidget(parent, name, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_view, SIGNAL(clicked()), this, SLOT(sView()));

  if (_privileges->check("MaintainTaxRegistrations"))
  {
    connect(_taxreg, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_taxreg, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_taxreg, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_taxreg, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  _taxreg->addColumn(tr("Tax Zone"),  100,  Qt::AlignCenter,   true,  "taxzone_code"   );
  _taxreg->addColumn(tr("Tax Authority"),  100,  Qt::AlignCenter, true,  "taxauth_code" );
  _taxreg->addColumn(tr("Registration #"),  100,  Qt::AlignLeft,   true,  "taxreg_number"   );
  _taxreg->addColumn(tr("Start Date"),  100,  Qt::AlignLeft,   true,  "taxreg_effective"   );
  _taxreg->addColumn(tr("End Date"),  -1,  Qt::AlignLeft,   true,  "taxreg_expires"   );

  sFillList();
}

taxRegistrations::~taxRegistrations()
{
  // no need to delete child widgets, Qt does it all for us
}

void taxRegistrations::languageChange()
{
  retranslateUi(this);
}

void taxRegistrations::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  taxRegistration newdlg(this, "", TRUE);
  newdlg.set(params);
  if(newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void taxRegistrations::sEdit()
{
  ParameterList params;
  params.append("taxreg_id", _taxreg->id());
  params.append("mode", "edit");

  taxRegistration newdlg(this, "", TRUE);
  newdlg.set(params);
  if(newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void taxRegistrations::sView()
{
  ParameterList params;
  params.append("taxreg_id", _taxreg->id());
  params.append("mode", "view");

  taxRegistration newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void taxRegistrations::sDelete()
{
  XSqlQuery taxDelete;
  taxDelete.prepare("DELETE FROM taxreg"
            " WHERE (taxreg_id=:taxreg_id);");
  taxDelete.bindValue(":taxreg_id", _taxreg->id());
  taxDelete.exec();
  if (taxDelete.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taxDelete.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  sFillList();
}

void taxRegistrations::sFillList()
{
  XSqlQuery taxFillList;
  taxFillList.prepare("SELECT taxreg_id, taxreg_taxzone_id, taxreg_taxauth_id, "
            "       CASE WHEN taxreg_taxzone_id ISNULL THEN '~Any~' "
			"		ELSE taxzone_code "
			"		END AS taxzone_code, "
			"		taxauth_code, taxreg_number, "
			"		taxreg_effective, taxreg_expires "
            "  FROM taxreg LEFT OUTER JOIN taxauth ON (taxreg_taxauth_id = taxauth_id) "
			"		LEFT OUTER JOIN taxzone ON (taxreg_taxzone_id = taxzone_id)"
            " WHERE (taxreg_rel_type IS NULL)"
			" ORDER BY taxzone_code, taxauth_code, taxreg_number;");
  taxFillList.exec();
  _taxreg->populate(taxFillList, true);
  if (taxFillList.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taxFillList.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

