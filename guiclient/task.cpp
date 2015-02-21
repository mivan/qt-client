/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "task.h"

#include <QSqlError>
#include <QVariant>
#include <QMessageBox>

#include "errorReporter.h"
#include "guiErrorCheck.h"
#include "userList.h"
#include "characteristicAssignment.h"

const char *_taskStatuses[] = { "P", "O", "C" };

task::task(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_buttonBox, SIGNAL(accepted()), this, SLOT(sSave()));
  connect(_actualExp, SIGNAL(editingFinished()), this, SLOT(sExpensesAdjusted()));
  connect(_budgetExp, SIGNAL(editingFinished()), this, SLOT(sExpensesAdjusted()));
  connect(_actualHours, SIGNAL(editingFinished()), this, SLOT(sHoursAdjusted()));
  connect(_budgetHours, SIGNAL(editingFinished()), this, SLOT(sHoursAdjusted()));
  connect(_newCharacteristic, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_editCharacteristic, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_deleteCharacteristic, SIGNAL(clicked()), this, SLOT(sDelete()));

  
  _budgetHours->setValidator(omfgThis->qtyVal());
  _actualHours->setValidator(omfgThis->qtyVal());
  _budgetExp->setValidator(omfgThis->costVal());
  _actualExp->setValidator(omfgThis->costVal());
  _balanceHours->setPrecision(omfgThis->qtyVal());
  _balanceExp->setPrecision(omfgThis->costVal());

  _prjid = -1;
  _prjtaskid = -1;
  
  _owner->setType(UsernameLineEdit::UsersActive);
  _assignedTo->setType(UsernameLineEdit::UsersActive);

  _charass->addColumn(tr("Characteristic"), _itemColumn, Qt::AlignLeft, true, "char_name" );
  _charass->addColumn(tr("Value"),          -1,          Qt::AlignLeft, true, "charass_value" );
  _charass->addColumn(tr("Default"),        _ynColumn*2,   Qt::AlignCenter, true, "charass_default" );

}

task::~task()
{
  // no need to delete child widgets, Qt does it all for us
}

void task::languageChange()
{
  retranslateUi(this);
}

enum SetResponse task::set(const ParameterList &pParams)
{
  XSqlQuery tasket;
  XDialog::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("prj_id", &valid);
  if (valid)
    _prjid = param.toInt();

  param = pParams.value("prj_owner_username", &valid);
  if (valid)
    _owner->setUsername(param.toString());

  param = pParams.value("prj_username", &valid);
  if (valid)
    _assignedTo->setUsername(param.toString());

  param = pParams.value("prj_start_date", &valid);
  if (valid)
    _started->setDate(param.toDate());

  param = pParams.value("prj_assigned_date", &valid);
  if (valid)
    _assigned->setDate(param.toDate());

  param = pParams.value("prj_due_date", &valid);
  if (valid)
    _due->setDate(param.toDate());

  param = pParams.value("prj_completed_date", &valid);
  if (valid)
    _completed->setDate(param.toDate());

