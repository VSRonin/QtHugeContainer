#include "hugecontainer.h"
#include <QtTest>
#include <QDebug>
#include <QDataStream>
#include "tst_hugemap.h"
#define SINGLE_ARG(...) __VA_ARGS__ // to allow templates with multiple parameters inside macros
using namespace HugeContainers;

class KeyClass{
    int m_val;
public:
    KeyClass(int val=0) : m_val(val){}
    KeyClass(const KeyClass& other) = default;
    KeyClass& operator=(const KeyClass&) = delete;
    bool operator<(const KeyClass& other)const { return m_val < other.m_val; }
    bool operator==(const KeyClass& other) const{ return m_val == other.m_val; }
    friend QDebug operator<< (QDebug d, const KeyClass &c);
};
Q_DECLARE_METATYPE(KeyClass)
QDebug operator<< (QDebug d, const KeyClass &c){
    d << c.m_val;
    return d;
}

class ValueClass
{
    QString str;
public:
    ValueClass() = default;
    ValueClass(const QString& c)
        :str(c)
    {}
    ~ValueClass() = default;
    ValueClass(const ValueClass& c) = default;
    ValueClass& operator=(const ValueClass &o) = default;
    bool operator==(const ValueClass& other) const { return str == other.str; }
    friend QDataStream& operator<<(QDataStream& steram, const ValueClass& target);
    friend QDataStream& operator>>(QDataStream& stream, ValueClass& target);
    friend QDebug operator<< (QDebug d, const ValueClass &c);
};
Q_DECLARE_METATYPE(ValueClass)

QDataStream& operator<<(QDataStream& steram, const ValueClass& target){
    return steram << target.str;
}
QDataStream& operator>>(QDataStream& stream, ValueClass& target){
    return stream >> target.str;
}

QDebug operator<< (QDebug d, const ValueClass &c)
{
    d << c.str;
    return d;
}

void tst_HugeMap::testConstructor()
{
    QFETCH(SINGLE_ARG(const HugeMap<KeyClass, ValueClass>), ConstructedContainer);
    QFETCH(const QVector<KeyClass>, ExpectedKey);
    QFETCH(const QVector <ValueClass>, ExpectedValues);
    int i = 0;
    for (auto j = ConstructedContainer.constBegin(); j != ConstructedContainer.constEnd(); ++j,++i){
        QCOMPARE(j.key(), ExpectedKey.at(i));
        QCOMPARE(j.value(), ExpectedValues.at(i));
    }
}

void tst_HugeMap::testConstructor_data()
{
    QTest::addColumn<HugeMap<KeyClass, ValueClass>>("ConstructedContainer");
    QTest::addColumn< QVector<KeyClass> >("ExpectedKey");
    QTest::addColumn< QVector<ValueClass> >("ExpectedValues");
    const QVector<KeyClass> expKey{ { 0, 1, 2, 4, 8 } };
    const QVector<ValueClass> expVal{ { QStringLiteral("zero"), QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("four"), QStringLiteral("eight") } };
    HugeMap<KeyClass, ValueClass> initList{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    HugeMap<KeyClass, ValueClass> fromstd{
        std::map<KeyClass, ValueClass>{
            std::make_pair(0, QStringLiteral("zero"))
            , std::make_pair(1, QStringLiteral("one"))
            , std::make_pair(2, QStringLiteral("two"))
            , std::make_pair(4, QStringLiteral("four"))
            , std::make_pair(8, QStringLiteral("eight"))
        }
    };
    HugeMap<KeyClass, ValueClass> fromQt{
        QMap<KeyClass, ValueClass>{
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

void tst_HugeMap::testMinimalFileSize()
{
    QFETCH(SINGLE_ARG(HugeMap<KeyClass, ValueClass>), ConstructedContainer);
    QFETCH(const qint64, ExpectedSize);
    auto loadcache = ConstructedContainer.value(0);
    ConstructedContainer.remove(3);
    QCOMPARE(ConstructedContainer.fileSize(), ExpectedSize);
}

void tst_HugeMap::testMinimalFileSize_data()
{
    QTest::addColumn<HugeMap<KeyClass, ValueClass>>("ConstructedContainer");
    QTest::addColumn< qint64 >("ExpectedSize");
    HugeMap<KeyClass, ValueClass> container;
    container.setMaxCache(1);
    container.insert(0, ValueClass()); // 0 items in file 
    container.insert(1, ValueClass());
    container.insert(2, ValueClass());
    const qint64 size1 = container.fileSize();
    container.insert(3, ValueClass());
    const qint64 size2 = container.fileSize();
    QTest::newRow("Remove last item") << container << size2;
    container.remove(2);
    QTest::newRow("Remove last 2 items") << container << size1;
}
