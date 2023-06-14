#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QWidget>

class FileSystem : public QWidget
{
    Q_OBJECT
public:
    explicit FileSystem(QWidget *parent = nullptr);

    static QStringList getMounts();
    static QStringList getDirectories(QStringList);
    static QStringList getTypes(QStringList);
    static QString convert(double);
    static QStringList parseMountNames(QStringList);
    static struct statvfs getDiskInfo(QString);

signals:

public slots:
};

#endif // FILESYSTEM_H