  param = pParams.value("prjtask_id", &valid);
  if (valid)
  {
    _prjtaskid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      tasket.exec("SELECT NEXTVAL('prjtask_prjtask_id_seq') AS prjtask_id;");
      if (tasket.first())
        _prjtaskid = tasket.value("prjtask_id").toInt();
      else
      {
        systemError(this, tr("A System Error occurred at %1::%2.")
                          .arg(__FILE__)
                          .arg(__LINE__) );
      }

      connect(_assignedTo, SIGNAL(newId(int)), this, SLOT(sAssignedToChanged(int)));
      connect(_status,  SIGNAL(currentIndexChanged(int)), this, SLOT(sStatusChanged(int)));
      connect(_charass, SIGNAL(valid(bool)), _editCharacteristic, SLOT(setEnabled(bool)));
      connect(_charass, SIGNAL(valid(bool)), _deleteCharacteristic, SLOT(setEnabled(bool)));

      _alarms->setId(_prjtaskid);
      _comments->setId(_prjtaskid);
      _documents->setId(_prjtaskid);

    }
    if (param.toString() == "edit")
    {
      _mode = cEdit;

      connect(_assignedTo, SIGNAL(newId(int)), this, SLOT(sAssignedToChanged(int)));
      connect(_status,  SIGNAL(currentIndexChanged(int)), this, SLOT(sStatusChanged(int)));
      connect(_charass, SIGNAL(valid(bool)), _editCharacteristic, SLOT(setEnabled(bool)));
      connect(_charass, SIGNAL(valid(bool)), _deleteCharacteristic, SLOT(setEnabled(bool)));
    }
    if (param.toString() == "view")
    {
      _mode = cView;

      _number->setEnabled(false);
      _name->setEnabled(false);
      _descrip->setEnabled(false);
      _status->setEnabled(false);
      _budgetHours->setEnabled(false);
      _actualHours->setEnabled(false);
      _budgetExp->setEnabled(false);
      _actualExp->setEnabled(false);
      _owner->setEnabled(false);
      _assignedTo->setEnabled(false);
      _due->setEnabled(false);
      _assigned->setEnabled(false);
      _started->setEnabled(false);
      _completed->setEnabled(false);
      _alarms->setEnabled(false);
      _comments->setReadOnly(true);
      _newCharacteristic->setEnabled(FALSE);
      _buttonBox->clear();
      _buttonBox->addButton(QDialogButtonBox::Close);
      _documents->setReadOnly(TRUE);
    }
  }

  return NoError;
}

