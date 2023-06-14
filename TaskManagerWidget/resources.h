#ifndef RESOURCES_H
#define RESOURCES_H

#include "timeseries.h"

#include <vector>
#include <QtGlobal>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QValueAxis>

#define MAX_CPU_COUNT (256)
#define MONITORING_PERIOD (0.8)

using namespace QtCharts;
using namespace std;

typedef struct ProcStat {
    long cpuUsed[MAX_CPU_COUNT];
    long cpuPassed[MAX_CPU_COUNT];
    int cpuCount;
} ProcStat;

typedef struct ProcMeminfo {
    long swapMaxKB;
    long swapFreeKB;
    long memoryMaxKB;
    long memoryFreeKB;
    long memoryAvailKB;
} ProcMeminfo;

typedef struct ProcNetDev {
    long bytesSent;
    long bytesRecv;
} ProcNetDev;

class Resources : public QWidget
{
    Q_OBJECT
private:
    ProcStat lastProcStat;
    ProcMeminfo lastMeminfo;
    ProcNetDev lastNetDev;

    vector<TimeSeries*> cpuSeries;
    vector<QLabel*> cpuLabels;

    TimeSeries *memFreeSeries;
    TimeSeries *swapFreeSeries;
    QLabel *memFreeLabel;
    QLabel *swapFreeLabel;

    TimeSeries *netSentSeries;
    TimeSeries *netRecvSeries;
    QValueAxis *netChartAxis;
    QLabel *dataSentLabel;
    QLabel *dataRecvLabel;

    void initCpu(QVBoxLayout *);
    void initMem(QVBoxLayout *);
    void initNet(QVBoxLayout *);

    void updateCpu();
    void updateMem();
    void updateNet();

public:
    explicit Resources(QWidget *parent = nullptr);
    ~Resources();
    ProcStat getProcStat();
    ProcMeminfo getProcMeminfo();
    ProcNetDev getProcNetDev();

signals:

public slots:
    void timerTick();
};

#endif // RESOURCES_H
