/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include <QAbstractItemView>
#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QPair>
#include <QPushButton>
#include <QSqlRecord>
#include <QSqlRelationalDelegate>
#include <QSqlTableModel>
#include <QtScript>

#include <xsqlquery.h>

#include "xcombobox.h"
#include "xcomboboxprivate.h"
#include "xdatawidgetmapper.h"
#include "xsqltablemodel.h"

#define DEBUG false

class XComboBoxEditorDescrip {
  public:
    XComboBoxEditorDescrip(int pType, const QString &pUi, const QString &pPriv)
      : obj(0), slot(0)
    {
      type   = pType;
      uiName = pUi;
      priv   = pPriv;
    }

    XComboBoxEditorDescrip(int   pType,       QObject *pObj,
                           const char *pSlot, const QString &pPriv)
    {
      type = pType;
      obj  = pObj;
      slot = pSlot;
      priv = pPriv;
    }

    int         type;   // XComboBoxTypes + possible out-of-range
    QString     priv;
    QString     uiName;
    QObject     *obj;
    const char  *slot;
};

XComboBoxPrivate::XComboBoxPrivate(XComboBox *pParent)
  : QObject(pParent),
    _default(XComboBox::First),
    _editButton(0),
    _label(0),
    _lastId(-1),
    _parent(pParent),
    _popupCounter(0),
    _mapper(0)
{
  _mapper = new XDataWidgetMapper(pParent);
  setType(XComboBox::Adhoc);
}

XComboBoxPrivate::~XComboBoxPrivate()
{
  foreach (int key, _editorMap.keys())
    delete _editorMap.take(key);
}

int XComboBoxPrivate::numberOfCurrencies()
{
  // run the query no more than once per instance
  static int numberOfCurrencies = -1;
  if (numberOfCurrencies <= 0)
  {
    XSqlQuery currCounter("SELECT COUNT(*) AS count FROM curr_symbol;");
    if (currCounter.first())
      numberOfCurrencies = currCounter.value("count").toInt();
    else
      numberOfCurrencies = 0;
  }

  return numberOfCurrencies;
}

void XComboBoxPrivate::sEdit()
{
  if (_parent->_guiClientInterface && _parent->parentWidget()->window())
  {
    XComboBoxEditorDescrip *editor = _editorMap.value(_parent->type());

    ParameterList params;
    params.append("mode", "edit");

    QWidget *w = _parent->_guiClientInterface->openWindow(editor->uiName,
                                                          params,
                                                          _parent->parentWidget()->window(),
                                                          Qt::ApplicationModal,
                                                          Qt::Dialog);
    connect(w, SIGNAL(destroyed()), _parent, SLOT(populate()));
    if (qobject_cast<QDialog*>(w))
      (qobject_cast<QDialog*>(w))->exec();
  }
}

/* let the user create/edit records of this XComboBoxType if s/he has privs */
void XComboBoxPrivate::setType(XComboBox::XComboBoxTypes ptype)
{
  _type = ptype;

  XComboBoxEditorDescrip *editor = _editorMap.value(_type);

  if (_x_privileges && editor &&
      (editor->priv.isEmpty() || _x_privileges->check(editor->priv)))
  {
    QAbstractItemView *view = _parent->view();

    if (view && ! _editButton)
    {
      QWidget *vp = qobject_cast<QWidget*>(view->parent());
      if (vp && vp->layout())
      {
        _editButton = new QPushButton(tr("Edit List"), _parent);
        vp->layout()->addWidget(_editButton);
        if (editor->obj)
          connect(_editButton, SIGNAL(clicked()), editor->obj, editor->slot);
        else
          connect(_editButton, SIGNAL(clicked()), this, SLOT(sEdit()));
      }
    }
  }
  else if (_editButton)
  {
    delete _editButton;       // TODO: deletelater?
    _editButton = 0;
  }
}

GuiClientInterface* XComboBox::_guiClientInterface = 0;

XComboBox::XComboBox(QWidget *pParent, const char *pName) :
  QComboBox(pParent),
  _data(0)
{
  if(pName)
    setObjectName(pName);
  init();
}

XComboBox::XComboBox(bool pEditable, QWidget *pParent, const char *pName) :
  QComboBox(pParent),
  _data(0)
{
  if(pName)
    setObjectName(pName);
  init();
  setEditable(pEditable);
}

XComboBox::~XComboBox()
{
}

