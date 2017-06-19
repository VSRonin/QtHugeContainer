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
using namespace HugeContainer;

class tst_HugeMap : public QObject
{
    Q_OBJECT
protected:
    template <class KEY, class VALUE>
    void sanityCheckTree(const HugeMap<KEY, VALUE> &m, int calledFromLine);
    public slots:
    void init();
    private slots:
    void ctor();
    void count();
    void clear();
    void beginEnd();
    void firstLast();
    void key();
    void swap();
    void operator_eq();
    void empty();
    void contains();
    void find();
    void constFind();
    void lowerUpperBound();
    void mergeCompare();
    void take();
    void iterators();
    void keyIterator();
    void keyValueIterator();
    void keys_values_uniqueKeys();
    void qmultimap_specific();
    void const_shared_null();
    void equal_range();
    void setSharable();
    void insert();
    void checkMostLeftNode();
    void initializerList();
    void testInsertWithHint();
    void testInsertMultiWithHint();
    void eraseValidIteratorOnSharedMap();
};
struct IdentityTracker
{
    int value, id;
};
inline bool operator<(IdentityTracker lhs, IdentityTracker rhs) { return lhs.value < rhs.value; }
typedef HugeMap<QString, QString> StringMap;
class MyClass
{
public:
    MyClass()
    {
        ++count;
    }
    MyClass(const QString& c)
    {
        count++; str = c;
    }
    ~MyClass()
    {
        count--;
    }
    MyClass(const MyClass& c)
    {
        count++; str = c.str;
    }
    MyClass &operator =(const MyClass &o)
    {
        str = o.str; return *this;
    }
    QString str;
    static int count;
};
int MyClass::count = 0;
typedef HugeMap<QString, MyClass> MyMap;
QDebug operator << (QDebug d, const MyClass &c)
{
    d << c.str;
    return d;
}
template <class KEY, class VALUE>
void tst_HugeMap::sanityCheckTree(const HugeMap<KEY, VALUE> &m, int calledFromLine)
{
    QString possibleFrom;
    possibleFrom.setNum(calledFromLine);
    possibleFrom = "Called from line: " + possibleFrom;
    int count = 0;
    typename HugeMap<KEY, VALUE>::const_iterator oldite = m.constBegin();
    for (typename HugeMap<KEY, VALUE>::const_iterator i = m.constBegin(); i != m.constEnd(); ++i) {
        count++;
        bool oldIteratorIsLarger = i.key() < oldite.key();
        QVERIFY2(!oldIteratorIsLarger, possibleFrom.toUtf8());
        oldite = i;
    }
    if (m.size() != count) { // Fail
        qDebug() << possibleFrom;
        QCOMPARE(m.size(), count);
    }
    if (m.size() == 0)
        QVERIFY(m.constBegin() == m.constEnd());
}
void tst_HugeMap::init()
{
    MyClass::count = 0;
}
void tst_HugeMap::ctor()
{
    std::map<int, int> map;
    for (int i = 0; i < 100000; ++i)
        map.insert(std::pair<int, int>(i * 3, i * 7));
    HugeMap<int, int> qmap(map); // ctor.
    // Check that we have the same
    std::map<int, int>::iterator j = map.begin();
    HugeMap<int, int>::const_iterator i = qmap.constBegin();
    while (i != qmap.constEnd()) {
        QCOMPARE((*j).first, i.key());
        QCOMPARE((*j).second, i.value());
        ++i;
        ++j;
    }
    QCOMPARE((int)map.size(), qmap.size());
}
void tst_HugeMap::count()
{
    {
        MyMap map;
        MyMap map2(map);
        QCOMPARE(map.count(), 0);
        QCOMPARE(map2.count(), 0);
        QCOMPARE(MyClass::count, int(0));
        // detach
        map2["Hallo"] = MyClass("Fritz");
        QCOMPARE(map.count(), 0);
        QCOMPARE(map2.count(), 1);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 1);
#endif
    }
    QCOMPARE(MyClass::count, int(0));
    {
        typedef HugeMap<QString, MyClass> Map;
        Map map;
        QCOMPARE(map.count(), 0);
        map.insert("Torben", MyClass("Weis"));
        QCOMPARE(map.count(), 1);
        map.insert("Claudia", MyClass("Sorg"));
        QCOMPARE(map.count(), 2);
        map.insert("Lars", MyClass("Linzbach"));
        map.insert("Matthias", MyClass("Ettrich"));
        map.insert("Sue", MyClass("Paludo"));
        map.insert("Eirik", MyClass("Eng"));
        map.insert("Haavard", MyClass("Nord"));
        map.insert("Arnt", MyClass("Gulbrandsen"));
        map.insert("Paul", MyClass("Tvete"));
        QCOMPARE(map.count(), 9);
        map.insert("Paul", MyClass("Tvete 1"));
        map.insert("Paul", MyClass("Tvete 2"));
        map.insert("Paul", MyClass("Tvete 3"));
        map.insert("Paul", MyClass("Tvete 4"));
        map.insert("Paul", MyClass("Tvete 5"));
        map.insert("Paul", MyClass("Tvete 6"));
        QCOMPARE(map.count(), 9);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 9);
