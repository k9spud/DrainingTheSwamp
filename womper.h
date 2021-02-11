#ifndef WOMPER_H
#define WOMPER_H

#include <signal.h>

#include <QStringList>
#include <QVector>
#include <QHash>

class QProcess;
class Womper
{
public:
    Womper(QString watchUser);

    void scan();
    void allowOne();
    void allowTwo();
    void allowAll();
    bool suspendToOne();

    struct ProcessInfo
    {
        pid_t pid;
        char status;
        QString cmd;
        long rss;
//        QString cwd;
    };

    int running;
    int swamped;
    int stopped;
    QVector<ProcessInfo> processes;
    QVector<ProcessInfo> oldProcesses;
    QVector<pid_t> pids;

    QHash<pid_t, long> highWaterMark; // largest size (in KB) of memory usage we've seen for each process
    long lastBigSize; // size in KB of the last exited biggest running process
    long biggestRunning; // size in KB of the currently biggest running process

private:
    QProcess* process;
    QStringList args;
    QStringList watches;
    QStringList compilers;
};

#endif // WOMPER_H