void XComboBox::init()
{
  _data = new XComboBoxPrivate(this);

  setAllowNull(false);
  setMaximumWidth(200);

  connect(this, SIGNAL(activated(int)), this, SLOT(sHandleNewIndex(int)));

#ifdef Q_WS_MAC
  QFont f = font();
  f.setPointSize(f.pointSize() - 2);
  setFont(f);
  setMinimumHeight(26);
#endif

  insertEditor(AddressCommentTypes,"commentTypes","MaintainCommentTypes");
  // insertEditor(AdHoc,"",""); caller must insert this at the implementation
  insertEditor(APBankAccounts,"bankAccounts","MaintainBankAccounts");
  insertEditor(APTerms,"termses","MaintainTerms");
  insertEditor(ARBankAccounts,"bankAccounts","MaintainBankAccounts");
  insertEditor(ARCMReasonCodes,"reasonCodes","MaintainReasonCodes");
  insertEditor(ARDMReasonCodes,"reasonCodes","MaintainReasonCodes");
  insertEditor(ARTerms,"termses","MaintainTerms");
  insertEditor(AccountingPeriods,"accountingPeriods","MaintainAccountingPeriods");
  insertEditor(Agent,"users","MaintainUsers");
  insertEditor(AllCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(AllProjects,"projects","MaintainAllProjects");
  insertEditor(BBOMHeadCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(BBOMItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(BOMHeadCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(BOMItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(BOOHeadCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(BOOItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(CRMAccounts,"crmaccounts","MaintainAllCRMAccounts");
  insertEditor(CRMAccountCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(ClassCodes,"classCodes","MaintainClassCodes");
  insertEditor(Companies,"companies","MaintainChartOfAccounts");
  insertEditor(ContactCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(CostCategories,"costCategories","MaintainCostCategories");
  insertEditor(Countries,"countries","MaintainCountries");
  insertEditor(Currencies,"currencies","MaintainCurrencies");
  insertEditor(CurrenciesNotBase,"currencies","MaintainCurrencies");
  insertEditor(CustomerCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(CustomerGroups,"customerGroups","MaintainCustomerGroups");
  insertEditor(CustomerTypes,"customerTypes","MaintainCustomerTypes");
  insertEditor(EmployeeCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(ExpenseCategories,"expenseCategories","MaintainCustomerTypes");
  insertEditor(FinancialLayouts,"financialLayouts","MaintainFinancialLayouts");
  insertEditor(FiscalYears,"accountingYearPeriods","MaintainAccountingPeriods");
  insertEditor(FreightClasses,"freightClasses","MaintainFreightClasses");
  insertEditor(Honorifics,"honorifics","MaintainTitles");
  insertEditor(IncidentCategory,"incidentCategories","MaintainIncidentCategories");
  insertEditor(IncidentCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(IncidentPriority,"incidentPriorities","MaintainIncidentPriorities");
  insertEditor(IncidentResolution,"incidentResolutions","MaintainIncidentResolutions");
  insertEditor(IncidentSeverity,"incidentSeverities","MaintainIncidentSeverities");
  insertEditor(ItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(ItemGroups,"itemGroups","MaintainItemGroups");
  insertEditor(ItemSiteCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(ItemSourceCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(Locales,"locales","MaintainLocales");
  // insertEditor(LocaleCountries,"",""); // This is based on Qt countries, can not be edited
  // insertEditor(LocaleLanguages,"",""); // This is based on Qt languages, no editor
  insertEditor(LocationCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(LotSerialCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(OpportunityCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(OpportunityStages,"opportunityStages","MaintainOpportunityStages");
  insertEditor(OpportunitySources,"opportunitySources","MaintainOpportunitySources");
  insertEditor(OpportunityTypes,"opportunityTypes","MaintainOpportunityTypes");
  insertEditor(PlannerCodes,"plannerCodes","MaintainPlannerCodes");
  insertEditor(PoProjects,"projects","MaintainAllProjects");
  insertEditor(ProductCategories,"productCategories","MaintainProductCategories");
  insertEditor(ProfitCenters,"profitCenters","MaintainChartOfAccounts");
  insertEditor(ProjectCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(PurchaseOrderCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(PurchaseOrderItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(ReasonCodes,"reasonCodes","MaintainReasonCodes");
  // insertEditor(RegistrationTypes,"","");  No editor currently exists
  insertEditor(Reports,"reports","MaintainReports");
  insertEditor(ReturnReasonCodes,"reasonCodes","MaintainReasonCodes");
  insertEditor(ReturnAuthCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(ReturnAuthItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(QuoteCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(QuoteItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(SalesOrderCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(SalesOrderItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(SalesCategories,"salesCategories","MaintainSalesCategories");
  insertEditor(SalesCategoriesActive,"salesCategories","MaintainSalesCategories");
  insertEditor(SalesReps,"salesReps","MaintainSalesReps");
  insertEditor(SalesRepsActive,"salesReps","MaintainSalesReps");
  insertEditor(SaleTypes,"saleTypes","MaintainSaleTypes");
  insertEditor(ShipVias,"shipVias","MaintainShipVias");
  insertEditor(ShippingCharges,"shippingChargeTypes","MaintainShippingChargeTypes");
  insertEditor(ShippingForms,"shippingForms","MaintainShippingForms");
  insertEditor(ShippingZones,"shippingZones","MaintainShippingZones");
  insertEditor(SiteTypes,"siteTypes","MaintainSiteTypes");
  insertEditor(SoProjects,"projects","MaintainAllProjects");
  insertEditor(Subaccounts,"subaccounts","MaintainChartOfAccounts");
  insertEditor(TaxAuths,"taxAuthorities","MaintainTaxAuthorities");
  insertEditor(TaxClasses,"taxClasses","MaintainTaxClasses");
  insertEditor(TaxCodes,"taxCodes","MaintainTaxCodes");
  insertEditor(TaxZones,"taxZones","MaintainTaxZones");
  insertEditor(TaxTypes,"taxTypes","MaintainTaxTypes");
  insertEditor(Terms,"terms","MaintainTerms");
  insertEditor(TaskCommentTypes,"commentTypes","MaintainCommentTypes");
  //insertEditor(TimeAttendanceCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(TodoItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(TransferOrderCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(TransferOrderItemCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(UOMs,"uoms","MaintainUOMs");
  insertEditor(Users,"users","MaintainUsers");
  insertEditor(ActiveUsers,"users","MaintainUsers");
  insertEditor(VendorCommentTypes,"commentTypes","MaintainCommentTypes");
  // insertEditor(VendorGroups,"","");  Not implemented yet
  insertEditor(VendorTypes,"vendorTypes","MaintainVendorTypes");
  insertEditor(WarehouseCommentTypes,"commentTypes","MaintainCommentTypes");
  insertEditor(WoProjects,"projects","MaintainAllProjects");
  insertEditor(WorkCenters,"workCenters","MaintainWorkCenters");
  insertEditor(WorkOrderCommentTypes,"commentTypes","MaintainCommentTypes");
}

bool XComboBox::allowNull() const
{
  return _allowNull;
}

XComboBox::Defaults XComboBox::defaultCode() const
{
  return _data->_default;
}

QString XComboBox::fieldName() const
{
  return _data->_fieldName;
}

QLabel* XComboBox::label() const
{
  return _data->_label;
}

QString XComboBox::listDisplayFieldName() const
{
  return _data->_listDisplayFieldName;
}

QString XComboBox::listIdFieldName() const
{
  return _data->_listIdFieldName;
}

QString XComboBox::listSchemaName() const
{
  return _data->_listSchemaName;
}

QString XComboBox::listTableName() const
{
  return _data->_listTableName;
}

QString XComboBox::nullStr() const
{
  return _data->_nullStr;
}

void XComboBox::setDefaultCode(Defaults p)
{
  _data->_default = p;
}

void XComboBox::setFieldName(QString p)
{
  _data->_fieldName = p;
}

void XComboBox::setListDisplayFieldName(QString p)
{
  _data->_listDisplayFieldName = p;
}

void XComboBox::setListIdFieldName(QString p)
{
  _data->_listIdFieldName = p;
} 

// exists only for script exposure
void XComboBox::removeItem(int idx)
{
  QComboBox::removeItem(idx);
}

enum XComboBox::XComboBoxTypes XComboBox::type()
{
  return _data->_type;
}

QString XComboBox::currentDefault()
{
  if (_data->_codes.count())
  {
    if (_data->_default == First)
      return _data->_codes.first();
    else
      return code();
  }
  else
    return QString("");
}

void XComboBox::setDataWidgetMap(XDataWidgetMapper* m)
{
  disconnect(this, SIGNAL(editTextChanged(QString)), this, SLOT(updateMapperData()));

  if (!_data->_listTableName.isEmpty())
  {
    QString tableName="";
    if (_data->_listSchemaName.length())
      tableName = _data->_listSchemaName + ".";
    tableName+= _data->_listTableName;
    static_cast<XSqlTableModel*>(m->model())->setRelation(static_cast<XSqlTableModel*>(m->model())->fieldIndex(_data->_fieldName),
                                 QSqlRelation(tableName, _data->_listIdFieldName, _data->_listDisplayFieldName));

    QSqlTableModel *rel =static_cast<XSqlTableModel*>(m->model())->relationModel(static_cast<XSqlTableModel*>(m->model())->fieldIndex(_data->_fieldName));
    setModel(rel);
    setModelColumn(rel->fieldIndex(_data->_listDisplayFieldName));

    m->setItemDelegate(new QSqlRelationalDelegate(this));
    m->addMapping(this, _data->_fieldName);
    return;
  }
  else if (_data->_codes.count())
    m->addMapping(this, _data->_fieldName, "code", "currentDefault");
  else
    m->addMapping(this, _data->_fieldName, "text", "text");

  _data->_mapper=m;
  connect(this, SIGNAL(editTextChanged(QString)), this, SLOT(updateMapperData()));
}

void XComboBox::setListSchemaName(QString p)
{
  if (_data->_listSchemaName == p)
    return;

  if (!p.isEmpty())
    setType(Adhoc);
  _data->_listSchemaName = p;
}

void XComboBox::setListTableName(QString p)
{
  if (_data->_listTableName == p)
    return;

  _data->_listTableName = p;
  if (!p.isEmpty())
    setType(Adhoc);
}

void XComboBox::setType(XComboBoxTypes pType)
{
  if (pType != Adhoc)
  {
    setListSchemaName("");
    setListTableName("");
  }
  _data->setType(pType);

  if (_x_metrics == 0)
    return;

  // If we're in Designer, don't populate
  for (QObject *ancestor = this; ancestor; ancestor = ancestor->parent())
  {
    if (ancestor->inherits("xTupleDesigner"))
      return;
  }

  XSqlQuery query;

  switch (pType)
  {
    case Adhoc:
      break;

    case UOMs:
      setAllowNull(TRUE);
      query.exec( "SELECT uom_id, uom_name, uom_name "
                  "FROM uom "
                  "ORDER BY uom_name;" );
    break;

    case ClassCodes:
      query.exec( "SELECT classcode_id, (classcode_code || '-' || classcode_descrip), classcode_code  "
                  "FROM classcode "
                  "ORDER BY classcode_code;" );
      break;

    case ItemGroups:
      query.exec( "SELECT itemgrp_id, itemgrp_name, itemgrp_name "
                  "FROM itemgrp "
                  "ORDER BY itemgrp_name;" );
      break;

    case CostCategories:
      query.exec( "SELECT costcat_id,  (costcat_code || '-' || costcat_descrip), costcat_code "
                  "FROM costcat "
                  "ORDER BY costcat_code;" );
      break;

    case ProductCategories:
      query.exec( "SELECT prodcat_id, (prodcat_code || ' - ' || prodcat_descrip), prodcat_code "
                  "FROM prodcat "
                  "ORDER BY prodcat_code;" );
      break;

    case PlannerCodes:
      query.exec( "SELECT plancode_id, (plancode_code || '-' || plancode_name), plancode_code "
                  "FROM plancode "
                  "ORDER BY plancode_code;" );
      break;

    case CustomerTypes:
      query.exec( "SELECT custtype_id, (custtype_code || '-' || custtype_descrip), custtype_code "
                  "FROM custtype "
                  "ORDER BY custtype_code;" );
      break;

    case CustomerGroups:
      query.exec( "SELECT custgrp_id, custgrp_name, custgrp_name "
                  "FROM custgrp "
                  "ORDER BY custgrp_name;" );
      break;

    case VendorTypes:
      query.exec( "SELECT vendtype_id, (vendtype_code || '-' || vendtype_descrip), vendtype_code "
                  "FROM vendtype "
                  "ORDER BY vendtype_code;" );
      break;

    case VendorGroups:
      query.exec( "SELECT vendgrp_id, vendgrp_name, vendgrp_name "
                  "FROM vendgrp "
                  "ORDER BY vendgrp_name;" );
      break;

    case Contracts:
      query.exec( "SELECT contrct_id, (vend_number || '-' || contrct_number), contrct_number "
                  "FROM contrct JOIN vendinfo ON (vend_id=contrct_vend_id) "
                  "ORDER BY vend_number, contrct_number;" );
      break;

    case SalesRepsActive:
      query.exec( "SELECT salesrep_id, (salesrep_number || '-' || salesrep_name), salesrep_number "
                  "FROM salesrep "
                  "WHERE (salesrep_active) "
                  "ORDER by salesrep_number;" );
      break;

    case ShipVias:
      setAllowNull(TRUE);
      setEditable(TRUE);
      query.exec( "SELECT shipvia_id, (shipvia_code || '-' || shipvia_descrip), shipvia_code "
                  "FROM shipvia "
                  "ORDER BY shipvia_code;" );
      break;

    case SalesReps:
      query.exec( "SELECT salesrep_id, (salesrep_number || '-' || salesrep_name), salesrep_number "
                  "FROM salesrep "
                  "ORDER by salesrep_number;" );
      break;

    case SaleTypes:
      query.exec( "SELECT saletype_id, (saletype_code || '-' || saletype_descr), saletype_code "
                  "FROM saletype "
                  "ORDER BY saletype_code;" );
      break;

    case ShippingCharges:
      query.exec( "SELECT shipchrg_id, (shipchrg_name || '-' || shipchrg_descrip), shipchrg_name "
                  "FROM shipchrg "
                  "ORDER by shipchrg_name;" );
      break;

    case ShippingForms:
      query.exec( "SELECT shipform_id, shipform_name, shipform_name "
                  "FROM shipform "
                  "ORDER BY shipform_name;" );
      break;

    case ShippingZones:
      query.exec( "SELECT shipzone_id, shipzone_name, shipzone_name "
                  "FROM shipzone "
                  "ORDER BY shipzone_name;" );
      break;
    case Terms:
      query.exec( "SELECT terms_id, (terms_code || '-' || terms_descrip), terms_code "
                  "FROM terms "
                  "ORDER by terms_code;" );
      break;

    case ARTerms:
      query.exec( "SELECT terms_id, (terms_code || '-' || terms_descrip), terms_code "
                  "FROM terms "
                  "WHERE (terms_ar) "
                  "ORDER by terms_code;" );
      break;

    case APTerms:
      query.exec( "SELECT terms_id, (terms_code || '-' || terms_descrip), terms_code "
                  "FROM terms "
                  "WHERE (terms_ap) "
                  "ORDER by terms_code;" );
      break;

    case ARBankAccounts:
      query.exec( "SELECT bankaccnt_id, (bankaccnt_name || '-' || bankaccnt_descrip), bankaccnt_name "
                  "FROM bankaccnt "
                  "WHERE (bankaccnt_ar) "
                  "ORDER BY bankaccnt_name;" );
      break;

    case APBankAccounts:
      query.exec( "SELECT bankaccnt_id, (bankaccnt_name || '-' || bankaccnt_descrip), bankaccnt_name "
                  "FROM bankaccnt "
                  "WHERE (bankaccnt_ap) "
                  "ORDER BY bankaccnt_name;" );
      break;

    case AccountingPeriods:
      query.exec( "SELECT period_id, (formatDate(period_start) || '-' || formatDate(period_end)), (formatDate(period_start) || '-' || formatDate(period_end)) "
                  "FROM period "
                  "ORDER BY period_start DESC;" );
      break;

    case FinancialLayouts:
      query.exec( "SELECT flhead_id, flhead_name, flhead_name "
                  "FROM flhead "
                  "WHERE (flhead_active) "
                  "ORDER BY flhead_name;" );
      break;

    case FiscalYears:
      query.exec( "SELECT yearperiod_id, formatdate(yearperiod_start) || '-' || formatdate(yearperiod_end), formatdate(yearperiod_start) || '-' || formatdate(yearperiod_end)"
                  "  FROM yearperiod"
                  " ORDER BY yearperiod_start DESC;" );
      break;

    case SoProjects:
      setAllowNull(TRUE);
      query.exec( "SELECT prj_id, (prj_number || '-' || prj_name), prj_number "
                  "FROM prj "
                  "WHERE (prj_so) "
                  "ORDER BY prj_name;" );
      break;

    case WoProjects:
      setAllowNull(TRUE);
      query.exec( "SELECT prj_id, (prj_number || '-' || prj_name), prj_number "
                  "FROM prj "
                  "WHERE (prj_wo) "
                  "ORDER BY prj_name;" );
      break;

    case PoProjects:
      setAllowNull(TRUE);
      query.exec( "SELECT prj_id, (prj_number || '-' || prj_name), prj_number "
                  "FROM prj "
                  "WHERE (prj_po) "
                  "ORDER BY prj_name;" );
      break;

    case Currencies:
      query.exec( "SELECT curr_id, currConcat(curr_abbr, curr_symbol), curr_abbr"
                  " FROM curr_symbol "
                  "ORDER BY curr_base DESC, curr_abbr;" );
      break;

    case CurrenciesNotBase:
      query.exec( "SELECT curr_id, currConcat(curr_abbr, curr_symbol), curr_abbr"
                  " FROM curr_symbol "
                  " WHERE curr_base = FALSE "
                  "ORDER BY curr_abbr;" );
      break;

    case Companies:
      query.exec( "SELECT company_id, company_number, company_number "
                  "FROM company "
                  "ORDER BY company_number;" );
      break;

    case ProfitCenters:
      setEditable(_x_metrics->boolean("GLFFProfitCenters"));
      query.exec( "SELECT prftcntr_id, prftcntr_number, prftcntr_number "
                  "FROM prftcntr "
                  "ORDER BY prftcntr_number;" );
      break;

    case Subaccounts:
      setEditable(_x_metrics->boolean("GLFFSubaccounts"));
      query.exec( "SELECT subaccnt_id, subaccnt_number, subaccnt_number "
                  "FROM subaccnt "
                  "ORDER BY subaccnt_number;" );
      break;

    case AddressCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='ADDR')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case BBOMHeadCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='BBH')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case BBOMItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='BBI')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case BOMHeadCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='BMH')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case BOMItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='BMI')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case BOOHeadCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='BOH')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case BOOItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='BOI')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case CRMAccountCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='CRMA')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case ContactCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='T')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case CustomerCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='C')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case EmployeeCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='EMP')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case IncidentCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='INCDT')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case ItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='I')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case ItemSiteCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='IS')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case ItemSourceCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='IR')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case LocationCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='L')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case LotSerialCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='LS')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case OpportunityCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='OPP')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case ProjectCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='J')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case PurchaseOrderCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='P')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case PurchaseOrderItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='PI')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case ReturnAuthCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='RA')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case ReturnAuthItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='RI')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case QuoteCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='Q')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case QuoteItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='QI')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case SalesOrderCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='S')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case SalesOrderItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='SI')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case TaskCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='TA')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;
      
     case TimeAttendanceCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='TATC')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;     

    case TodoItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='TD')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case TransferOrderCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='TO')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case TransferOrderItemCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='TI')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case VendorCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='V')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case WarehouseCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='WH')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case WorkOrderCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype JOIN cmnttypesource ON (cmnttypesource_cmnttype_id=cmnttype_id)"
                  "              JOIN source ON (source_id=cmnttypesource_source_id) "
                  "WHERE (source_name='W')"
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case AllCommentTypes:
      query.exec( "SELECT cmnttype_id, cmnttype_name, cmnttype_name "
                  "FROM cmnttype "
                  "ORDER BY cmnttype_order, cmnttype_name;" );
      break;

    case AllProjects:
      query.exec( "SELECT prj_id, prj_name, prj_name "
                  "FROM prj "
                  "ORDER BY prj_name;" );
      break;

    case Users:
      query.exec( "SELECT usr_id, usr_username, usr_username "
                  "FROM usr "
                  "ORDER BY usr_username;" );

    case ActiveUsers:
      query.exec( "SELECT usr_id, usr_username, usr_username "
                  "FROM usr "
                  "WHERE usr_active = true "
                  "ORDER BY usr_username;" );

      break;

    case SalesCategories:
      query.exec( "SELECT salescat_id, (salescat_name || '-' || salescat_descrip), salescat_name "
                  "FROM salescat "
                  "ORDER BY salescat_name;" );
      break;

    case SalesCategoriesActive:
      query.exec( "SELECT salescat_id, (salescat_name || '-' || salescat_descrip), salescat_name "
                  "FROM salescat "
                  "WHERE (salescat_active) "
                  "ORDER BY salescat_name;" );
      break;

    case ExpenseCategories:
      query.exec( "SELECT expcat_id, (expcat_code || '-' || expcat_descrip), expcat_code "
                  "FROM expcat "
                  "ORDER BY expcat_code;" );
      break;

    case ReasonCodes:
      query.exec( "SELECT rsncode_id, (rsncode_code || '-' || rsncode_descrip), rsncode_code"
                  "  FROM rsncode "
                  "ORDER BY rsncode_code;" );
      break;

    case ARCMReasonCodes:
      query.exec( "SELECT rsncode_id, (rsncode_code || '-' || rsncode_descrip), rsncode_code"
                  "  FROM rsncode "
                  " WHERE ( (rsncode_doctype IS NULL) OR (rsncode_doctype='ARCM') ) "
                  "ORDER BY rsncode_code;" );
      break;

    case ARDMReasonCodes:
      query.exec( "SELECT rsncode_id, (rsncode_code || '-' || rsncode_descrip), rsncode_code"
                  "  FROM rsncode "
                  " WHERE ( (rsncode_doctype IS NULL) OR (rsncode_doctype='ARDM') ) "
                  "ORDER BY rsncode_code;" );
      break;

    case ReturnReasonCodes:
      query.exec( "SELECT rsncode_id, (rsncode_code || '-' || rsncode_descrip), rsncode_code"
                  "  FROM rsncode "
                  " WHERE ( (rsncode_doctype IS NULL) OR (rsncode_doctype='RA') ) "
                  "ORDER BY rsncode_code;" );
      break;

    case TaxCodes:
      query.exec( "SELECT tax_id, (tax_code || '-' || tax_descrip), tax_code"
                  "  FROM tax "
                  "ORDER BY tax_code;" );
      break;

    case WorkCenters:
      query.exec( "SELECT wrkcnt_id, (wrkcnt_code || '-' || wrkcnt_descrip), wrkcnt_code"
                  "  FROM xtmfg.wrkcnt "
                  "ORDER BY wrkcnt_code;" );
      break;

    case CRMAccounts:
      setAllowNull(TRUE);
      query.exec( "SELECT crmacct_id, (crmacct_number || '-' || crmacct_name), crmacct_number"
                  "  FROM crmacct "
                  "ORDER BY crmacct_number;" );
      break;

    case Honorifics:
      setAllowNull(TRUE);
      query.exec( "SELECT hnfc_id, hnfc_code, hnfc_code"
                  "  FROM hnfc "
                  "ORDER BY hnfc_code;" );
      break;

    case IncidentSeverity:
      query.exec( "SELECT incdtseverity_id, incdtseverity_name, incdtseverity_name"
                  "  FROM incdtseverity"
                  " ORDER BY incdtseverity_order, incdtseverity_name;" );
      break;

    case IncidentPriority:
      query.exec( "SELECT incdtpriority_id, incdtpriority_name, incdtpriority_name"
                  "  FROM incdtpriority"
                  " ORDER BY incdtpriority_order, incdtpriority_name;" );
      break;

    case IncidentResolution:
      query.exec( "SELECT incdtresolution_id, incdtresolution_name, incdtresolution_name"
                  "  FROM incdtresolution"
                  " ORDER BY incdtresolution_order, incdtresolution_name;" );
      break;

    case IncidentCategory:
      query.exec( "SELECT incdtcat_id, incdtcat_name, incdtcat_name"
                  "  FROM incdtcat"
                  " ORDER BY incdtcat_order, incdtcat_name;" );
      break;

    case TaxAuths:
      query.exec( "SELECT taxauth_id, taxauth_code, taxauth_code"
                  "  FROM taxauth"
                  " ORDER BY taxauth_code;" );
      break;

    case TaxTypes:
      query.exec( "SELECT taxtype_id, taxtype_name, taxtype_name"
                  "  FROM taxtype"
                  " ORDER BY taxtype_name;" );
      break;

    case Agent:
      query.exec( "SELECT usr_id, usr_username, usr_username "
                  "  FROM usr"
                  " WHERE (usr_agent) "
                  " ORDER BY usr_username;" );
      break;

    case Reports:
      query.exec( "SELECT a.report_id, a.report_name, a.report_name "
                  "FROM report a, "
                  "    (SELECT MIN(report_grade) AS report_grade, report_name "
                  "     FROM report "
                  "     GROUP BY report_name) b "
                  "WHERE ((a.report_name=b.report_name)"
                  "  AND  (a.report_grade=b.report_grade)) "
                  "ORDER BY report_name;" );
      break;

    case OpportunityStages:
      query.exec("SELECT opstage_id, opstage_name, opstage_name "
                 "  FROM opstage"
                 " ORDER BY opstage_order;");
      break;

    case OpportunitySources:
      query.exec("SELECT opsource_id, opsource_name, opsource_name "
                 "  FROM opsource;");
      break;

    case OpportunityTypes:
      query.exec("SELECT optype_id, optype_name, optype_name "
                 "  FROM optype;");
      break;

    case Locales:
      query.exec("SELECT locale_id, locale_code, locale_code "
                 "  FROM locale"
                 " ORDER BY locale_code;");
      break;

    case LocaleLanguages:
      query.exec("SELECT lang_id, lang_name, lang_name "
                 "  FROM lang"
                 " WHERE lang_qt_number IS NOT NULL"
                 " ORDER BY lang_name;");
      break;

    case Countries:
      query.exec("SELECT country_id, country_name, country_name "
                 "  FROM country"
                 " ORDER BY country_name;");
      break;

    case LocaleCountries:
      query.exec("SELECT country_id, country_name, country_name "
                 "  FROM country"
                 " WHERE country_qt_number IS NOT NULL"
                 " ORDER BY country_name;");
      break;

    case RegistrationTypes:
      query.exec("SELECT regtype_id, regtype_code, regtype_code "
                 "  FROM regtype"
                 " ORDER BY regtype_code;");
      break;

    case SiteTypes:
      query.exec("SELECT sitetype_id, sitetype_name, sitetype_name "
                 "  FROM sitetype"
                 " ORDER BY sitetype_name;");
      break;

    case FreightClasses:
      query.exec( "SELECT freightclass_id, (freightclass_code || '-' || freightclass_descrip), freightclass_code  "
                  "FROM freightclass "
                  "ORDER BY freightclass_code;" );
      break;

   case TaxClasses:
         query.exec( "SELECT taxclass_id, (taxclass_code || '-' || taxclass_descrip), taxclass_code  "
                     "FROM taxclass "
                     "ORDER BY taxclass_code;" );
      break;

   case TaxZones:
         query.exec( "SELECT taxzone_id, (taxzone_code || '-' || taxzone_descrip), taxzone_code  "
		                      "FROM taxzone "
		                      "ORDER BY taxzone_code;" );
     break;
  }

  populate(query);

  switch (pType)
  {
    case SoProjects:
    case WoProjects:
    case PoProjects:
      setEnabled(count() > 1);
      break;

    case Currencies:
    case CurrenciesNotBase:
      if (_data->numberOfCurrencies() <= 1)
      {
        hide();
        if (_data->_label)
          _data->_label->hide();
      }
      break;

    default:
      break;
  }
}

