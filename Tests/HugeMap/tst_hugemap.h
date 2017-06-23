#ifndef tst_hugemap_h__
#define tst_hugemap_h__


#include <QObject>
#include "../../hugecontainer.h"
class KeyClass;
class ValueClass;
class tst_HugeMap : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();


    void testConstructor();
    void testConstructor_data();
    //void testCopyConstructor();
    
    // Test QMap API
    void testClear();
    void testConstFind();
    void testContains();
    void testCount();
    void testEmpty();
    void testErase();
    void testFind();
    void testFirst();
    void testFirstKey();
    //void testInsertValue();
    //void testInsertPointer();
    void testIsEmpty();
    void testKey();
    void testKeys();
    void testLast();
    void testLastKey();
    //void testRemove();
    void testSize();
    void testSwap();
    //void testTake();
    void testToQtContainer();
    void testToStdContainer();
    void testUniqueKeys();
    void testUnite();
    //void testValue();

    // test specific public API
    //void testDefrag();
    void testFragmentation();
    //void testCompression();
    //void testCacheSizeChange();
    void testFileSize();

    // test iterators
    //void testIterator();
    //void testConstIterator();
    //void testKeyIterator();

    // Test Operators
    void testEquality();
    void testUnEquality();
    //void testAssignment();
    void testMoveAssignment();
    //void testOperatorGet();
    //void testOperatorDebug();
    void testSerialisation();
    void testSerialisationOldVersion();
    
    // Additional Tests
    void testMinimalFileSize();
    void testMinimalFileSize_data();
    void testIteratorDetatch();
    //void testWithStdAlgorithms();
    //void threadSafety();

private:
    void testGenericSize(int(HugeContainers::HugeMap<KeyClass, ValueClass>::*fn)() const);
    void testGenericEmpty(bool(HugeContainers::HugeMap<KeyClass, ValueClass>::*fn)() const);
    
};

#endif // tst_hugemap_h__