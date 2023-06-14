#include "processes.h"
#include "processeswindow.h"

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>
#include <math.h>
#include <time.h>

#include <QLabel>
#include <QtGlobal>
#include <QFile>
#include <QTextStream>
#include <QFrame>
#include <QVBoxLayout>
#include <QDebug>
#include <QTreeView>
#include <QTreeWidget>
#include <QTableWidget>
#include <QPushButton>

Processes::Processes(QWidget *parent) : QWidget(parent) {
    auto mainFrame = new QFrame();
    auto layout = new QVBoxLayout();
    mainFrame->setLayout(layout);

    auto *refresh_btn = new QPushButton;
    refresh_btn->setText("Refresh");
    layout->addWidget(refresh_btn);

    auto *opt_btn = new QPushButton;
    opt_btn->setText("Options");
    layout->addWidget(opt_btn);

    processesWidget = new QTreeWidget;
    processesWidget->setColumnCount(5);
    processesWidget->setHeaderLabels(QStringList({QString("System"), QString("Status"), QString("\% CPU"), QString("ID"), QString("Memory")}));

    QList<QTreeWidgetItem *> processes;

    g_processes_list = getProcesses();

    int index = 0;
    for(unsigned long i = 0; i < g_processes_list.size(); ++i) {
        QString name = QString( g_processes_list.at(i)->name.c_str());
        QString state = QString( g_processes_list.at(i)->state.c_str());
        double cpu_usage = g_processes_list.at(i)->cpu_usage;
        QString pid = QString (g_processes_list.at(i)->pid);
        std::string mem_string = "0.0";
        mem_string = std::to_string(g_processes_list.at(i)->mem);
        int pos = mem_string.find('.');
        mem_string = mem_string.substr(0, pos + 2);

        processes.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList({name , state, QString::number(cpu_usage), QString::number(g_processes_list.at(i)->pid), QString((mem_string + " MiB").c_str()), QString::number(index)})));
        index++;
    }

    processesWidget->insertTopLevelItems(0, processes);
    processesWidget->setSortingEnabled(true);

    //Refresh Button
    connect(refresh_btn, SIGNAL(clicked()), this, SLOT(OnRefresh()));
    //Options Button
    connect(opt_btn, SIGNAL(clicked()), this, SLOT(OnRightClick()));

    connect(processesWidget, &QTreeWidget::itemDoubleClicked, this, &Processes::displayDetInfo);

    layout->addWidget(processesWidget);

    parent->layout()->setAlignment(Qt::AlignTop);
    parent->layout()->addWidget(mainFrame);
}

