/****************************************************************************
** Meta object code from reading C++ file 'MyWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.9)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../MyWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MyWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.9. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MyWidget_t {
    QByteArrayData data[1];
    char stringdata0[9];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MyWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MyWidget_t qt_meta_stringdata_MyWidget = {
    {
QT_MOC_LITERAL(0, 0, 8) // "MyWidget"

    },
    "MyWidget"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MyWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void MyWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject MyWidget::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_MyWidget.data,
    qt_meta_data_MyWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MyWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MyWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MyWidget.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "PluginInterfaceBase"))
        return static_cast< PluginInterfaceBase*>(this);
    if (!strcmp(_clname, "org.example.PluginInterface/1.0"))
        return static_cast< PluginInterfaceBase*>(this);
    return QWidget::qt_metacast(_clname);
}

int MyWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    return _id;
}

QT_PLUGIN_METADATA_SECTION
static constexpr unsigned char qt_pluginMetaData[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',
    // metadata version, Qt version, architectural requirements
    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),
    0xbf, 
    // "IID"
    0x02,  0x78,  0x1f,  'o',  'r',  'g',  '.',  'e', 
    'x',  'a',  'm',  'p',  'l',  'e',  '.',  'P', 
    'l',  'u',  'g',  'i',  'n',  'I',  'n',  't', 
    'e',  'r',  'f',  'a',  'c',  'e',  '/',  '1', 
    '.',  '0', 
    // "className"
    0x03,  0x68,  'M',  'y',  'W',  'i',  'd',  'g', 
    'e',  't', 
    // "MetaData"
    0x04,  0xa5,  0x69,  'c',  'l',  'a',  's',  's', 
    'N',  'a',  'm',  'e',  0x68,  'M',  'y',  'W', 
    'i',  'd',  'g',  'e',  't',  0x6b,  'd',  'e', 
    's',  'c',  'r',  'i',  'p',  't',  'i',  'o', 
    'n',  0x72,  uchar('\xe8'), uchar('\xbf'), uchar('\x99'), uchar('\xe6'), uchar('\x98'), uchar('\xaf'),
    uchar('\xe4'), uchar('\xb8'), uchar('\x80'), uchar('\xe4'), uchar('\xb8'), uchar('\xaa'), uchar('\xe7'), uchar('\xbb'),
    uchar('\x84'), uchar('\xe4'), uchar('\xbb'), uchar('\xb6'), 0x6b,  'd',  'i',  's', 
    'p',  'l',  'a',  'y',  'N',  'a',  'm',  'e', 
    0x68,  'M',  'y',  'W',  'i',  'd',  'g',  'e', 
    't',  0x64,  'n',  'a',  'm',  'e',  0x68,  'M', 
    'y',  'W',  'i',  'd',  'g',  'e',  't',  0x67, 
    'v',  'e',  'r',  's',  'i',  'o',  'n',  0x65, 
    '1',  '.',  '0',  '.',  '0', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(MyWidget, MyWidget)

QT_WARNING_POP
QT_END_MOC_NAMESPACE
