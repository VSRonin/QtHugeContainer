#ifndef tst_hugemap_h__
#define tst_hugemap_h__


#include <QObject>

class tst_HugeMap : public QObject
{
    Q_OBJECT
private slots:
    void testConstructor();
    void testConstructor_data();
    void testMinimalFileSize();
    void testMinimalFileSize_data();
    void testContains();
    void testCount();
    void testSize();
    void testEmpty();
    void testIsEmpty();
    void testErase();
    void testFind();
    void testConstFind();
    void testFirst();
    void testFirstKey();
    void testEquality();
    void testSerialisation();
    void testSerialisationOldVersion();
    void testIteratorDetatch();
};

#endif // tst_hugemap_h__