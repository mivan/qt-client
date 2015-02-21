/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "commentTypes.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <openreports.h>
#include "commentType.h"

commentTypes::commentTypes(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  if (_privileges->check("MaintainCommentTypes"))
    connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  else
    _new->setEnabled(false);
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_cmnttype, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(sHandleButtons()));

  _cmnttype->addColumn(tr("Name"), _itemColumn, Qt::AlignLeft,  true, "cmnttype_name");
  _cmnttype->addColumn(tr("Sys."),   _ynColumn, Qt::AlignCenter,true, "cmnttype_sys");
  _cmnttype->addColumn(tr("Description"),   -1, Qt::AlignLeft,  true, "cmnttype_descrip");
  _cmnttype->addColumn(tr("Order"),  _ynColumn, Qt::AlignLeft, false, "cmnttype_order");

  sFillList();
}

commentTypes::~commentTypes()
{
  // no need to delete child widgets, Qt does it all for us
}

void commentTypes::languageChange()
{
  retranslateUi(this);
}

void commentTypes::sFillList()
{
  XSqlQuery commentFillList;
  commentFillList.prepare( "SELECT cmnttype_id, cmnttype_name,"
             "       cmnttype_sys, cmnttype_descrip, cmnttype_order "
             "FROM cmnttype "
             "ORDER BY cmnttype_order, cmnttype_name;" );
  commentFillList.exec();
  _cmnttype->populate(commentFillList);
  if (commentFillList.lastError().type() != QSqlError::NoError)
  {
    systemError(this, commentFillList.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void commentTypes::sHandleButtons()
{
  XTreeWidgetItem *selected = (XTreeWidgetItem*)_cmnttype->currentItem();
  if (selected && _privileges->check("MaintainCommentTypes"))
  {
    _edit->setEnabled(true);
    _delete->setEnabled(!selected->rawValue("cmnttype_sys").toBool());
  }
  else
  {
    _edit->setEnabled(false);
    _delete->setEnabled(false);
  }
}

void commentTypes::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  commentType newdlg(this, "", TRUE);
  newdlg.set(params);
  
  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void commentTypes::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("cmnttype_id", _cmnttype->id());

  commentType newdlg(this, "", TRUE);
  newdlg.set(params);
  
  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void commentTypes::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("cmnttype_id", _cmnttype->id());

  commentType newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void commentTypes::sDelete()
{
  XSqlQuery commentDelete;
  commentDelete.prepare( "SELECT comment_id "
             "FROM comment "
             "WHERE (comment_cmnttype_id=:cmnttype_id) "
             "LIMIT 1;" );
  commentDelete.bindValue(":cmnttype_id", _cmnttype->id());
  commentDelete.exec();
  if (commentDelete.first())
  {
    QMessageBox::critical( this, tr("Cannot Delete Comment Type"),
                           tr("The selected Comment Type cannot be deleted because there are Comments that are assigned to it.\n") );
    return;
  }
  else
  {
    commentDelete.prepare( "DELETE FROM cmnttypesource "
               "WHERE (cmnttypesource_cmnttype_id=:cmnttype_id);" );
    commentDelete.bindValue(":cmnttype_id", _cmnttype->id());
    commentDelete.exec();

    commentDelete.prepare( "DELETE FROM cmnttype "
               "WHERE (cmnttype_id=:cmnttype_id);" );
    commentDelete.bindValue(":cmnttype_id", _cmnttype->id());
    commentDelete.exec();

    sFillList();
  }
}
