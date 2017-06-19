/************************************************************************\
Copyright 2017 Luca Beldi

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the 
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  
\************************************************************************/

#ifndef hugecontainer_h__
#define hugecontainer_h__
#include <QHash>
#include <QMap>
#include <QTemporaryFile>
#include <QDataStream>
#include <memory>
#include <QCache>
#include <QSharedDataPointer>
#include <QSharedData>

//! Private do not use
template <class KeyType, class ValueType, bool sorted = false>
class HugeContainerData : public QSharedData{
    using ItemMapType = typename std::conditional<sorted, QMap<KeyType, qint64>, QHash<KeyType, qint64> >::type;
public:
    ItemMapType m_itemsMap;
    QMap<qint64, bool> m_memoryMap;
    std::unique_ptr<QTemporaryFile> m_device;
    std::unique_ptr<QCache<KeyType, ValueType> > m_cache;
    HugeContainerData()
        :m_device(std::make_unique<QTemporaryFile>())
        , m_cache(std::make_unique<QCache<KeyType, ValueType>>(0))
    {   
    }
    ~HugeContainerData() = default;
    HugeContainerData(HugeContainerData& other)
        :m_device(std::make_unique<QTemporaryFile>())
        , m_cache(std::make_unique<QCache<KeyType, ValueType>>(other.m_cache->maxCost()))
        , m_memoryMap(other.m_memoryMap)
        , m_itemsMap(other.m_itemsMap)
    {
        other.m_device->seek(0);
        qint64 totalSize = other.m_device->size();
        for (; totalSize > 1024; totalSize-=1024) 
            m_device->write(other.m_device->read(1024));
        m_device->write(other.m_device->read(totalSize));
    }
};

