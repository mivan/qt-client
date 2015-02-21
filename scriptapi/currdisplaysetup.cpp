/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "currcluster.h"
#include <QtScript>

void setupCurrDisplay(QScriptEngine *engine)
{
  QScriptValue widget = engine->newObject();

  widget.setProperty("Money",	  QScriptValue(engine, CurrDisplay::Money),	QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("SalesPrice",QScriptValue(engine, CurrDisplay::SalesPrice),QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("PurchPrice",QScriptValue(engine, CurrDisplay::PurchPrice),QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ExtPrice",  QScriptValue(engine, CurrDisplay::ExtPrice),  QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Cost",	  QScriptValue(engine, CurrDisplay::Cost),	QScriptValue::ReadOnly | QScriptValue::Undeletable);

  engine->globalObject().setProperty("CurrDisplay", widget, QScriptValue::ReadOnly | QScriptValue::Undeletable);
}
