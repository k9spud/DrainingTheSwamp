// Copyright (c) 2021, K9spud LLC.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "womper.h"

#include <stdio.h>
#include <QProcess>
#include <QString>

Womper::Womper(QString watchUser)
{
    process = new QProcess();
    lastBigSize = 0;
    args << "-u" << watchUser << "-o" << "pid,state,rss,%cpu,comm" << "--sort" << "-rss" << "--no-headers" << "-w" << "-w";
    printf("ps");
    foreach(QString s, args)
    {
        printf(" %s", s.toStdString().c_str());
    }
    printf("\n");
    compilers << "cc1" << "cc1plus" << "ld" << "aarch64-unknown" << "node";
    watches << "ninja" << "make" << compilers;
}

void Womper::scan()
{
    process->start("/bin/ps", args);
    QStringList columns;
    ProcessInfo pi;

    running = 0;
    swamped = 0;
    stopped = 0;
    oldProcesses.clear();
    foreach(pi, processes)
    {
        if(compilers.contains(pi.cmd))
        {
            oldProcesses.append(pi);
        }
    }
    processes.clear();
    pids.clear();
    biggestRunning = 0;

    process->waitForFinished();
    QString stat;
    QString data = process->readAllStandardOutput();    
    QStringList lines = data.split('\n');
    foreach(QString line, lines)
    {
        columns = line.split(' ', Qt::SkipEmptyParts);
        if(columns.count() < 5)
        {
            break;
        }

        pi.cmd = columns.at(4);
        if(watches.contains(pi.cmd) == false && pi.status != 'T')
        {
            continue;
        }

        pi.pid = columns.at(0).toInt();
        stat = columns.at(1);
        if(stat.count())
        {
            pi.status = stat.front().toLatin1();
        }
        else
        {
            pi.status = ' ';
        }
        pi.rss = columns.at(2).toLong();

        if(pi.status != 'Z')
        {
            pids.append(pi.pid);
            processes.append(pi);
        }

        if(pi.rss > highWaterMark[pi.pid])
        {
            highWaterMark[pi.pid] = pi.rss;
        }

        if(pi.status == 'R')
        {
            running++;
            if(biggestRunning == 0)
            {
                biggestRunning = pi.rss;
            }
        }
        else if(pi.status == 'D')
        {
            swamped++;
            if(biggestRunning == 0)
            {
                biggestRunning = pi.rss;
            }
        }
        else if(pi.status == 'T')
        {
            stopped++;
        }
    }

//    bool foundFirst = false;
    foreach(pi, oldProcesses)
    {
        if(pids.contains(pi.pid))
        {
            if(lastBigSize < pi.rss)
            {
                lastBigSize = pi.rss;
            }
            continue;
        }
/*
        if(foundFirst == false)
        {
            foundFirst = true;
            printf("\npid[%d] %ldMB %s\n", pi.pid, highWaterMark[pi.pid] / 1024, pi.cmd.toStdString().c_str());
        }
        else
        {
            printf("pid[%d] %ldMB %s\n", pi.pid, highWaterMark[pi.pid] / 1024, pi.cmd.toStdString().c_str());
        }
*/
        if(highWaterMark[pi.pid] < 1024 * 750)
        {
            lastBigSize = (static_cast<float>(lastBigSize) * 0.90f + static_cast<float>(highWaterMark[pi.pid]) * 0.10f);
        }
        else
        {
            lastBigSize = highWaterMark[pi.pid];
        }
        highWaterMark.remove(pi.pid);
    }
}

void Womper::allowOne()
{
    bool foundFirst = false;
    ProcessInfo pi;

    foreach(pi, processes)
    {
        if(pi.status == 'T' && watches.contains(pi.cmd) == false)
        {
            kill(pi.pid, SIGCONT);
            continue;
        }

        if(foundFirst)
        {
            if(pi.status != 'T')
            {
                kill(pi.pid, SIGSTOP);
            }
        }
        else
        {
            if(compilers.contains(pi.cmd))
            {
                if(pi.status == 'R' || pi.status == 'T' || pi.status == 'D')
                {
                    foundFirst = true;
                }

                if(pi.status == 'T')
                {
                    kill(pi.pid, SIGCONT);
                }
            }
        }
    }

    if(foundFirst == false)
    {
        // couldn't find any compilers to start running, better start ninja/make
        foreach(pi, processes)
        {
            if(compilers.contains(pi.cmd) == false && pi.status == 'T')
            {
                kill(pi.pid, SIGCONT);
            }
        }
    }
}

