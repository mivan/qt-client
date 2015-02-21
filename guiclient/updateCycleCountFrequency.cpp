/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "updateCycleCountFrequency.h"

#include <QVariant>

updateCycleCountFrequency::updateCycleCountFrequency(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
  : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_update, SIGNAL(clicked()), this, SLOT(sUpdate()));

  _classCode->setType(ParameterGroup::ClassCode);
}

updateCycleCountFrequency::~updateCycleCountFrequency()
{
  // no need to delete child widgets, Qt does it all for us
}

void updateCycleCountFrequency::languageChange()
{
  retranslateUi(this);
}

enum SetResponse updateCycleCountFrequency::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("aFrequency", &valid);
  if (valid)
    _classAFrequency->setValue(param.toInt());
  else
    _classAFrequency->setValue(0);

  param = pParams.value("bFrequency", &valid);
  if (valid)
    _classBFrequency->setValue(param.toInt());
  else
    _classBFrequency->setValue(0);

  param = pParams.value("cFrequency", &valid);
  if (valid)
    _classCFrequency->setValue(param.toInt());
  else
    _classCFrequency->setValue(0);

  param = pParams.value("warehous_id", &valid);
  if (valid)
    _warehouse->setId(param.toInt());

  param = pParams.value("classcode_id", &valid);
  if (valid)
    _classCode->setId(param.toInt());

  param = pParams.value("classcode_code", &valid);
  if (valid)
    _classCode->setPattern(param.toString());

  if (pParams.inList("run"))
  {
    sUpdate();
    return NoError_Run;
  }

  return NoError;
}

void updateCycleCountFrequency::sUpdate()
{
  XSqlQuery updateUpdate;
  QString sql( "UPDATE itemsite "
               "SET itemsite_cyclecountfreq=:afreq "
               "FROM item "
               "WHERE ( (itemsite_item_id=item_id)"
               " AND (itemsite_abcclass='A')" );

  if (_warehouse->isSelected())
    sql += " AND (itemsite_warehous_id=:warehous_id)";

  if (_classCode->isSelected())
    sql += " AND (item_classcode_id=:classcode_id)";
  else if (_classCode->isPattern())
    sql += " AND (item_classcode_id IN (SELECT classcode_id FROM classcode WHERE (classcode_code ~ :classcode_pattern)))";

  sql += "); "
         "UPDATE itemsite "
         "SET itemsite_cyclecountfreq=:bfreq "
         "FROM item "
         "WHERE ( (itemsite_item_id=item_id)"
         " AND (itemsite_abcclass='B')" ;

  if (_warehouse->isSelected())
    sql += " AND (itemsite_warehous_id=:warehous_id)";

  if (_classCode->isSelected())
    sql += " AND (item_classcode_id=:classcode_id)";
  else if (_classCode->isPattern())
    sql += " AND (item_classcode_id IN (SELECT classcode_id FROM classcode WHERE (classcode_code ~ :classcode_pattern)))";

  sql += "); "
         "UPDATE itemsite "
         "SET itemsite_cyclecountfreq=:cfreq "
         "FROM item "
         "WHERE ( (itemsite_item_id=item_id)"
         " AND (itemsite_abcclass='C')" ;

  if (_warehouse->isSelected())
    sql += " AND (itemsite_warehous_id=:warehous_id)";

  if (_classCode->isSelected())
    sql += " AND (item_classcode_id=:classcode_id)";
  else if (_classCode->isPattern())
    sql += " AND (item_classcode_id IN (SELECT classcode_id FROM classcode WHERE (classcode_code ~ :classcode_pattern)))";

  sql += ");";

  updateUpdate.prepare(sql);
  updateUpdate.bindValue(":afreq", _classAFrequency->value());
  updateUpdate.bindValue(":bfreq", _classBFrequency->value());
  updateUpdate.bindValue(":cfreq", _classCFrequency->value());
  _warehouse->bindValue(updateUpdate);
  _classCode->bindValue(updateUpdate);
  updateUpdate.exec();

  accept();
}