std::vector<Processes::process_struct *> Processes::getProcesses() {
    Processes::process_struct *curProc;
    long hz = sysconf(_SC_CLK_TCK);
    std::string line;
    double idle;
    double uptime;

    std::string comm;

    double utime;
    double stime;
    //long cutime;
    //long cstime;
    double starttime;

    std::vector<Processes::process_struct *> processes;

    //Open Kernel Stat
    std::ifstream file;
    file.open("/proc/uptime");
    getline(file, line);

    std::istringstream in(line);
    in >> uptime >> idle;

    file.close();

    //Open Currently Open Processes
    struct dirent *de;
    DIR *dr = opendir("/proc");

    if (dr == nullptr) {
        return processes;
    }

    while ((de = readdir(dr)) != nullptr) {
        std::string dir = de->d_name;
        std::string path = ("/proc/" + dir + "/stat");

        if (de->d_type == DT_DIR && access( path.c_str(), F_OK ) != -1 && de->d_name[0] != '.') {
            std::vector<std::string> info = Processes::getInfo(path);
            curProc = new process_struct();
            curProc->pid = stoi(info.at(0));

            switch (*info.at(2).begin()) {
                case 'R':
                    curProc->state = ("Running");
                    break;
                case 'S':
                    curProc->state = ("Sleeping");
                    break;
                case 'I':
                    curProc->state = ("Sleeping");
                    break;
                case 'C':
                    curProc->state = "Sleeping";
                    break;
                case 'Z':
                    curProc->state = ("Zombie");
                    break;
                case 'X':
                    curProc->state = ("Dead");
                    break;
                default:
                    curProc->state = ("Other");
                    break;
            }

            curProc->cpuTime = stol(info.at(13));
            curProc->startTime = stol(info.at(21));

            curProc->info = info;

            processes.push_back(curProc);
        }
    }

    closedir(dr);

    for (unsigned long i = 0; i < processes.size(); i++) {
        //CPU %
        double cpu_usage = 0;
        if (processes.at(i)->info.at(2)[0] == 'R') {

            utime = stol(processes.at(i)->info.at(13));
            stime = stol(processes.at(i)->info.at(14));
            starttime = processes.at(i)->startTime;

            //qInfo("ID: utime: %ld starttime: %ld hz: %ld \n", utime, starttime, hz);

            double total_time = utime + stime;
            double elapse_time = std::max<double>(uptime - (starttime / hz), 1);

            cpu_usage = 100 * ( (total_time / hz) / elapse_time);
            processes.at(i)->cpu_usage = cpu_usage;

            //qInfo("total_time: %ld elapsed: %ld \n", total_time, elapse_time);
            //qInfo("CPU Usage\n%f= 100 *  ((%ld  / %ld) / %f\n", cpu_usage, total_time, hz, elapse_time);

            /* if We want to include time in child processes
            total_time += cutime + cstime;
            */
        }

        //Name and Memory
        double mem;
        std::string status_path = "/proc/" + std::to_string(processes.at(i)->pid) + "/status";
        std::ifstream file;
        std::string line;
        std::string key;
        std::string value;

        file.open(status_path);

        while (std::getline(file, line)) {
            std::istringstream in(line);
            in >> key >> value;
            if (key.compare("Name:") == 0) {
                processes.at(i)->name = value;
            }else if (key.compare("Uid:") == 0) {
                processes.at(i)->user = value;
            }else if (key.compare("RssAnon:") == 0) {
                mem = ::atof(value.c_str());
                mem *= 0.00095367431640625;

                if( mem < 0 ){
                    mem = ceil((mem * 100) - 0.5) / 100;
                }else {
                    mem = floor((mem * 100) + 0.5) / 100;
                }
                processes.at(i)->mem = mem;
            }else if (key.compare("VmSize:") == 0) {
                mem = ::atof(value.c_str());
                mem *= 0.00095367431640625;

                if( mem < 0 ){
                    mem = ceil((mem * 100) - 0.5) / 100;
                }else {
                    mem = floor((mem * 100) + 0.5) / 100;
                }
                processes.at(i)->vmem = mem;
            }else if (key.compare("VmRSS:") == 0) {
                mem = ::atof(value.c_str());
                mem *= 0.00095367431640625;

                if( mem < 0 ){
                    mem = ceil((mem * 100) - 0.5) / 100;
                }else {
                    mem = floor((mem * 100) + 0.5) / 100;
                }
                processes.at(i)->resmem = mem;
            }else if (key.compare("RssShmem:") == 0) {
                mem = ::atof(value.c_str());
                mem *= 0.00095367431640625;

                if( mem < 0 ){
                    mem = ceil((mem * 100) - 0.5) / 100;
                }else {
                    mem = floor((mem * 100) + 0.5) / 100;
                }
                processes.at(i)->shrmem = mem;
            }

        }
        file.close();
    }
    return processes;
}

std::vector<std::string> Processes::getInfo(std::string dir) {
    std::vector<std::string> info;
    std::ifstream file;
    std::string line;
    std::string value;

    file.open(dir);

    while (std::getline(file, line)) {
        std::istringstream in(line);
        while (in >> value) {
            info.push_back(value);
        }
    }

    file.close();
    return info;
}

