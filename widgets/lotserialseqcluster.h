/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef _lotserialseqCluster_h

#define _lotserialseqCluster_h

#include "virtualCluster.h"

class XTUPLEWIDGETS_EXPORT LotserialseqClusterLineEdit : public VirtualClusterLineEdit
{
    Q_OBJECT

    public:
      LotserialseqClusterLineEdit(QWidget*, const char* = 0);
};

class XTUPLEWIDGETS_EXPORT LotserialseqCluster : public VirtualCluster
{
    Q_OBJECT

    public:
      LotserialseqCluster(QWidget*, const char* = 0);

};

#endif
