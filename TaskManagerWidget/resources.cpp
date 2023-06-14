#include "resources.h"

#include <algorithm>
#include <cmath>
#include <stdlib.h>
#include <QtGlobal>
#include <QLabel>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFile>
#include <QTextStream>
#include <QFrame>
#include <QTimer>
#include <QValueAxis>
#include <QDebug>

using namespace  std;
using namespace QtCharts;

Resources::Resources(QWidget *parent) : QWidget(parent) {
    QFrame *mainFrame = new QFrame();
    auto mainLayout = new QVBoxLayout();
    mainFrame->setLayout(mainLayout);
    mainLayout->setMargin(0);

    lastProcStat = getProcStat();
    lastMeminfo = getProcMeminfo();
    lastNetDev = getProcNetDev();

    initCpu(mainLayout);
    {
        auto divider = new QFrame();
        divider->setStyleSheet("background-color: rgb(100, 110, 130); margin-left: 60px; margin-right: 60px;");
        divider->setMinimumHeight(2);
        mainLayout->addWidget(divider);
    }
    initMem(mainLayout);
    {
        auto divider = new QFrame();
        divider->setStyleSheet("background-color: rgb(100, 110, 130); margin-left: 60px; margin-right: 60px;");
        divider->setMinimumHeight(2);
        mainLayout->addWidget(divider);
    }
    initNet(mainLayout);

    parent->layout()->setMargin(0);
    parent->layout()->addWidget(mainFrame);

    // Start the monitoring
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTick()));
    timer->start((int)(MONITORING_PERIOD * 1000));
}

Resources::~Resources() {
    delete netRecvSeries;
    delete netSentSeries;
    delete swapFreeSeries;
    delete memFreeSeries;
    for (auto series : cpuSeries)
        delete series;
}

void Resources::updateCpu() {
    auto stat = getProcStat();

    for (size_t i = 0; i < stat.cpuCount; i++) {
        auto label = cpuLabels[i];
        auto series = cpuSeries[i];
        double percentUsage = (stat.cpuUsed[i] - lastProcStat.cpuUsed[i]) / max<double>((double)(stat.cpuPassed[i] - lastProcStat.cpuPassed[i]), 1) * 100.0;

        QString str;
        str.sprintf("CPU%ld %.1f%%", i, percentUsage);
        label->setText(str);

        series->addPoint(percentUsage, MONITORING_PERIOD);
    }

    lastProcStat = stat;
}

void Resources::updateMem() {
    auto meminfo = getProcMeminfo();

    double percentMemUsed = (1 - meminfo.memoryAvailKB / (double)(meminfo.memoryMaxKB)) * 100.0;
    double percentSwapUsed = (1 - meminfo.swapFreeKB / (double)(meminfo.swapMaxKB)) * 100.0;

    QString str;
    str.sprintf("Mem Used: %.1f%% (%ld of %ld kB)", percentMemUsed, meminfo.memoryMaxKB - meminfo.memoryAvailKB, meminfo.memoryMaxKB);
    memFreeLabel->setText(str);

    str.clear();
    str.sprintf("Swap Used: %.1f%% (%ld of %ld kB)", percentSwapUsed, meminfo.swapMaxKB - meminfo.swapFreeKB, meminfo.swapMaxKB);
    swapFreeLabel->setText(str);

    memFreeSeries->addPoint(percentMemUsed, MONITORING_PERIOD);
    swapFreeSeries->addPoint(percentSwapUsed, MONITORING_PERIOD);

    lastMeminfo = meminfo;
}

void Resources::updateNet() {
    auto net = getProcNetDev();

    double sentPerSec = (net.bytesSent - lastNetDev.bytesSent) / MONITORING_PERIOD / 1024.0;
    double recvPerSec = (net.bytesRecv - lastNetDev.bytesRecv) / MONITORING_PERIOD / 1024.0;

    QString str;
    str.sprintf("Data Sent: %.1f kB/sec", sentPerSec);
    dataSentLabel->setText(str);

    str.clear();
    str.sprintf("Data Recv: %.1f kB/sec", recvPerSec);
    dataRecvLabel->setText(str);

    netSentSeries->addPoint(sentPerSec, MONITORING_PERIOD);
    netRecvSeries->addPoint(recvPerSec, MONITORING_PERIOD);

    netChartAxis->setMax(max<double>(1024, 1.1 * max<double>(netSentSeries->max(), netRecvSeries->max())));

    lastNetDev = net;
}

