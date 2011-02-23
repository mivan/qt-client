/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef DSPTIMEPHASEDBOOKINGSBYITEM_H
#define DSPTIMEPHASEDBOOKINGSBYITEM_H

#include "displayTimePhased.h"

#include "ui_dspTimePhasedBookingsByItem.h"

class dspTimePhasedBookingsByItem : public displayTimePhased, public Ui::dspTimePhasedBookingsByItem
{
    Q_OBJECT

public:
    dspTimePhasedBookingsByItem(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);

    virtual bool setParamsTP(ParameterList &);

public slots:
    virtual void sViewBookings();
    virtual void sPopulateMenu(QMenu * pMenu, QTreeWidgetItem * pSelected, int pColumn);

protected slots:
    virtual void languageChange();

};

#endif // DSPTIMEPHASEDBOOKINGSBYITEM_H