#endif
        Map map2(map);
        QVERIFY(map2.count() == 9);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 9);
#endif
        map2.insert("Kay", MyClass("Roemer"));
        QVERIFY(map2.count() == 10);
        QVERIFY(map.count() == 9);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 19);
#endif
        map2 = map;
        QVERIFY(map.count() == 9);
        QVERIFY(map2.count() == 9);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 9);
#endif
        map2.insert("Kay", MyClass("Roemer"));
        QVERIFY(map2.count() == 10);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 19);
#endif
        map2.clear();
        QVERIFY(map.count() == 9);
        QVERIFY(map2.count() == 0);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 9);
#endif
        map2 = map;
        QVERIFY(map.count() == 9);
        QVERIFY(map2.count() == 9);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 9);
#endif
        map2.clear();
        QVERIFY(map.count() == 9);
        QVERIFY(map2.count() == 0);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 9);
#endif
        map.remove("Lars");
        QVERIFY(map.count() == 8);
        QVERIFY(map2.count() == 0);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 8);
#endif
        map.remove("Mist");
        QVERIFY(map.count() == 8);
        QVERIFY(map2.count() == 0);
#ifndef Q_CC_SUN
        QCOMPARE(MyClass::count, 8);
#endif
    }
    QVERIFY(MyClass::count == 0);
    {
        typedef HugeMap<QString, MyClass> Map;
        Map map;
        map["Torben"] = MyClass("Weis");
#ifndef Q_CC_SUN
        QVERIFY(MyClass::count == 1);
#endif
        QVERIFY(map.count() == 1);
        (void)map["Torben"].str;
        (void)map["Lars"].str;
#ifndef Q_CC_SUN
        QVERIFY(MyClass::count == 2);
#endif
        QVERIFY(map.count() == 2);
        const Map& cmap = map;
        (void)cmap["Depp"].str;
#ifndef Q_CC_SUN
        QVERIFY(MyClass::count == 2);
#endif
        QVERIFY(map.count() == 2);
        QVERIFY(cmap.count() == 2);
    }
    QCOMPARE(MyClass::count, 0);
    {
        for (int i = 0; i < 100; ++i) {
            HugeMap<int, MyClass> map;
            for (int j = 0; j < i; ++j)
                map.insert(j, MyClass(QString::number(j)));
        }
        QCOMPARE(MyClass::count, 0);
    }
    QCOMPARE(MyClass::count, 0);
}
void tst_HugeMap::clear()
{
    {
        MyMap map;
        map.clear();
        QVERIFY(map.isEmpty());
        map.insert("key", MyClass("value"));
        map.clear();
        QVERIFY(map.isEmpty());
        map.insert("key0", MyClass("value0"));
        map.insert("key0", MyClass("value1"));
        map.insert("key1", MyClass("value2"));
        map.clear();
        sanityCheckTree(map, __LINE__);
        QVERIFY(map.isEmpty());
    }
    QCOMPARE(MyClass::count, int(0));
}
void tst_HugeMap::beginEnd()
{
    StringMap m0;
    QVERIFY(m0.begin() == m0.end());
    QVERIFY(m0.begin() == m0.begin());
    // sample string->string map
    StringMap map;
    QVERIFY(map.constBegin() == map.constEnd());
    map.insert("0", "a");
    map.insert("1", "b");
    QVERIFY(map.constBegin() == map.cbegin());
    QVERIFY(map.constEnd() == map.cend());
    // make a copy. const function shouldn't detach
    StringMap map2 = map;
    QVERIFY(map.constBegin() == map2.constBegin());
    QVERIFY(map.constEnd() == map2.constEnd());
    // test iteration
    QString result;
    for (StringMap::ConstIterator it = map.constBegin();
        it != map.constEnd(); ++it)
        result += *it;
    QCOMPARE(result, QString("ab"));
    // maps should still be identical
    QVERIFY(map.constBegin() == map2.constBegin());
    QVERIFY(map.constEnd() == map2.constEnd());
    // detach
    map2.insert("2", "c");
    QVERIFY(map.constBegin() == map.constBegin());
    QVERIFY(map.constBegin() != map2.constBegin());
}
void tst_HugeMap::firstLast()
{
    // sample string->string map
    StringMap map;
    map.insert("0", "a");
    map.insert("1", "b");
    map.insert("5", "e");
    QCOMPARE(map.firstKey(), QStringLiteral("0"));
    QCOMPARE(map.lastKey(), QStringLiteral("5"));
    QCOMPARE(map.first(), QStringLiteral("a"));
    QCOMPARE(map.last(), QStringLiteral("e"));
    // const map
    const StringMap const_map = map;
    QCOMPARE(map.firstKey(), const_map.firstKey());
    QCOMPARE(map.lastKey(), const_map.lastKey());
    QCOMPARE(map.first(), const_map.first());
    QCOMPARE(map.last(), const_map.last());
    map.take(map.firstKey());
    QCOMPARE(map.firstKey(), QStringLiteral("1"));
    QCOMPARE(map.lastKey(), QStringLiteral("5"));
    map.take(map.lastKey());
    QCOMPARE(map.lastKey(), map.lastKey());
}

