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

    QVector<pid_t> running;
    QVector<pid_t> swamped;
    QVector<pid_t> stopped;
    QHash<pid_t, long> memory;

    pid_t ninja;

private:
    QProcess* process;
    QStringList args;
    QStringList watches;

};

#endif // WOMPER_H
