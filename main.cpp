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
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <QStringList>
#include <proc/sysinfo.h>

QStringList args;

int main(int argc, char* argv[])
{
    double totalRam, availableRam, percentFree;
    long estimatedSum;
    useconds_t sleepTime;
    char bounce[] = ".oO0Oo. ";
    char allowWhich;
    QString watchUser = "portage";
    int i = 0;

    printf("Draining The Swamp v1.0.8\n");

    bool stayAtOne = false;
    bool stayAtTwo = false;
    bool suspendBuild = false;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-1") == 0)
        {
            stayAtOne = true;
            printf("staying at one process allowed to run.\n");
        }
        else if(strcmp(argv[i], "-2") == 0)
        {
            stayAtTwo = true;
            printf("staying at two processes allowed to run.\n");
        }
        else if(strcmp(argv[i], "-s") == 0)
        {
            suspendBuild = true;
            printf("Letting only existing compilations complete, one at a time (suspend the build).\n");
        }
        else if(strcmp(argv[i], "-u") == 0)
        {
            i++;
            if(i >= argc)
            {
                printf("must provide user name for '-u userid'\n");
                return 1;
            }
            watchUser = QString::fromLatin1(argv[i]);
        }
        else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            printf(R"EOF(
Command line options:
    -1  Stay at one running process max.

    -2  Stay at two/three running processes max.

    -u "user id"
        Select which user's processes should be monitored/managed.
        The default is "portage" when this option is not used.

    -s  Let existing builds finish, one at a time (suspend the build).
)EOF");
            return 0;
        }
    }

    if(suspendBuild)
    {
        Womper womper(watchUser);
        womper.scan();
        while(1)
        {
            meminfo();
            totalRam = kb_main_total;
            availableRam = kb_main_available;
            estimatedSum = (totalRam - availableRam) - (womper.biggestRunning) + (womper.lastBigSize);
            percentFree = (availableRam / totalRam) * 100.0f;
            if(womper.suspendToOne())
            {
                printf("\nAll compilation processes have been cleared and the build is suspended.\n");
                return 0;
            }
            sleepTime = 500000;
            printf("%c est[%ld/%ld MB] free[%.1f%%/%.0fMB] (%d stopped, %d running, %d swamped)   \r",
                   bounce[i], womper.lastBigSize / 1024, estimatedSum / 1024, percentFree, availableRam / 1000.0d,
                   womper.stopped, womper.running, womper.swamped);
            i++;
            if(i > 7)
            {
                i = 0;
            }

            fflush(stdout);
            usleep(sleepTime);

            womper.scan();
        }
    }

    Womper womper(watchUser);
    womper.scan();
    while(1)
    {
        meminfo();
        totalRam = kb_main_total;
        availableRam = kb_main_available;
        estimatedSum = (totalRam - availableRam) - (womper.biggestRunning) + (womper.lastBigSize);
        percentFree = (availableRam / totalRam) * 100.0f;
        if(percentFree < 13.0f || stayAtOne || estimatedSum > (totalRam - 500000))
        {
            womper.allowOne();
            allowWhich = '1';
            sleepTime = 400000;
        }
        else if(percentFree < 22.0f || stayAtTwo || estimatedSum > (totalRam - 750000) || womper.swamped > 1)
        {
            womper.allowTwo();
            allowWhich = '2';
            sleepTime = 500000;
        }
        else
        {
            womper.allowAll();
            allowWhich = 'A';
            sleepTime = 1000000;
        }

        printf("%c est[%ld/%ld MB] free[%.1f%%/%.0fMB] A%c(%d stopped, %d running, %d swamped)   \r",
               bounce[i], womper.lastBigSize / 1024, estimatedSum / 1024, percentFree, availableRam / 1000.0d,
               allowWhich, womper.stopped, womper.running, womper.swamped);
        i++;
        if(i > 7)
        {
            i = 0;
        }

        fflush(stdout);
        usleep(sleepTime);

        womper.scan();
    }
}

