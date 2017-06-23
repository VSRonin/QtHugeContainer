
#include "../../hugecontainer.h"
#include <QDebug>
#include "windows.h"
#include "psapi.h"
#include <QCoreApplication>
#include <QFile>
#include <QTimer>
#include <QXmlStreamWriter>

double getUsedMemory()
{
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return static_cast<double>(pmc.WorkingSetSize) / static_cast<double>(1024 * 1024);
}

class MBdata
{
    quint8* m_data;
public:
    ~MBdata() { delete[] m_data; }
    MBdata(quint8 val = 0)
        :m_data(new quint8[1024 * 1024])
    {
        std::fill(m_data, m_data + (1024 * 1024), val);
    }
    MBdata(const MBdata& other)
        :m_data(new quint8[1024 * 1024])
    {
        std::copy(other.m_data, other.m_data + (1024 * 1024), m_data);
    }
    friend QDataStream& operator<<(QDataStream& steram, const MBdata& target);
    friend QDataStream& operator>>(QDataStream& stream, MBdata& target);
};
QDataStream& operator<<(QDataStream& steram, const MBdata& target)
{
    for (int i = 0; i < (1024 * 1024); ++i)
        steram << target.m_data[i];
    return steram;
}
QDataStream& operator>>(QDataStream& stream, MBdata& target)
{
    for (auto i = target.m_data; i != target.m_data + (1024 * 1024); ++i) {
        quint8 val;
        return stream >> val;
        *i = val;
    }
    return stream;
}


int main(int argc, char *argv[])
{
    using namespace HugeContainers;
    cleanUp();
    QCoreApplication app(argc, argv);
    HugeMap<int, MBdata> testMap;
    testMap.setMaxCache(200);
    QFile resFile("MemResult.html");
    resFile.open(QFile::WriteOnly);
    QXmlStreamWriter writer(&resFile);
    writer.writeDTD(QStringLiteral("<!DOCTYPE html>"));
    writer.writeStartElement(QStringLiteral("html"));
    writer.writeStartElement(QStringLiteral("head"));
    writer.writeStartElement(QStringLiteral("script"));
    writer.writeAttribute(QStringLiteral("type"), QStringLiteral("text/javascript"));
    writer.writeAttribute(QStringLiteral("src"), QStringLiteral("https://www.gstatic.com/charts/loader.js"));
    writer.writeCharacters(QString());
    writer.writeEndElement(); //script
    writer.writeStartElement(QStringLiteral("script"));
    writer.writeAttribute(QStringLiteral("type"), QStringLiteral("text/javascript"));
    writer.writeCharacters(
        "google.charts.load('current', {'packages':['corechart']});"
        "google.charts.setOnLoadCallback(drawChart);"
        "function drawChart() {"
        "var data = google.visualization.arrayToDataTable(["
        "['Count','RAM Used','HD Used']"
        );

    double origMem = 0;
    double currMem = 0;
    QTimer addTimer;
    addTimer.setInterval(50);
    int i = 0;
    QTimer::connect(&addTimer, &QTimer::timeout, [&]()->void {
        if (i == 0) {
            origMem = getUsedMemory();
        }
        testMap.insert(i, MBdata(i & 0xFF));
        currMem = getUsedMemory();
        writer.writeCharacters(",[" + QString::number(i + 1) + ',' + QString::number(currMem - origMem) + ',' + QString::number(static_cast<double>(testMap.fileSize()) / static_cast<double>(1024 * 1024)) + ']');
        qDebug() << i + 1 << '\t' << currMem - origMem;
        if (++i >= 1000)
            QCoreApplication::quit();
    });
    addTimer.start();
    app.exec();
    writer.writeCharacters(
        "]);"
        "var options = {"
        "title: 'HugeMap memory usage per container size',"
        "width: 900,"
        "height: 500,"
        "vAxes: {0: {title: 'Memory Used (MB)'}},"
        "hAxes: {0: {title: 'Container Size'}},"
        "legend: { position: 'bottom' }"
        "};"
        "var chart = new google.visualization.LineChart(document.getElementById('memorychart'));"
        "chart.draw(data, options);}"
        );
    writer.writeEndElement(); //script
    writer.writeEndElement(); //head
    writer.writeStartElement(QStringLiteral("body"));
    writer.writeStartElement(QStringLiteral("h1"));
    writer.writeCharacters(QStringLiteral("HugeMap Memory Benchmark"));
    writer.writeEndElement(); //h1
    writer.writeStartElement(QStringLiteral("p"));
    writer.writeCharacters(QStringLiteral("The chart below shows the RAM and Hard-Disk memory consumption of HugeMap as a function of the container size."));
    writer.writeEndElement(); //p
    writer.writeStartElement(QStringLiteral("p"));
    writer.writeCharacters(QStringLiteral("This example uses a HugeMap<int, MBdata> with a cache size of 200"));
    writer.writeEmptyElement(QStringLiteral("br"));
    writer.writeCharacters("MBdata is basically a wrapper around char[1048576] which is 1MB in size.");
    writer.writeEndElement(); //p
    writer.writeStartElement(QStringLiteral("div"));
    writer.writeAttribute(QStringLiteral("id"), QStringLiteral("memorychart"));
    //writer.writeAttribute(QStringLiteral("style"), QStringLiteral("width: 900px; height: 500px"));
    writer.writeEndElement(); //div
    writer.writeEndElement(); //body
    writer.writeEndElement(); //html
    writer.writeEndDocument();
    return 0;
}
