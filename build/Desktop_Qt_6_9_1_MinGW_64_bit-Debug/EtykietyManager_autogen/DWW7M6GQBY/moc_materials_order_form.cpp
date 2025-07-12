/****************************************************************************
** Meta object code from reading C++ file 'materials_order_form.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../views/materials_order_form.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'materials_order_form.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN18MaterialsOrderFormE_t {};
} // unnamed namespace

template <> constexpr inline auto MaterialsOrderForm::qt_create_metaobjectdata<qt_meta_tag_ZN18MaterialsOrderFormE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MaterialsOrderForm",
        "addMaterialRow",
        "",
        "handleDeleteMaterial",
        "removeMaterialRow",
        "row",
        "handleSelectSupplier",
        "handleSaveSupplier",
        "handleSelectDelivery",
        "handleSaveDelivery",
        "handleEditSupplier",
        "handleEditDelivery",
        "handleSaveOrder",
        "updateMaterialCompleter",
        "showOrderSelectDialog",
        "sendOrderByEmail",
        "pdfPath",
        "email",
        "supplierName",
        "orderNumber",
        "handleEmailSent",
        "success",
        "message"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'addMaterialRow'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleDeleteMaterial'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'removeMaterialRow'
        QtMocHelpers::SlotData<void(int)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'handleSelectSupplier'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSaveSupplier'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSelectDelivery'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSaveDelivery'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleEditSupplier'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleEditDelivery'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSaveOrder'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateMaterialCompleter'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'showOrderSelectDialog'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'sendOrderByEmail'
        QtMocHelpers::SlotData<void(const QString &, const QString &, const QString &, const QString &)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 16 }, { QMetaType::QString, 17 }, { QMetaType::QString, 18 }, { QMetaType::QString, 19 },
        }}),
        // Slot 'handleEmailSent'
        QtMocHelpers::SlotData<void(bool, const QString &)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 21 }, { QMetaType::QString, 22 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MaterialsOrderForm, qt_meta_tag_ZN18MaterialsOrderFormE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MaterialsOrderForm::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MaterialsOrderFormE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MaterialsOrderFormE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18MaterialsOrderFormE_t>.metaTypes,
    nullptr
} };

void MaterialsOrderForm::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MaterialsOrderForm *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->addMaterialRow(); break;
        case 1: _t->handleDeleteMaterial(); break;
        case 2: _t->removeMaterialRow((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->handleSelectSupplier(); break;
        case 4: _t->handleSaveSupplier(); break;
        case 5: _t->handleSelectDelivery(); break;
        case 6: _t->handleSaveDelivery(); break;
        case 7: _t->handleEditSupplier(); break;
        case 8: _t->handleEditDelivery(); break;
        case 9: _t->handleSaveOrder(); break;
        case 10: _t->updateMaterialCompleter(); break;
        case 11: _t->showOrderSelectDialog(); break;
        case 12: _t->sendOrderByEmail((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 13: _t->handleEmailSent((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObject *MaterialsOrderForm::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MaterialsOrderForm::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MaterialsOrderFormE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MaterialsOrderForm::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}
QT_WARNING_POP
