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
    int val() const { return m_val; }
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
    QString m_str;
public:
    QString val() const { return m_str; }
    ValueClass() = default;
    ValueClass(const QString& c)
        :m_str(c)
    {}
    ~ValueClass() = default;
    ValueClass(const ValueClass& c) = default;
    ValueClass& operator=(const ValueClass &o) = default;
    bool operator==(const ValueClass& other) const { return m_str == other.m_str; }
    friend QDataStream& operator<<(QDataStream& steram, const ValueClass& target);
    friend QDataStream& operator>>(QDataStream& stream, ValueClass& target);
    friend QDebug operator<< (QDebug d, const ValueClass &c);
};
Q_DECLARE_METATYPE(ValueClass)

QDataStream& operator<<(QDataStream& steram, const ValueClass& target){
    return steram << target.m_str;
}
QDataStream& operator>>(QDataStream& stream, ValueClass& target){
    return stream >> target.m_str;
}

QDebug operator<< (QDebug d, const ValueClass &c)
{
    d << c.m_str;
    return d;
}
namespace QTest {
    char *toString(const KeyClass &key) 
    {
        return qstrdup(QString::number(key.val()).toLatin1().data());
    }
    char *toString(const ValueClass &value)
    {
        return qstrdup(value.val().toLatin1().data());
    }
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
    auto container2 = ConstructedContainer;
    while (!container2.isEmpty())
        container2.erase(container2.begin());
    QCOMPARE(container2.fileSize(), 0);
    ConstructedContainer.clear();
    QCOMPARE(ConstructedContainer.fileSize(), 0);
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
    container.remove(1);
    QTest::newRow("Remove all but 1 item") << container << static_cast<qint64>(0);
}

void tst_HugeMap::testContains()
{
    HugeMap<KeyClass, ValueClass> container;
    container.insert(0, ValueClass());
    container.insert(1, ValueClass());
    container.insert(2, ValueClass());
    QVERIFY(container.contains(0));
    QVERIFY(container.contains(1));
    QVERIFY(container.contains(2));
    QVERIFY(!container.contains(3));
    QVERIFY(!container.contains(-1));
}

void tst_HugeMap::testCount()
{
    HugeMap<KeyClass, ValueClass> container;
    QCOMPARE(container.count(), 0);
    container.insert(0, ValueClass());
    QCOMPARE(container.count(), 1);
    container.insert(1, ValueClass());
    QCOMPARE(container.count(), 2);
    container.insert(2, ValueClass());
    QCOMPARE(container.count(), 3);
    container.remove(1);
    QCOMPARE(container.count(), 2);
    container.clear();
    QCOMPARE(container.count(), 0);
}

void tst_HugeMap::testSize()
{
    HugeMap<KeyClass, ValueClass> container;
    QCOMPARE(container.size(), 0);
    container.insert(0, ValueClass());
    QCOMPARE(container.size(), 1);
    container.insert(1, ValueClass());
    QCOMPARE(container.size(), 2);
    container.insert(2, ValueClass());
    QCOMPARE(container.size(), 3);
    container.remove(1);
    QCOMPARE(container.size(), 2);
    container.clear();
    QCOMPARE(container.size(), 0);
}

void tst_HugeMap::testEmpty()
{
    HugeMap<KeyClass, ValueClass> container;
    QVERIFY(container.empty());
    container.insert(0, ValueClass());
    QVERIFY(!container.empty());
    container.insert(1, ValueClass());
    QVERIFY(!container.empty());
    container.insert(2, ValueClass());
    QVERIFY(!container.empty());
    auto container2 = container;
    container2.remove(0);
    container2.remove(1);
    container2.remove(2);
    QVERIFY(container2.empty());
    QVERIFY(!container.empty());
    container.clear();
    QVERIFY(container.empty());
}

void tst_HugeMap::testIsEmpty()
{
    HugeMap<KeyClass, ValueClass> container;
    QVERIFY(container.isEmpty());
    container.insert(0, ValueClass());
    QVERIFY(!container.isEmpty());
    container.insert(1, ValueClass());
    QVERIFY(!container.isEmpty());
    container.insert(2, ValueClass());
    QVERIFY(!container.isEmpty());
    auto container2 = container;
    container2.remove(0);
    container2.remove(1);
    container2.remove(2);
    QVERIFY(container2.isEmpty());
    QVERIFY(!container.isEmpty());
    container.clear();
    QVERIFY(container.isEmpty());
}