void XComboBox::setLabel(QLabel* pLab)
{
  _data->_label = pLab;

  switch (_data->_type)
  {
    case Currencies:
    case CurrenciesNotBase:
      if (_data->numberOfCurrencies() <= 1)
      {
        hide();
        if (_data->_label)
          _data->_label->hide();
      }
      break;

    default:
      break;
  }
}

void XComboBox::setCode(const QString &pString)
{
  if (DEBUG)
    qDebug("%s::setCode(%d %d %s) with _codes.count %d and _ids.count %d",
           objectName().toAscii().data(), pString.isNull(), pString.isEmpty(),
           pString.toAscii().data(), _data->_codes.count(), _data->_ids.count());

  if (pString.isEmpty())
  {
    setId(-1);
    setItemText(0, pString);
  }
  else if (_data->_codes.count())
  {
    for (int counter = 0; counter < _data->_codes.count(); counter++)
    {
      if (_data->_codes.at(counter) == pString)
      {
        if (DEBUG)
          qDebug("%s::setCode(%s) found at %d with _ids.count %d & _lastId %d",
                 objectName().toAscii().data(), pString.toAscii().data(),
                 counter, _data->_ids.count(), _data->_lastId);
        setCurrentIndex(counter);

        if (_data->_ids.count() && _data->_lastId!=_data->_ids.at(counter))
          setId(_data->_ids.at(counter));

        return;
      }
      else if (DEBUG)
        qDebug("%s::setCode(%s) not found (%s)",
               qPrintable(objectName()), qPrintable(pString),
               qPrintable(_data->_codes.at(counter)));
    }
  }
  else  // this is an ad-hoc combobox without a query behind it?
  {
    setCurrentIndex(findText(pString));
    if (DEBUG)
      qDebug("%s::setCode(%s) set current item to %d using findData()",
             objectName().toAscii().data(), pString.toAscii().data(),
             currentIndex());
    if (_data->_ids.count() > currentIndex())
      setId(_data->_ids.at(currentIndex()));
    if (DEBUG)
      qDebug("%s::setCode(%s) current item is %d after setId",
             objectName().toAscii().data(), pString.toAscii().data(),
             currentIndex());
  }

  if (isEditable())
  {
    setId(-1);
    setItemText(0, pString);
  }
}

