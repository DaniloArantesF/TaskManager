#include "system.h"
#include "filesystem.h"

#include <stdlib.h>
#include <QtGlobal>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QFrame>
#include <QVBoxLayout>
#include <QDebug>
#include <sys/statvfs.h>

System::System(QWidget *parent) : QWidget(parent) {
    auto mainFrame = new QFrame();
    auto layout = new QVBoxLayout();
    mainFrame->setLayout(layout);

    {
        auto label = new QLabel(getOS());
        auto font = label->font();
        font.setPointSize(25);
        font.setBold(true);
        label->setFont(font);
        layout->addWidget(label);
    }

    {
        auto label = new QLabel("Release: " + getRelease());
        layout->addWidget(label);
    }

    {
        auto label = new QLabel("Kernel: " + getKernel());
        layout->addWidget(label);
    }

    {
        auto label = new QLabel("\nHardware");
        auto font = label->font();
        font.setPointSize(15);
        font.setBold(true);
        label->setFont(font);
        layout->addWidget(label);
    }

    {
        QString memory = getMemory();
        auto label = new QLabel("Memory: " + FileSystem::convert(memory.toDouble() * 1024));
        layout->addWidget(label);
    }

    {
        auto label = new QLabel("Processor: " + getProcessor());
        layout->addWidget(label);
    }

    {
        auto label = new QLabel("\nSystem Status");
        auto font = label->font();
        font.setPointSize(15);
        font.setBold(true);
        label->setFont(font);
        layout->addWidget(label);
    }

    {
        auto label = new QLabel("Available Disk space: " + getDiskSpace());
        layout->addWidget(label);
    }
    //mainFrame->setMaximumHeight(150);
    parent->layout()->setAlignment(Qt::AlignTop);
    parent->layout()->addWidget(mainFrame);
}

QString System::getMemory() {
    QString memory;
    QFile inputFile(QString("/proc/meminfo"));
    inputFile.open(QIODevice::ReadOnly);
    if (!inputFile.isOpen()) {
        memory = "error";
    }
    QTextStream stream(&inputFile);
    QString line = stream.readLine();
    while (!line.isNull()) {
        QStringList list = line.split("        ");
        if (QString::compare(list[0], "MemTotal:") == 0) {
            auto list2 = list[1].split(" ");
            memory = list2[0];
            break;
        }
        line = stream.readLine();
    }
    return memory;
}

QString System::getOS() {
    QFile inputFile(QString("/proc/version_signature"));
    inputFile.open(QIODevice::ReadOnly);
    QTextStream stream(&inputFile);
    QString line = stream.readLine();
    QStringList list = line.split(' ');
    //qDebug("%s", qUtf8Printable(list[1]));
    return list[0];
}

QString System::getKernel() {
    QFile inputFile(QString("/proc/version"));
    inputFile.open(QIODevice::ReadOnly);
    QTextStream stream(&inputFile);
    QString line = stream.readLine();
    QStringList list = line.split(' ');
    return list[2];
}

QString System::getProcessor() {
    QFile inputFile(QString("/proc/cpuinfo"));
    inputFile.open(QIODevice::ReadOnly);
    QTextStream stream(&inputFile);
    QString line = stream.readLine();
    QString processor;
    while (!line.isNull()) {
        QStringList list = line.split(": ");
        if (list[0].contains("model name")) {
            processor = list[1];
            break;
        }
        line = stream.readLine();
    }
    return processor;
}

QString System::getRelease() {
    QFile inputFile(QString("/etc/issue"));
    inputFile.open(QIODevice::ReadOnly);
    QTextStream stream(&inputFile);
    QString line = stream.readLine();
    QStringList list = line.split(' ');
    return list[1];
}

QString System::getDiskSpace() {
    QStringList mounts = FileSystem::getMounts();
    QStringList directs = FileSystem::getDirectories(mounts);
    double sum = 0;
    int count = mounts.count();
    for(int i = 0; i < count; i++) {
        struct statvfs stat = FileSystem::getDiskInfo(directs[i]);
        double block_size = static_cast<double>(stat.f_bsize);
        double avail = static_cast<double>(stat.f_bavail);
        sum += (block_size * avail);
    }
    return QString(FileSystem::convert(sum));
}
