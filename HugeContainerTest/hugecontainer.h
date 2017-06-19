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

#include <QMap>
#include <QHash>
#include <QSharedData>
#include <QSharedDataPointer>
#include <memory>
#include <QTemporaryFile>
#include <QQueue>
#include <QDataStream>
#include <map>
#include <unordered_map>

namespace HugeContainer{
    template <class ValueType>
    struct ContainerObjectData : public QSharedData{
        bool m_isAvailable;
        union ObjectData
        {
            explicit ObjectData(qint64 fPos)
                :m_fPos(fPos)
            {}
            explicit ObjectData(ValueType* val)
                :m_val(val)
            {}
            qint64 m_fPos;
            ValueType* m_val;
        } m_data;
        explicit ContainerObjectData(qint64 fPos)
            :m_isAvailable(false)
            , m_data(fPos)
        {}
        explicit ContainerObjectData(ValueType* val)
            :m_isAvailable(true)
            , m_data(val)
        {
            Q_ASSERT(val);
        }
        ~ContainerObjectData()
        {
            if (m_isAvailable)
                delete m_data.m_val;
        }
        ContainerObjectData(const ContainerObjectData& other)
            :m_isAvailable(other.m_isAvailable)
            , m_data(other.m_data.m_fPos)
        {
            if (m_isAvailable)
                m_data.m_val = new ValueType(*(other.m_data.m_val));
        }
    };
    template <class ValueType>
    struct ContainerObject{
        QSharedDataPointer<ContainerObjectData<ValueType> > m_d;
        explicit ContainerObject(qint64 fPos)
            :m_d(new ContainerObjectData(fPos))
        {}
        explicit ContainerObject(ValueType* val)
            :m_d(new ContainerObjectData(val))
        {}
        ContainerObject(const ContainerObject& other) = default;
    };
    template <class KeyType, class ValueType, bool sorted>
    class HugeContainerData : public QSharedData
    {
        using ItemMapType = typename std::conditional<sorted, QMap<KeyType, ContainerObject<ValueType> >, QHash<KeyType, ContainerObject<ValueType> > >::type;
    public:
        std::unique_ptr<ItemMapType> m_itemsMap;
        std::unique_ptr<QMap<qint64, bool> > m_memoryMap;
        std::unique_ptr<QTemporaryFile> m_device;
        std::unique_ptr<QQueue<KeyType> > m_cache;
        int m_maxCache;
        HugeContainerData()
            :m_device(std::make_unique<QTemporaryFile>(QStringLiteral("HugeContainerDataXXXXXX")))
            , m_cache(std::make_unique<QQueue<KeyType> >())
            , m_memoryMap(std::make_unique<QMap<qint64, bool> >())
            , m_itemsMap(std::make_unique<ItemMapType>())
            , m_maxCache(1)
        {}
        ~HugeContainerData() = default;
        HugeContainerData(HugeContainerData& other)
            :m_device(std::make_unique<QTemporaryFile>(QStringLiteral("HugeContainerDataXXXXXX")))
            , m_cache(std::make_unique<QQueue<KeyType> >(*(other.m_cache)))
            , m_memoryMap(std::make_unique<QMap<qint64, bool> >(*(other.m_memoryMap)))
            , m_itemsMap(std::make_unique<ItemMapType>(*(other.m_itemsMap)))
            , m_maxCache(other.m_maxCache)
        {
            other.m_device->seek(0);
            qint64 totalSize = other.m_device->size();
            for (; totalSize > 1024; totalSize -= 1024)
                m_device->write(other.m_device->read(1024));
            m_device->write(other.m_device->read(totalSize));
        }
        
    };
    template <class KeyType, class ValueType, bool sorted = false>
    class HugeContainer
    {
        static_assert(std::is_default_constructible<ValueType>::value, "ValueType must provide a default constructor");
        static_assert(std::is_copy_constructible<ValueType>::value, "ValueType must provide a copy constructor");
    private:
        using NormalContaineType = typename std::conditional<sorted, QMap<KeyType, ValueType>, QHash<KeyType, ValueType> >::type;
        using NormalStdContaineType = typename std::conditional<sorted, std::map<KeyType, ValueType>, std::unordered_map<KeyType, ValueType> >::type;
        QSharedDataPointer<HugeContainerData<KeyType,ValueType,sorted> > m_d;
    protected:
        qint64 writeInMap(const QByteArray& block) const
        {
            if (!m_d->m_device->isWritable())
                return -1;
            for (auto i = m_d->m_memoryMap->begin(); i != m_d->m_memoryMap->end(); ++i) {
                if (i.value()) {
                    // Space is available
                    auto j = i + 1;
                    if (j != m_d->m_memoryMap->end()) {
                        if (i.key() + static_cast<qint64>(block.size()) > j.key())
                            // Not enough space to save here 
                            continue;
                        if (i.key() + static_cast<qint64>(block.size()) < j.key()) {
                            // Item smaller than available space
                            m_d->m_memoryMap->insert(i.key() + static_cast<qint64>(block.size()), true);
                        }
                    }
                    else {
                        m_d->m_memoryMap->insert(i.key() + block.size(), true);
                    }
                    i.value() = false;
                    m_d->m_device->seek(i.key());
                    if(m_d->m_device->write(block)>=0)
                        return i.key();
                    return -1;
                }
            }
            Q_UNREACHABLE();
            return 0;
        }
        void removeFromMap(qint64 pos) const {
            auto fileIter = m_d->m_memoryMap->find(pos);
            Q_ASSERT(fileIter != m_d->m_memoryMap->end());
            if (fileIter.value())
                return;
            if (fileIter != m_d->m_memoryMap->begin()) {
                if ((fileIter - 1).value()) {
                    fileIter = m_d->m_memoryMap->erase(fileIter);
                    if (fileIter.value())
                        m_d->m_memoryMap->erase(fileIter);
                    return;
                }
            }
            if ((fileIter + 1) != m_d->m_memoryMap->end()) {
                if ((fileIter + 1).value())
                    m_d->m_memoryMap->erase(fileIter + 1);
            }
            fileIter.value() = true;
        }
 