void XComboBox::setId(int pTarget)
{
  // reports are a special case: they should really be stored by name, not id
  if (_data->_type == Reports)
  {
    XSqlQuery query;
    query.prepare("SELECT report_id "
                  "FROM report "
                  "WHERE (report_name IN (SELECT report_name "
                  "                       FROM report "
                  "                       WHERE (report_id=:report_id)));");
    query.bindValue(":report_id", pTarget);
    query.exec();
    while (query.next())
    {
      int id = query.value("report_id").toInt();
      for (int counter = 0; counter < count(); counter++)
      {
        if (_data->_ids.at(counter) == id)
        {
          setCurrentIndex(counter);

          if(_data->_lastId!=id)
          {
            _data->_lastId = id;
            updateMapperData();
            emit newID(pTarget);
            emit valid(TRUE);

            if (allowNull())
              emit notNull(TRUE);
          }

          return;
        }
      }
    }
  }
  else
  {
    for (int counter = 0; counter < _data->_ids.count(); counter++)
    {
      if (_data->_ids.at(counter) == pTarget)
      {
        setCurrentIndex(counter);

        if(_data->_lastId!=pTarget)
        {
          _data->_lastId = pTarget;
          updateMapperData();
          emit newID(pTarget);
          emit valid(TRUE);

          if (allowNull())
            emit notNull(TRUE);
        }

        return;
      }
    }
  }

  setNull();
}