void tst_HugeMap::testErase()
{
    HugeMap<KeyClass, ValueClass> container;
    container.insert(0, ValueClass());
    container.insert(1, ValueClass());
    auto iterRemove = container.insert(2, ValueClass());
    auto container2 = container;
    container.erase(iterRemove);
    QCOMPARE(container.size() , 2);
    QVERIFY(container.contains(0));
    QVERIFY(container.contains(1));
    QVERIFY(!container.contains(2));

    auto iterRemove2 = container2.begin();
    iterRemove2 = container2.erase(iterRemove2);
    QCOMPARE(iterRemove2, container2.begin());
    QCOMPARE(container2.size(), 2);
    QVERIFY(!container2.contains(0));
    QVERIFY(container2.contains(1));
    QVERIFY(container2.contains(2));
    iterRemove2 = container2.erase(iterRemove2);
    QCOMPARE(iterRemove2, container2.begin());
    QCOMPARE(container2.size(), 1);
    QVERIFY(!container2.contains(0));
    QVERIFY(!container2.contains(1));
    QVERIFY(container2.contains(2));
    iterRemove2 = container2.erase(iterRemove2);
    QCOMPARE(iterRemove2, container2.end());
    QVERIFY(container2.isEmpty());
    QVERIFY(!container2.contains(0));
    QVERIFY(!container2.contains(1));
    QVERIFY(!container2.contains(2));
    auto iterRemove3 = container2.erase(iterRemove2);
    QCOMPARE(iterRemove3, iterRemove2);
}

void tst_HugeMap::testFind()
{
    HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    QCOMPARE(container.find(0), container.begin());
    QCOMPARE(container.find(1), container.begin() + 1);
    QCOMPARE(container.find(2), container.begin() + 2);
    QCOMPARE(container.find(4), container.begin() + 3);
    QCOMPARE(container.find(8), container.begin() + 4);
    QCOMPARE(container.find(-1), container.end());
    QCOMPARE(container.find(0).value(), ValueClass(QStringLiteral("zero")));
    QCOMPARE(container.find(0).key(), KeyClass(0));
    QCOMPARE(container.find(4).value(), ValueClass(QStringLiteral("four")));
    QCOMPARE(container.find(4).key(), KeyClass(4));
    decltype(container) container2;
    QCOMPARE(container2.find(0), container2.end());
}

void tst_HugeMap::testConstFind()
{
    const HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    QCOMPARE(container.constFind(0), container.constBegin());
    QCOMPARE(container.constFind(1), container.constBegin() + 1);
    QCOMPARE(container.constFind(2), container.constBegin() + 2);
    QCOMPARE(container.constFind(4), container.constBegin() + 3);
    QCOMPARE(container.constFind(8), container.constBegin() + 4);
    QCOMPARE(container.constFind(-1), container.constEnd());
    QCOMPARE(container.constFind(0).value(), ValueClass(QStringLiteral("zero")));
    QCOMPARE(container.constFind(0).key(), KeyClass(0));
    QCOMPARE(container.constFind(4).value(), ValueClass(QStringLiteral("four")));
    QCOMPARE(container.constFind(4).key(), KeyClass(4));
    decltype(container) container2;
    QCOMPARE(container2.constFind(0), container2.constEnd());
}

void tst_HugeMap::testFirst()
{
    HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    
    const auto container2 = container;
    QCOMPARE(container.first(), *container.constBegin());
    QCOMPARE(container.first(), ValueClass(QStringLiteral("zero")));
    container.first() = ValueClass(QStringLiteral("zero1"));
    QCOMPARE(container.first(), *container.constBegin());
    QCOMPARE(container.first(), ValueClass(QStringLiteral("zero1")));
    container.insert(-1, QStringLiteral("-1"));
    QCOMPARE(container.first(), container.constBegin().value());
    QCOMPARE(container.first(), ValueClass(QStringLiteral("-1")));
    QCOMPARE(container2.first(), ValueClass(QStringLiteral("zero")));
    QCOMPARE(container2.first(), *container2.constBegin());

}

void tst_HugeMap::testFirstKey()
{
    HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    QCOMPARE(container.firstKey(), container.constBegin().key());
    QCOMPARE(container.firstKey(), KeyClass(0));
    container.insert(-1,QStringLiteral("-1"));
    QCOMPARE(container.firstKey(), container.constBegin().key());
    QCOMPARE(container.firstKey(), KeyClass(-1));
}
