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
            :m_d(new ContainerObjectData<ValueType>(fPos))
        {}
        explicit ContainerObject(ValueType* val)
            :m_d(new ContainerObjectData<ValueType>(val))
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
        {
            if (!m_device->open())
                Q_ASSERT_X(false, "HugeContainer::HugeContainer", "Unable to create a temporary file");
            m_memoryMap->insert(0, true);
        }
        ~HugeContainerData() = default;
        HugeContainerData(HugeContainerData& other)
            :m_device(std::make_unique<QTemporaryFile>(QStringLiteral("HugeContainerDataXXXXXX")))
            , m_cache(std::make_unique<QQueue<KeyType> >(*(other.m_cache)))
            , m_memoryMap(std::make_unique<QMap<qint64, bool> >(*(other.m_memoryMap)))
            , m_itemsMap(std::make_unique<ItemMapType>(*(other.m_itemsMap)))
            , m_maxCache(other.m_maxCache)
        {
            if (!m_device->open())
                Q_ASSERT_X(false, "HugeContainer::HugeContainer", "Unable to create a temporary file");
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
        qint64 writeElementInMap(const ValueType& val) const
        {
            QByteArray block;
            {
                QDataStream writerStream(&block, QIODevice::WriteOnly);
                writerStream << val;
            }
            const qint64 result = writeInMap(block);
            return result;
        }
        bool saveQueue(int numElements = 1) const{
            bool allOk=true;
            for (; allOk && numElements > 0; --numElements) {
                Q_ASSERT(!m_d->m_cache->isEmpty());
                const KeyType keyToWrite = m_d->m_cache->dequeue();
                auto valToWrite = m_d->m_itemsMap->find(keyToWrite);
                Q_ASSERT(valToWrite != m_d->m_itemsMap->end());
                Q_ASSERT(valToWrite->m_d->m_isAvailable);
                const qint64 result = writeElementInMap(*(valToWrite->m_d->m_data.m_val));
                if (result>=0) {
                    valToWrite->m_d->m_isAvailable = false;
                    valToWrite->m_d->m_data.m_fPos = result;
                }
                else{
                    m_d->m_cache->prepend(keyToWrite);
                    allOk = false;
                }
            }
            return allOk;
        }

        QByteArray readBlock(KeyType key) const
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
            int m_baseIterShift;
            iterator(const HugeContainer<KeyType, ValueType, sorted>* const  cont, int baseItrShift)
                :m_container(cont)
                , m_baseIterShift(baseItrShift)
            {}
        public:
            iterator();
            iterator(const iterator& other) = default;
            iterator& operator=(const iterator& other) = default;
            iterator operator+(int j) const { return iterator(m_container, m_baseIterShift + j); }
            iterator &operator++() { ++m_baseIterShift; return *this; }
            iterator operator++(int) { iterator result(*this); ++m_baseIterShift; return result; }
            iterator &operator+=(int j) { m_baseIterShift += j; return *this; }
            iterator operator-(int j) const { return iterator(m_container, m_baseIterShift - j); }
            iterator &operator--() { --m_baseIterShift; return *this; }
            iterator operator--(int) { iterator result(*this); --m_baseIterShift; return result; }
            iterator &operator-=(int j) { m_baseIterShift -= j; return *this; }
            const KeyType& key() const { return (m_container->m_d->m_itemsMap->begin() + m_baseIterShift).key(); }
            ValueType& operator*() const { return value(); }
            ValueType& value() const { 
                ValueType* const result = m_container->value(key());
                Q_ASSERT(result);
                return *result;
            }
            ValueType* operator->() const
            {
                const ValueType* const result = m_container->value(key());
                Q_ASSERT(result);
                return result;
            }
            bool operator!=(const iterator &other) const { return !operator==(other); }
            bool operator==(const iterator &other) const { return m_container == other.m_container &&  m_baseIterShift == other.m_baseIterShift; }
        };
        using Iterator = iterator;
        
 
        class const_iterator
        {
            friend class HugeContainer;
            const HugeContainer<KeyType, ValueType, sorted>* m_container;
            int m_baseIterShift;
            const_iterator(const HugeContainer<KeyType, ValueType, sorted>* const  cont, int baseItrShift)
                :m_container(cont)
                , m_baseIterShift(baseItrShift)
            {}
        public:
            const_iterator();
            const_iterator(const const_iterator& other) = default;
            const_iterator& operator=(const const_iterator& other) = default;
            const_iterator operator+(int j) const { return const_iterator(m_container, m_baseIterShift + j); }
            const_iterator &operator++() { ++m_baseIterShift; return *this; }
            const_iterator operator++(int) { const_iterator result(*this); ++m_baseIterShift; return result; }
            const_iterator &operator+=(int j) { m_baseIterShift += j; return *this; }
            const_iterator operator-(int j) const { return const_iterator(m_container, m_baseIterShift - j); }
            const_iterator &operator--() { --m_baseIterShift; return *this; }
            const_iterator operator--(int) { const_iterator result(*this); --m_baseIterShift; return result; }
            const_iterator &operator-=(int j) { m_baseIterShift -= j; return *this; }
            const KeyType& key() const { return (m_container->m_d->m_itemsMap->constBegin() + m_baseIterShift).key(); }
            const ValueType& operator*() const { return value(); }
            const ValueType& value() const
            {
                const ValueType* const result = m_container->value(key());
                Q_ASSERT(result);
                return *result;
            }
            const ValueType* operator->() const
            {
                const ValueType* const result = m_container->value(key());
                Q_ASSERT(result);
                return result;
            }
            bool operator!=(const const_iterator &other) const { return !operator==(other); }
            bool operator==(const const_iterator &other) const { return m_container == other.m_container &&  m_baseIterShift == other.m_baseIterShift; }
        };
        using ConstIterator = const_iterator;
        HugeContainer(const NormalStdContaineType& list){
            const auto listBegin = std::begin(list);
            const auto listEnd = std::end(list);
            auto prevIter = std::begin(list);
            for (auto i = listBegin; i != listEnd; ++i) {
                if (i != listBegin) {
                    if (i->first == prevIter->first)
                        continue;
                }
                setValue(i->first, i->second);
                prevIter = i;
            }
        }
        HugeContainer(std::initializer_list<std::pair<KeyType, ValueType> > list)
            :HugeContainer(NormalContaineType(list))
        {
            const auto listBegin = std::begin(list);
            const auto listEnd = std::end(list);
            auto prevIter = std::begin(list);
            for (auto i = listBegin; i != listEnd; ++i) {
                if (i != listBegin) {
                    if (i->first == prevIter->first)
                        continue;
                }
                setValue(i->first, i->second);
                prevIter = i;
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
                m_d->m_itemsMap->insert(key, ContainerObject<ValueType>(val));
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
        iterator insert(const KeyType &key, const ValueType &val)
        {
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
        ValueType* valueFromBlock(const KeyType& key) const{
            QByteArray block = readBlock(key);
            if (block.isEmpty())
                return nullptr;
            ValueType* result = new ValueType;
            QDataStream readerStream(block);
            readerStream >> *result;
            return result;
        }
        ValueType* value(const KeyType& key) const
        {
            auto valueIter = m_d->m_itemsMap->find(key);
            if (valueIter == m_d->m_itemsMap->end())
                return nullptr;
            if(!valueIter->m_d->m_isAvailable){
                ValueType* const result = valueFromBlock(key);
                if (!result)
                    return nullptr;
                if (!enqueueValue(key, result))
                    return nullptr;
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
            return *result;
        }
        ValueType operator[](const KeyType& key) const
        {
            ValueType* result = value(key);
            if (!result)
                return ValueType{};
            return *result;
        }
        bool unite(const HugeContainer<KeyType,ValueType,sorted>& other, bool overWrite = false){
            const auto endItemMap = other.m_d->m_itemsMap->constEnd();
            for (auto i = other.m_d->m_itemsMap->constBegin(); i != endItemMap;++i){
                auto currItmIter = m_d->m_itemsMap->find(i.key());
                if (currItmIter != m_d->m_itemsMap->end() && !overWrite)
                    continue;
                if(i->m_d->m_isAvailable){
                    if (currItmIter != m_d->m_itemsMap->end()) { // contains(i.key())
                        if (m_d->m_cache->contains(i.key())) {
                            Q_ASSERT(currItmIter->m_d->m_isAvailable);
                            delete currItmIter->m_d->m_data.m_val;
                            currItmIter->m_d->m_data.m_val = new ValueType(*(i->m_d->m_data.m_val));
                        }
                        else {
                            Q_ASSERT(!currItmIter->m_d->m_isAvailable);
                            const qint64 newPos = writeElementInMap(*(i->m_d->m_data.m_val));
                            if (newPos >= 0)
                                removeFromMap(currItmIter->m_d->m_data.m_fPos);
                            else
                                return false;
                            currItmIter->m_d->m_data.m_fPos = newPos;
                        }
                    }
                    else{
                        const qint64 newPos = writeElementInMap(*(i->m_d->m_data.m_val));
                        if (newPos >= 0)
                            m_d->m_itemsMap->insert(i.key(), ContainerObject<ValueType>(newPos));
                        else
                            return false;
                    }
                }
                else{
                    if (currItmIter != m_d->m_itemsMap->end()) {
                        if (m_d->m_cache->contains(i.key())) {
                            Q_ASSERT(currItmIter->m_d->m_isAvailable);
                            ValueType* newVal = other.valueFromBlock(i.key());
                            if (!newVal)
                                return false;
                            delete currItmIter->m_d->m_data.m_val;
                            currItmIter->m_d->m_data.m_val = newVal;
                        }
                        else{
                            Q_ASSERT(!currItmIter->m_d->m_isAvailable);
                            const qint64 newPos = writeInMap(other.readBlock(i.key()));
                            if (newPos >= 0)
                                removeFromMap(currItmIter->m_d->m_data.m_fPos);
                            else
                                return false;
                            currItmIter->m_d->m_data.m_fPos = newPos;
                        }
                        
                    }
                    else{
                        const qint64 newPos = writeInMap(other.readBlock(i.key()));
                        if (newPos >= 0)
                            m_d->m_itemsMap->insert(i.key(), ContainerObject<ValueType>(newPos));
                        else
                            return false;
                    }
                }

            }
            return true;
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
            return iterator(this, 0);
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
            return const_iterator(this, 0);
        }
        iterator end()
        {
            return iterator(this, m_d->m_itemsMap->size());
        }
        const_iterator constEnd() const
        {
            return const_iterator(this, m_d->m_itemsMap->size());
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
            return iterator(this, std::distance(m_d->m_itemsMap->begin(), m_d->m_itemsMap->find(val)));
        }
        const_iterator constFind(const KeyType& val)
        {
            return const_iterator(this, std::distance(m_d->m_itemsMap->constBegin(), m_d->m_itemsMap->constFind(val)));
        }
        iterator erase(iterator pos)
        {
            if (pos != end())
                remove(pos.key());
            return pos;
        }
        ValueType take(const KeyType& key)
        {
            const ValueType* const resultP = value(key);
            if (!resultP)
                return ValueType();
            ValueType result = std::move(*resultP);
            remove(key);
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
            
        }
        HugeContainer(const HugeContainer& other) = default;
        HugeContainer& operator=(HugeContainer&& other) Q_DECL_NOTHROW{
            swap(other);
            return *this;
        }
        double fragmentation() const{
            return 
                std::accumulate(m_d->m_memoryMap.constBegin(), m_d->m_memoryMap.constEnd() - 1, 0.0, [](double curr, bool val)->double {return val ? (curr + 1.0) : curr; }) 
                / static_cast<double>(size())
        }
        bool defrag(){
            if (m_d->m_memoryMap.size()<=1)
                return true;
            if (std::all_of(m_d->m_memoryMap.constBegin(), m_d->m_memoryMap.constEnd() - 1, [](bool val) ->bool {return !val}))
                return true;
            auto newFile = std::make_unique<QTemporaryFile>(QStringLiteral("HugeContainerDataXXXXXX"));
            if (!newFile.open())
                return false;
            auto newMap = std::make_unique<QMap<qint64, bool> >();
            QHash<KeyType, qint64> oldPos;
            bool allGood=true;
            for (auto i = m_d->m_itemsMap.begin(); allGood && i != m_d->m_itemsMap.end(); ++i) {
                if (i->m_d->m_isAvaliable)
                    continue;
                const auto newMapIter = newMap->insert(newFile->pos(), false);
                if(newFile->write(readBlock(i.key())>=0)){
                    oldPos.insert(i.key(), i->m_d->m_data.m_fpos);
                    i->m_d->m_data.m_fpos = newMapIter.key();
                }
                else{
                    allGood = false;
                }
            }
            if(!allGood){
                for (auto i = oldPos.constEnd(); i != oldPos.constEnd(); ++i)
                    m_d->m_itemsMap->operator[](i.key)->m_d->m_data.m_fpos = i.value();
                return false;
            }
            newMap->insert(newFile->pos(), true);
            m_d->m_device = std::move(newFile);
            m_d->m_memoryMap = std::move(newMap);
            return true;
        }
        bool operator==(const HugeContainer<KeyType, ValueType,sorted>& other){
            if(size()!=other.size())
                return false;
            for (auto i = m_d->m_itemsMap->constBegin(); i != m_d->m_itemsMap->constBegin(); ++i){
                if (!other.contains(i.key()))
                    return false;
            }
            // Compare values as last resort
            for (auto i = m_d->m_itemsMap->constBegin(); i != m_d->m_itemsMap->constBegin(); ++i) {
                if (!(*(other.value(i.key())) == *value(i.key())))
                    return false;
            }
            return true;
        }
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