void Processes::stopProc() {
    if (processesWidget->selectedItems().size() == 0 || processesWidget->selectedItems().size() > 1) {
        return;
    }
    QTreeWidgetItem *item = processesWidget->selectedItems().front();

    int pid = item->text(3).toInt(nullptr, 10);
    kill(pid, SIGTERM);

    Processes::OnRefresh();
}

void Processes::killProc() {
    if (processesWidget->selectedItems().size() == 0 || processesWidget->selectedItems().size() > 1) {
        return;
    }
    QTreeWidgetItem *item = processesWidget->selectedItems().front();

    int pid = item->text(3).toInt(nullptr, 10);
    kill(pid, SIGKILL);

    Processes::OnRefresh();
}

void Processes::displayProcInfo() {
    if (processesWidget->selectedItems().size() == 0 || processesWidget->selectedItems().size() > 1) {
        return;
    }

    QTreeWidgetItem *item = processesWidget->selectedItems().front();
    int pid = item->text(3).toInt(nullptr, 10);

    int row = 0;
    QWidget *widget = new QWidget;
    auto mainFrame = new QFrame();
    QVBoxLayout *layout = new QVBoxLayout();
    mainFrame->setLayout(layout);

    QPushButton *btn =  new QPushButton;
    auto label = new QLabel("Files opened by process" + QString::number(pid));
    layout->addWidget(label);

    QTableWidget *tableWidget = new QTableWidget(20, 3, nullptr);
    tableWidget->setHorizontalHeaderLabels(QStringList({QString("FD"), QString("Type"), QString("Object")}));

    std::string path = "/proc/" + std::to_string(pid) + "/fd/";

    struct dirent *de;
    DIR *dr = opendir(path.c_str());

    if (dr == nullptr) {
        return;
    }

    while ((de = readdir(dr)) != nullptr) {
        std::string dir = de->d_name;
        char buff[1024];

        if (de->d_name[0] != '.') {
            QTableWidgetItem *cur_fd = new QTableWidgetItem(QString(de->d_name));
            cur_fd->setFlags(item->flags() &  ~Qt::ItemIsEditable);
            tableWidget->setItem(row, 0, cur_fd);

            path = "/proc/" + std::to_string(pid) + "/fd/" + de->d_name;
            ssize_t len = ::readlink(path.c_str(), buff, sizeof(buff) -1 );

            if (len != -1) {
                buff[len] = '\0';

            }else {
                buff[0] = '\0';
            }


            std::string type;
            QTableWidgetItem *cur_obj;
            if ((buff[0] == 'p' && buff[1] == 'i')) {
                cur_obj = new QTableWidgetItem(QString("pipe"));
                cur_obj->setFlags(item->flags() &  ~Qt::ItemIsEditable);
                tableWidget->setItem(row, 1, cur_obj);
            }else if ((buff[0] == 's' && buff[1] == 'o')) {
                cur_obj = new QTableWidgetItem(QString("local socket"));
                cur_obj->setFlags(item->flags() &  ~Qt::ItemIsEditable);
                tableWidget->setItem(row, 1, cur_obj);
            } else {
                cur_obj = new QTableWidgetItem(QString("file"));
                cur_obj->setFlags(item->flags() &  ~Qt::ItemIsEditable);
                tableWidget->setItem(row, 1, cur_obj);

                cur_obj = new QTableWidgetItem(QString(buff));
                cur_obj->setFlags(item->flags() &  ~Qt::ItemIsEditable);
                tableWidget->setItem(row, 2, cur_obj);
            }

            row++;
            if (row + 1 > tableWidget->columnCount()) {
                tableWidget->setRowCount(row  + 1);
            }
        }
    }

    closedir(dr);

    layout->addWidget(tableWidget);

    btn->setText("Close");
    layout->addWidget(btn);

    widget->setLayout(layout);
    widget->layout()->setAlignment(Qt::AlignTop);
    widget->layout()->addWidget(mainFrame);
    widget->show();
}