void XComboBox::setText(QVariant &pVariant)
{
  XComboBox::setText(pVariant.toString());
}

void XComboBox::setText(const QString &pString)
{
  if (pString == currentText())
    return;

  if (count())
  {
    for (int counter = ((allowNull()) ? 1 : 0); counter < count(); counter++)
    {
      if (itemText(counter) == pString)
      {
        setCurrentIndex(counter);
        updateMapperData();
        emit newID(id());
        return;
      }
    }
  }

  if (isEditable())
  {
    setId(-1);
    setItemText(0, pString);
  }
}

void XComboBox::setAllowNull(bool pAllowNull)
{
  if (DEBUG)
    qDebug("%s::setAllowNull(%d)",
           qPrintable(objectName()), pAllowNull);
  _allowNull = pAllowNull;
  if (pAllowNull)
  {
    append(-1, _data->_nullStr);
    setItemText(0, _data->_nullStr);
  }
}

void XComboBox::setNull()
{
  if (allowNull())
  {
    _data->_lastId = -1;

    setCurrentIndex(0);
    updateMapperData();
    emit newID(-1);
    emit valid(FALSE);
    emit notNull(FALSE);
  }
}

void XComboBox::setNullStr(const QString& pNullStr)
{
  if (DEBUG)
    qDebug("%s::setNullStr(%s)",
           qPrintable(objectName()), qPrintable(pNullStr));
  _data->_nullStr = pNullStr;
  if (allowNull())
  {
    append(-1, _data->_nullStr);
    setItemText(0, pNullStr);
  }
}

