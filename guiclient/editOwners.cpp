/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "editOwners.h"

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>
#include <QVariant>

#include "storedProcErrorLookup.h"

editOwners::editOwners(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  XSqlQuery editeditOwners;
  setupUi(this);

  connect(_close,	SIGNAL(clicked()),	this,	SLOT(sClose()));
  connect(_query,	SIGNAL(clicked()),	this,	SLOT(sFillList()));
  connect(_modify,  	SIGNAL(clicked()),	this,	SLOT(sModify()));
  connect(_modifyAll,   SIGNAL(clicked()),      this,   SLOT(sModifyAll()));
  connect(_list, 	SIGNAL(itemClicked(QTreeWidgetItem*, int)),	this, 	SLOT(sItemClicked()));

  _list->addColumn(tr("Type"),    _statusColumn,  Qt::AlignCenter, true, "type_name");
  _list->addColumn(tr("Name"),              100,  Qt::AlignLeft,   true, "name");
  _list->addColumn(tr("Description"),        -1,  Qt::AlignLeft,   true, "description");
  _list->addColumn(tr("Owner"),     _userColumn,  Qt::AlignLeft,   true, "owner_username");

  _modify->setEnabled(false);
  _modifyAll->setEnabled(false);

  _newOwner->setType(UsernameLineEdit::UsersActive);

  _first = true;

  editeditOwners.prepare("SELECT usr_id "
	    "FROM usr "
	    "WHERE (usr_username=getEffectiveXtUser());");
  editeditOwners.exec();
  if (editeditOwners.first())
  {
    _owner->setId(editeditOwners.value("usr_id").toInt());
  }  
  else if (editeditOwners.lastError().type() != QSqlError::NoError)
  {
    systemError(this, editeditOwners.lastError().databaseText(), __FILE__, __LINE__);
    reject();
  }
}

void editOwners::sFillList()
{
  XSqlQuery editFillList;

  if(_todo->isChecked())
  {
    _queryString += "SELECT todoitem_id AS id, "
                    "       todoitem_owner_username AS owner_username, "
                    "       todoitem_name AS name, "
                    "       todoitem_description AS description, "
                    "       'To Do' AS type_name, "
                    "       'todoitem' AS table "
                    "FROM todoitem "
                    "WHERE todoitem_owner_username = :owner ";
    _first = false;
  }
  if(_project->isChecked())
  {
    if(!_first) _queryString += "UNION ALL ";
    _queryString += "SELECT prj_id AS id, "
                    "       prj_owner_username AS owner_username, "
                    "       prj_name AS name, "
                    "       prj_descrip AS description, "
                    "       'Project' AS type_name, "
                    "       'prj' AS table "
                    "FROM prj "
                    "WHERE prj_owner_username = :owner ";
    _first = false;
  }
  if(_contact->isChecked())
  {
    if(!_first) _queryString += "UNION ALL ";
    _queryString += "SELECT cntct_id AS id, "
                    "       cntct_owner_username AS owner_username, "
                    "       cntct_first_name || ' ' || cntct_last_name AS name, "
                    "       '' AS description, "
                    "       'Contact' AS type_name, "
                    "       'cntct' AS table "
                    "FROM cntct "
                    "WHERE cntct_owner_username = :owner ";
    _first = false;
  }
  if(_incident->isChecked())
  {
    if(!_first) _queryString += "UNION ALL ";
    _queryString += "SELECT incdt_id AS id, "
                    "       incdt_owner_username AS owner_username, "
                    "       incdt_summary AS name, "
                    "       incdt_descrip AS description, "
                    "       'Incident' AS type_name, "
                    "       'incdt' AS table "
                    "FROM incdt "
                    "WHERE incdt_owner_username = :owner ";
    _first = false;
  }
  if(_account->isChecked())
  {
    if(!_first) _queryString += "UNION ALL ";
    _queryString += "SELECT crmacct_id AS id, "
                    "       crmacct_owner_username AS owner_username, "
                    "       crmacct_name AS name, "
                    "       crmacct_notes AS description, "
                    "       'Account' AS type_name, "
                    "       'crmacct' AS table "
                    "FROM crmacct "
                    "WHERE crmacct_owner_username = :owner ";
    _first = false;
  }
  if(_oppourtunity->isChecked())
  {
    if(!_first) _queryString += "UNION ALL ";
    _queryString += "SELECT ophead_id AS id, "
                    "       ophead_owner_username AS owner_username, "
                    "       ophead_name AS name, "
                    "       ophead_notes AS description, "
                    "       'Opportunity' AS type_name, "
                    "       'ophead' AS table "
                    "FROM ophead "
                    "WHERE ophead_owner_username = :owner ";
    _first = false;
  }

  if(_queryString == "")
    _list->clear();
  else
  {
    editFillList.prepare(_queryString);
    editFillList.bindValue(":owner", _owner->username());
    editFillList.exec();
    _list->populate(editFillList);
  }

  _modifyAll->setEnabled(_list->topLevelItemCount() > 0);
  _modify->setEnabled(false);
  _first = true;
  _queryString = "";
}

