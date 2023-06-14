#include "timeseries.h"

TimeSeries::TimeSeries() {
    series = new QSplineSeries();
}

TimeSeries::~TimeSeries() {
    delete series;
}

double TimeSeries::max() {
    double max = 0;
    for (auto p : points)
        max = std::max(p.y(), max);
    return max;
}

void TimeSeries::addPoint(double val, double shift) {
    int i = 0;
    for (; i < points.size(); i++) {
        auto p = points[i];
        p.setX(p.x() + shift);
        if (p.x() > maxX)
            break;
        points[i] = p;
    }
    if (i != points.size())
        points.resize(i);
    points.push_front(QPointF(0, val));
    series->replace(points);
}
