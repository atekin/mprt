/****************************************************************************
** Meta object code from reading C++ file 'playlist_view.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/plugins/ui_plugins/ui_plugin_qt/playlist_view.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'playlist_view.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_mprt_ui_plugin_qt__playlist_view_t {
    QByteArrayData data[6];
    char stringdata0[79];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_mprt_ui_plugin_qt__playlist_view_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_mprt_ui_plugin_qt__playlist_view_t qt_meta_stringdata_mprt_ui_plugin_qt__playlist_view = {
    {
QT_MOC_LITERAL(0, 0, 32), // "mprt_ui_plugin_qt::playlist_view"
QT_MOC_LITERAL(1, 33, 13), // "linkActivated"
QT_MOC_LITERAL(2, 47, 0), // ""
QT_MOC_LITERAL(3, 48, 4), // "link"
QT_MOC_LITERAL(4, 53, 11), // "linkHovered"
QT_MOC_LITERAL(5, 65, 13) // "linkUnhovered"

    },
    "mprt_ui_plugin_qt::playlist_view\0"
    "linkActivated\0\0link\0linkHovered\0"
    "linkUnhovered"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_mprt_ui_plugin_qt__playlist_view[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,
       4,    1,   32,    2, 0x06 /* Public */,
       5,    0,   35,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,

       0        // eod
};

void mprt_ui_plugin_qt::playlist_view::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        playlist_view *_t = static_cast<playlist_view *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->linkActivated((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->linkHovered((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->linkUnhovered(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (playlist_view::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&playlist_view::linkActivated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (playlist_view::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&playlist_view::linkHovered)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (playlist_view::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&playlist_view::linkUnhovered)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject mprt_ui_plugin_qt::playlist_view::staticMetaObject = {
    { &QTableView::staticMetaObject, qt_meta_stringdata_mprt_ui_plugin_qt__playlist_view.data,
      qt_meta_data_mprt_ui_plugin_qt__playlist_view,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *mprt_ui_plugin_qt::playlist_view::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mprt_ui_plugin_qt::playlist_view::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mprt_ui_plugin_qt__playlist_view.stringdata0))
        return static_cast<void*>(this);
    return QTableView::qt_metacast(_clname);
}

int mprt_ui_plugin_qt::playlist_view::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTableView::qt_metacall(_c, _id, _a);
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
void mprt_ui_plugin_qt::playlist_view::linkActivated(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void mprt_ui_plugin_qt::playlist_view::linkHovered(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void mprt_ui_plugin_qt::playlist_view::linkUnhovered()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