void XComboBox::setText(const QVariant &pVariant)
{
  setText(pVariant.toString());
}

void XComboBox::clear()
{
  QComboBox::clear();

  if (_data->_ids.count())
    _data->_ids.clear();

  if (_data->_codes.count())
    _data->_codes.clear();

  if (allowNull())
    append(-1, _data->_nullStr);
}

// allow repopulating after the underlying contents have changed (e.g. #3698)
void XComboBox::populate()
{
  setType(_data->_type);
}

void XComboBox::populate(XSqlQuery pQuery, int pSelected)
{
  int selected = (pSelected >= 0) ? pSelected : id();
  clear();

  // strange if/loop construct allows multiple comboboxes to use the same query
  if (pQuery.first())
    do
    {
      if (pQuery.record().count() < 3)
        append(pQuery.value(0).toInt(), pQuery.value(1).toString());
      else
        append(pQuery.value(0).toInt(), pQuery.value(1).toString(), pQuery.value(2).toString());
    } while (pQuery.next());

  setId(selected);

  // TODO: why doesn't setId() handle the following as expected? {
  updateMapperData();
  emit newID(_data->_lastId);
  emit valid((_data->_lastId != -1));
  // } end why
}

void XComboBox::populate(const QString & pSql, int pSelected)
{
  qApp->setOverrideCursor(Qt::WaitCursor);
  XSqlQuery query(pSql);
  populate(query, pSelected);
  qApp->restoreOverrideCursor();
}

void XComboBox::append(int pId, const QString &pText)
{
  append(pId,pText,pText);
}

void XComboBox::append(int pId, const QString &pText, const QString &pCode)
{
  if (DEBUG)
      qDebug("%s::append(%d, %s, %s)",
             qPrintable(objectName()), pId,
             qPrintable(pText), qPrintable(pCode));

  if (! _data->_ids.contains(pId))
  {
    addItem(pText);
    _data->_ids.append(pId);
    _data->_codes.append(pCode);
  }
}

int XComboBox::id(int pIndex) const
{
  if ((pIndex >= 0) && (pIndex < count()))
  {
    if ( (allowNull()) && (currentIndex() <= 0) )
      return -1;
    else if(pIndex < _data->_ids.count())
      return _data->_ids.at(pIndex);
  }
  return -1;
}

