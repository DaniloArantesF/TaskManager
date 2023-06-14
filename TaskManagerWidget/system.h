#ifndef SYSTEM_H
#define SYSTEM_H

#include <QWidget>

class System : public QWidget
{
    Q_OBJECT
public:
    explicit System(QWidget *parent = nullptr);

    static QString getMemory();
    static QString getOS();
    static QString getKernel();
    static QString getProcessor();
    static QString getRelease();
    static QString getDiskSpace();

signals:

public slots:
};

#endif // SYSTEM_H
