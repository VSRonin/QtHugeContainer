#ifndef tst_hugemap_h__
#define tst_hugemap_h__

#include "hugecontainer.h"
#include <QtTest>
#include <QDebug>
using namespace HugeContainers;

class tst_HugeMap : public QObject
{
    Q_OBJECT
private slots:
    void testConstructor();
    void testConstructor_data();
    void testMinimalFileSize();
    void testMinimalFileSize_data();
};

#endif // tst_hugemap_h__