int XComboBox::id() const
{
  if (_data->_ids.count() && currentIndex() != -1)
  {
    if ( (allowNull()) && (currentIndex() <= 0) )
      return -1;
    else
      return _data->_ids.at(currentIndex());
  }
  else
    return -1;
}

QString XComboBox::code() const
{
  if (DEBUG)
    qDebug("%s::code() with currentIndex %d, allowNull %d, and _codes.count %d",
           objectName().toAscii().data(), currentIndex(), allowNull(),
           _data->_codes.count());

  QString returnValue;

  if ( allowNull() && (currentIndex() <= 0) )
    returnValue = QString::Null();
  else if (currentIndex() >= 0 && _data->_codes.count() > currentIndex())
    returnValue = _data->_codes.at(currentIndex());
  else if (currentIndex() >= 0)
    returnValue = currentText();
  else
    returnValue = QString::Null();

  if (DEBUG)
    qDebug("%s::code() returning %s",
           objectName().toAscii().data(), returnValue.toAscii().data());
  return returnValue;
}

bool XComboBox::isValid() const
{
  if ((allowNull()) && (id() == -1))
    return FALSE;
  else
    return TRUE;
}

void XComboBox::sHandleNewIndex(int pIndex)
{
  if (DEBUG)
    qDebug("%s::sHandleNewIndex(%d)",objectName().toAscii().data(), pIndex);

  if ((pIndex >= 0) && (pIndex < _data->_ids.count()) &&
      (_data->_ids.at(pIndex) != _data->_lastId))
  {
    _data->_lastId = _data->_ids.at(pIndex);
    updateMapperData();
    emit newID(_data->_lastId);

    if (DEBUG)
      qDebug("%s::sHandleNewIndex() emitted %d",
             objectName().toAscii().data(), _data->_lastId);

    if (allowNull())
    {
      emit valid((pIndex != 0));
      emit notNull((pIndex != 0));
    }
  }

  if (DEBUG)
    qDebug("%s::sHandleNewIndex() returning",
           objectName().toAscii().data());
}

void XComboBox::mousePressEvent(QMouseEvent *event)
{
  emit clicked();

  QComboBox::mousePressEvent(event);
}

void XComboBox::showPopup()
{
  QComboBox::showPopup();
  QAbstractItemView *itemView = view();
  if (_data->_editButton && _data->_popupCounter == 0)
  {
    _data->_popupCounter++;
    itemView->setFixedHeight(itemView->height() + _data->_editButton->height() + 5);
  }
}

QSize XComboBox::sizeHint() const
{
  QSize s = QComboBox::sizeHint();
#ifdef Q_WS_MAC
  s.setWidth(s.width() + 12);
#endif
  return s;
}

void XComboBox::updateMapperData()
{
  QString val;
  if (_data->_codes.count())
    val = code();
  else
    val = currentText();

  if (_data->_mapper->model() &&
    _data->_mapper->model()->data(_data->_mapper->model()->index(_data->_mapper->currentIndex(),_data->_mapper->mappedSection(this))).toString() != val)
  _data->_mapper->model()->setData(_data->_mapper->model()->index(_data->_mapper->currentIndex(),_data->_mapper->mappedSection(this)), val);
}

void XComboBox::insertEditor(XComboBoxTypes type, const QString &uiName,
                             const QString &privilege)
{
  _data->_editorMap.insert(type, new XComboBoxEditorDescrip(type, uiName,
                                                            privilege));
  if (_data->_type == type && ! _data->_editButton) // add edit button if needed
    _data->setType(type);
}

void XComboBox::insertEditor(XComboBoxTypes type, QObject *obj,
                             const char *slot, const QString &privilege)
{
  _data->_editorMap.insert(type, new XComboBoxEditorDescrip(type, obj,
                                                            slot, privilege));
  if (_data->_type == type && ! _data->_editButton) // add edit button if needed
    _data->setType(type);
}

// scripting exposure /////////////////////////////////////////////////////////

QScriptValue XComboBoxtoScriptValue(QScriptEngine *engine, XComboBox* const &item)
{
  return engine->newQObject(item);
}

void XComboBoxfromScriptValue(const QScriptValue &obj, XComboBox* &item)
{
  item = qobject_cast<XComboBox*>(obj.toQObject());
}

QScriptValue XComboBoxDefaultstoScriptValue(QScriptEngine *engine, XComboBox::Defaults const &item)
{
  return QScriptValue(engine, (int)item);
}

void XComboBoxDefaultsfromScriptValue(const QScriptValue &obj, XComboBox::Defaults &item)
{
  item = (XComboBox::Defaults)(obj.toInt32());
}

QScriptValue constructXComboBox(QScriptContext *context,
                                QScriptEngine  *engine)
{
  XComboBox *cbox = 0;

  if (context->argumentCount() == 0)
    cbox = new XComboBox();

  else if (context->argumentCount() == 1 &&
           context->argument(0).isBool())
    cbox = new XComboBox(context->argument(0).toBool());
  else if (context->argumentCount() == 1 &&
           qscriptvalue_cast<QWidget*>(context->argument(0)))
    cbox = new XComboBox(qscriptvalue_cast<QWidget*>(context->argument(0)));

  else if (context->argumentCount() == 2 &&
           context->argument(0).isBool() &&
           qscriptvalue_cast<QWidget*>(context->argument(1)))
    cbox = new XComboBox(context->argument(0).toBool(),
                         qscriptvalue_cast<QWidget*>(context->argument(1)));
  else if (context->argumentCount() == 2 &&
           qscriptvalue_cast<QWidget*>(context->argument(0)) &&
           context->argument(1).isString())
    cbox = new XComboBox(qscriptvalue_cast<QWidget*>(context->argument(0)),
                         context->argument(1).toString().toAscii().data());

  else if (context->argumentCount() >= 3  &&
           context->argument(0).isBool() &&
           qscriptvalue_cast<QWidget*>(context->argument(1)))
  {
    cbox = new XComboBox(context->argument(0).toBool(),
                         qscriptvalue_cast<QWidget*>(context->argument(1)),
                         context->argument(2).toString().toAscii().data());
  }

  else
    context->throwError(QScriptContext::UnknownError,
                        QString("Could not find an appropriate XComboBox constructor"));

  return engine->toScriptValue(cbox);
}

