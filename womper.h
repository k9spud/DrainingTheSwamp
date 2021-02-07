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
    Womper();

    void scan();
    void allowOne();
    void allowTwo();
    void allowAll();

    QVector<pid_t> running;
    QVector<pid_t> swamped;
    QVector<pid_t> stopped;
    QHash<pid_t, long> memory;

private:
    QProcess* process;
    QStringList args;
    QStringList watches;

    pid_t ninja;
};

#endif // WOMPER_H