void Womper::allowTwo()
{
    ProcessInfo pi;
    int compilersRunning = 0;
    int sizeOfFirst = 0;
    foreach(pi, processes)
    {
        if(pi.status == 'T' && watches.contains(pi.cmd) == false)
        {
            kill(pi.pid, SIGCONT);
            continue;
        }

        if(compilersRunning >= 2)
        {
            if(pi.status != 'T')
            {
                kill(pi.pid, SIGSTOP);
            }
        }
        else
        {
            if(compilers.contains(pi.cmd))
            {
                if(compilersRunning == 0)
                {
                    if(pi.status == 'R' || pi.status == 'T' || pi.status == 'D')
                    {
                        sizeOfFirst = pi.rss;
                        compilersRunning++;
                    }

                    if(pi.status == 'T')
                    {
                        kill(pi.pid, SIGCONT);
                    }
                }
                else if(compilersRunning == 1)
                {
                    if(sizeOfFirst < 1000000 && lastBigSize < 1500000)
                    {
                        // running process is not very big and not estimated to be big,
                        // allow the next biggest to run
                        if(pi.status == 'R' || pi.status == 'T' || pi.status == 'D')
                        {
                            compilersRunning++;
                        }

                        if(pi.status == 'T')
                        {
                            kill(pi.pid, SIGCONT);
                        }
                    }
                    else
                    {
                        // running process is big, allow the next to only be medium sized
                        if(pi.rss < 500000)
                        {
                            if(pi.status == 'R' || pi.status == 'T' || pi.status == 'D')
                            {
                                compilersRunning++;
                            }

                            if(pi.status == 'T')
                            {
                                kill(pi.pid, SIGCONT);
                            }
                        }
                        else if(pi.status != 'T')
                        {
                            // this one is too big, stop it
                            kill(pi.pid, SIGSTOP);
                        }
                    }
                }
            }
        }
    }

    if(compilersRunning < 2)
    {
        // couldn't find enough compilers to start running, better start ninja/make
        foreach(pi, processes)
        {
            if(compilers.contains(pi.cmd) == false && pi.status == 'T')
            {
                kill(pi.pid, SIGCONT);
            }
        }
    }
}

void Womper::allowAll()
{
    ProcessInfo pi;
    foreach(pi, processes)
    {
        if(pi.status == 'T')
        {
            kill(pi.pid, SIGCONT);
        }
    }
}

bool Womper::suspendToOne()
{
    bool foundFirst = false;
    ProcessInfo pi;
    foreach(pi, processes)
    {
        if(pi.status == 'T' && watches.contains(pi.cmd) == false)
        {
            kill(pi.pid, SIGCONT);
            continue;
        }

        if(compilers.contains(pi.cmd) == false)
        {
            if(pi.status == 'R' || pi.status == 'D' || pi.status == 'S')
            {
                kill(pi.pid, SIGSTOP); // make sure all ninja/make processes are stopped.
            }
        }
        else
        {
            if(pi.status == 'R' || pi.status == 'D' || pi.status == 'S')
            {
                if(foundFirst)
                {
                    kill(pi.pid, SIGSTOP);
                }
                else
                {
                    if(pi.status == 'R' || pi.status == 'D')
                    {
                        foundFirst = true;
                    }
                }
            }
        }
    }

    if(foundFirst == false)
    {
        foreach(pi, processes)
        {
            if(compilers.contains(pi.cmd) && pi.status == 'T')
            {
                kill(pi.pid, SIGCONT);
                foundFirst = true;
                break;
            }
        }
    }

    return !foundFirst;
}
