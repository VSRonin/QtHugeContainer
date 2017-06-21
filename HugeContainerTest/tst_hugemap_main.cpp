

#include "tst_hugemap.h"
#include <QtTest/QtTest>
QTEST_APPLESS_MAIN(tst_HugeMap)

/*

#include <hugecontainer.h>
#include <QDebug>

int main(int argc, char *argv[])
{
    using namespace HugeContainer;
    cleanUp();
    HugeMap<int, QString> testMap;
    testMap.setMaxCache(10);
    testMap.insert(0, "zero1");
    testMap.insert(1, "one");
    testMap.insert(2, "two");
    testMap.insert(3, "three");
    testMap.insert(4, "four");
    HugeMap<int, QString> testMap2;
    testMap2.setMaxCache(10);
    testMap2.insert(0, "zero2");
    testMap2.insert(5, "five");
    testMap2.insert(6, "six");
    testMap2.insert(7, "seven");
    testMap2.insert(8, "eight");
    
    qDebug("Map 1 Before");
    for (auto i = testMap.constBegin(); i != testMap.constEnd(); ++i)
        qDebug() << i.key() << i.value();
    qDebug("Map 2");
    for (auto i = testMap2.constBegin(); i != testMap2.constEnd(); ++i)
        qDebug() << i.key() << i.value();
    testMap.unite(testMap2,false);
    qDebug("Map 1 After");
    for (auto i = testMap.constBegin(); i != testMap.constEnd(); ++i)
        qDebug() << i.key() << i.value();
  
    return 0;
}*/