void Processes::displayMemMaps() {
    if (processesWidget->selectedItems().size() == 0 || processesWidget->selectedItems().size() > 1) {
        return;
    }

    QTreeWidgetItem *item = processesWidget->selectedItems().front();
    int pid = item->text(3).toInt(nullptr, 10);

    int row = 0;
    int col = 0;

    QWidget *widget = new QWidget;
    auto mainFrame = new QFrame();
    QVBoxLayout *layout = new QVBoxLayout();
    mainFrame->setLayout(layout);

    auto label = new QLabel("Memory maps for process" + QString::number(pid));
    layout->addWidget(label);

    QTableWidget *tableWidget = new QTableWidget(30, 6, nullptr);

    tableWidget->setHorizontalHeaderLabels(QStringList({QString("VM Start - VM End"), QString("perms"), QString("VM offset"), QString("dev"), QString("inode"), QString("Filename")}));

    std::string path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream file;
    std::string line;
    std::string value;

    file.open(path);
    QTableWidgetItem *cur_value;

    while (std::getline(file, line)) {
        col = 0;
        std::istringstream in(line);
        while (in >> value) {
            cur_value = new QTableWidgetItem(QString(value.c_str()));
            cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
            tableWidget->setItem(row, col++, cur_value);
        }

        row++;
        if (row + 1 > tableWidget->columnCount()) {
            tableWidget->setRowCount(row  + 1);
        }
    }

    file.close();

    layout->addWidget(tableWidget);
    widget->setLayout(layout);
    widget->layout()->setAlignment(Qt::AlignTop);
    widget->layout()->addWidget(mainFrame);
    widget->show();
}

void Processes::displayDetInfo(QTreeWidgetItem *item, int column) {
    QWidget *widget = new QWidget;
    auto mainFrame = new QFrame();
    QVBoxLayout *layout = new QVBoxLayout();
    mainFrame->setLayout(layout);

    int pid = item->text(3).toLong(nullptr, 10);

    QPushButton *btn =  new QPushButton;
    auto label = new QLabel("Properties or Process" + QString::number(pid));
    layout->addWidget(label);

    QTableWidget *tableWidget = new QTableWidget(8, 2, nullptr);

    QTableWidgetItem *cur_value;

    long index = item->text(5).toLong(nullptr, 10);


    Processes::process_struct *proc = g_processes_list[(unsigned long)index];


    int seconds = proc->cpuTime;
    int day = seconds / (24 * 3600);
    seconds = seconds % (24 * 3600);
    int hour = seconds / 3600;
    seconds %= 3600;
    int minutes = seconds / 60 ;
    seconds %= 60;
    std::string cpuTime = std::to_string(hour) + ":" + std::to_string(minutes) + ":" + std::to_string(seconds);

    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime(&rawtime);
    printf ("The current date/time is: %s", asctime (timeinfo) );

    tableWidget->setHorizontalHeaderLabels(QStringList({QString("Process Name"), QString(proc->name.c_str())}));

    cur_value = new QTableWidgetItem(QString("User"));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(0, 0, cur_value);

    cur_value = new QTableWidgetItem(QString(proc->user.c_str()));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(0, 1, cur_value);

    cur_value = new QTableWidgetItem(QString("State"));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(1, 0, cur_value);

    cur_value = new QTableWidgetItem(QString(proc->state.c_str()));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(1, 1, cur_value);

    cur_value = new QTableWidgetItem(QString("Memory"));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(2, 0, cur_value);

    cur_value = new QTableWidgetItem(QString::number(proc->mem)+ "MiB");
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(2, 1, cur_value);

    cur_value = new QTableWidgetItem(QString("Resident Memory"));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(3, 0, cur_value);


    cur_value = new QTableWidgetItem(QString::number(proc->resmem) + "MiB");
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(3, 1, cur_value);


    cur_value = new QTableWidgetItem(QString("Shared Memory"));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(4, 0, cur_value);


    cur_value = new QTableWidgetItem(QString::number(proc->shrmem) + "MiB");
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(4, 1, cur_value);


    cur_value = new QTableWidgetItem(QString("CPU Time"));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(5, 0, cur_value);


    cur_value = new QTableWidgetItem(QString(cpuTime.c_str()));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(5, 1, cur_value);

    cur_value = new QTableWidgetItem(QString("Started"));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(6, 0, cur_value);


    cur_value = new QTableWidgetItem(QString(std::to_string(proc->cpuTime).c_str()));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(6, 1, cur_value);

    cur_value = new QTableWidgetItem(QString("Virtual Memory"));
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(7, 0, cur_value);


    cur_value = new QTableWidgetItem(QString::number(proc->vmem) + "MiB");
    cur_value->setFlags(item->flags() &  ~Qt::ItemIsEditable);
    tableWidget->setItem(7, 1, cur_value);

    layout->addWidget(tableWidget);

    btn->setText("Close");
    layout->addWidget(btn);


    widget->setLayout(layout);
    widget->layout()->setAlignment(Qt::AlignTop);
    widget->layout()->addWidget(mainFrame);
    widget->show();
}

