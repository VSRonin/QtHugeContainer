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

#include <QDataStream>
#include <QDir>
#include <QDirIterator>
#include <QHash>
#include <QMap>
#include <QQueue>
#include <QSharedData>
#include <QSharedDataPointer>
#include <QExplicitlySharedDataPointer>
#include <QTemporaryFile>
#include <initializer_list>
#include <map>
#include <memory>
#include <unordered_map>
#include <QDebug>

namespace HugeContainers {
    template <class KeyType, class ValueType, bool sorted>
    class HugeContainer;
}
template <class KeyType, class ValueType, bool sorted>
QDataStream& operator<<(QDataStream &out, const HugeContainers::HugeContainer<KeyType, ValueType, sorted>& cont);
template <class KeyType, class ValueType, bool sorted>
QDataStream& operator>>(QDataStream &in, HugeContainers::HugeContainer<KeyType, ValueType, sorted>& cont);
namespace HugeContainers{
    //! Removes any leftover data from previous crashes
    inline void cleanUp(){
        QDirIterator cleanIter{ QDir::tempPath(), QStringList(QStringLiteral("HugeContainerData*")), QDir::Files | QDir::Writable | QDir::CaseSensitive | QDir::NoDotAndDotDot };
        while (cleanIter.hasNext()) {
            cleanIter.next();
            QFile::remove(cleanIter.filePath());
        }

    }

    template <class KeyType, class ValueType, bool sorted>
    class HugeContainer
    {
        static_assert(std::is_default_constructible<ValueType>::value, "ValueType must provide a default constructor");
        static_assert(std::is_copy_constructible<ValueType>::value, "ValueType must provide a copy constructor");
    private:

        template <class ValueType>
        struct ContainerObjectData : public QSharedData
        {
            bool m_isAvailable;
            union ObjectData
            {
                explicit ObjectData(qint64 fp)
                    :m_fPos(fp)
                {}
                explicit ObjectData(ValueType* v)
                    :m_val(v)
                {}
                qint64 m_fPos;
                ValueType* m_val;
            } m_data;            
            explicit ContainerObjectData(qint64 fp)
                :QSharedData()
                , m_isAvailable(false)
                , m_data(fp)
            {}
            explicit ContainerObjectData(ValueType* v)
                :QSharedData()
                , m_isAvailable(true)
                , m_data(v)
            {
                Q_ASSERT(v);
            }
            ~ContainerObjectData()
            {
                if (m_isAvailable)
                    delete m_data.m_val;
            }
            ContainerObjectData(const ContainerObjectData& other)
                :QSharedData(other)
                , m_isAvailable(other.m_isAvailable)
                , m_data(other.m_data.m_fPos)
            {
                if (m_isAvailable)
                    m_data.m_val = new ValueType(*(other.m_data.m_val));
            }
        };

        template <class ValueType>
        class ContainerObject
        {
            QExplicitlySharedDataPointer<ContainerObjectData<ValueType> > m_d;
        public:
            explicit ContainerObject(qint64 fPos)
                :m_d(new ContainerObjectData<ValueType>(fPos))
            {}
            explicit ContainerObject(ValueType* val)
                :m_d(new ContainerObjectData<ValueType>(val))
            {}
            ContainerObject(const ContainerObject& other) = default;
            bool isAvailable() const { return m_d->m_isAvailable; }
            qint64 fPos() const { return m_d->m_data.m_fPos; }
            const ValueType* val() const { Q_ASSERT(m_d->m_isAvailable); return m_d->m_data.m_val; }
            ValueType* val() { Q_ASSERT(m_d->m_isAvailable); m_d.detach(); return m_d->m_data.m_val; }
            void setFPos(qint64 fp)
            {
                if (!m_d->m_isAvailable && m_d->m_data.m_fPos == fp)
                    return;
                m_d.detach();
                if (m_d->m_isAvailable)
                    delete m_d->m_data.m_val;
                m_d->m_data.m_fPos = fp;
                m_d->m_isAvailable = false;
            }
            void setVal(ValueType* vl)
            {
                m_d.detach();
                if (m_d->m_isAvailable)
                    delete m_d->m_data.m_val;
                m_d->m_data.m_val = vl;
                m_d->m_isAvailable = true;
            }
        };

