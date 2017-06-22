#ifndef tst_hugemap_h__
#define tst_hugemap_h__


#include <QtTest>

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
//////////////////////////////////////////////////////////////////////////
    void benchHugeInsert_data();
    void benchHugeReadKey_data() { benchHugeInsert_data(); }
    void benchHugeReadIter_data() { benchHugeInsert_data(); }
    void benchHugeReadKeyReverse_data() { benchHugeInsert_data(); }
    void benchHugeReadIterReverse_data() { benchHugeInsert_data(); }
    void benchHugeInsert();
    void benchHugeReadKey();
    void benchHugeReadIter();
    void benchHugeReadKeyReverse();
    void benchHugeReadIterReverse();


    void benchQtInsert();
    void benchQtReadKey();
    void benchQtReadIter();
    void benchStdInsert();
    void benchStdReadKey();
    void benchStdReadIter();
private:
    const int benchContSize = 100;
};

#endif // tst_hugemap_h__