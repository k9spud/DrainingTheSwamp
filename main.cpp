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
#include <unistd.h>
#include <signal.h>
#include <QProcess>
#include <QStringList>
#include <proc/sysinfo.h>

QProcess exec;
QStringList args;

int main(void)
{
    double totalRam, availableRam, percentFree;
    useconds_t sleepTime;
    char bounce[] = ".oO0Oo. ";
    int i = 0;

    printf("Draining the Swamp v1.0.0\n");

    Womper womper;
    womper.scan();
    while(1)
    {
        meminfo();
        totalRam = kb_main_total;
        availableRam = kb_main_available;
        percentFree = (availableRam / totalRam) * 100.0f;
        if(percentFree < 10.0f)
        {
            womper.allowOne();
            sleepTime = 400000;
        }
        else if(percentFree < 20.0f)
        {
            womper.allowTwo();
            sleepTime = 500000;
        }
        else
        {
            womper.allowAll();
            sleepTime = 1000000;
        }

        printf("%c Free memory: %.1f%% (%d stopped, %d running, %d swamped)      \r", bounce[i], percentFree, womper.stopped.count(), womper.running.count(), womper.swamped.count());
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

