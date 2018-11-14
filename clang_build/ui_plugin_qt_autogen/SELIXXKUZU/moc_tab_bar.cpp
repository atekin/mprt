/****************************************************************************
** Meta object code from reading C++ file 'tab_bar.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/plugins/ui_plugins/ui_plugin_qt/tab_bar.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tab_bar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_mprt_ui_plugin_qt__tab_bar_t {
    QByteArrayData data[7];
    char stringdata0[107];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_mprt_ui_plugin_qt__tab_bar_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_mprt_ui_plugin_qt__tab_bar_t qt_meta_stringdata_mprt_ui_plugin_qt__tab_bar = {
    {
QT_MOC_LITERAL(0, 0, 26), // "mprt_ui_plugin_qt::tab_bar"
QT_MOC_LITERAL(1, 27, 26), // "sig_tabtext_change_request"
QT_MOC_LITERAL(2, 54, 0), // ""
QT_MOC_LITERAL(3, 55, 9), // "tab_index"
QT_MOC_LITERAL(4, 65, 4), // "text"
QT_MOC_LITERAL(5, 70, 17), // "slot_start_rename"
QT_MOC_LITERAL(6, 88, 18) // "slot_finish_rename"

    },
    "mprt_ui_plugin_qt::tab_bar\0"
    "sig_tabtext_change_request\0\0tab_index\0"
    "text\0slot_start_rename\0slot_finish_rename"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_mprt_ui_plugin_qt__tab_bar[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   29,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    1,   34,    2, 0x0a /* Public */,
       6,    0,   37,    2, 0x09 /* Protected */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,    4,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,

       0        // eod
};

void mprt_ui_plugin_qt::tab_bar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        tab_bar *_t = static_cast<tab_bar *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->sig_tabtext_change_request((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->slot_start_rename((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->slot_finish_rename(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (tab_bar::*)(int , QString const & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&tab_bar::sig_tabtext_change_request)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject mprt_ui_plugin_qt::tab_bar::staticMetaObject = {
    { &QTabBar::staticMetaObject, qt_meta_stringdata_mprt_ui_plugin_qt__tab_bar.data,
      qt_meta_data_mprt_ui_plugin_qt__tab_bar,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *mprt_ui_plugin_qt::tab_bar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mprt_ui_plugin_qt::tab_bar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mprt_ui_plugin_qt__tab_bar.stringdata0))
        return static_cast<void*>(this);
    return QTabBar::qt_metacast(_clname);
}

int mprt_ui_plugin_qt::tab_bar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTabBar::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void mprt_ui_plugin_qt::tab_bar::sig_tabtext_change_request(int _t1, QString const & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
