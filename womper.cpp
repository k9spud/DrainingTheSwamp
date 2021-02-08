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
    ninja = -1;
    args << "-u" << watchUser << "-o" << "pid,state,rss,%cpu,comm" << "--sort" << "-rss" << "--no-headers" << "-w" << "-w";
    printf("ps");
    foreach(QString s, args)
    {
        printf(" %s", s.toStdString().c_str());
    }
    printf("\n");
    watches << "ninja" << "make" << "cc1" << "cc1plus" << "ld" << "aarch64-unknown";
}

void Womper::scan()
{
    process->start("/bin/ps", args);
    QStringList columns;
    QString cmd;
    QString status;
    QString rss;
    pid_t pid;

    running.clear();
    swamped.clear();
    stopped.clear();
    memory.clear();
    ninja = -1;
    process->waitForFinished();
    QString data = process->readAllStandardOutput();
    QStringList lines = data.split('\n');
    foreach(QString line, lines)
    {
        columns = line.split(' ', Qt::SkipEmptyParts);
        if(columns.count() < 5)
        {
            break;
        }

        cmd = columns.at(4);
        if(watches.contains(cmd) == false)
        {
            continue;
        }

        pid = columns.at(0).toInt();
        status = columns.at(1);
        rss = columns.at(2);
        memory[pid] = rss.toLong();
        if(cmd == "ninja")
        {
            ninja = pid;
        }

        if(status.startsWith("R") || status.startsWith("S"))
        {
            running.append(pid);
        }
        else if(status.startsWith("T"))
        {
            stopped.append(pid);
        }
        else if(status.startsWith("D"))
        {
            swamped.append(pid);
        }
    }
}

void Womper::allowOne()
{
    bool foundFirst = false;
    pid_t pid;
/*
    if(swamped.count() + running.count() == 1)
    {
        // already allowing only one process to run
        return;
    }
*/
    foreach(pid_t pid, running)
    {
        if(foundFirst || pid == ninja)
        {
            kill(pid, SIGSTOP);
        }
        else
        {
            foundFirst = true;
        }
    }

    foreach(pid, swamped)
    {
        if(foundFirst || pid == ninja)
        {
            kill(pid, SIGSTOP);
        }
        else
        {
            foundFirst = true;
        }
    }

    if(foundFirst == false)
    {
        if(stopped.contains(ninja))
        {
            kill(ninja, SIGCONT);
        }
        else
        {
            if(stopped.count())
            {
                kill(stopped.first(), SIGCONT);
            }
        }
    }
}

void Womper::allowTwo()
{
    pid_t pid;

    if(running.count() > 2)
    {
        pid = running.first();
        if(memory[pid] < 1000000)
        {
            // running process is not very big, allow the next biggest to run
            for(int i = 2; i < running.count(); i++)
            {
                pid = running.at(i);
                kill(pid, SIGSTOP);
            }
        }
        else
        {
            bool found = false;
            // running process is big, allow the next to only be medium sized
            for(int i = 1; i < running.count(); i++)
            {
                pid = running.at(i);
                if(memory[pid] < 500000 && found == false)
                {
                    found = true;
                    continue;
                }

                kill(pid, SIGSTOP);
            }
        }

        return;
    }

    if(stopped.count() == 0)
    {
        // no processes available to start
        return;
    }

    int run = running.count();
    bool ninjaRunning = running.contains(ninja);
    if(ninjaRunning && run == 3)
    {
        // already allowing only two processes to run (plus ninja)
        return;
    }

    if(!ninjaRunning && run == 2)
    {
        // already allowing only two processes to run (but no ninja)
        if(ninja > 0)
        {
            kill(ninja, SIGCONT);
        }
        return;
    }

    if((ninjaRunning && run == 2) || (!ninjaRunning && run == 1))
    {
        // not running enough processes, find a stopped one to allow

        // first, who is already running and how big is it?
        pid = running.first();
        if(pid == ninja)
        {
            pid = running.at(1);
        }
        if(memory[pid] < 1000000)
        {
            // running process is not very big, allow the next biggest to run
            for(int i = 0; i < stopped.count(); i++)
            {
                pid = stopped.at(i);
                if(pid != ninja)
                {
                    kill(pid, SIGCONT);
                    break;
                }
            }
        }
        else
        {
            // allow one big (already running) and one medium process to run
            for(int i = 0; i < stopped.count(); i++)
            {
                pid = stopped.at(i);
                if(pid == ninja)
                {
                    continue;
                }

                if(memory[pid] < 500000)
                {
                    kill(pid, SIGCONT);
                    break;
                }
            }
        }
        return;
    }

    if((ninjaRunning && run == 1) || (!ninjaRunning && run == 0))
    {
        // nothing running right now
        if(stopped.count() == 1)
        {
            // only one process available to start
            kill(stopped.first(), SIGCONT);
            return;
        }

        // find two processes to allow
        for(int i = 0; i < stopped.count(); i++)
        {
            pid = stopped.at(i);
            if(pid == ninja)
            {
                continue;
            }

            kill(pid, SIGCONT);
            if(memory[pid] < 1000000)
            {
                // running process is not very big, allow the next biggest to run
                for(; i < stopped.count(); i++)
                {
                    pid = stopped.at(i);
                    if(pid != ninja)
                    {
                        kill(pid, SIGCONT);
                        return;
                    }
                }
            }
            else
            {
                // allow one big (already running) and one medium process to run
                for(; i < stopped.count(); i++)
                {
                    pid = stopped.at(i);
                    if(pid == ninja)
                    {
                        continue;
                    }

                    if(memory[pid] < 500000)
                    {
                        kill(pid, SIGCONT);
                        return;
                    }
                }
            }
        }
    }
}

void Womper::allowAll()
{
    pid_t pid;
    for(int i = 0; i < stopped.count(); i++)
    {
        pid = stopped.at(i);
        kill(pid, SIGCONT);
    }
}


bool Womper::suspendToOne()
{
    bool foundFirst = false;
    pid_t pid;

    foreach(pid_t pid, running)
    {
        if(foundFirst || pid == ninja)
        {
            kill(pid, SIGSTOP);
        }
        else
        {
            foundFirst = true;
        }
    }

    foreach(pid, swamped)
    {
        if(foundFirst || pid == ninja)
        {
            kill(pid, SIGSTOP);
        }
        else
        {
            foundFirst = true;
        }
    }

    if(foundFirst == false)
    {
        foreach(pid_t pid, stopped)
        {
            if(pid != ninja)
            {
                kill(pid, SIGCONT);
                foundFirst = true;
                break;
            }
        }
    }

    return !foundFirst;
}