        template <class KeyType, class ValueType, bool sorted>
        class HugeContainerData : public QSharedData
        {
        public:
            using ItemMapType = typename std::conditional<sorted, QMap<KeyType, ContainerObject<ValueType> >, QHash<KeyType, ContainerObject<ValueType> > >::type;
            std::unique_ptr<ItemMapType> m_itemsMap;
            std::unique_ptr<QMap<qint64, bool> > m_memoryMap;
            std::unique_ptr<QTemporaryFile> m_device;
            std::unique_ptr<QQueue<KeyType> > m_cache;
            int m_maxCache;
            int m_compressionLevel;
            HugeContainerData()
                : QSharedData()
                , m_device(std::make_unique<QTemporaryFile>(QDir::tempPath() + QDir::separator() + QStringLiteral("HugeContainerDataXXXXXX")))
                , m_cache(std::make_unique<QQueue<KeyType> >())
                , m_memoryMap(std::make_unique<QMap<qint64, bool> >())
                , m_itemsMap(std::make_unique<ItemMapType>())
                , m_maxCache(1)
                , m_compressionLevel(0)
            {
                if (!m_device->open())
                    Q_ASSERT_X(false, "HugeContainer::HugeContainer", "Unable to create a temporary file");
                m_memoryMap->insert(0, true);
            }
            ~HugeContainerData() = default;
            HugeContainerData(HugeContainerData& other)
                : QSharedData(other)
                , m_device(std::make_unique<QTemporaryFile>(QDir::tempPath() + QDir::separator() + QStringLiteral("HugeContainerDataXXXXXX")))
                , m_cache(std::make_unique<QQueue<KeyType> >(*(other.m_cache)))
                , m_memoryMap(std::make_unique<QMap<qint64, bool> >(*(other.m_memoryMap)))
                , m_itemsMap(std::make_unique<ItemMapType>(*(other.m_itemsMap)))
                , m_maxCache(other.m_maxCache)
                , m_compressionLevel(other.m_compressionLevel)
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