void tst_HugeMap::swap()
{
    HugeMap<int, QString> m1, m2;
    m1[0] = "m1[0]";
    m2[1] = "m2[1]";
    m1.swap(m2);
    QCOMPARE(m1.value(1), QLatin1String("m2[1]"));
    QCOMPARE(m2.value(0), QLatin1String("m1[0]"));
    sanityCheckTree(m1, __LINE__);
    sanityCheckTree(m2, __LINE__);
}

void tst_HugeMap::empty()
{
    HugeMap<int, QString> map1;
    QVERIFY(map1.isEmpty());
    map1.insert(1, "one");
    QVERIFY(!map1.isEmpty());
    map1.clear();
    QVERIFY(map1.isEmpty());
}
void tst_HugeMap::contains()
{
    HugeMap<int, QString> map1;
    int i;
    map1.insert(1, "one");
    QVERIFY(map1.contains(1));
    for (i = 2; i < 100; ++i)
        map1.insert(i, "teststring");
    for (i = 99; i > 1; --i)
        QVERIFY(map1.contains(i));
    map1.remove(43);
    QVERIFY(!map1.contains(43));
}
void tst_HugeMap::find()
{
    HugeMap<int, QString> map1;
    QString testString = "Teststring %0";
    QString compareString;
    int i, count = 0;
    QVERIFY(map1.find(1) == map1.end());
    map1.insert(1, "Mensch");
    map1.insert(1, "Mayer");
    map1.insert(2, "Hej");
    QCOMPARE(map1.find(1).value(), QLatin1String("Mayer"));
    QCOMPARE(map1.find(2).value(), QLatin1String("Hej"));
    HugeMap<int, QString>::const_iterator it = map1.constFind(4);
    for (i = 9; i > 2 && it != map1.constEnd() && it.key() == 4; --i) {
        compareString = testString.arg(i);
        QVERIFY(it.value() == compareString);
        ++it;
        ++count;
    }
    QCOMPARE(count, 7);
}
void tst_HugeMap::constFind()
{
    HugeMap<int, QString> map1;
    QString testString = "Teststring %0";
    QString compareString;
    int i, count = 0;
    QVERIFY(map1.constFind(1) == map1.constEnd());
    map1.insert(1, "Mensch");
    map1.insert(1, "Mayer");
    map1.insert(2, "Hej");
    QVERIFY(map1.constFind(4) == map1.constEnd());
    QCOMPARE(map1.constFind(1).value(), QLatin1String("Mayer"));
    QCOMPARE(map1.constFind(2).value(), QLatin1String("Hej"));
    HugeMap<int, QString>::const_iterator it = map1.constFind(4);
    for (i = 9; i > 2 && it != map1.constEnd() && it.key() == 4; --i) {
        compareString = testString.arg(i);
        QVERIFY(it.value() == compareString);
        ++it;
        ++count;
    }
    QCOMPARE(count, 7);
}

