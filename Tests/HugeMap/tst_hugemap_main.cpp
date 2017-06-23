



/*
#include "tst_hugemap.h"
#include <QtTest>
QTEST_APPLESS_MAIN(tst_HugeMap)
*/

#include "..\..\hugecontainer.h"
#include <QString>
void testingFunc(){
    HugeContainers::HugeMap<int, QString> tempMap;
    while (tempMap.size() < 10000)
        tempMap.insert(tempMap.size(), QStringLiteral("testing"));
    return;
}
int main(){
    int a = 0;
    testingFunc();
    return a;
}


/*
#include <hugecontainer.h>
#include <QDebug>
#include "windows.h"
#include "psapi.h"

SIZE_T getUsedMemory()
{
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
}

class MBdata{
    quint8 m_data[1024];
public:
    MBdata(quint8 val = 0)
    {
        std::fill(std::begin(m_data), std::end(m_data), val);
    }
    MBdata(const MBdata& other){
        std::copy(std::begin(other.m_data), std::end(other.m_data), std::begin(m_data));
    }
    friend QDataStream& operator<<(QDataStream& steram, const MBdata& target);
    friend QDataStream& operator>>(QDataStream& stream, MBdata& target);
};
QDataStream& operator<<(QDataStream& steram, const MBdata& target)
{
    for (quint8 val : target.m_data)
        steram << val;
    return steram;
}
QDataStream& operator>>(QDataStream& stream, MBdata& target)
{
    for (auto i = std::begin(target.m_data); i != std::end(target.m_data);++i){
        quint8 val;
        return stream >> val;
        *i = val;
    }
    return stream;
}

#include <QCoreApplication>
#include <QTimer>
int main(int argc, char *argv[])
{
    using namespace HugeContainers;
    cleanUp();
    QCoreApplication app(argc, argv);
    HugeMap<int, MBdata> testMap;
    testMap.setMaxCache(200);
    QFile resFile("MemResult.txt");
    resFile.open(QFile::WriteOnly | QFile::Text);
    QTextStream resStream(&resFile);
    auto origMem = getUsedMemory();
    auto currMem = origMem;
    auto cacheMem = origMem;
    
    QTimer addTimer;
    addTimer.setInterval(20);
    int i = 0;
    QTimer::connect(&addTimer, &QTimer::timeout, [&]()->void {
        if (i == 0) {
            origMem = getUsedMemory();
            resStream << 0 << '\t' << origMem << '\n';
        }
        testMap.setValue(i, MBdata(i & 0xFF));
        currMem = getUsedMemory();
        if (i < testMap.maxCache())
            cacheMem = currMem;
        resStream << i + 1 << '\t' << currMem << '\n';
        qDebug() << i + 1 << '\t' << (cacheMem - origMem) - (currMem - cacheMem);
        if (i>0 && (currMem - cacheMem >= cacheMem - origMem))
            QCoreApplication::quit();
        ++i;
    });
    addTimer.start();

  
    return app.exec();
}

*/