void Processes::OnRefresh() {
    processesWidget->clear();
    processesWidget->setColumnCount(5);
    processesWidget->setHeaderLabels(QStringList({QString("System"), QString("Status"), QString("\% CPU"), QString("ID"), QString("Memory")}));

    QList<QTreeWidgetItem *> processes;
    g_processes_list = getProcesses();

    int index = 0;
    for(unsigned long i = 0; i < g_processes_list.size(); ++i) {
        QString name = QString( g_processes_list.at(i)->name.c_str());
        QString state = QString( g_processes_list.at(i)->state.c_str());
        double cpu_usage = g_processes_list.at(i)->cpu_usage;
        QString pid = QString (g_processes_list.at(i)->pid);
        std::string mem_string = "0.0";
        mem_string = std::to_string(g_processes_list.at(i)->mem);
        int pos = mem_string.find('.');
        mem_string = mem_string.substr(0, pos + 2);

        processes.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList({name , state, QString::number(cpu_usage), QString::number(g_processes_list.at(i)->pid), QString((mem_string + " MiB").c_str()), QString::number(index)})));
        index++;
    }

    processesWidget->insertTopLevelItems(0, processes);
    processesWidget->setSortingEnabled(true);

    layout()->update();
    this->update();
}

void Processes::OnRightClick() {
    QWidget *widget = new QWidget;
    auto mainFrame = new QFrame();
    QVBoxLayout *layout = new QVBoxLayout();
    mainFrame->setLayout(layout);

    auto label = new QLabel("Options");
    layout->addWidget(label);

    QPushButton *stop =  new QPushButton;
    stop->setText("Stop");
    connect(stop, SIGNAL(clicked()), this, SLOT(stopProc()));
    layout->addWidget(stop);

    QPushButton *kill =  new QPushButton;
    kill->setText("Kill");
    connect(kill, SIGNAL(clicked()), this, SLOT(killProc()));
    layout->addWidget(kill);

    QPushButton *memMaps =  new QPushButton;
    memMaps->setText("List Memory Maps");
    connect(memMaps, SIGNAL(clicked()), this, SLOT(displayMemMaps()));
    layout->addWidget(memMaps);

    QPushButton *openFD =  new QPushButton;
    openFD->setText("List Open Files");
    connect(openFD, SIGNAL(clicked()), this, SLOT(displayProcInfo()));
    layout->addWidget(openFD);

    widget->setLayout(layout);
    widget->layout()->setAlignment(Qt::AlignTop);
    widget->layout()->addWidget(mainFrame);
    widget->show();
}