void tst_HugeMap::mergeCompare()
{
    HugeMap<int, QString> map1, map2, map3, map1b, map2b;
    map1.insert(1, "ett");
    map1.insert(3, "tre");
    map1.insert(5, "fem");
    map2.insert(2, "tvo");
    map2.insert(4, "fyra");
    map1.unite(map2);
    sanityCheckTree(map1, __LINE__);
    map1b = map1;
    map2b = map2;
    map2b.insert(0, "nul");
    map1b.unite(map2b);
    sanityCheckTree(map1b, __LINE__);
    QCOMPARE(map1.value(1), QLatin1String("ett"));
    QCOMPARE(map1.value(2), QLatin1String("tvo"));
    QCOMPARE(map1.value(3), QLatin1String("tre"));
    QCOMPARE(map1.value(4), QLatin1String("fyra"));
    QCOMPARE(map1.value(5), QLatin1String("fem"));
    map3.insert(1, "ett");
    map3.insert(2, "tvo");
    map3.insert(3, "tre");
    map3.insert(4, "fyra");
    map3.insert(5, "fem");
    QVERIFY(map1 == map3);
}
void tst_HugeMap::take()
{
    HugeMap<int, QString> map;
    map.insert(2, "zwei");
    map.insert(3, "drei");
    QCOMPARE(map.take(3), QLatin1String("drei"));
    QVERIFY(!map.contains(3));
}
void tst_HugeMap::iterators()
{
    HugeMap<int, QString> map;
    QString testString = "Teststring %1";
    int i;
    for (i = 1; i < 100; ++i)
        map.insert(i, testString.arg(i));
    //STL-Style iterators
    HugeMap<int, QString>::iterator stlIt = map.begin();
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 1"));
    stlIt += 5;
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 6"));
    stlIt++;
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 7"));
    stlIt -= 3;
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 4"));
    stlIt--;
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 3"));
    for (stlIt = map.begin(), i = 1; stlIt != map.end(); ++stlIt, ++i)
        QVERIFY(stlIt.value() == testString.arg(i));
    QCOMPARE(i, 100);
    //STL-Style const-iterators
    HugeMap<int, QString>::const_iterator cstlIt = map.constBegin();
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 1"));
    cstlIt += 5;
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 6"));
    cstlIt++;
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 7"));
    cstlIt -= 3;
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 4"));
    cstlIt--;
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 3"));
    for (cstlIt = map.constBegin(), i = 1; cstlIt != map.constEnd(); ++cstlIt, ++i)
        QVERIFY(cstlIt.value() == testString.arg(i));
    QCOMPARE(i, 100);
    //Java-Style iterators
    HugeMapIterator<int, QString> javaIt(map);
    i = 0;
    while (javaIt.hasNext()) {
        ++i;
        javaIt.next();
        QVERIFY(javaIt.value() == testString.arg(i));
    }
    ++i;
    while (javaIt.hasPrevious()) {
        --i;
        javaIt.previous();
        QVERIFY(javaIt.value() == testString.arg(i));
    }
    i = 51;
    while (javaIt.hasPrevious()) {
        --i;
        javaIt.previous();
        QVERIFY(javaIt.value() == testString.arg(i));
    }
}
void tst_HugeMap::keyIterator()
{
    HugeMap<int, int> map;
    for (int i = 0; i < 100; ++i)
        map.insert(i, i * 100);
    HugeMap<int, int>::key_iterator key_it = map.keyBegin();
    HugeMap<int, int>::const_iterator it = map.cbegin();
    for (int i = 0; i < 100; ++i) {
        QCOMPARE(*key_it, it.key());
        ++key_it;
        ++it;
    }
    key_it = std::find(map.keyBegin(), map.keyEnd(), 50);
    it = std::find(map.cbegin(), map.cend(), 50 * 100);
    QVERIFY(key_it != map.keyEnd());
    QCOMPARE(*key_it, it.key());
    QCOMPARE(*(key_it++), (it++).key());
    QCOMPARE(*(key_it--), (it--).key());
    QCOMPARE(*(++key_it), (++it).key());
    QCOMPARE(*(--key_it), (--it).key());
    QCOMPARE(std::count(map.keyBegin(), map.keyEnd(), 99), 1);
    // DefaultConstructible test
    typedef HugeMap<int, int>::key_iterator keyIterator;
    Q_STATIC_ASSERT(std::is_default_constructible<keyIterator>::value);
}
void tst_HugeMap::keyValueIterator()
{
    HugeMap<int, int> map;
    typedef HugeMap<int, int>::const_key_value_iterator::value_type entry_type;
    for (int i = 0; i < 100; ++i)
        map.insert(i, i * 100);
    auto key_value_it = map.constKeyValueBegin();
    auto it = map.cbegin();
    for (int i = 0; i < map.size(); ++i) {
        QVERIFY(key_value_it != map.constKeyValueEnd());
        QVERIFY(it != map.cend());
        entry_type pair(it.key(), it.value());
        QCOMPARE(*key_value_it, pair);
        ++key_value_it;
        ++it;
    }
    QVERIFY(key_value_it == map.constKeyValueEnd());
    QVERIFY(it == map.cend());
    int key = 50;
    int value = 50 * 100;
    entry_type pair(key, value);
    key_value_it = std::find(map.constKeyValueBegin(), map.constKeyValueEnd(), pair);
    it = std::find(map.cbegin(), map.cend(), value);
    QVERIFY(key_value_it != map.constKeyValueEnd());
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));
    ++it;
    ++key_value_it;
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));
    --it;
    --key_value_it;
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));
    ++it;
    ++key_value_it;
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));
    --it;
    --key_value_it;
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));
    key = 99;
    value = 99 * 100;
    QCOMPARE(std::count(map.constKeyValueBegin(), map.constKeyValueEnd(), entry_type(key, value)), 1);
}
void tst_HugeMap::keys_values_uniqueKeys()
{
    HugeMap<QString, int> map;
    QVERIFY(map.uniqueKeys().isEmpty());
    QVERIFY(map.keys().isEmpty());

    map.setValue("alpha", 1);
    QVERIFY(map.keys() == (QList<QString>() << "alpha"));
    QVERIFY(map.uniqueKeys() == map.keys());

    map.setValue("beta", -2);
    QVERIFY(map.keys() == (QList<QString>() << "alpha" << "beta"));
    QVERIFY(map.keys() == map.uniqueKeys());
    map.setValue("alpha", 2);
    QVERIFY(map.uniqueKeys() == (QList<QString>() << "alpha" << "beta"));
    QVERIFY(map.keys() == map.uniqueKeys());
}


