/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "contactwidget.h"
#include <QtScript>

void setupContactWidget(QScriptEngine *engine)
{
  QScriptValue widget = engine->newObject();

  widget.setProperty("Employee",  QScriptValue(engine, ContactWidget::Employee),  QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Customer",  QScriptValue(engine, ContactWidget::Customer),  QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Vendor",	  QScriptValue(engine, ContactWidget::Vendor),    QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Partner ",  QScriptValue(engine, ContactWidget::Partner),   QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Prospect",  QScriptValue(engine, ContactWidget::Prospect),  QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Competitor",QScriptValue(engine, ContactWidget::Competitor),QScriptValue::ReadOnly | QScriptValue::Undeletable);

  engine->globalObject().setProperty("ContactWidget", widget, QScriptValue::ReadOnly | QScriptValue::Undeletable);
}
