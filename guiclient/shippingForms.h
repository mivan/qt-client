/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef SHIPPINGFORMS_H
#define SHIPPINGFORMS_H

#include "guiclient.h"
#include "xwidget.h"
#include "ui_shippingForms.h"

class shippingForms : public XWidget, public Ui::shippingForms
{
    Q_OBJECT

public:
    shippingForms(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~shippingForms();

public slots:
    virtual void sNew();
    virtual void sEdit();
    virtual void sView();
    virtual void sDelete();
    virtual void sFillList();

protected slots:
    virtual void languageChange();

};

#endif // SHIPPINGFORMS_H