        using NormalContaineType = typename std::conditional<sorted, QMap<KeyType, ValueType>, QHash<KeyType, ValueType> >::type;
        using NormalStdContaineType = typename std::conditional<sorted, std::map<KeyType, ValueType>, std::unordered_map<KeyType, ValueType> >::type;
        QExplicitlySharedDataPointer<HugeContainerData<KeyType, ValueType, sorted> > m_d;
        std::unique_ptr<ValueType> valueFromBlock(const KeyType& key) const
        {
            QByteArray block = readBlock(key);
            if (block.isEmpty())
                return nullptr;
            auto result = std::make_unique<ValueType>();
            QDataStream readerStream(block);
            readerStream >> *result;
            return result;
        }
        bool defrag(bool readCompressed, int writeCompression)
        {
            if (isEmpty()) 
                return true;
            Q_ASSERT(m_d->m_memoryMap->size() > 1);
            if (std::all_of(m_d->m_memoryMap->constBegin(), m_d->m_memoryMap->constEnd() - 1, [](bool val) ->bool {return !val; }))
                return true;
            auto newFile = std::make_unique<QTemporaryFile>(QDir::tempPath() + QDir::separator() + QStringLiteral("HugeContainerDataXXXXXX"));
            if (!newFile->open())
                return false;
            auto newMap = std::make_unique<QMap<qint64, bool> >();
            std::conditional<sorted, QMap<KeyType, qint64>, QHash<KeyType, qint64> >::type oldPos;
            bool allGood = true;
            for (auto i = m_d->m_itemsMap->begin(); allGood && i != m_d->m_itemsMap->end(); ++i) {
                if (i->isAvailable())
                    continue;
                const auto newMapIter = newMap->insert(newFile->pos(), false);
                QByteArray blockToWrite = readBlock(i.key(), readCompressed);
                if (writeCompression != 0)
                    blockToWrite = qCompress(blockToWrite, writeCompression);
                if (newFile->write(blockToWrite) >= 0) {
                    oldPos.insert(i.key(), i->fPos());
                    i->setFPos (newMapIter.key());
                }
                else {
                    allGood = false;
                }
            }
            if (!allGood) {
                for (auto i = oldPos.constEnd(); i != oldPos.constEnd(); ++i) {
                    auto oldMapIter = m_d->m_itemsMap->find(i.key());
                    Q_ASSERT(oldMapIter != m_d->m_itemsMap->end());
                    oldMapIter.value().setFPos(i.value());
                }
                return false;
            }
            newMap->insert(newFile->pos(), true);
            m_d->m_device = std::move(newFile);
            m_d->m_memoryMap = std::move(newMap);
            return true;
        }
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
                    Q_ASSERT(fileIter != m_d->m_memoryMap->end());
                    if (fileIter.value())
                        fileIter = m_d->m_memoryMap->erase(fileIter);
                    if (fileIter == m_d->m_memoryMap->end())
                        m_d->m_device->resize(m_d->m_memoryMap->lastKey());
                    return;
                }
            }
            fileIter.value() = true;
            if (++fileIter != m_d->m_memoryMap->end()) {
                if (fileIter.value())
                    fileIter = m_d->m_memoryMap->erase(fileIter);
            }
            if (fileIter == m_d->m_memoryMap->end())
                m_d->m_device->resize(m_d->m_memoryMap->lastKey());
        }
        qint64 writeElementInMap(const ValueType& val) const
        {
            QByteArray block;
            {
                QDataStream writerStream(&block, QIODevice::WriteOnly);
                writerStream << val;
            }
            if (m_d->m_compressionLevel!=0)
                block = qCompress(block, m_d->m_compressionLevel);
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
                Q_ASSERT(valToWrite->isAvailable());
                const qint64 result = writeElementInMap(*(valToWrite->val()));
                if (result>=0) {
                    valToWrite->setFPos(result);
                }
                else{
                    m_d->m_cache->prepend(keyToWrite);
                    allOk = false;
                }
            }
            return allOk;
        }

        bool enqueueValue(const KeyType& key, std::unique_ptr<ValueType>& val) const
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
            auto itemIter = m_d->m_itemsMap->find(key);
            if (itemIter == m_d->m_itemsMap->end()) {
                m_d->m_itemsMap->insert(key, ContainerObject<ValueType>(val.release()));
            }
            else {
                if (!itemIter->isAvailable())
                    removeFromMap(itemIter->fPos());
                itemIter->setVal(val.release());
            }
            return true;
        }
        QByteArray readBlock(const KeyType& key) const{
            return readBlock(key, m_d->m_compressionLevel != 0);
        }
        QByteArray readBlock(const KeyType& key, bool compressed) const
        {
            if (Q_UNLIKELY(!m_d->m_device->isReadable()))
                return QByteArray();
            m_d->m_device->setTextModeEnabled(false);
            auto itemIter = m_d->m_itemsMap->constFind(key);
            Q_ASSERT(itemIter != m_d->m_itemsMap->constEnd());
            Q_ASSERT(!itemIter->isAvailable());
            auto fileIter = m_d->m_memoryMap->constFind(itemIter->fPos());
            Q_ASSERT(fileIter != m_d->m_memoryMap->constEnd());
            if (fileIter.value())
                return QByteArray();
            auto nextIter = fileIter + 1;
            m_d->m_device->seek(fileIter.key());
            QByteArray result;
            if (nextIter == m_d->m_memoryMap->constEnd()) 
                result = m_d->m_device->readAll();
            else 
                result = m_d->m_device->read(nextIter.key() - fileIter.key());
            if (compressed)
                return qUncompress(result);
            return result;
        }
    public:
        
        class iterator
        {
            friend class HugeContainer;
            HugeContainer<KeyType, ValueType, sorted>* m_container;
            using BaseIterType = typename HugeContainerData<KeyType, ValueType, sorted>::ItemMapType::iterator;
            BaseIterType m_baseIter;
            iterator(HugeContainer<KeyType, ValueType, sorted>* const  cont, const BaseIterType& baseItr)
                :m_container(cont)
                , m_baseIter(baseItr)
            {}
        public:
            iterator()
                :m_container(nullptr)
            {}
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
                return m_container->operator[](key());
            }
            ValueType* operator->() const
            {
                return &m_container->operator[](key());
            }
            bool operator!=(const iterator &other) const { return !operator==(other); }
            bool operator==(const iterator &other) const { return m_container == other.m_container &&  m_baseIter == other.m_baseIter; }
        };
        using Iterator = iterator;
        
 
        class const_iterator
        {
            friend class HugeContainer;
            using BaseIterType = typename HugeContainerData<KeyType, ValueType, sorted>::ItemMapType::const_iterator;
            const HugeContainer<KeyType, ValueType, sorted>* m_container;
            BaseIterType m_baseIter;
            const_iterator(const HugeContainer<KeyType, ValueType, sorted>* const  cont, const BaseIterType& baseItr)
                :m_container(cont)
                , m_baseIter(baseItr)
            {}
        public:
            const_iterator() 
                :m_container(nullptr)
            {}
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
                return  m_container->value(key());
            }
            const ValueType* operator->() const
            {
                return &(m_container->value(key()));
            }
            bool operator!=(const const_iterator &other) const { return !operator==(other); }
            bool operator==(const const_iterator &other) const { return m_container == other.m_container &&  m_baseIter == other.m_baseIter; }
        };
        using ConstIterator = const_iterator;
        class key_iterator
        {
            friend class HugeContainer;
            const_iterator m_base;
            key_iterator(const const_iterator& base)
                :m_base(base)
            {}
        public:
            key_iterator() = default;
            key_iterator(const key_iterator& other) = default;
            key_iterator& operator=(const key_iterator& other) = default;
            key_iterator operator+(int j) const { return key_iterator(m_base+1); }
            key_iterator &operator++() { ++m_base; return *this; }
            key_iterator operator++(int) { key_iterator result(*this); ++m_base; return result; }
            key_iterator &operator+=(int j) { m_base += j; return *this; }
            key_iterator operator-(int j) const { return key_iterator(m_base - j); }
            key_iterator &operator--() { --m_base; return *this; }
            key_iterator operator--(int) { key_iterator result(*this); --m_base; return result; }
            key_iterator &operator-=(int j) { m_base -= j; return *this; }
            const_iterator base() const { return m_base; }
            const KeyType& operator*() const { return m_base.key(); }
            const KeyType* operator->() const { return &(m_base.key()); }
            bool operator!=(const key_iterator &other) const { return !operator==(other); }
            bool operator==(const key_iterator &other) const { return m_base == other.m_base; }
        };
        HugeContainer(const NormalStdContaineType& list)
            :HugeContainer()
        {
            const auto listBegin = std::begin(list);
            const auto listEnd = std::end(list);
            auto prevIter = std::begin(list);
            for (auto i = listBegin; i != listEnd; ++i) {
                if (contains(i->first))
                    continue;
                insert(i->first, i->second);
                prevIter = i;
            }
        }
        HugeContainer(std::initializer_list<std::pair<KeyType, ValueType> > list)
            :HugeContainer()
        {
            const auto listBegin = std::begin(list);
            const auto listEnd = std::end(list);
            auto prevIter = std::begin(list);
            for (auto i = listBegin; i != listEnd; ++i) {
                if (contains(i->first))
                    continue;
                insert(i->first, i->second);
                prevIter = i;
            }
        }
        HugeContainer(const NormalContaineType& list)
            :HugeContainer()
        {
            for (auto i = list.constBegin(); i != list.constEnd(); ++i) {
                if (contains(i.key()))
                    continue;
                insert(i.key(), i.value());
            }
        }
        QList<KeyType> keys() const
        {
            return m_d->m_itemsMap->keys();
        }
        QList<KeyType> uniqueKeys() const
        {
            Q_ASSERT(m_d->m_itemsMap->keys() == m_d->m_itemsMap->uniqueKeys());
            return keys();
        }
        int maxCache() const {
            return m_d->m_maxCache;
        }
        bool setMaxCache(int val) { 
            val = qMax(1, val);
            if (val == m_d->m_maxCache)
                return true;
            m_d.detach();
            if (val < m_d->m_cache->size()) {
                if (!saveQueue(m_d->m_cache->size() - val))
                    return false;
            }
            m_d->m_maxCache = val;
            return true;
        }
        
        void swap(HugeContainer<KeyType, ValueType, sorted>& other) Q_DECL_NOTHROW{
            std::swap(m_d, other.m_d);
        }

        bool remove(const KeyType& key)
        {
            if (!contains(key))
                return false;
            m_d.detach();
            auto itemIter = m_d->m_itemsMap->find(key);
            Q_ASSERT(itemIter != m_d->m_itemsMap->end());
            m_d->m_cache->removeAll(key);
            if (!itemIter->isAvailable())
                removeFromMap(itemIter->fPos());
            m_d->m_itemsMap->erase(itemIter);
            return true;
        }
        iterator insert(const KeyType &key, const ValueType &val)
        {
            m_d.detach();
            auto tempval = std::make_unique<ValueType>(val);
            if (enqueueValue(key, tempval))
                return find(key);
            return end();
        }
        iterator insert(const KeyType &key, ValueType* val)
        {
            if(!val)
                return end();
            m_d.detach();
            std::unique_ptr<ValueType> tempval(val);
            if (enqueueValue(key, tempval))
                return find(key);
            return end();
        }
        const KeyType& key(const ValueType& val) const{
            const auto itemMapEnd = m_d->m_itemsMap->constEnd();
            for (auto i = m_d->m_itemsMap->constBegin(); i != itemMapEnd; ++i) {
                if (value(i.key()) == val)
                    return i.key();
            }
            Q_UNREACHABLE();
        }
        KeyType key(const ValueType& val, const KeyType& defaultKey) const
        {
            const auto itemMapEnd = m_d->m_itemsMap->constEnd();
            for (auto i = m_d->m_itemsMap->constBegin(); i != itemMapEnd; ++i) {
                if (value(i.key()) == val)
                    return i.key();
            }
            return defaultKey;
        }
        void clear()
        {
            if (isEmpty())
                return;
            m_d.detach();
            if (!m_d->m_device->resize(0)) {
                Q_ASSERT_X(false, "HugeContainer::HugeContainer", "Unable to resize temporary file");
            }
            m_d->m_itemsMap->clear();
            m_d->m_memoryMap->clear();
            m_d->m_memoryMap->insert(0, true);
            m_d->m_cache->clear();
        }

        ValueType value(const KeyType& key, const ValueType& defaultValue) const{
            if (contains(key))
                return value(key);
            return defaultValue;
        }
        const ValueType& value(const KeyType& key) const
        {
            auto valueIter = m_d->m_itemsMap->find(key);
            Q_ASSERT(valueIter != m_d->m_itemsMap->end());
            if(!valueIter->isAvailable()){
                auto result = valueFromBlock(key);
                Q_ASSERT(result);
                const bool enqueueRes = enqueueValue(key, result);
                Q_ASSERT(enqueueRes);
            }
            else {
                m_d->m_cache->removeAll(key);
                m_d->m_cache->enqueue(key);
            }
            return *(valueIter->val());
        }
        
        ValueType& operator[](const KeyType& key){
            m_d.detach();
            auto valueIter = m_d->m_itemsMap->find(key);
            if (valueIter == m_d->m_itemsMap->end()){
                insert(key, ValueType());
                valueIter = m_d->m_itemsMap->find(key);
            }
            Q_ASSERT(valueIter != m_d->m_itemsMap->end());
            if (!valueIter->isAvailable()) {
                auto result = valueFromBlock(key);
                Q_ASSERT(result);
                const bool enqueueRes = enqueueValue(key, result);
                Q_ASSERT(enqueueRes);
            }
            else {
                m_d->m_cache->removeAll(key);
                m_d->m_cache->enqueue(key);
            }
            return *(valueIter->val());
        }
        ValueType operator[](const KeyType& key) const{
            if(contains(key))
                return value(key);
            return ValueType{};
        }
        int compressionLevel() const { return m_compressionLevel; }
        bool setCompressionLevel(int val) { 
            if (m_d->m_compressionLevel == val || val < -1 || val>9)
                return false;
            m_d.detach();
            defrag(m_d->m_compressionLevel != 0, val);
            m_compressionLevel = val; 
        }
        bool unite(const HugeContainer<KeyType,ValueType,sorted>& other, bool overWrite = false){
            if (other.isEmpty())
                return true;
            if(isEmpty()){
                operator=(other);
                return true;
            }
            bool neverDetatched = true;
            const auto endItemMap = other.m_d->m_itemsMap->constEnd();
            for (auto oterItmIter = other.m_d->m_itemsMap->constBegin(); oterItmIter != endItemMap;++oterItmIter){
                auto currItmIter = m_d->m_itemsMap->find(oterItmIter.key());
                if (currItmIter != m_d->m_itemsMap->end() && !overWrite)
                    continue;
                if(neverDetatched){
                    m_d.detach();
                    currItmIter = m_d->m_itemsMap->find(oterItmIter.key());
                    neverDetatched = false;
                }
                if (oterItmIter->isAvailable()) {
                    if (currItmIter != m_d->m_itemsMap->end()) { // contains(i.key())
                        if (m_d->m_cache->contains(oterItmIter.key())) {
                            Q_ASSERT(currItmIter->isAvailable());
                            currItmIter->setVal(new ValueType(*(oterItmIter->val())));
                        }
                        else {
                            Q_ASSERT(!currItmIter->isAvailable());
                            const qint64 newPos = writeElementInMap(*(oterItmIter->val()));
                            if (newPos >= 0)
                                removeFromMap(currItmIter->fPos());
                            else
                                return false;
                            currItmIter->setFPos(newPos);
                        }
                    }
                    else{
                        const qint64 newPos = writeElementInMap(*(oterItmIter->val()));
                        if (newPos >= 0)
                            m_d->m_itemsMap->insert(oterItmIter.key(), ContainerObject<ValueType>(newPos));
                        else
                            return false;
                    }
                }
                else{
                    if (currItmIter != m_d->m_itemsMap->end()) {
                        if (currItmIter->isAvailable()) {
                            Q_ASSERT(m_d->m_cache->contains(oterItmIter.key()));
                            auto newVal = other.valueFromBlock(oterItmIter.key());
                            if (!newVal)
                                return false;
                            currItmIter->setVal(newVal.release());
                        }
                        else{
                            Q_ASSERT(!currItmIter->isAvailable());
                            const qint64 newPos = writeInMap(other.readBlock(oterItmIter.key()));
                            if (newPos >= 0)
                                removeFromMap(currItmIter->fPos());
                            else
                                return false;
                            currItmIter->setFPos(newPos);
                        }
                        
                    }
                    else{
                        const qint64 newPos = writeInMap(other.readBlock(oterItmIter.key()));
                        if (newPos >= 0)
                            m_d->m_itemsMap->insert(oterItmIter.key(), ContainerObject<ValueType>(newPos));
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
        qint64 fileSize() const{
            return m_d->m_device->size();
        }
        bool isEmpty() const
        {
            return m_d->m_itemsMap->isEmpty();
        }
        bool empty() const
        {
            return isEmpty();
        }
        key_iterator keyBegin() const{
            return key_iterator(constBegin());
        }
        key_iterator keyEnd() const
        {
            return key_iterator(constEnd());
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
        const_iterator find(const KeyType& val) const
        {
            return const_iterator(this, m_d->m_itemsMap->find(val));
        }
        const_iterator constFind(const KeyType& val) const
        {
            return const_iterator(this, m_d->m_itemsMap->constFind(val));
        }
        iterator erase(iterator pos)
        {
            Q_ASSERT(pos.m_container == this);
            if (pos == end())
                return pos;
            m_d.detach();
            const auto result = pos + 1;
            remove(pos.key());
            return result;
        }
        ValueType take(const KeyType& key)
        {
            if(!contains(key))
                return ValueType();
            const ValueType result = value(key);
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
        HugeContainer& operator=(const HugeContainer& other) = default;
        HugeContainer& operator=(HugeContainer&& other) Q_DECL_NOTHROW{
            swap(other);
            return *this;
        }
        NormalStdContaineType toStdContainer() const{
            NormalStdContaineType result;
            for (auto i = constBegin(); i != constEnd(); ++i)
                result.insert(std::make_pair(i.key(), i.value()));
            return result;
        }
        NormalContaineType toQContainer() const{
            NormalContaineType result;
            for (auto i = constBegin(); i != constEnd(); ++i)
                result.insert(i.key(), i.value());
            return result;
        }
        QList<ValueType> values() const
        {
            QList<ValueType> result;
            for (auto i = constBegin(); i != constEnd(); ++i)
                result.append(i.value());
            return result;
        }
        double fragmentation() const{
            if (m_d->m_memoryMap->size() <= 1)
                return 0.0;
            qint64 result = 0.0;
            const auto endIter = m_d->m_memoryMap->constEnd() - 1;
            for (auto i = m_d->m_memoryMap->constBegin(); i != endIter ; ++i) {
                if (i.value())
                    result += (i + 1).key() - i.key();
            }
            return static_cast<double>(result) / static_cast<double>(endIter.key());
        }
        
        bool defrag(){
            return defrag(m_d->m_compressionLevel != 0, m_d->m_compressionLevel);
        }
        bool operator==(const HugeContainer<KeyType, ValueType,sorted>& other)const{
            if(size()!=other.size())
                return false;
            const auto itmMapEnd = m_d->m_itemsMap->constEnd();
            for (auto i = m_d->m_itemsMap->constBegin(); i != itmMapEnd; ++i) {
                if (!other.contains(i.key()))
                    return false;
            }
            // Compare values as last resort
            for (auto i = m_d->m_itemsMap->constBegin(); i != itmMapEnd; ++i) {
                if (!(other.value(i.key()) == value(i.key())))
                    return false;
            }
            return true;
        }
        bool operator!=(const HugeContainer<KeyType, ValueType, sorted>& other)const{
            return !operator==(other);
        }
        virtual ~HugeContainer() = default;
        using difference_type = qptrdiff;
        using key_type = KeyType;
        using mapped_type = ValueType;
        using size_type = int;
        template<class KeyType, class ValueType, bool sorted>
        friend QDataStream& (::operator<<)(QDataStream &out, const HugeContainers::HugeContainer<KeyType, ValueType, sorted>& cont);
        template<class KeyType, class ValueType, bool sorted>
        friend QDataStream& (::operator>>)(QDataStream &in, HugeContainers::HugeContainer<KeyType, ValueType, sorted>& cont);
    };
    template <class KeyType, class ValueType>
    using HugeMap = HugeContainer<KeyType, ValueType, true>;
    template <class KeyType, class ValueType>
    using HugeHash = HugeContainer<KeyType, ValueType, false>;
}
template<class KeyType, class ValueType, bool sorted>
QDataStream& operator<<(QDataStream &out, const HugeContainers::HugeContainer<KeyType, ValueType, sorted>& cont){
    bool sameVersion;
    {
        QDataStream temp;
        sameVersion = temp.version() == out.version();
    }
    out << static_cast<qint32>(cont.size());
    const auto itmEnd = cont.m_d->m_itemsMap->constEnd();
    for (auto i = cont.m_d->m_itemsMap->constBegin(); i != itmEnd;++i){
        out << i.key();
        if (i->isAvailable()) {
            out << *(i->val());
        }
        else{
            const QByteArray block = cont.readBlock(i.key());
            if (sameVersion) {
                out.writeRawData(block.constData(), block.size());
            }
            else {
                ValueType result;
                QDataStream readerStream(block);
                readerStream >> result;
                out << result;
            }
        }
    }
    return out;
}
template<class KeyType, class ValueType, bool sorted>
QDataStream& operator>>(QDataStream& in, HugeContainers::HugeContainer<KeyType, ValueType, sorted>& cont)
{
    KeyType tempKey;
    ValueType tempVal;
    qint32 tempSize;
    for (in >> tempSize; tempSize > 0; --tempSize) {
        in >> tempKey >> tempVal;
        cont.insert(tempKey, tempVal);
    }
    return in;
}
template<class KeyType, class ValueType, bool sorted>
QDebug operator<< (QDebug d, const HugeContainers::HugeContainer<KeyType, ValueType, sorted>& cont)
{
    d << "HugeContainer, size: " << cont.size() << " Elements: \n";
    const auto endIter = cont.constEnd();
    for (auto i = cont.constBegin(); i != endIter; ++i)
        d << "Key: " << i.key() << "; Value: " << i.value() << '\n';
    return d;
}
Q_DECLARE_METATYPE_TEMPLATE_2ARG(HugeContainers::HugeHash)
Q_DECLARE_METATYPE_TEMPLATE_2ARG(HugeContainers::HugeMap)
#endif // hugecontainer_h__