void editOwners::sClose()
{
  close();
}

void editOwners::sModify()
{
  QString table;

  if(!_newOwner->isValid())
  {
    QMessageBox::critical( this, tr("No New Owner"),
      tr("A new owner must be selected before you can continue."));
    _newOwner->setFocus();
    return;
  }
  int ret = QMessageBox::warning(this, tr("Confirm Ownership Modification"),
                   tr("<p>Are you sure that you want to change the new owner "
                      "to '%1' for the selected records?")
                      .arg(_newOwner->username()),
                   QMessageBox::Yes | QMessageBox::No,
                   QMessageBox::Yes);

  if(ret == QMessageBox::Yes)
  {
  if (modifyOne(_list->currentItem()))
    sFillList();
  }
}

void editOwners::sModifyAll()
{
  if(!_newOwner->isValid())
  {
    QMessageBox::critical( this, tr("No New Owner"),
      tr("A new owner must be selected before you can continue."));
    _newOwner->setFocus();
    return;
  }

  int ret = QMessageBox::warning(this, tr("Confirm Ownership Modification"),
                   tr("<p>Are you sure that you want to change the new owner "
                      "to '%1' for the selected records?")
                      .arg(_newOwner->username()),
                   QMessageBox::Yes | QMessageBox::No,
                   QMessageBox::Yes);
  
  if(ret == QMessageBox::Yes)
  {

    QList<XTreeWidgetItem*> all = _list->findItems("", Qt::MatchContains);

    for (int i = 0; i < all.size(); i++)
    {
      XTreeWidgetItem *currentItem = static_cast<XTreeWidgetItem*>(all[i]);
      if (currentItem->rawValue("type_name").toString() != "")
        modifyOne(currentItem);
    }
    sFillList();
  }
}

bool editOwners::modifyOne(XTreeWidgetItem * currentItem)
{
  XSqlQuery editmodifyOne;
  QString table;

  if(currentItem->rawValue("type_name").toString() == "To Do") table = "todoitem";
  if(currentItem->rawValue("type_name").toString() == "Project") table = "prj";
  if(currentItem->rawValue("type_name").toString() == "Contact") table = "cntct";
  if(currentItem->rawValue("type_name").toString() == "Incident") table = "incdt";
  if(currentItem->rawValue("type_name").toString() == "Account") table = "crmacct";
  if(currentItem->rawValue("type_name").toString() == "Opportunity") table = "ophead";

  editmodifyOne.prepare("UPDATE "+table+" "
            "SET "+table+"_owner_username = :new_owner_username "
            "WHERE "+table+"_id = :id ");
  editmodifyOne.bindValue(":new_owner_username", _newOwner->username());
  editmodifyOne.bindValue(":id", currentItem->id());
  editmodifyOne.exec();
  if (editmodifyOne.lastError().type() != QSqlError::NoError)
  {
    systemError(this, editmodifyOne.lastError().databaseText(), __FILE__, __LINE__);
    return false;
  }
  return true;
}

void editOwners::sItemClicked()
{
  _modify->setEnabled(true);
}