void Resources::timerTick() {
    updateCpu();
    updateMem();
    updateNet();
}

void Resources::initCpu(QVBoxLayout *mainLayout) {
    QFrame *cpuFrame = new QFrame();
    auto cpuLayout = new QVBoxLayout();
    cpuFrame->setLayout(cpuLayout);
    cpuLayout->setMargin(0);
    cpuFrame->setStyleSheet("background-color: rgb(255, 255, 255);");

    QChart *chart = new QChart();
    chart->legend()->hide();

    QValueAxis *yAxis = new QValueAxis;
    yAxis->setRange(0, 100);
    yAxis->setTickCount(6);
    yAxis->setLabelFormat("%.0f%");
    auto font = yAxis->labelsFont();
    font.setPointSize(7);
    yAxis->setLabelsFont(font);
    chart->addAxis(yAxis, Qt::AlignRight);

    QValueAxis *axisX = new QValueAxis;
    axisX->setRange(0, 60);
    axisX->setTickCount(13);
    axisX->setLabelFormat("%.0fs");
    chart->addAxis(axisX, Qt::AlignBottom);

    chart->setTitle("CPU");

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    cpuLayout->addWidget(chartView);

    QFrame *legendFrame = new QFrame();
    legendFrame->setStyleSheet("margin-left: 20px; margin-right: 20px;");
    auto legendLayout = new QGridLayout();
    legendFrame->setLayout(legendLayout);
    for (int i = 0; i < lastProcStat.cpuCount; i++) {
        // Map each cpu to a hue from red to magenta.
        double hue = lastProcStat.cpuCount == 0 ? 0 : i / (double)(lastProcStat.cpuCount - 1) * 300.0 / 360.0;

        // Standard way of converting from HSV to RGB tho its annoying that qt has no vec4
        double r = hue + 1;
        r = min<double>(max<double>(abs((r - floor(r)) * 6 - 3) - 1, 0), 1);
        double g = hue + 2.0 / 3.0;
        g = min<double>(max<double>(abs((g - floor(g)) * 6 - 3) - 1, 0), 1);
        double b = hue + 1.0 / 3.0;
        b = min<double>(max<double>(abs((b - floor(b)) * 6 - 3) - 1, 0), 1);

        QString style;
        style.sprintf("background-color: rgb(%d, %d, %d);", (int)(r * 255), (int)(g * 255), (int)(b * 255));
        QFrame *legend = new QFrame();
        legend->setMinimumWidth(10);
        legend->setStyleSheet(style);
        legendLayout->addWidget(legend, i / 2, 2 * (i % 2));

        QString label;
        label.sprintf("CPU%d", i);
        auto qlabel = new QLabel(label);
        legendLayout->addWidget(qlabel, i / 2, 2 * (i % 2) + 1);

        auto series = new TimeSeries();
        series->series->setColor(QColor((int)(r * 255), (int)(g * 255), (int)(b * 255)));
        chart->addSeries(series->series);
        series->series->attachAxis(axisX);
        series->series->attachAxis(yAxis);

        cpuLabels.push_back(qlabel);
        cpuSeries.push_back(series);
    }
    cpuLayout->addWidget(legendFrame);

    mainLayout->addWidget(cpuFrame);
}

