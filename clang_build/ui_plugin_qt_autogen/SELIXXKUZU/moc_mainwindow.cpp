/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/plugins/ui_plugins/ui_plugin_qt/mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_mprt_ui_plugin_qt__main_window_t {
    QByteArrayData data[16];
    char stringdata0[230];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_mprt_ui_plugin_qt__main_window_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_mprt_ui_plugin_qt__main_window_t qt_meta_stringdata_mprt_ui_plugin_qt__main_window = {
    {
QT_MOC_LITERAL(0, 0, 30), // "mprt_ui_plugin_qt::main_window"
QT_MOC_LITERAL(1, 31, 9), // "slot_play"
QT_MOC_LITERAL(2, 41, 0), // ""
QT_MOC_LITERAL(3, 42, 10), // "slot_pause"
QT_MOC_LITERAL(4, 53, 9), // "slot_stop"
QT_MOC_LITERAL(5, 63, 13), // "slot_previous"
QT_MOC_LITERAL(6, 77, 9), // "slot_next"
QT_MOC_LITERAL(7, 87, 11), // "slot_random"
QT_MOC_LITERAL(8, 99, 14), // "slot_add_files"
QT_MOC_LITERAL(9, 114, 18), // "slot_add_directory"
QT_MOC_LITERAL(10, 133, 20), // "slot_create_playlist"
QT_MOC_LITERAL(11, 154, 18), // "slot_load_playlist"
QT_MOC_LITERAL(12, 173, 18), // "slot_save_playlist"
QT_MOC_LITERAL(13, 192, 16), // "slot_preferences"
QT_MOC_LITERAL(14, 209, 9), // "slot_exit"
QT_MOC_LITERAL(15, 219, 10) // "slot_about"

    },
    "mprt_ui_plugin_qt::main_window\0slot_play\0"
    "\0slot_pause\0slot_stop\0slot_previous\0"
    "slot_next\0slot_random\0slot_add_files\0"
    "slot_add_directory\0slot_create_playlist\0"
    "slot_load_playlist\0slot_save_playlist\0"
    "slot_preferences\0slot_exit\0slot_about"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_mprt_ui_plugin_qt__main_window[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   84,    2, 0x08 /* Private */,
       3,    0,   85,    2, 0x08 /* Private */,
       4,    0,   86,    2, 0x08 /* Private */,
       5,    0,   87,    2, 0x08 /* Private */,
       6,    0,   88,    2, 0x08 /* Private */,
       7,    0,   89,    2, 0x08 /* Private */,
       8,    0,   90,    2, 0x08 /* Private */,
       9,    0,   91,    2, 0x08 /* Private */,
      10,    0,   92,    2, 0x08 /* Private */,
      11,    0,   93,    2, 0x08 /* Private */,
      12,    0,   94,    2, 0x08 /* Private */,
      13,    0,   95,    2, 0x08 /* Private */,
      14,    0,   96,    2, 0x08 /* Private */,
      15,    0,   97,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void mprt_ui_plugin_qt::main_window::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        main_window *_t = static_cast<main_window *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->slot_play(); break;
        case 1: _t->slot_pause(); break;
        case 2: _t->slot_stop(); break;
        case 3: _t->slot_previous(); break;
        case 4: _t->slot_next(); break;
        case 5: _t->slot_random(); break;
        case 6: _t->slot_add_files(); break;
        case 7: _t->slot_add_directory(); break;
        case 8: _t->slot_create_playlist(); break;
        case 9: _t->slot_load_playlist(); break;
        case 10: _t->slot_save_playlist(); break;
        case 11: _t->slot_preferences(); break;
        case 12: _t->slot_exit(); break;
        case 13: _t->slot_about(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject mprt_ui_plugin_qt::main_window::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_mprt_ui_plugin_qt__main_window.data,
      qt_meta_data_mprt_ui_plugin_qt__main_window,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *mprt_ui_plugin_qt::main_window::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mprt_ui_plugin_qt::main_window::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mprt_ui_plugin_qt__main_window.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int mprt_ui_plugin_qt::main_window::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 14;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
