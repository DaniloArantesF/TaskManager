#include "filesystem.h"
#include <QLabel>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QString>
#include <QGridLayout>
#include <QSizePolicy>
#include <sys/statvfs.h>

FileSystem::FileSystem(QWidget *parent) : QWidget(parent)
{
    auto frame = new QFrame;
    auto layout = new QGridLayout;
    frame->setLayout(layout);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QStringList mounts = getMounts();
    QStringList directs = getDirectories(mounts);
    QStringList types = getTypes(mounts);
    int count = mounts.count();
    int i = 0;

    /* set up top row of output */

    QStringList columHeads = {"Device", "Directory", "Type", "Total", "Free", "Available", "Used", "Percentage Used"};
    for(int j = 0; j < 8; j++) {
        auto label = new QLabel(columHeads[j]);
        label->setStyleSheet("border: 1px solid black");
        layout->addWidget(label, 0, j);
    }

    while (i < count) {

        /* get sizes from statvfs */

        struct statvfs stat = getDiskInfo(directs[i]);
        double block_size = static_cast<double>(stat.f_bsize);
        double size = static_cast<double>(stat.f_blocks);
        double avail = static_cast<double>(stat.f_bavail);
        double free = static_cast<double>(stat.f_bfree);
        int percent = static_cast<int>(((size - free) / size) * 100);

        /* create labels */

        auto label1 = new QLabel(mounts[i]);
        auto label2 = new QLabel(directs[i]);
        auto label3 = new QLabel(types[i]);
        auto label4 = new QLabel(convert(block_size * size));
        auto label5 = new QLabel(convert(block_size * free));
        auto label6 = new QLabel(convert(block_size * avail));
        auto label7 = new QLabel(convert((block_size * size) - (block_size * free)));
        auto label8 = new QLabel(QString::number(percent) + "%");

        /* add labels to layout */

        layout->addWidget(label1, i + 1, 0);
        layout->addWidget(label2, i + 1, 1);
        layout->addWidget(label3, i + 1, 2);
        layout->addWidget(label4, i + 1, 3);
        layout->addWidget(label5, i + 1, 4);
        layout->addWidget(label6, i + 1, 5);
        layout->addWidget(label7, i + 1, 6);
        layout->addWidget(label8, i + 1, 7);
        i++;
    }
    parent->layout()->setAlignment(Qt::AlignTop);
    parent->layout()->addWidget(frame);
}

struct statvfs FileSystem::getDiskInfo(QString path) {
    struct statvfs *stat = new struct statvfs();
    if(statvfs(qPrintable(path), stat) != 0) {
        //error
    }
    struct statvfs r = *stat;
    delete stat;
    return r;
}

QStringList FileSystem::getMounts() {
    QStringList mounts;
    QFile inputFile(QString("/proc/mounts"));
    inputFile.open(QIODevice::ReadOnly);
    if (!inputFile.isOpen()) {
    }
    QTextStream stream(&inputFile);
    QString line = stream.readLine();
    while (!line.isNull()) {
        QStringList list = line.split(" ");
        if (list[0].contains("/dev")) {
            if (!list[0].contains("loop")) {
                mounts.append(list[0]);
            }
        }
        line = stream.readLine();
    }

    return mounts;
}

QStringList FileSystem::getDirectories(QStringList mounts) {
    QStringList directs;
    QFile inputFile(QString("/proc/mounts"));
    inputFile.open(QIODevice::ReadOnly);
    if (!inputFile.isOpen()) {
    }
    QTextStream stream(&inputFile);
    QString line = stream.readLine();
    int i = 0;
    while (!line.isNull()) {
        QStringList list = line.split(" ");
        if (QString::compare(list[0], mounts[i]) == 0) {
            i++;
            //qDebug("%d", i);
            directs.append(list[1]);
            //qDebug("%s", qUtf8Printable(types[i - 1]));
            if (i == mounts.count()) {
                break;
            }
        }
        line = stream.readLine();
    }
    return directs;
}

QStringList FileSystem::getTypes(QStringList mounts) {
    QStringList types;
    QFile inputFile(QString("/proc/mounts"));
    inputFile.open(QIODevice::ReadOnly);
    if (!inputFile.isOpen()) {
    }
    QTextStream stream(&inputFile);
    QString line = stream.readLine();
    int i = 0;
    while (!line.isNull()) {
        QStringList list = line.split(" ");
        if (QString::compare(list[0], mounts[i]) == 0) {
            i++;
            //qDebug("%d", i);
            types.append(list[2]);
            //qDebug("%s", qUtf8Printable(types[i - 1]));
            if (i == mounts.count()) {
                break;
            }
        }
        line = stream.readLine();
    }
    return types;
}

QString FileSystem::convert(double size) {
    QStringList convert = {"B", "KB", "MB", "GB", "TB"};
    int count = 0;
    while(size >= 1024) {
        count++;
        size /= 1024;
    }
    return QString::number(size, 'f', 1) + " " + convert[count];
}

QStringList FileSystem::parseMountNames(QStringList mounts) {
    QStringList mountNames;
    int size = mounts.count();
    for (int i = 0; i < size; i++) {
        int j = mounts[i].length() - 1;
        int count = 0;
        while (mounts[i].at(j) != "/") {
            count++;
            j--;
        }
        QString temp = mounts[i].right(count);
        mountNames.append(temp);
    }
    return mountNames;
}