void Resources::initMem(QVBoxLayout *mainLayout) {
    QFrame *memoryFrame = new QFrame();
    auto memoryLayout = new QVBoxLayout();
    memoryFrame->setLayout(memoryLayout);
    memoryLayout->setMargin(0);
    memoryFrame->setStyleSheet("background-color: rgb(255, 255, 255);");

    QChart *chart = new QChart();
    chart->legend()->hide();

    QValueAxis *yAxis = new QValueAxis;
    yAxis->setRange(0, 100);
    yAxis->setTickCount(6);
    auto font = yAxis->labelsFont();
    font.setPointSize(7);
    yAxis->setLabelsFont(font);
    yAxis->setLabelFormat("%.0f%");
    chart->addAxis(yAxis, Qt::AlignRight);

    QValueAxis *axisX = new QValueAxis;
    axisX->setRange(0, 60);
    axisX->setTickCount(13);
    axisX->setLabelFormat("%.0fs");
    chart->addAxis(axisX, Qt::AlignBottom);

    memFreeSeries = new TimeSeries();
    memFreeSeries->series->setColor(QColor(30, 30, 200));
    chart->addSeries(memFreeSeries->series);
    memFreeSeries->series->attachAxis(axisX);
    memFreeSeries->series->attachAxis(yAxis);

    swapFreeSeries = new TimeSeries();
    swapFreeSeries->series->setColor(QColor(30, 200, 30));
    chart->addSeries(swapFreeSeries->series);
    swapFreeSeries->series->attachAxis(axisX);
    swapFreeSeries->series->attachAxis(yAxis);

    chart->setTitle("Memory");

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    memoryLayout->addWidget(chartView);

    QFrame *legendFrame = new QFrame();
    legendFrame->setStyleSheet("margin-left: 20px; margin-right: 20px;");
    auto legendLayout = new QHBoxLayout();
    legendFrame->setLayout(legendLayout);
    {
        auto legend = new QFrame();
        legend->setMinimumWidth(10);
        legendLayout->addWidget(legend);
        legend->setStyleSheet("background-color: rgb(30, 30, 200);");

        memFreeLabel = new QLabel("Memory used: 0/0 bytes");
        legendLayout->addWidget(memFreeLabel);

        legend = new QFrame();
        legend->setMinimumWidth(10);
        legendLayout->addWidget(legend);
        legend->setStyleSheet("background-color: rgb(30, 200, 30);");
        swapFreeLabel = new QLabel("Swap used: 0/0 bytes");
        legendLayout->addWidget(swapFreeLabel);
    }
    memoryLayout->addWidget(legendFrame);

    mainLayout->addWidget(memoryFrame);
}

void Resources::initNet(QVBoxLayout *mainLayout) {
    QFrame *networkFrame = new QFrame();
    auto networkLayout = new QVBoxLayout();
    networkFrame->setLayout(networkLayout);
    networkLayout->setMargin(0);
    networkFrame->setStyleSheet("background-color: rgb(255, 255, 255);");

    QChart *chart = new QChart();
    chart->legend()->hide();

    netChartAxis = new QValueAxis;
    netChartAxis->setRange(0, 1024);
    netChartAxis->setTickCount(9);
    auto font = netChartAxis->labelsFont();
    font.setPointSize(7);
    netChartAxis->setLabelsFont(font);
    netChartAxis->setLabelFormat("%.1f kB/sec");

    chart->addAxis(netChartAxis, Qt::AlignRight);

    QValueAxis *axisX = new QValueAxis;
    axisX->setRange(0, 60);
    axisX->setTickCount(13);
    axisX->setLabelFormat("%.0fs");
    chart->addAxis(axisX, Qt::AlignBottom);

    netSentSeries = new TimeSeries();
    netSentSeries->series->setColor(QColor(30, 30, 200));
    chart->addSeries(netSentSeries->series);
    netSentSeries->series->attachAxis(axisX);
    netSentSeries->series->attachAxis(netChartAxis);

    netRecvSeries = new TimeSeries();
    netRecvSeries->series->setColor(QColor(30, 200, 30));
    chart->addSeries(netRecvSeries->series);
    netRecvSeries->series->attachAxis(axisX);
    netRecvSeries->series->attachAxis(netChartAxis);

    chart->setTitle("Networking");

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    networkLayout->addWidget(chartView);

    QFrame *networkLegendFrame = new QFrame();
    networkLegendFrame->setStyleSheet("margin-left: 20px; margin-right: 20px;");
    auto networkLegendLayout = new QHBoxLayout();
    networkLegendFrame->setLayout(networkLegendLayout);
    {
        auto legend = new QFrame();
        legend->setMinimumWidth(10);
        networkLegendLayout->addWidget(legend);
        legend->setStyleSheet("background-color: rgb(30, 30, 200);");

        dataSentLabel = new QLabel("Send: 0 kB/sec");
        networkLegendLayout->addWidget(dataSentLabel);

        legend = new QFrame();
        legend->setMinimumWidth(10);
        networkLegendLayout->addWidget(legend);
        legend->setStyleSheet("background-color: rgb(30, 200, 30);");
        dataRecvLabel = new QLabel("Recv: 0 kB/sec");
        networkLegendLayout->addWidget(dataRecvLabel);
    }
    networkLayout->addWidget(networkLegendFrame);

    mainLayout->addWidget(networkFrame);
}