void tst_HugeMap::equal_range()
{
    HugeMap<int, QString> map;
    const HugeMap<int, QString> &cmap = map;
    QPair<HugeMap<int, QString>::iterator, HugeMap<int, QString>::iterator> result = map.equal_range(0);
    QCOMPARE(result.first, map.end());
    QCOMPARE(result.second, map.end());
    QPair<HugeMap<int, QString>::const_iterator, HugeMap<int, QString>::const_iterator> cresult = cmap.equal_range(0);
    QCOMPARE(cresult.first, cmap.cend());
    QCOMPARE(cresult.second, cmap.cend());
    map.insert(1, "one");
    result = map.equal_range(0);
    QCOMPARE(result.first, map.find(1));
    QCOMPARE(result.second, map.find(1));
    result = map.equal_range(1);
    QCOMPARE(result.first, map.find(1));
    QCOMPARE(result.second, map.end());
    result = map.equal_range(2);
    QCOMPARE(result.first, map.end());
    QCOMPARE(result.second, map.end());
    cresult = cmap.equal_range(0);
    QCOMPARE(cresult.first, cmap.find(1));
    QCOMPARE(cresult.second, cmap.find(1));
    cresult = cmap.equal_range(1);
    QCOMPARE(cresult.first, cmap.find(1));
    QCOMPARE(cresult.second, cmap.cend());
    cresult = cmap.equal_range(2);
    QCOMPARE(cresult.first, cmap.cend());
    QCOMPARE(cresult.second, cmap.cend());
    for (int i = -10; i < 10; i += 2)
        map.insert(i, QString::number(i));
    result = map.equal_range(0);
    QCOMPARE(result.first, map.find(0));
    QCOMPARE(result.second, map.find(1));
    result = map.equal_range(1);
    QCOMPARE(result.first, map.find(1));
    QCOMPARE(result.second, map.find(2));
    result = map.equal_range(2);
    QCOMPARE(result.first, map.find(2));
    QCOMPARE(result.second, map.find(4));
    cresult = cmap.equal_range(0);
    QCOMPARE(cresult.first, cmap.find(0));
    QCOMPARE(cresult.second, cmap.find(1));
    cresult = cmap.equal_range(1);
    QCOMPARE(cresult.first, cmap.find(1));
    QCOMPARE(cresult.second, cmap.find(2));
    cresult = cmap.equal_range(2);
    QCOMPARE(cresult.first, cmap.find(2));
    QCOMPARE(cresult.second, cmap.find(4));
    map.insertMulti(1, "another one");
    result = map.equal_range(1);
    QCOMPARE(result.first, map.find(1));
    QCOMPARE(result.second, map.find(2));
    cresult = cmap.equal_range(1);
    QCOMPARE(cresult.first, cmap.find(1));
    QCOMPARE(cresult.second, cmap.find(2));
    QCOMPARE(map.count(1), 2);
}
template <class T>
const T &const_(const T &t)
{
    return t;
}
void tst_HugeMap::insert()
{
    HugeMap<QString, float> map;
    map.insert("cs/key1", 1);
    map.insert("cs/key2", 2);
    map.insert("cs/key1", 3);
    QCOMPARE(map.count(), 2);
    HugeMap<int, int> intMap;
    for (int i = 0; i < 1000; ++i) {
        intMap.insert(i, i);
    }
    QCOMPARE(intMap.size(), 1000);
    for (int i = 0; i < 1000; ++i) {
        QCOMPARE(intMap.value(i), i);
        intMap.insert(i, -1);
        QCOMPARE(intMap.size(), 1000);
        QCOMPARE(intMap.value(i), -1);
    }
    {
        HugeMap<IdentityTracker, int> map;
        QCOMPARE(map.size(), 0);
        const int dummy = -1;
        IdentityTracker id00 = { 0, 0 }, id01 = { 0, 1 }, searchKey = { 0, dummy };
        QCOMPARE(map.insert(id00, id00.id).key().id, id00.id);
        QCOMPARE(map.size(), 1);
        QCOMPARE(map.insert(id01, id01.id).key().id, id00.id); // first key inserted is kept
        QCOMPARE(map.size(), 1);
        QCOMPARE(map.find(searchKey).value(), id01.id);  // last-inserted value
        QCOMPARE(map.find(searchKey).key().id, id00.id); // but first-inserted key
    }
    {
        QMultiMap<IdentityTracker, int> map;
        QCOMPARE(map.size(), 0);
        const int dummy = -1;
        IdentityTracker id00 = { 0, 0 }, id01 = { 0, 1 }, searchKey = { 0, dummy };
        QCOMPARE(map.insert(id00, id00.id).key().id, id00.id);
        QCOMPARE(map.size(), 1);
        QCOMPARE(map.insert(id01, id01.id).key().id, id01.id);
        QCOMPARE(map.size(), 2);
        QMultiMap<IdentityTracker, int>::const_iterator pos = map.constFind(searchKey);
        QCOMPARE(pos.value(), pos.key().id); // key fits to value it was inserted with
        ++pos;
        QCOMPARE(pos.value(), pos.key().id); // key fits to value it was inserted with
    }
}
void tst_HugeMap::checkMostLeftNode()
{
    HugeMap<int, int> map;
    map.insert(100, 1);
    sanityCheckTree(map, __LINE__);
    // insert
    map.insert(99, 1);
    sanityCheckTree(map, __LINE__);
    map.insert(98, 1);
    sanityCheckTree(map, __LINE__);
    map.insert(97, 1);
    sanityCheckTree(map, __LINE__);
    map.insert(96, 1);
    sanityCheckTree(map, __LINE__);
    map.insert(95, 1);
    // remove
    sanityCheckTree(map, __LINE__);
    map.take(95);
    sanityCheckTree(map, __LINE__);
    map.remove(96);
    sanityCheckTree(map, __LINE__);
    map.erase(map.begin());
    sanityCheckTree(map, __LINE__);
    map.remove(97);
    sanityCheckTree(map, __LINE__);
    map.remove(98);
    sanityCheckTree(map, __LINE__);
    map.remove(99);
    sanityCheckTree(map, __LINE__);
    map.remove(100);
    sanityCheckTree(map, __LINE__);
    map.insert(200, 1);
    QCOMPARE(map.constBegin().key(), 200);
    sanityCheckTree(map, __LINE__);
    // remove the non left most node
    map.insert(202, 2);
    map.insert(203, 3);
    map.insert(204, 4);
    map.remove(202);
    sanityCheckTree(map, __LINE__);
    map.remove(203);
    sanityCheckTree(map, __LINE__);
    map.remove(204);
    sanityCheckTree(map, __LINE__);
    // erase last item
    map.erase(map.begin());
    sanityCheckTree(map, __LINE__);
}
void tst_HugeMap::initializerList()
{
    HugeMap<int, QString> map = { { 1, "bar" }, { 1, "hello" }, { 2, "initializer_list" } };
    QCOMPARE(map.count(), 2);
    QCOMPARE(map[1], QString("hello"));
    QCOMPARE(map[2], QString("initializer_list"));
    // note the difference to std::map:
    // std::map<int, QString> stdm = {{1, "bar"}, {1, "hello"}, {2, "initializer_list"}};
    // QCOMPARE(stdm.size(), 2UL);
    // QCOMPARE(stdm[1], QString("bar"));
    QMultiMap<QString, int> multiMap{ { "il", 1 }, { "il", 2 }, { "il", 3 } };
    QCOMPARE(multiMap.count(), 3);
    QList<int> values = multiMap.values("il");
    QCOMPARE(values.count(), 3);
    HugeMap<int, int> emptyMap{};
    QVERIFY(emptyMap.isEmpty());
    HugeMap<char, char> emptyPairs{ {}, {} };
    QVERIFY(!emptyPairs.isEmpty());
    QMultiMap<double, double> emptyMultiMap{};
    QVERIFY(emptyMultiMap.isEmpty());
    QMultiMap<float, float> emptyPairs2{ {}, {} };
    QVERIFY(!emptyPairs2.isEmpty());
}
void tst_HugeMap::eraseValidIteratorOnSharedMap()
{
    HugeMap<int, int> a, b;
    a.insert(10, 10);
    a.insert(30, 40);
    a.insert(40, 25);
    a.insert(50, 30);
    a.insert(20, 20);
    HugeMap<int, int>::iterator i = a.begin();
    while (i.value() != 25)
        ++i;
    b = a;
    a.erase(i);
    QCOMPARE(b.size(), 5);
    QCOMPARE(a.size(), 4);
    for (i = a.begin(); i != a.end(); ++i)
        QVERIFY(i.value() != 25);
    int itemsWith10 = 0;
    for (i = b.begin(); i != b.end(); ++i)
        itemsWith10 += (i.key() == 10);
    QCOMPARE(itemsWith10, 4);
    // Border cases
    HugeMap <QString, QString> ms1, ms2, ms3;
    ms1.insert("foo", "bar");
    ms1.insert("foo1", "quux");
    ms1.insert("foo2", "bar");
    HugeMap <QString, QString>::iterator si = ms1.begin();
    ms2 = ms1;
    ms1.erase(si);
    si = ms1.begin();
    QCOMPARE(si.value(), QString("quux"));
    ++si;
    QCOMPARE(si.value(), QString("bar"));
    si = ms2.begin();
    ++si;
    ++si;
    ms3 = ms2;
    ms2.erase(si);
    si = ms2.begin();
    QCOMPARE(si.value(), QString("bar"));
    ++si;
    QCOMPARE(si.value(), QString("quux"));
    QCOMPARE(ms1.size(), 2);
    QCOMPARE(ms2.size(), 2);
    QCOMPARE(ms3.size(), 3);
}
QTEST_APPLESS_MAIN(tst_HugeMap)
#include "tst_hugemap.moc"