void task::populate()
{
  XSqlQuery taskpopulate;
  taskpopulate.prepare( "SELECT prjtask.* "
             "FROM prjtask "
             "WHERE (prjtask_id=:prjtask_id);" );
  taskpopulate.bindValue(":prjtask_id", _prjtaskid);
  taskpopulate.exec();
  if (taskpopulate.first())
  {
    _number->setText(taskpopulate.value("prjtask_number"));
    _name->setText(taskpopulate.value("prjtask_name"));
    _descrip->setText(taskpopulate.value("prjtask_descrip").toString());
    _owner->setUsername(taskpopulate.value("prjtask_owner_username").toString());
    _assignedTo->setUsername(taskpopulate.value("prjtask_username").toString());
    _started->setDate(taskpopulate.value("prjtask_start_date").toDate());
    _assigned->setDate(taskpopulate.value("prjtask_assigned_date").toDate());
    _due->setDate(taskpopulate.value("prjtask_due_date").toDate());
    _completed->setDate(taskpopulate.value("prjtask_completed_date").toDate());

    for (int counter = 0; counter < _status->count(); counter++)
    {
      if (QString(taskpopulate.value("prjtask_status").toString()[0]) == _taskStatuses[counter])
        _status->setCurrentIndex(counter);
    }

    _budgetHours->setText(formatQty(taskpopulate.value("prjtask_hours_budget").toDouble()));
    _actualHours->setText(formatQty(taskpopulate.value("prjtask_hours_actual").toDouble()));
    _budgetExp->setText(formatCost(taskpopulate.value("prjtask_exp_budget").toDouble()));
    _actualExp->setText(formatCost(taskpopulate.value("prjtask_exp_actual").toDouble()));

    _alarms->setId(_prjtaskid);
    _comments->setId(_prjtaskid);   
    _documents->setId(_prjtaskid); 
    _documents->setType(Documents::ProjectTask);
    sHoursAdjusted();
    sExpensesAdjusted();
    sFillList();

    //if (taskpopulate.value("prjtask_anyuser").toBool())
    //  _anyUser->setChecked(TRUE);
    //else
    //  _userList->setChecked(TRUE);
  }
  else if (taskpopulate.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taskpopulate.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  //sFillUserList();
}

void task::sNew()
{
  ParameterList params;
  params.append("mode", "new");
  params.append("prjtask_id", _prjtaskid);

  characteristicAssignment newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void task::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("charass_id", _charass->id());

  characteristicAssignment newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void task::sDelete()
{
  XSqlQuery taskDelete;
  taskDelete.prepare( "DELETE FROM charass "
             "WHERE (charass_id=:charass_id);" );
  taskDelete.bindValue(":charass_id", _charass->id());
  taskDelete.exec();

  sFillList();
}

void task::sFillList()
{
  XSqlQuery taskFillList;
  taskFillList.prepare( "SELECT charass_id, char_name, "
             " CASE WHEN char_type < 2 THEN "
             "   charass_value "
             " ELSE "
             "   formatDate(charass_value::date) "
             "END AS charass_value, "
             " charass_default "
             "FROM charass, char "
             "WHERE ( (charass_target_type='TASK')"
             " AND (charass_char_id=char_id)"
             " AND (charass_target_id=:prjtask_id) ) "
             "ORDER BY char_order, char_name;" );
  taskFillList.bindValue(":prjtask_id", _prjtaskid);
  taskFillList.exec();
  _charass->populate(taskFillList);
}

void task::sSave()
{
  XSqlQuery taskSave;
  QList<GuiErrorCheck> errors;
  errors<< GuiErrorCheck(_number->text().length() == 0, _number,
                         tr("You must enter a valid Number."))
        << GuiErrorCheck(_name->text().length() == 0, _name,
                         tr("You must enter a valid Name."))
        << GuiErrorCheck(!_due->isValid(), _due,
                         tr("You must enter a valid due date."))
  ;
  if (GuiErrorCheck::reportErrors(this, tr("Cannot Save Project Task"), errors))
    return;

  if (_mode == cNew)
  {
    taskSave.prepare( "INSERT INTO prjtask "
               "( prjtask_id, prjtask_prj_id, prjtask_number,"
               "  prjtask_name, prjtask_descrip, prjtask_status,"
               "  prjtask_hours_budget, prjtask_hours_actual,"
               "  prjtask_exp_budget, prjtask_exp_actual,"
               "  prjtask_start_date, prjtask_due_date,"
               "  prjtask_assigned_date, prjtask_completed_date,"
               "  prjtask_owner_username, prjtask_username ) "
               "VALUES "
               "( :prjtask_id, :prjtask_prj_id, :prjtask_number,"
               "  :prjtask_name, :prjtask_descrip, :prjtask_status,"
               "  :prjtask_hours_budget, :prjtask_hours_actual,"
               "  :prjtask_exp_budget, :prjtask_exp_actual,"
               "  :prjtask_start_date, :prjtask_due_date,"
               "  :prjtask_assigned_date, :prjtask_completed_date,"
               "  :prjtask_owner_username, :username );" );
    taskSave.bindValue(":prjtask_prj_id", _prjid);
  }
  else if (_mode == cEdit)
    taskSave.prepare( "UPDATE prjtask "
               "SET prjtask_number=:prjtask_number, prjtask_name=:prjtask_name,"
               "    prjtask_descrip=:prjtask_descrip, prjtask_status=:prjtask_status,"
               "    prjtask_hours_budget=:prjtask_hours_budget,"
               "    prjtask_hours_actual=:prjtask_hours_actual,"
               "    prjtask_exp_budget=:prjtask_exp_budget,"
               "    prjtask_exp_actual=:prjtask_exp_actual,"
               "    prjtask_owner_username=:prjtask_owner_username,"
               "    prjtask_username=:username,"
               "    prjtask_start_date=:prjtask_start_date,"
               "    prjtask_due_date=:prjtask_due_date,"
               "    prjtask_assigned_date=:prjtask_assigned_date,"
               "    prjtask_completed_date=:prjtask_completed_date "
               "WHERE (prjtask_id=:prjtask_id);" );

  taskSave.bindValue(":prjtask_id", _prjtaskid);
  taskSave.bindValue(":prjtask_number", _number->text());
  taskSave.bindValue(":prjtask_name", _name->text());
  taskSave.bindValue(":prjtask_descrip", _descrip->toPlainText());
  taskSave.bindValue(":prjtask_status", _taskStatuses[_status->currentIndex()]);
  taskSave.bindValue(":prjtask_owner_username", _owner->username());
  taskSave.bindValue(":username",   _assignedTo->username());
  taskSave.bindValue(":prjtask_start_date", _started->date());
  taskSave.bindValue(":prjtask_due_date", _due->date());
  taskSave.bindValue(":prjtask_assigned_date",	_assigned->date());
  taskSave.bindValue(":prjtask_completed_date", _completed->date());
  //taskSave.bindValue(":prjtask_anyuser", QVariant(_anyUser->isChecked()));
  taskSave.bindValue(":prjtask_hours_budget", _budgetHours->text().toDouble());
  taskSave.bindValue(":prjtask_hours_actual", _actualHours->text().toDouble());
  taskSave.bindValue(":prjtask_exp_budget", _budgetExp->text().toDouble());
  taskSave.bindValue(":prjtask_exp_actual", _actualExp->text().toDouble());

  taskSave.exec();
  if (taskSave.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taskSave.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  done(_prjtaskid);
}

void task::sAssignedToChanged(const int newid)
{
  if (newid == -1)
    _assigned->clear();
  else
    _assigned->setDate(omfgThis->dbDate());
}

void task::sStatusChanged(const int pStatus)
{
  switch(pStatus)
  {
    case 0: // Concept
    default:
      _started->clear();
      _completed->clear();
      break;
    case 1: // In Process
      _started->setDate(omfgThis->dbDate());
      _completed->clear();
      break;
    case 2: // Completed
      _completed->setDate(omfgThis->dbDate());
      break;
  }
}

void task::sHoursAdjusted()
{
  _balanceHours->setText(formatQty(_budgetHours->text().toDouble() - _actualHours->text().toDouble()));
}

void task::sExpensesAdjusted()
{
  _balanceExp->setText(formatCost(_budgetExp->text().toDouble() - _actualExp->text().toDouble()));
}

void task::sNewUser()
{
  XSqlQuery taskNewUser;
/*
  userList newdlg(this, "", TRUE);
  int result = newdlg.exec();
  if (result != XDialog::Rejected)
  {
    QString username = newdlg.username();
    taskNewUser.prepare( "SELECT prjtaskuser_id "
               "FROM prjtaskuser "
               "WHERE ( (prjtaskuser_username=:username)"
               " AND (prjtaskuser_prjtask_id=:prjtask_id) );" );
    taskNewUser.bindValue(":username", username);
    taskNewUser.bindValue(":prjtask_id", _prjtaskid);
    taskNewUser.exec();
    if (!taskNewUser.first())
    {
      taskNewUser.prepare( "INSERT INTO prjtaskuser "
                 "( prjtaskuser_prjtask_id, prjtaskuser_username ) "
                 "VALUES "
                 "( :prjtaskuser_prjtask_id, :prjtaskuser_username );" );
      taskNewUser.bindValue(":prjtaskuser_username", username);
      taskNewUser.bindValue(":prjtaskuser_prjtask_id", _prjtaskid);
      taskNewUser.exec();
      sFillUserList();
    }
    if (taskNewUser.lastError().type() != QSqlError::NoError)
    {
      systemError(this, taskNewUser.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
*/
}

void task::sDeleteUser()
{
  XSqlQuery taskDeleteUser;
/*
  taskDeleteUser.prepare( "DELETE FROM prjtaskuser "
             "WHERE ( (prjtaskuser_username=:username)"
             " AND (prjtaskuser_prjtask_id=:prjtask_id) );" );
  taskDeleteUser.bindValue(":username", _usr->username());
  taskDeleteUser.bindValue(":prjtask_id", _prjtaskid);
  taskDeleteUser.exec();
  if (taskDeleteUser.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taskDeleteUser.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  sFillUserList();
*/
}


void task::sFillUserList()
{
  XSqlQuery taskFillUserList;
/*
  taskFillUserList.prepare( "SELECT prjtaskuser_id, usr_username, usr_propername "
             "FROM prjtaskuser, usr "
             "WHERE ( (prjtaskuser_username=usr_username)"
             " AND (prjtaskuser_prjtask_id=:prjtask_id) );" );
  taskFillUserList.bindValue(":prjtask_id", _prjtaskid);
  taskFillUserList.exec();
  _usr->populate(taskFillUserList);
  if (taskFillUserList.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taskFillUserList.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
*/
}

