#ifndef PROCESSESWINDOW_H
#define PROCESSESWINDOW_H

#include <QWidget>

namespace Ui {
class ProcessesWindow;
}

class ProcessesWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessesWindow(QWidget *parent = nullptr);
    ~ProcessesWindow();

private:
    Ui::ProcessesWindow *ui;
};

#endif // PROCESSESWINDOW_H
