/****************************************************************************
** Based on a modified version of tst_HugeMap
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
**
****************************************************************************/
#include "hugecontainer.h"
#include <QtTest/QtTest>
#include <QDebug>
#include <QDataStream>
#include "tst_hugemap.h"
using namespace HugeContainers;

class KeyClass{
    int m_val;
    KeyClass(int val=0) : m_val(val){}
    KeyClass(const KeyClass&) = delete;
    KeyClass& operator=(const KeyClass&) = delete;
    KeyClass(KeyClass&&) = delete;
    KeyClass& operator=(KeyClass&&) = delete;
    bool operator<(const KeyClass& other) { return m_val < other.m_val; }
};

class MyClass
{
public:
    MyClass() = default;
    MyClass(const QString& c)
        :str(c)
    {}
    ~MyClass() = default;
    MyClass(const MyClass& c) = default;
    MyClass& operator=(const MyClass &o) = default;
    QString str;
    friend QDataStream& operator<<(QDataStream& steram, const MyClass& target);
    friend QDataStream& operator<<(QDataStream& stream, MyClass& target);
};
QDataStream& operator<<(QDataStream& steram, const MyClass& target){
    return steram << target.str;
}
QDataStream& operator>>(QDataStream& stream, MyClass& target){
    return stream >> target.str;
}

QDebug operator<< (QDebug d, const MyClass &c)
{
    d << c.str;
    return d;
}


void tst_HugeMap::testConstructor()
{
    using MapType = HugeMap<int, QString>;
    QFETCH(const MapType, ConstructedContainer);
    QFETCH(const QList<int>, ExpectedKey);
    QFETCH(const QList<QString>, ExpectedValues);
    int i = 0;
    for (auto j = ConstructedContainer.constBegin(); j != ConstructedContainer.constEnd(); ++j,++i){
        QCOMPARE(j.key(), ExpectedKey.at(i));
        QCOMPARE(j.value(), ExpectedValues.at(i));
    }
}

void tst_HugeMap::testConstructor_data()
{
    QTest::addColumn<HugeMap<int,QString>>("ConstructedContainer");
    QTest::addColumn<QList<int>>("ExpectedKey");
    QTest::addColumn<QList<QString>>("ExpectedValues");
    const QList<int> expKey = { 0, 1, 2, 4, 8 };
    const QList<QString> expVal = { QStringLiteral("zero"), QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("four"), QStringLiteral("eight") };
    HugeMap<int, QString> initList{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    HugeMap<int, QString> fromstd{
        std::map<int,QString>{
            std::make_pair(0, QStringLiteral("zero"))
            , std::make_pair(1, QStringLiteral("one"))
            , std::make_pair(2, QStringLiteral("two"))
            , std::make_pair(4, QStringLiteral("four"))
            , std::make_pair(8, QStringLiteral("eight"))
        }
    };
    HugeMap<int, QString> fromQt{
        QMap<int, QString>{
            std::make_pair(0, QStringLiteral("zero"))
                , std::make_pair(1, QStringLiteral("one"))
                , std::make_pair(2, QStringLiteral("two"))
                , std::make_pair(4, QStringLiteral("four"))
                , std::make_pair(8, QStringLiteral("eight"))
        }
    };
    QTest::newRow("from initializer list") << initList << expKey << expVal;
    QTest::newRow("from std container") << fromstd << expKey << expVal;
    QTest::newRow("from Qt container") << fromQt << expKey << expVal;
}
