#ifndef PROCESSES_H
#define PROCESSES_H

#include <QWidget>
#include <string>
#include <QTreeView>
#include <QTreeWidget>
#include <vector>

class Processes : public QWidget
{
    Q_OBJECT
public:
    explicit Processes(QWidget *parent = nullptr);

    typedef struct {
        int pid;
        std::string name;
        std::string user;
        std::string state;
        double mem;
        double vmem;
        double resmem;
        double wmem;
        double shrmem;
        double cpuTime;
        double startTime;
        double cpu_usage;
        std::vector<std::string> info;

    } process_struct;

    static std::vector<process_struct *> getProcesses();
    static std::vector<std::string> getInfo(std::string);

private:
    std::vector<process_struct *> g_processes_list;
    QTreeWidget *processesWidget;

public slots:
        void displayProcInfo();
        void displayMemMaps();
        void displayDetInfo(QTreeWidgetItem *item, int column);
        void killProc();
        void stopProc();

private slots:

void OnRefresh();
void OnRightClick();

};


#endif // PROCESSES_H