ProcStat Resources::getProcStat() {
    ProcStat out = { 0 };
    QFile statsFile("/proc/stat");
    if (!statsFile.open(QFile::ReadOnly | QFile::Text)) {
        qInfo("Failed to read /proc/stat");
        return out;
    }
    QTextStream stream(&statsFile);
    stream.readLine();
    out.cpuCount = 0;

    while (true) {
        QString token;
        stream >> token;
        if (!token.startsWith("cpu"))
            break;

        long sum = 0;
        long idle = 0;
        // There are atleast 7 cols in proc/stat
        for (int i = 0; i < 7; i++) {
            long read;
            stream >> read;
            sum += read;
            // The fourth column is the idle value. We store that and subtrate it from the total at the end.
            if (i == 3)
                idle = read;
        }

        // Ignore the rest of the line.
        stream.readLine();

        out.cpuPassed[out.cpuCount] = sum;
        out.cpuUsed[out.cpuCount] = sum - idle;
        out.cpuCount++;

        if (out.cpuCount >= MAX_CPU_COUNT)
            break;
    }
    return out;
}

ProcMeminfo Resources::getProcMeminfo() {
    ProcMeminfo out = { 0 };
    QFile meminfoFile("/proc/meminfo");
    if (!meminfoFile.open(QFile::ReadOnly | QFile::Text)) {
        qInfo("Failed to read /proc/meminfo");
        return out;
    }
    QTextStream stream(&meminfoFile);

    while (true) {
        QString token;
        stream >> token;
        if (stream.status() != QTextStream::Ok) break;

        if (!token.compare("MemFree:"))
            stream >> out.memoryFreeKB;
        else if (!token.compare("MemTotal:"))
            stream >> out.memoryMaxKB;
        else if (!token.compare("MemAvailable:"))
            stream >> out.memoryAvailKB;
        else if (!token.compare("SwapFree:"))
            stream >> out.swapFreeKB;
        else if (!token.compare("SwapTotal:"))
            stream >> out.swapMaxKB;
    }
    return out;
}

ProcNetDev Resources::getProcNetDev() {
    ProcNetDev out = { 0 };
    QFile netDevFile("/proc/net/dev");
    if (!netDevFile.open(QFile::ReadOnly | QFile::Text)) {
        qInfo("Failed to read /proc/net/dev");
        return out;
    }
    QTextStream stream(&netDevFile);
    // First two lines are headers. Ignore those.
    stream.readLine();
    stream.readLine();

    while (true) {
        QString token;
        stream >> token;
        if (stream.status() != QTextStream::Ok) break;

        long read;
        stream >> read;
        out.bytesRecv += read;

        // We dont care about the next 7 numbers
        for (int i = 0; i < 7; i++)
            stream >> read;

        stream >> read;
        out.bytesSent += read;

        // We dont care about the next 7 numbers
        for (int i = 0; i < 7; i++)
            stream >> read;
    }
    return out;
}
