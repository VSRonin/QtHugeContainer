#include "..\..\hugecontainer.h"
#include <QTest>
#include "bench_hugemap.h"
#define SINGLE_ARG(...) __VA_ARGS__ // to allow templates with multiple parameters inside macros
using namespace HugeContainers;
void bench_hugemap::benchHugeInsert_data()
{
    QTest::addColumn<HugeMap<int, QString> >("container");
    QTest::addColumn<int >("benchContSize");
    HugeMap<int, QString> container;
    for (int totalSize = 10; totalSize <= 1000; totalSize += 10) {
        container.setMaxCache(0);
        QTest::newRow(("No Cache " + QString::number(totalSize)).toLatin1().constData()) << container << totalSize;
        container.setMaxCache(totalSize / 2);
        QTest::newRow(("Half Cache " + QString::number(totalSize)).toLatin1().constData()) << container << totalSize;
        container.setMaxCache(totalSize);
        QTest::newRow(("Full Cache " + QString::number(totalSize)).toLatin1().constData()) << container << totalSize;
    }
}

void bench_hugemap::benchHugeInsert()
{
    QFETCH(SINGLE_ARG(HugeMap<int, QString>), container);
    QFETCH(const int, benchContSize);
    QBENCHMARK{
        for (int i = 0; i < benchContSize; ++i)
        container.insert(i, QString::number(i));
    }
}

void bench_hugemap::benchHugeReadKey()
{
    QFETCH(SINGLE_ARG(HugeMap<int, QString>), container);
    QFETCH(const int, benchContSize);
    QString valueRead;
    for (int i = 0; i < benchContSize; ++i)
        container.insert(i, QString::number(i));
    QBENCHMARK{
        for (int i = 0; i < benchContSize; ++i)
        valueRead = container.value(i);
    }
}

void bench_hugemap::benchHugeReadIter()
{
    QFETCH(SINGLE_ARG(HugeMap<int, QString>), container);
    QFETCH(const int, benchContSize);
    for (int i = 0; i < benchContSize; ++i)
        container.insert(i, QString::number(i));
    QString valueRead;
    const auto contEnd = container.constEnd();
    QBENCHMARK{
        for (auto i = container.constBegin(); i != contEnd; ++i)
        valueRead = i.value();
    }
}

void bench_hugemap::benchHugeReadKeyReverse()
{
    QFETCH(SINGLE_ARG(HugeMap<int, QString>), container);
    QFETCH(const int, benchContSize);
    for (int i = 0; i < benchContSize; ++i)
        container.insert(i, QString::number(i));
    QString valueRead;
    QBENCHMARK{
        for (int i = benchContSize - 1; i >= 0; --i)
        valueRead = container.value(i);
    }
}

void bench_hugemap::benchHugeReadIterReverse()
{
    QFETCH(SINGLE_ARG(HugeMap<int, QString>), container);
    QFETCH(const int, benchContSize);
    for (int i = 0; i < benchContSize; ++i)
        container.insert(i, QString::number(i));
    QString valueRead;
    const auto contEnd = container.constBegin();
    QBENCHMARK{
        for (auto i = container.constEnd() - 1; i != contEnd; --i)
        valueRead = i.value();
    }
}

void bench_hugemap::benchQtInsert_data()
{
    QTest::addColumn<int >("benchContSize");
    for (int totalSize = 10; totalSize <= 1000; totalSize += 10) {
        QTest::newRow(QString::number(totalSize).toLatin1().constData()) << totalSize;
        QTest::newRow(QString::number(totalSize).toLatin1().constData()) << totalSize;
        QTest::newRow(QString::number(totalSize).toLatin1().constData()) << totalSize;
    }
}

void bench_hugemap::benchQtInsert()
{
    QFETCH(const int, benchContSize);
    QMap<int, QString> benchQMap;
    QBENCHMARK{
        for (int i = 0; i < benchContSize; ++i)
        benchQMap.insert(i, QString::number(i));
    }
}

void bench_hugemap::benchQtReadKey()
{
    QFETCH(const int, benchContSize);
    QMap<int, QString> benchQMap;
    for (int i = 0; i < benchContSize; ++i)
        benchQMap.insert(i, QString::number(i));
    QString valueRead;
    QBENCHMARK{
        for (int i = 0; i < benchContSize; ++i)
        valueRead = benchQMap.value(i);
    }
}

void bench_hugemap::benchQtReadIter()
{
    QFETCH(const int, benchContSize);
    QMap<int, QString> benchQMap;
    for (int i = 0; i < benchContSize; ++i)
        benchQMap.insert(i, QString::number(i));
    QString valueRead;
    const auto contQtEnd = benchQMap.constEnd();
    QBENCHMARK{
        for (auto i = benchQMap.constBegin(); i != contQtEnd; ++i)
        valueRead = i.value();
    }
}

void bench_hugemap::benchStdInsert()
{
    QFETCH(const int, benchContSize);
    std::map<int, QString> benchStdMap;
    QBENCHMARK{
        for (int i = 0; i < benchContSize; ++i)
        benchStdMap.insert(std::make_pair(i, QString::number(i)));
    }
}

void bench_hugemap::benchStdReadKey()
{
    QFETCH(const int, benchContSize);
    std::map<int, QString> benchStdMap;
    for (int i = 0; i < benchContSize; ++i)
        benchStdMap.insert(std::make_pair(i, QString::number(i)));
    QString valueRead;
    QBENCHMARK{
        for (int i = 0; i < benchContSize; ++i)
        valueRead = benchStdMap[i];
    }
}

void bench_hugemap::benchStdReadIter()
{
    QFETCH(const int, benchContSize);
    std::map<int, QString> benchStdMap;
    for (int i = 0; i < benchContSize; ++i)
        benchStdMap.insert(std::make_pair(i, QString::number(i)));
    QString valueRead;
    const auto contStdEnd = benchStdMap.cend();
    QBENCHMARK{
        for (auto i = benchStdMap.cbegin(); i != contStdEnd; ++i)
        valueRead = i->second;
    }
}
