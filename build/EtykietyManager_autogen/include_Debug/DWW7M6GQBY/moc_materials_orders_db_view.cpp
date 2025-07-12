/****************************************************************************
** Meta object code from reading C++ file 'materials_orders_db_view.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../views/materials_orders_db_view.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'materials_orders_db_view.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN21MaterialsOrdersDbViewE_t {};
} // unnamed namespace

template <> constexpr inline auto MaterialsOrdersDbView::qt_create_metaobjectdata<qt_meta_tag_ZN21MaterialsOrdersDbViewE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MaterialsOrdersDbView",
        "orderSelected",
        "",
        "orderId",
        "requestEditOrder",
        "requestPrintOrder",
        "requestDuplicateOrder",
        "refreshOrders",
        "handleEdit",
        "handleDelete",
        "handlePrint",
        "handleView",
        "handleDoneChanged",
        "QTableWidgetItem*",
        "item",
        "handleDuplicate"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'orderSelected'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'requestEditOrder'
        QtMocHelpers::SignalData<void(int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'requestPrintOrder'
        QtMocHelpers::SignalData<void(int)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'requestDuplicateOrder'
        QtMocHelpers::SignalData<void(int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'refreshOrders'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'handleEdit'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleDelete'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handlePrint'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleView'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleDoneChanged'
        QtMocHelpers::SlotData<void(QTableWidgetItem *)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Slot 'handleDuplicate'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MaterialsOrdersDbView, qt_meta_tag_ZN21MaterialsOrdersDbViewE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MaterialsOrdersDbView::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21MaterialsOrdersDbViewE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21MaterialsOrdersDbViewE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN21MaterialsOrdersDbViewE_t>.metaTypes,
    nullptr
} };

void MaterialsOrdersDbView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MaterialsOrdersDbView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->orderSelected((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->requestEditOrder((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->requestPrintOrder((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->requestDuplicateOrder((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->refreshOrders(); break;
        case 5: _t->handleEdit(); break;
        case 6: _t->handleDelete(); break;
        case 7: _t->handlePrint(); break;
        case 8: _t->handleView(); break;
        case 9: _t->handleDoneChanged((*reinterpret_cast< std::add_pointer_t<QTableWidgetItem*>>(_a[1]))); break;
        case 10: _t->handleDuplicate(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MaterialsOrdersDbView::*)(int )>(_a, &MaterialsOrdersDbView::orderSelected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MaterialsOrdersDbView::*)(int )>(_a, &MaterialsOrdersDbView::requestEditOrder, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MaterialsOrdersDbView::*)(int )>(_a, &MaterialsOrdersDbView::requestPrintOrder, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MaterialsOrdersDbView::*)(int )>(_a, &MaterialsOrdersDbView::requestDuplicateOrder, 3))
            return;
    }
}

const QMetaObject *MaterialsOrdersDbView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MaterialsOrdersDbView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21MaterialsOrdersDbViewE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MaterialsOrdersDbView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void MaterialsOrdersDbView::orderSelected(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void MaterialsOrdersDbView::requestEditOrder(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void MaterialsOrdersDbView::requestPrintOrder(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void MaterialsOrdersDbView::requestDuplicateOrder(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