        bool saveQueue(int numElements = 1) const{
            for (; numElements > 0; --numElements) {
                Q_ASSERT(!m_d->m_cache->isEmpty());
                const KeyType keyToWrite = m_d->m_cache->dequeue();
                auto valToWrite = m_d->m_itemsMap->find(keyToWrite);
                Q_ASSERT(valToWrite != m_d->m_itemsMap->end());
                Q_ASSERT(valToWrite->m_d->m_isAvailable);
                QByteArray block;
                {
                    QDataStream writerStream(&block, QIODevice::WriteOnly);
                    writerStream << *(valToWrite->m_d->m_data.m_val);
                }
                const qint64 result = writeInMap(block);
                if(result>=0){
                    valToWrite->m_d->m_isAvailable = false;
                    valToWrite->m_d->m_data.m_fPos = result;
                }
                else{
                    m_d->m_cache->prepend(keyToWrite);
                }
            }
        }
        QByteArray readBlock(qint32 key) const
        {
            if (!m_d->m_device->isReadable())
                return QByteArray();
            m_d->m_device->setTextModeEnabled(false);
            auto itemIter = m_d->m_itemsMap->constFind(key);
            Q_ASSERT(itemIter != m_d->m_itemsMap->constEnd());
            Q_ASSERT(!itemIter->m_d->m_isAvailable);
            auto fileIter = m_d->m_memoryMap->constFind(itemIter->m_d->m_data.m_fPos);
            Q_ASSERT(fileIter != m_d->m_memoryMap->constEnd());
            if (fileIter.value())
                return QByteArray();
            auto nextIter = fileIter + 1;
            m_d->m_device->seek(fileIter.key());
            if (nextIter == m_d->m_memoryMap->constEnd())
                return m_d->m_device->readAll();
            return m_d->m_device->read(nextIter.key() - fileIter.key());
        }
    public:
        class iterator
        {
            friend class HugeContainer;
            const HugeContainer<KeyType, ValueType, sorted>* m_container;
            using baseIterType = ItemMapType::iterator;
            baseIterType m_baseIter;
            iterator(const HugeContainer* const  cont, const baseIterType& baseItr)
                :m_container(cont)
                , m_baseIter(baseItr)
            {}
        public:
            iterator();
            iterator(const iterator& other) = default;
            iterator& operator=(const iterator& other) = default;
            iterator operator+(int j) const { return iterator(m_container, m_baseIter + j); }
            iterator &operator++() { ++m_baseIter; return *this; }
            iterator operator++(int) { iterator result(*this); ++m_baseIter; return result; }
            iterator &operator+=(int j) { m_baseIter += j; return *this; }
            iterator operator-(int j) const { return iterator(m_container, m_baseIter - j); }
            iterator &operator--() { --m_baseIter; return *this; }
            iterator operator--(int) { iterator result(*this); --m_baseIter; return result; }
            iterator &operator-=(int j) { m_baseIter -= j; return *this; }
            const KeyType& key() const { return m_baseIter.key(); }
            ValueType& operator*() const { return value(); }
            ValueType& value() const { 
                const ValueType* const result = m_container->value(m_baseIter.key());
                Q_ASSERT(result);
                return *result;
            }
            ValueType* operator->() const
            {
                const ValueType* const result = m_container->value(m_baseIter.key());
                Q_ASSERT(result);
                return result;
            }
            bool operator!=(const iterator &other) const { return !operator==(other); }
            bool operator==(const iterator &other) const { return m_container == other.m_container &&  m_baseIter == other.m_baseIter; }
        };
        using Iterator = iterator;
        

