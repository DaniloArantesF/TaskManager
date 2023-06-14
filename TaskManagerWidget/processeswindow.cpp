#include "processeswindow.h"
#include "ui_processeswindow.h"

ProcessesWindow::ProcessesWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProcessesWindow)
{
    ui->setupUi(this);
}

ProcessesWindow::~ProcessesWindow()
{
    delete ui;
}
