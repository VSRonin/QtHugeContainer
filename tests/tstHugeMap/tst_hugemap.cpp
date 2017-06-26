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

void tst_HugeMap::initTestCase()
{
    HugeContainers::cleanUp();
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

void tst_HugeMap::testClear()
{
    HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    QVERIFY(!container.isEmpty());
    QVERIFY(!container.keys().isEmpty());
    QCOMPARE(container.size(), 5);
    QVERIFY(!container.fileSize()==0);
    container.clear();
    QVERIFY(container.isEmpty());
    QVERIFY(container.keys().isEmpty());
    QCOMPARE(container.size(),0);
    QCOMPARE(container.fileSize(), 0);

    container.setMaxCache(10);
    container.insert(0, QStringLiteral("zero"));
    container.insert(1, QStringLiteral("one"));
    container.insert(2, QStringLiteral("two"));
    container.insert(4, QStringLiteral("four"));
    container.insert(8, QStringLiteral("eight"));
    QVERIFY(!container.isEmpty());
    QVERIFY(!container.keys().isEmpty());
    QCOMPARE(container.size(), 5);
    QVERIFY(container.fileSize() == 0);
    container.clear();
    QVERIFY(container.isEmpty());
    QVERIFY(container.keys().isEmpty());
    QCOMPARE(container.size(), 0);
    QCOMPARE(container.fileSize(), 0);
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
    container.remove(1);
    QVERIFY(!container.contains(1));
}

void tst_HugeMap::testGenericSize(int(HugeMap<KeyClass, ValueClass>::*fn)() const){
    HugeMap<KeyClass, ValueClass> container;
    auto funcCall = std::bind(fn, &container);
    QCOMPARE(funcCall(), 0);
    container.insert(0, ValueClass());
    QCOMPARE(funcCall(), 1);
    container.insert(1, ValueClass());
    QCOMPARE(funcCall(), 2);
    container.insert(2, ValueClass());
    QCOMPARE(funcCall(), 3);
    container.remove(1);
    QCOMPARE(funcCall(), 2);
    container.clear();
    QCOMPARE(funcCall(), 0);
}

void tst_HugeMap::testCount()
{
    testGenericSize(&HugeMap<KeyClass, ValueClass>::count);
}

void tst_HugeMap::testSize()
{
    testGenericSize(&HugeMap<KeyClass, ValueClass>::size);
}

void tst_HugeMap::testSwap()
{
    HugeMap<KeyClass, ValueClass> container1{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    HugeMap<KeyClass, ValueClass> container2{
        std::make_pair(3, QStringLiteral("three"))
        , std::make_pair(5, QStringLiteral("five"))
        , std::make_pair(6, QStringLiteral("six"))
        , std::make_pair(7, QStringLiteral("seven"))
        , std::make_pair(9, QStringLiteral("nine"))
    };
    const auto container3 = container1;
    const auto container4 = container2;

    std::swap(container1, container2);
    
    QCOMPARE(container1.value(KeyClass(3), ValueClass()), ValueClass(QStringLiteral("three")));
    QCOMPARE(container1.value(KeyClass(2), ValueClass()), ValueClass());
    QCOMPARE(container2.value(KeyClass(2), ValueClass()), ValueClass(QStringLiteral("two")));
    QCOMPARE(container2.value(KeyClass(3), ValueClass()), ValueClass());

    QCOMPARE(container3, container2);
    QCOMPARE(container4, container1);
}

void tst_HugeMap::testToQtContainer()
{
    const HugeMap<KeyClass, ValueClass> container1{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    const auto qtContainer = container1.toQContainer();
    QCOMPARE(qtContainer.size(), 5);
    QCOMPARE(qtContainer.value(KeyClass(0), ValueClass()), ValueClass(QStringLiteral("zero")));
    QCOMPARE(qtContainer.value(KeyClass(1), ValueClass()), ValueClass(QStringLiteral("one")));
    QCOMPARE(qtContainer.value(KeyClass(2), ValueClass()), ValueClass(QStringLiteral("two")));
    QCOMPARE(qtContainer.value(KeyClass(4), ValueClass()), ValueClass(QStringLiteral("four")));
    QCOMPARE(qtContainer.value(KeyClass(8), ValueClass()), ValueClass(QStringLiteral("eight")));
}

void tst_HugeMap::testToStdContainer()
{
    const HugeMap<KeyClass, ValueClass> container1{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    auto stdContainer = container1.toStdContainer();
    QCOMPARE(stdContainer.size(), 5U);
    QCOMPARE(stdContainer[KeyClass(0)], ValueClass(QStringLiteral("zero")));
    QCOMPARE(stdContainer[KeyClass(1)], ValueClass(QStringLiteral("one")));
    QCOMPARE(stdContainer[KeyClass(2)], ValueClass(QStringLiteral("two")));
    QCOMPARE(stdContainer[KeyClass(4)], ValueClass(QStringLiteral("four")));
    QCOMPARE(stdContainer[KeyClass(8)], ValueClass(QStringLiteral("eight")));
    QCOMPARE(stdContainer[KeyClass(9)], ValueClass());
    QCOMPARE(stdContainer.size(), 6U);
}

void tst_HugeMap::testUniqueKeys()
{
    const HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    auto containerKeys = container.uniqueKeys();
    QCOMPARE(containerKeys.size(), 5);
    QCOMPARE(containerKeys.first(), KeyClass(0));
    QCOMPARE(containerKeys.at(1), KeyClass(1));
    QCOMPARE(containerKeys.at(2), KeyClass(2));
    QCOMPARE(containerKeys.at(3), KeyClass(4));
    QCOMPARE(containerKeys.last(), KeyClass(8));
}

void tst_HugeMap::testUnite()
{
    HugeMap<KeyClass, ValueClass> container1{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    const HugeMap<KeyClass, ValueClass> container2{
        std::make_pair(3, QStringLiteral("three"))
        , std::make_pair(5, QStringLiteral("five"))
        , std::make_pair(6, QStringLiteral("six"))
        , std::make_pair(7, QStringLiteral("seven"))
        , std::make_pair(0, QStringLiteral("zero1"))
    };

    auto container3 = container1;

    QVERIFY(container1.unite(container2, false));
    QCOMPARE(container1.size(), 9);
    QCOMPARE(container1.value(KeyClass(0), ValueClass()), ValueClass(QStringLiteral("zero")));
    QCOMPARE(container1.value(KeyClass(1), ValueClass()), ValueClass(QStringLiteral("one")));
    QCOMPARE(container1.value(KeyClass(2), ValueClass()), ValueClass(QStringLiteral("two")));
    QCOMPARE(container1.value(KeyClass(3), ValueClass()), ValueClass(QStringLiteral("three")));
    QCOMPARE(container1.value(KeyClass(4), ValueClass()), ValueClass(QStringLiteral("four")));
    QCOMPARE(container1.value(KeyClass(5), ValueClass()), ValueClass(QStringLiteral("five")));
    QCOMPARE(container1.value(KeyClass(6), ValueClass()), ValueClass(QStringLiteral("six")));
    QCOMPARE(container1.value(KeyClass(7), ValueClass()), ValueClass(QStringLiteral("seven")));
    QCOMPARE(container1.value(KeyClass(8), ValueClass()), ValueClass(QStringLiteral("eight")));

    QVERIFY(container3.unite(container2, true));
    QCOMPARE(container3.size(), 9);
    QCOMPARE(container3.value(KeyClass(0), ValueClass()), ValueClass(QStringLiteral("zero1")));
    QCOMPARE(container3.value(KeyClass(1), ValueClass()), ValueClass(QStringLiteral("one")));
    QCOMPARE(container3.value(KeyClass(2), ValueClass()), ValueClass(QStringLiteral("two")));
    QCOMPARE(container3.value(KeyClass(3), ValueClass()), ValueClass(QStringLiteral("three")));
    QCOMPARE(container3.value(KeyClass(4), ValueClass()), ValueClass(QStringLiteral("four")));
    QCOMPARE(container3.value(KeyClass(5), ValueClass()), ValueClass(QStringLiteral("five")));
    QCOMPARE(container3.value(KeyClass(6), ValueClass()), ValueClass(QStringLiteral("six")));
    QCOMPARE(container3.value(KeyClass(7), ValueClass()), ValueClass(QStringLiteral("seven")));
    QCOMPARE(container3.value(KeyClass(8), ValueClass()), ValueClass(QStringLiteral("eight")));

    container3.clear();
    QVERIFY(container3.unite(container2));
    QCOMPARE(container3, container2);

    const auto container4 = container1;
    QVERIFY(container1.unite(HugeMap<KeyClass, ValueClass>()));
    QCOMPARE(container4, container1);
}

void tst_HugeMap::testDefrag_data()
{
    QTest::addColumn<HugeMap<KeyClass, qint8>>("dataToDefrag");
    HugeMap<KeyClass, qint8> container;
    QTest::newRow("Empty") << container;
    QCOMPARE(container.fragmentation(), 0.0);
    container.insert(0, 'A');
    container.insert(1, 'B');
    container.insert(2, 'C');
    container.insert(4, 'D');
    container.insert(8, 'E');
    container.insert(16, 'F');
    QTest::newRow("No Frag") << container;
    const auto junk = container.value(0);
    Q_UNUSED(junk);
    QTest::newRow("1 Hole") << container;
    container.remove(8);
    QTest::newRow("2 Separate Holes") << container;
    container.remove(2);
    QTest::newRow("3 Separate Holes") << container;
    container.remove(4);
    QTest::newRow("2 Contiguous Holes") << container;
}

void tst_HugeMap::testDefrag()
{
    QFETCH(SINGLE_ARG(HugeMap<KeyClass, qint8>), dataToDefrag);
    dataToDefrag.defrag();
    QCOMPARE(dataToDefrag.fragmentation(), 0.0);
}

void tst_HugeMap::testFragmentation()
{
    HugeMap<KeyClass, qint8> container;
    QCOMPARE(container.fragmentation(), 0.0);
    container.insert(0, 'A');
    QCOMPARE(container.fragmentation(), 0.0);
    container.insert(1, 'B');
    QCOMPARE(container.fragmentation(), 0.0);
    container.insert(2, 'C');
    QCOMPARE(container.fragmentation(), 0.0);
    container.insert(4, 'D');
    QCOMPARE(container.fragmentation(), 0.0);
    container.insert(8, 'E');
    QCOMPARE(container.fragmentation(), 0.0);
    const auto junk = container.value(0);
    Q_UNUSED(junk);
    QCOMPARE(container.fragmentation(), 0.2);
    container.remove(8);
    QCOMPARE(container.fragmentation(), 0.25);
    container.remove(2);
    QCOMPARE(container.fragmentation(), 0.5);
    container.remove(4);
    QCOMPARE(container.fragmentation(), 0.5);
    container.insert(9, 'F');
    QCOMPARE(container.fragmentation(), 0.0);
    container.clear();
    QCOMPARE(container.fragmentation(), 0.0);
}

void tst_HugeMap::testCacheSizeChange_data()
{
    QTest::addColumn<HugeMap<KeyClass, ValueClass>>("baseContainer");
    QTest::addColumn<int>("newCache");

    HugeMap<KeyClass, ValueClass> smallToBig;
    smallToBig.setMaxCache(1);
    smallToBig.insert(0, QStringLiteral("zero"));
    smallToBig.insert(1, QStringLiteral("one"));
    smallToBig.insert(2, QStringLiteral("two"));
    smallToBig.insert(4, QStringLiteral("four"));
    smallToBig.insert(8, QStringLiteral("eight"));
    QTest::newRow("Small to big") << smallToBig << 10;

    HugeMap<KeyClass, ValueClass> bigToSmall;
    bigToSmall.setMaxCache(10);
    bigToSmall.insert(0, QStringLiteral("zero"));
    bigToSmall.insert(1, QStringLiteral("one"));
    bigToSmall.insert(2, QStringLiteral("two"));
    bigToSmall.insert(4, QStringLiteral("four"));
    bigToSmall.insert(8, QStringLiteral("eight"));
    QTest::newRow("Big to small") << bigToSmall << 1;

    HugeMap<KeyClass, ValueClass> equalSize;
    equalSize.setMaxCache(3);
    equalSize.insert(0, QStringLiteral("zero"));
    equalSize.insert(1, QStringLiteral("one"));
    equalSize.insert(2, QStringLiteral("two"));
    equalSize.insert(4, QStringLiteral("four"));
    equalSize.insert(8, QStringLiteral("eight"));
    QTest::newRow("Equal size") << equalSize << 3;

}

void tst_HugeMap::testCacheSizeChange()
{
    QFETCH(SINGLE_ARG(HugeMap<KeyClass, ValueClass>), baseContainer);
    QFETCH(const int, newCache);

    const auto container2 = baseContainer;
    baseContainer.setMaxCache(newCache);
    QCOMPARE(container2, baseContainer);
}

void tst_HugeMap::testGenericEmpty(bool(HugeMap<KeyClass, ValueClass>::*fn)() const){
    HugeMap<KeyClass, ValueClass> container;
    auto funcCall = std::bind(fn, &container);
    QVERIFY(funcCall());
    container.insert(0, ValueClass());
    QVERIFY(!funcCall());
    container.insert(1, ValueClass());
    QVERIFY(!funcCall());
    container.insert(2, ValueClass());
    QVERIFY(!funcCall());
    auto container2 = container;
    auto funcCall2 = std::bind(fn, &container2);
    container2.remove(0);
    container2.remove(1);
    container2.remove(2);
    QVERIFY(funcCall2());
    QVERIFY(!funcCall());
    container.clear();
    QVERIFY(funcCall());
}

void tst_HugeMap::testEmpty()
{
    testGenericEmpty(&HugeMap<KeyClass, ValueClass>::empty);
}

void tst_HugeMap::testIsEmpty()
{
    testGenericEmpty(&HugeMap<KeyClass, ValueClass>::isEmpty);
}

void tst_HugeMap::testKey()
{
    const HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    QCOMPARE(container.key(ValueClass(QStringLiteral("zero"))), KeyClass(0));
    QCOMPARE(container.key(ValueClass(QStringLiteral("one"))), KeyClass(1));
    QCOMPARE(container.key(ValueClass(QStringLiteral("two"))), KeyClass(2));
    QCOMPARE(container.key(ValueClass(QStringLiteral("four"))), KeyClass(4));
    QCOMPARE(container.key(ValueClass(QStringLiteral("eight"))), KeyClass(8));
    QCOMPARE(container.key(ValueClass(QStringLiteral("nine")), KeyClass(-1)), KeyClass(-1));
}

void tst_HugeMap::testKeys()
{
    const HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    auto containerKeys = container.keys();
    QCOMPARE(containerKeys.size(), 5);
    QCOMPARE(containerKeys.first(), KeyClass(0));
    QCOMPARE(containerKeys.at(1), KeyClass(1));
    QCOMPARE(containerKeys.at(2), KeyClass(2));
    QCOMPARE(containerKeys.at(3), KeyClass(4));
    QCOMPARE(containerKeys.last(), KeyClass(8));
}

void tst_HugeMap::testLast()
{
    HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };

    const auto container2 = container;
    QCOMPARE(container.last(), *(container.constEnd()-1));
    QCOMPARE(container.last(), ValueClass(QStringLiteral("eight")));
    container.last() = ValueClass(QStringLiteral("eight1"));
    QCOMPARE(container.last(), *(container.constEnd() - 1));
    QCOMPARE(container.last(), ValueClass(QStringLiteral("eight1")));
    container.insert(9, QStringLiteral("nine"));
    QCOMPARE(container.last(), (container.constEnd() - 1).value());
    QCOMPARE(container.last(), ValueClass(QStringLiteral("nine")));
    QCOMPARE(container2.last(), ValueClass(QStringLiteral("eight")));
    QCOMPARE(container2.last(), *(container2.constEnd() - 1));
}

void tst_HugeMap::testLastKey()
{
    HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    QCOMPARE(container.lastKey(), (container.constEnd() - 1).key());
    QCOMPARE(container.lastKey(), KeyClass(8));
    container.insert(9, QStringLiteral("nine"));
    QCOMPARE(container.lastKey(), (container.constEnd() - 1).key());
    QCOMPARE(container.lastKey(), KeyClass(9));
}

void tst_HugeMap::testRemove()
{
    HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };

    QVERIFY(container.remove(KeyClass(0)));
    QCOMPARE(container.size(), 4);
    QCOMPARE(container.value(0, ValueClass()), ValueClass());
    QVERIFY(!container.remove(KeyClass(9)));
    QCOMPARE(container.size(), 4);
    const auto keyList = container.uniqueKeys();
    int counter = 4;
    for (auto& singleKey : keyList) {
        QVERIFY(container.remove(singleKey));
        QCOMPARE(container.size(), --counter);
    }
    QVERIFY(!container.remove(KeyClass(0)));
    QCOMPARE(container.size(), 0);
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
    QCOMPARE(container2.size(), 3);
    QVERIFY(container2.contains(0));
    QVERIFY(container2.contains(1));
    QVERIFY(container2.contains(2));

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

void tst_HugeMap::testFileSize()
{
    HugeMap<KeyClass, qint8> container;
    QCOMPARE(container.fileSize(), 0);
    container.insert(0, 'A');
    QCOMPARE(container.fileSize(), 0);
    container.insert(1, 'B');
    QCOMPARE(container.fileSize(), 1);
    container.insert(2, 'C');
    QCOMPARE(container.fileSize(), 2);
    container.insert(4, 'D');
    QCOMPARE(container.fileSize(), 3);
    container.insert(8, 'E');
    QCOMPARE(container.fileSize(), 4);
    const auto junk = container.value(0);
    Q_UNUSED(junk);
    QCOMPARE(container.fileSize(), 5);
    container.remove(8);
    QCOMPARE(container.fileSize(), 4);
    container.remove(2);
    QCOMPARE(container.fileSize(), 4);
    container.remove(4);
    QCOMPARE(container.fileSize(), 2);
    container.clear();
    QCOMPARE(container.fileSize(), 0);
}

void tst_HugeMap::testEquality()
{
    const HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    HugeMap<KeyClass, ValueClass> container2{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    auto container3 = container;
    QVERIFY(container == container2);
    QVERIFY(container == container3);
    container2.remove(2);
    QVERIFY(!(container == container2));
    QVERIFY(container == container3);
    container3.insert(0,QStringLiteral("zero1"));
    container2.insert(2, QStringLiteral("two"));
    QVERIFY(container == container2);
    QVERIFY(!(container == container3));
}

void tst_HugeMap::testUnEquality()
{
    const HugeMap<KeyClass, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    HugeMap<KeyClass, ValueClass> container2{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    auto container3 = container;
    QVERIFY(!(container != container2));
    QVERIFY(!(container != container3));
    container2.remove(2);
    QVERIFY(container != container2);
    QVERIFY(!(container != container3));
    container3.insert(0, QStringLiteral("zero1"));
    container2.insert(2, QStringLiteral("two"));
    QVERIFY(!(container != container2));
    QVERIFY(container != container3);
}

void tst_HugeMap::testMoveAssignment()
{
    HugeMap<KeyClass, ValueClass> container1{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    HugeMap<KeyClass, ValueClass> container2{
        std::make_pair(3, QStringLiteral("three"))
        , std::make_pair(5, QStringLiteral("five"))
        , std::make_pair(6, QStringLiteral("six"))
        , std::make_pair(7, QStringLiteral("seven"))
        , std::make_pair(9, QStringLiteral("nine"))
    };
    const auto container4 = container2;

    container1 = std::move(container2);

    QCOMPARE(container1.value(KeyClass(3), ValueClass()), ValueClass(QStringLiteral("three")));
    QCOMPARE(container1.value(KeyClass(2), ValueClass()), ValueClass());

    QCOMPARE(container4, container1);
}

void tst_HugeMap::testSerialisation()
{
    
    const HugeMap<int, ValueClass> container{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    HugeMap<int, ValueClass> container2;
    QFile serialFile("testSerial.dat");
    
    QVERIFY(serialFile.open(QFile::WriteOnly));
    QDataStream writeStream(&serialFile);
    writeStream << container;
    serialFile.close();
    QVERIFY(serialFile.open(QFile::ReadOnly));
    QDataStream readStream(&serialFile);
    readStream >> container2;
    serialFile.close();
    QVERIFY(container == container2);
    container2.clear();
    QVERIFY(container != container2);
    
    
    serialFile.remove();
}

void tst_HugeMap::testSerialisationOldVersion()
{
    using ValueMap = QMap<int, int>;

    const HugeMap<int, ValueMap> container{
        std::make_pair(0, ValueMap({  std::make_pair(0, 0) }))
        , std::make_pair(1, ValueMap({ std::make_pair(1, 1) }))
        , std::make_pair(2, ValueMap({ std::make_pair(2, 2) }))
        , std::make_pair(4, ValueMap({ std::make_pair(4, 4) }))
        , std::make_pair(8, ValueMap({ std::make_pair(8, 8) }))
    };
    HugeMap<int, ValueMap> container2;
    QFile serialFile("testSerial.dat");
    QVERIFY(serialFile.open(QFile::WriteOnly));
    QDataStream writeStream(&serialFile);
    writeStream.setVersion(QDataStream::Qt_4_0);
    writeStream << container;
    serialFile.close();
    QVERIFY(serialFile.open(QFile::ReadOnly));
    QDataStream readStream(&serialFile);
    readStream.setVersion(QDataStream::Qt_4_0);
    readStream >> container2;
    serialFile.close();
    QVERIFY(container == container2);
    serialFile.remove();
}


void tst_HugeMap::testIteratorDetatch()
{
    HugeMap<KeyClass, ValueClass> container1{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    auto container2 = container1;
    auto iter1 = container1.begin();
    auto iter2 = container2.begin();
    iter2.value() = ValueClass(QStringLiteral("zero1"));
    QCOMPARE(iter2.value(), ValueClass(QStringLiteral("zero1")));
    QCOMPARE(iter1.value(), ValueClass(QStringLiteral("zero")));

    HugeMap<KeyClass, ValueClass> container3{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    auto container4 = container3;
    auto iter3 = container3.begin();
    auto iter4 = container4.begin();
    iter3.value() = ValueClass(QStringLiteral("zero1"));
    QCOMPARE(iter3.value(), ValueClass(QStringLiteral("zero1")));
    QCOMPARE(iter4.value(), ValueClass(QStringLiteral("zero")));

    HugeMap<KeyClass, ValueClass> container5{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    auto container6 = container5;
    auto iter5 = container5.begin();
    auto iter6 = container6.begin();
    container6[KeyClass(0)] = ValueClass(QStringLiteral("zero1"));
    QCOMPARE(iter6.value(), ValueClass(QStringLiteral("zero1")));
    QCOMPARE(iter5.value(), ValueClass(QStringLiteral("zero")));


    HugeMap<KeyClass, ValueClass> container7{
        std::make_pair(0, QStringLiteral("zero"))
        , std::make_pair(1, QStringLiteral("one"))
        , std::make_pair(2, QStringLiteral("two"))
        , std::make_pair(4, QStringLiteral("four"))
        , std::make_pair(8, QStringLiteral("eight"))
    };
    auto container8 = container7;
    auto iter7 = container7.begin();
    auto iter8 = container8.begin();
    container7[KeyClass(0)] = ValueClass(QStringLiteral("zero1"));
    QCOMPARE(iter7.value(), ValueClass(QStringLiteral("zero1")));
    QCOMPARE(iter8.value(), ValueClass(QStringLiteral("zero")));

}