template <class KeyType, class ValueType, bool sorted = false>
class HugeContainer{
    static_assert(std::is_default_constructible<ValueType>::value, "ValueType must provide a default constructor");
    static_assert(std::is_copy_constructible<ValueType>::value, "ValueType must provide a copy constructor");
private:
    using NormalContaineType = typename std::conditional<sorted, QMap<KeyType, ValueType>, QHash<KeyType, ValueType> >::type;
    QSharedDataPointer<HugeContainerData<KeyType, ValueType, sorted> > m_d;
public:
    class iterator
    {
        friend class HugeContainer;
        const HugeContainer* m_container;
        using baseIterType = decltype(m_container->m_itemsMap.begin());
        baseIterType m_baseIter;
        iterator(const HugeContainer* const  cont, const baseIterType& baseItr)
            :m_container(cont)
            , m_baseIter(baseItr)
        {}
        class proxy_holder
        {
            const ValueType m_t;
        public:
            proxy_holder(const ValueType& t) : m_t(t) {}
            const ValueType* operator->() const { return &m_t; }
        };
    public:
        iterator();
        iterator(const iterator& other) = default;
        iterator& operator=(const iterator& other) = default;
        const KeyType& key() const { return m_baseIter.key(); }
        ValueType value() const { return std::get<1>(m_container->value(m_baseIter.key())); }
        bool operator!=(const iterator &other) const { return !operator==(other); }
        bool operator==(const iterator &other) const { return m_container == other.m_container &&  m_baseIter == other.m_baseIter; }
        ValueType operator*() const { return value(); }
        iterator operator+(int j) const { return iterator(m_container, m_baseIter + j); }
        iterator &operator++() { ++m_baseIter; return *this; }
        iterator operator++(int) { iterator result(*this); ++m_baseIter; return result; }
        iterator &operator+=(int j) { m_baseIter += j; return *this; }
        iterator operator-(int j) const { return iterator(m_container, m_baseIter - j); }
        iterator &operator--() { --m_baseIter; return *this; }
        iterator operator--(int) { iterator result(*this); --m_baseIter; return result; }
        iterator &operator-=(int j) { m_baseIter -= j; return *this; }
        proxy_holder operator->() const { return proxy_holder(value()); }
    };
    using const_iterator = iterator;
    HugeContainer(std::initializer_list<std::pair<KeyType, ValueType> > list)
        :HugeContainer(NormalContaineType(list)){}
    HugeContainer(const NormalContaineType& list)
        :HugeContainer()
    {
        for (auto i = list.constBegin(); i != list.constEnd(); ++i) {
            if (i != list.constBegin()){
                if (i.key() == (i - 1).key())
                    continue;
            }
            setValue(i.key(), i.value());
        }
    }
    HugeContainer()
        :m_d(new HugeContainerData<KeyType, ValueType, sorted>)
    {
        if (!m_d->m_device->open())
            Q_ASSERT_X(false, "HugeContainer::HugeContainer", "Unable to create a temporary file");
        m_d->m_memoryMap.insert(0, true);
    }
    HugeContainer(const HugeContainer& other)
        :m_d(other.m_d)
    {}
    virtual ~HugeContainer() = default;
    bool setValue(const KeyType& key, const ValueType& val)
    {
        QByteArray block;
        {
            QDataStream writerStream(&block, QIODevice::WriteOnly);
            writerStream << val;
        }
        const bool result = writeBlock(key, block);
        if (result && maxCacheSize() > 0)
            m_d->m_cache->insert(key, new ValueType(val));
        return result;
    }
    std::tuple<bool, ValueType> value(const KeyType& key, const ValueType& defaultVal = ValueType()) const
    {
        ValueType* const foundItem = m_d->m_cache->object(key);
        if (foundItem)
            return std::make_tuple(true, *foundItem);
        QByteArray block = readBlock(key);
        if (block.isEmpty())
            return std::make_tuple(false, defaultVal);
        ValueType result;
        QDataStream readerStream(block);
        readerStream >> result;
        if (maxCacheSize() > 0)
            m_d->m_cache->insert(key, new ValueType(result));
        return std::make_tuple(true, result);
    }
    ValueType value(const KeyType& key, bool* ok, const ValueType& defaultVal = ValueType()) const
    {
        const std::tuple<bool, ValueType> tupleResult = value(key);
        if (ok)
            *ok = std::get<0>(tupleResult);
        return std::get<1>(tupleResult);
    }
    void clear()
    {
        if (m_d->m_device->remove()) {
            if (!m_d->m_device->open())
                Q_ASSERT_X(false, "HugeContainer::HugeContainer", "Unable to create a temporary file");
        }        
        m_d->m_itemsMap.clear();
        m_d->m_memoryMap.clear();
        m_d->m_memoryMap.insert(0, true);
        m_d->m_cache->clear();
    }
    bool removeValue(const KeyType& key)
    {
        auto itemIter = m_d->m_itemsMap.find(key);
        if (itemIter == m_d->m_itemsMap.end())
            return false;
        m_d->m_cache->remove(key);
        auto fileIter = m_d->m_memoryMap.find(itemIter.value());
        Q_ASSERT(fileIter != m_d->m_memoryMap.end());
        if (fileIter.value())
            return false;
        m_d->m_itemsMap.erase(itemIter);
        if (fileIter != m_d->m_memoryMap.begin()) {
            if ((fileIter - 1).value()) {
                fileIter = m_d->m_memoryMap.erase(fileIter);
                if (fileIter.value())
                    m_d->m_memoryMap.erase(fileIter);
                return true;
            }
        }
        if ((fileIter + 1) != m_d->m_memoryMap.end()) {
            if ((fileIter + 1).value())
                m_d->m_memoryMap.erase(fileIter + 1);
        }
        fileIter.value() = true;
        return true;
    }
    QList<KeyType> keys() const
    {
        return m_d->m_itemsMap.keys();
    }
    QList<KeyType> uniqueKeys() const
    {
        return m_d->m_itemsMap.uniqueKeys();
    }
    QList<ValueType> values() const{
        QList<ValueType> result;
        bool check=true;
        for (auto i = m_d->m_itemsMap.constBegin(); check && i != m_d->m_itemsMap.constEnd(); ++i)
            result.append(value(i.key(), &check));
        if (!check)
            return QList<ValueType>();
        return result;
    }
    bool contains(const KeyType& key) const
    {
        return m_d->m_itemsMap.contains(key);
    }
    int count() const
    {
        return size();
    }
    int size() const
    {
        return m_d->m_itemsMap.size();
    }
    qint64 memorySize() const{
        return m_d->m_device->size();
    }
    bool isEmpty() const{
        return m_d->m_itemsMap.isEmpty();
    }
    iterator erase(iterator pos){
        iterator endIter = end();
        if (pos == endIter)
            return endIter;
        iterator result = pos+1;
        if (removeValue(result.key()))
            return result;
        return pos;
    }
    iterator begin(){
        return iterator(this, m_d->m_itemsMap.begin());
    }
    iterator end()
    {
        return iterator(this, m_d->m_itemsMap.end());
    }
    iterator find(const KeyType& key){
        auto baseFind = m_d->m_itemsMap.find(key);
        if (baseFind == m_d->m_itemsMap.end())
            return end();
        return iterator(this, baseFind);
    }
    ValueType take(const KeyType& key){
        ValueType result = std::get<1>(value(key));
        removeValue(key);
        return result;
    }
    int maxCacheSize() const { return m_d->m_cache->maxCost(); }
    void setMaxCacheSize(int val) { if (val >= 0) m_d->m_cache->setMaxCost(val); }
protected:
    bool writeBlock(qint32 key, const QByteArray& block)
    {
        if (!m_d->m_device->isWritable())
            return false;
        m_d->m_device->setTextModeEnabled(false);
        auto itemIter = m_d->m_itemsMap.find(key);
        if (itemIter != m_d->m_itemsMap.end()) {
            removeValue(key);
        }
        m_d->m_itemsMap.insert(key, writeInMap(block));
        return true;
    }
    QByteArray readBlock(qint32 key) const
    {
        if (!m_d->m_device->isReadable())
            return QByteArray();
        m_d->m_device->setTextModeEnabled(false);
        auto itemIter = m_d->m_itemsMap.constFind(key);
        if (itemIter == m_d->m_itemsMap.constEnd())
            return QByteArray();
        auto fileIter = m_d->m_memoryMap.constFind(itemIter.value());
        Q_ASSERT(fileIter != m_d->m_memoryMap.constEnd());
        if (fileIter.value())
            return QByteArray();
        auto nextIter = fileIter + 1;
        m_d->m_device->seek(fileIter.key());
        if (nextIter == m_d->m_memoryMap.constEnd())
            return m_d->m_device->readAll();
        return m_d->m_device->read(nextIter.key() - fileIter.key());
    }
    qint64 writeInMap(const QByteArray& block)
    {
        for (auto i = m_d->m_memoryMap.begin(); i != m_d->m_memoryMap.end(); ++i) {
            if (i.value()) {
                // Space is available
                auto j = i + 1;
                if (j != m_d->m_memoryMap.end()) {
                    if (i.key() + static_cast<qint64>(block.size()) > j.key())
                        // Not enough space to save here 
                        continue;
                    if (i.key() + static_cast<qint64>(block.size()) < j.key()) {
                        // Item smaller than available space
                        m_d->m_memoryMap.insert(i.key() + static_cast<qint64>(block.size()), true);
                    }
                }
                else {
                    m_d->m_memoryMap.insert(i.key() + block.size(), true);
                }
                i.value() = false;
                m_d->m_device->seek(i.key());
                m_d->m_device->write(block);
                return i.key();
            }
        }
        Q_UNREACHABLE();
        return 0;
    }
};
template <class KeyType, class ValueType>
using HugeMap = HugeContainer<KeyType, ValueType, true>;
template <class KeyType, class ValueType>
using HugeHash = HugeContainer<KeyType, ValueType, false>;

#endif // hugecontainer_h__