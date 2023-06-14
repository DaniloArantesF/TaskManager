#ifndef TIMESERIES_H
#define TIMESERIES_H

#include <vector>
#include <QtGlobal>
#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QChartView>
#include <QValueAxis>

using namespace QtCharts;
using namespace std;

class TimeSeries {
public:
    QVector<QPointF> points;
    QSplineSeries *series;
    double maxX = 100;

    TimeSeries();
    ~TimeSeries();

    double max();
    void addPoint(double, double);
};

#endif // TIMESERIES_H