        class const_iterator
        {
            friend class HugeContainer;
            const HugeContainer<KeyType, ValueType, sorted>* m_container;
            using baseIterType = ItemMapType::const_iterator;
            baseIterType m_baseIter;
            const_iterator(const HugeContainer* const  cont, const baseIterType& baseItr)
                :m_container(cont)
                , m_baseIter(baseItr)
            {}
        public:
            const_iterator();
            const_iterator(const const_iterator& other) = default;
            const_iterator& operator=(const const_iterator& other) = default;
            const_iterator operator+(int j) const { return const_iterator(m_container, m_baseIter + j); }
            const_iterator &operator++() { ++m_baseIter; return *this; }
            const_iterator operator++(int) { const_iterator result(*this); ++m_baseIter; return result; }
            const_iterator &operator+=(int j) { m_baseIter += j; return *this; }
            const_iterator operator-(int j) const { return const_iterator(m_container, m_baseIter - j); }
            const_iterator &operator--() { --m_baseIter; return *this; }
            const_iterator operator--(int) { const_iterator result(*this); --m_baseIter; return result; }
            const_iterator &operator-=(int j) { m_baseIter -= j; return *this; }
            const KeyType& key() const { return m_baseIter.key(); }
            const ValueType& operator*() const { return value(); }
            const ValueType& value() const
            {
                const ValueType* const result = m_container->value(m_baseIter.key());
                Q_ASSERT(result);
                return *result;
            }
            const ValueType* operator->() const
            {
                const ValueType* const result = m_container->value(m_baseIter.key());
                Q_ASSERT(result);
                return result;
            }
            bool operator!=(const const_iterator &other) const { return !operator==(other); }
            bool operator==(const const_iterator &other) const { return m_container == other.m_container &&  m_baseIter == other.m_baseIter; }
        };
        using ConstIterator = const_iterator;
        HugeContainer(const NormalStdContaineType& list){
            const auto listBegin = std::begin(list);
            const auto listEnd = std::end(list);
            for (auto i = listBegin; i != listEnd; ++i) {
                if (i != listBegin) {
                    if (i.key() == (i - 1).key())
                        continue;
                }
                setValue(i->first, i->second);
            }
        }
        HugeContainer(std::initializer_list<std::pair<KeyType, ValueType> > list)
            :HugeContainer(NormalContaineType(list))
        {
            const auto listBegin = std::begin(list);
            const auto listEnd = std::end(list);
            for (auto i = listBegin; i != listEnd; ++i) {
                if (i != listBegin) {
                    if (i.key() == (i - 1).key())
                        continue;
                }
                setValue(i->first, i->second);
            }
        }
        HugeContainer(const NormalContaineType& list)
            :HugeContainer()
        {
            for (auto i = list.constBegin(); i != list.constEnd(); ++i) {
                if (i != list.constBegin()) {
                    if (i.key() == (i - 1).key())
                        continue;
                }
                setValue(i.key(), i.value());
            }
        }
        QList<KeyType> keys() const
        {
            return m_d->m_itemsMap->keys();
        }
        QList<KeyType> uniqueKeys() const
        {

            return m_d->m_itemsMap->uniqueKeys();
        }
        int maxCache() const { return m_d->m_maxCache; }
        bool setMaxCache(int val) { 
            val = qMax(1, val);
            if (val == m_d->m_maxCache)
                return true;
            if (val < m_d->m_cache->size()) {
                if (!saveQueue(m_d->m_cache->size() - val))
                    return false;
            }
            m_d->m_maxCache = val;
            return true;
        }
        bool enqueueValue(const KeyType& key, ValueType* val) const
        {
            int cacheIdx = m_d->m_cache->indexOf(key);
            if (cacheIdx >= 0) {
                m_d->m_cache->removeAt(cacheIdx);
            }
            else if (m_d->m_cache->size() == m_d->m_maxCache) {
                if (!saveQueue())
                    return false;
            }
            m_d->m_cache->enqueue(key);
            auto itemIter= m_d->m_itemsMap->find(key);
            if (itemIter == m_d->m_itemsMap->end()){
                m_d->m_itemsMap->insert(key, ContainerObject(val));
            }
            else{
                if (!itemIter->m_d->m_isAvailable)
                    removeFromMap(itemIter->m_d->m_data.m_fPos);
                itemIter->m_d->m_isAvailable = true;
                itemIter->m_d->m_data.m_val = val;
            }
            return true;
        }
        void swap(HugeContainer<KeyType, ValueType, sorted>& other) Q_DECL_NOTHROW{
            std::swap(m_d, other.m_d);
        }
        void remove(const KeyType& key)
        {
            m_d->m_cache->removeAll(key);
            auto itemIter = m_d->m_itemsMap->find(key);
            if (itemIter != m_d->m_itemsMap->end()){
                if (!itemIter->m_d->m_isAvailable)
                    removeFromMap(itemIter->m_d->m_data.m_fPos);
                m_d->m_itemsMap->erase(itemIter);
            }
        }
        bool setValue(const KeyType& key, const ValueType& val)
        {
            return enqueueValue(key,new ValueType(val));
        }
        iterator insert(const KeyType &key, const ValueType &val){
            if (setValue(key, val))
                return find(key);
            return end();
        }
        void clear()
        {
            if (m_d->m_device->remove()) {
                if (!m_d->m_device->open())
                    Q_ASSERT_X(false, "HugeContainer::HugeContainer", "Unable to create a temporary file");
            }
            m_d->m_itemsMap->clear();
            m_d->m_memoryMap->clear();
            m_d->m_memoryMap->insert(0, true);
            m_d->m_cache->clear();
        }
        ValueType* value(const KeyType& key) const
        {
            auto valueIter = m_d->m_itemsMap->find(key);
            if (valueIter == m_d->m_itemsMap->end())
                return nullptr;
            if(!valueIter->m_d->m_isAvailable){
                const qint64 oldPos = valueIter->m_d->m_data.m_fPos;
                QByteArray block = readBlock(key);
                if (block.isEmpty())
                    return nullptr;
                ValueType* result = new ValueType;
                QDataStream readerStream(block);
                readerStream >> *result;
                if (!enqueueValue(key, result))
                    return nullptr;
                removeFromMap(oldPos);
            }
            else {
                m_d->m_cache->removeAll(key);
                m_d->m_cache->enqueue(key);
            }
            
            return valueIter->m_d->m_data.m_val;
        }
        ValueType& operator[](const KeyType& key){
            ValueType* result = value(key);
            if (!result) {
                setValue(key, ValueType());
                result = value(key);
            }
            return result;
        }
        ValueType operator[](const KeyType& key) const
        {
            ValueType* result = value(key);
            if (!result)
                return ValueType{};
            return *result;
        }
        bool contains(const KeyType& key) const
        {
            return m_d->m_itemsMap->contains(key);
        }
        int count() const
        {
            return size();
        }
        int size() const
        {
            return m_d->m_itemsMap->size();
        }
        bool isEmpty() const
        {
            return m_d->m_itemsMap->isEmpty();
        }
        iterator begin(){
            return iterator(this, m_d->m_itemsMap->begin());
        }
        