void setupXComboBox(QScriptEngine *engine)
{
  qScriptRegisterMetaType(engine, XComboBoxtoScriptValue, XComboBoxfromScriptValue);
  //qScriptRegisterMetaType(engine, XComboBoxTypestoScriptValue, XComboBoxTypesfromScriptValue);
  qScriptRegisterMetaType(engine, XComboBoxDefaultstoScriptValue, XComboBoxDefaultsfromScriptValue);

  QScriptValue widget = engine->newFunction(constructXComboBox);

  widget.setProperty("First",   QScriptValue(engine, XComboBox::First), QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("None",    QScriptValue(engine, XComboBox::None),  QScriptValue::ReadOnly | QScriptValue::Undeletable);

  widget.setProperty("Adhoc",                QScriptValue(engine, XComboBox::Adhoc),                QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("APBankAccounts",       QScriptValue(engine, XComboBox::APBankAccounts),       QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("APTerms",              QScriptValue(engine, XComboBox::APTerms),              QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ARBankAccounts",       QScriptValue(engine, XComboBox::ARBankAccounts),       QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ARTerms",              QScriptValue(engine, XComboBox::ARTerms),              QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("AccountingPeriods",    QScriptValue(engine, XComboBox::AccountingPeriods),    QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Agent",                QScriptValue(engine, XComboBox::Agent),                QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("AllCommentTypes",      QScriptValue(engine, XComboBox::AllCommentTypes),      QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("AllProjects",          QScriptValue(engine, XComboBox::AllProjects),          QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("CRMAccounts",          QScriptValue(engine, XComboBox::CRMAccounts),          QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ClassCodes",           QScriptValue(engine, XComboBox::ClassCodes),           QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Companies",            QScriptValue(engine, XComboBox::Companies),            QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("CostCategories",       QScriptValue(engine, XComboBox::CostCategories),       QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Countries",            QScriptValue(engine, XComboBox::Countries),            QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Currencies",           QScriptValue(engine, XComboBox::Currencies),           QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("CurrenciesNotBase",    QScriptValue(engine, XComboBox::CurrenciesNotBase),    QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("CustomerCommentTypes", QScriptValue(engine, XComboBox::CustomerCommentTypes), QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("CustomerGroups",       QScriptValue(engine, XComboBox::CustomerGroups),       QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("CustomerTypes",        QScriptValue(engine, XComboBox::CustomerTypes),        QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ExpenseCategories",    QScriptValue(engine, XComboBox::ExpenseCategories),    QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("FinancialLayouts",     QScriptValue(engine, XComboBox::FinancialLayouts),     QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("FiscalYears",          QScriptValue(engine, XComboBox::FiscalYears),          QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("FreightClasses",       QScriptValue(engine, XComboBox::FreightClasses),       QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Honorifics",           QScriptValue(engine, XComboBox::Honorifics),           QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("IncidentCategory",     QScriptValue(engine, XComboBox::IncidentCategory),     QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("IncidentPriority",     QScriptValue(engine, XComboBox::IncidentPriority),     QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("IncidentResolution",   QScriptValue(engine, XComboBox::IncidentResolution),   QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("IncidentSeverity",     QScriptValue(engine, XComboBox::IncidentSeverity),     QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ItemCommentTypes",     QScriptValue(engine, XComboBox::ItemCommentTypes),     QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ItemGroups",           QScriptValue(engine, XComboBox::ItemGroups),           QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Locales",              QScriptValue(engine, XComboBox::Locales),              QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("LocaleCountries",      QScriptValue(engine, XComboBox::LocaleCountries),      QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("LocaleLanguages",      QScriptValue(engine, XComboBox::LocaleLanguages),      QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("LotSerialCommentTypes",QScriptValue(engine, XComboBox::LotSerialCommentTypes),QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("OpportunityStages",    QScriptValue(engine, XComboBox::OpportunityStages),    QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("OpportunitySources",   QScriptValue(engine, XComboBox::OpportunitySources),   QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("OpportunityTypes",     QScriptValue(engine, XComboBox::OpportunityTypes),     QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("PlannerCodes",         QScriptValue(engine, XComboBox::PlannerCodes),         QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("PoProjects",           QScriptValue(engine, XComboBox::PoProjects),           QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ProductCategories",    QScriptValue(engine, XComboBox::ProductCategories),    QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ProfitCenters",        QScriptValue(engine, XComboBox::ProfitCenters),        QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ProjectCommentTypes",  QScriptValue(engine, XComboBox::ProjectCommentTypes),  QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ReasonCodes",          QScriptValue(engine, XComboBox::ReasonCodes),          QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("RegistrationTypes",    QScriptValue(engine, XComboBox::RegistrationTypes),    QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Reports",              QScriptValue(engine, XComboBox::Reports),              QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("SalesCategories",      QScriptValue(engine, XComboBox::SalesCategories),      QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("SalesCategoriesActive",QScriptValue(engine, XComboBox::SalesCategoriesActive),QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("SalesReps",            QScriptValue(engine, XComboBox::SalesReps),            QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("SalesRepsActive",      QScriptValue(engine, XComboBox::SalesRepsActive),      QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("SaleTypes",            QScriptValue(engine, XComboBox::SaleTypes),            QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ShipVias",             QScriptValue(engine, XComboBox::ShipVias),             QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ShippingCharges",      QScriptValue(engine, XComboBox::ShippingCharges),      QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ShippingForms",        QScriptValue(engine, XComboBox::ShippingForms),        QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("ShippingZones",        QScriptValue(engine, XComboBox::ShippingZones),        QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("SiteTypes",            QScriptValue(engine, XComboBox::SiteTypes),            QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("SoProjects",           QScriptValue(engine, XComboBox::SoProjects),           QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Subaccounts",          QScriptValue(engine, XComboBox::Subaccounts),          QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("TaxAuths",             QScriptValue(engine, XComboBox::TaxAuths),             QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("TaxCodes",             QScriptValue(engine, XComboBox::TaxCodes),             QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("TaxTypes",             QScriptValue(engine, XComboBox::TaxTypes),             QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Terms",              QScriptValue(engine, XComboBox::Terms),                QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("UOMs",                 QScriptValue(engine, XComboBox::UOMs),                 QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("Users",                QScriptValue(engine, XComboBox::Users),                QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("VendorCommentTypes",   QScriptValue(engine, XComboBox::VendorCommentTypes),   QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("VendorGroups",         QScriptValue(engine, XComboBox::VendorGroups),         QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("VendorTypes",          QScriptValue(engine, XComboBox::VendorTypes),          QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("WoProjects",           QScriptValue(engine, XComboBox::WoProjects),           QScriptValue::ReadOnly | QScriptValue::Undeletable);
  widget.setProperty("WorkCenters",          QScriptValue(engine, XComboBox::WorkCenters),          QScriptValue::ReadOnly | QScriptValue::Undeletable);

  engine->globalObject().setProperty("XComboBox", widget, QScriptValue::ReadOnly | QScriptValue::Undeletable);
}