        const_iterator begin() const
        {
            return constBegin();
        }
        const_iterator cbegin() const
        {
            return constBegin();
        }
        const_iterator constBegin() const
        {
            return const_iterator(this, m_d->m_itemsMap->constBegin());
        }
        iterator end()
        {
            return iterator(this, m_d->m_itemsMap->end());
        }
        const_iterator constEnd() const
        {
            return const_iterator(this, m_d->m_itemsMap->constEnd());
        }
        const_iterator end() const
        {
            return constEnd();
        }
        const_iterator cend() const
        {
            return constEnd();
        }
        iterator find(const KeyType& val)
        {
            return iterator(this, m_d->m_itemsMap->find(val));
        }
        const_iterator constFind(const KeyType& val)
        {
            return const_iterator(this, m_d->m_itemsMap->constFind(val));
        }
        iterator erase(iterator pos)
        {
            iterator endIter = end();
            if (pos == endIter)
                return endIter;
            iterator result = pos + 1;
            if (removeValue(result.key()))
                return result;
            return pos;
        }
        ValueType take(const KeyType& key)
        {
            const ValueType* const resultP = value(key);
            if (!resultP)
                return ValueType();
            ValueType result = std::move(*resultP);
            removeValue(key);
            return result;
        }
        ValueType& last(){
            Q_ASSERT(!isEmpty());
            return *(end() - 1);
        }
        const ValueType& last() const
        {
            Q_ASSERT(!isEmpty());
            return *(constEnd() - 1);
        }
        const KeyType&	lastKey() const{
            Q_ASSERT(!isEmpty());
            return (constEnd() - 1).key();
        }
        ValueType& first()
        {
            Q_ASSERT(!isEmpty());
            return *(begin());
        }
        const ValueType& first() const
        {
            Q_ASSERT(!isEmpty());
            return *(constBegin());
        }
        const KeyType& firstKey() const
        {
            Q_ASSERT(!isEmpty());
            return (constBegin()).key();
        }
        HugeContainer()
            :m_d(new HugeContainerData<KeyType, ValueType, sorted>{})
        {
            if (!m_d->m_device->open())
                Q_ASSERT_X(false, "HugeContainer::HugeContainer", "Unable to create a temporary file");
            m_d->m_memoryMap->insert(0, true);
        }
        HugeContainer(const HugeContainer& other) = default;
        virtual ~HugeContainer() = default;
        using difference_type = qptrdiff;
        using key_type = KeyType;
        using mapped_type = ValueType;
        using size_type = int;
    };
    template <class KeyType, class ValueType>
    using HugeMap = HugeContainer<KeyType, ValueType, true>;
    template <class KeyType, class ValueType>
    using HugeHash = HugeContainer<KeyType, ValueType, false>;
}
#endif // hugecontainer_h__