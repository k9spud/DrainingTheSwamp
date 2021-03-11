# Draining The Swamp
A tool for pipelining memory intensive builds in Gentoo.

When parallel (-j4) compiling QtWebEngine, Chromium, or other memory hungry applications, my Raspberry Pi 4/4GB often runs out of memory and the build fails because the Linux OOM killer takes out one of the compilation processes. Even if I add copious swap space, that doesn't really help when 4 cores are standing in each other's way, fighting against each other to swap their process into RAM all at the same time.

This program attempts to "pipeline" builds by temporarily suspending processes until the largest memory consuming process is able to finish. It automatically resumes suspended processes once memory pressure has been relieved. 

The ideal situation is to have the build processes staggered:

1. one large memory consumer that is just about to finish, 
2. one medium sized process that will become the largest memory consumer next, and
3. a couple smaller processes that aren't using much memory yet.

By keeping the processes pipelined in this fashion, we can keep all 4 cores of the Raspberry Pi 4 largely utilized, without running into out of memory situations and swamping the system with swap thrash.

As a side benefit, this tool also ends up automatically throttling back emerge builds whenever you're busy using your computer for other tasks. Sure, your builds will take longer to complete, but at least they won't completely fail or prevent you from being able to use your computer at all, like they would otherwise.

What's New in v1.0.7
====================

Ran into a deadlocked build of Chromium where a couple "aarch64-unknown" 
processes were in the "D" state (waiting on disk I/O), but were 
actually apparently waiting on the other pair of stopped "aarch64-unknown" 
processes to finish doing whatever.

Added a workaround so that "Draining The Swamp" will not count 
"aarch64-unknown" processes in the "D" state towards the "allowed to run" 
process count. This is kind of a hack, but it's working for me so far. 

Finished compiling Chromium 90.0.4427.5 after five days or so on my RPi4 
with 4GB RAM, zswap enabled, and 4GB of swap space on an external USB HDD 
partition. I did, however, spend a bit of time during those 5 days at "-1" 
process limiting, so that I could safely browse the web or play MegaGlest 
while the build continued cranking along in the background... Yay for 
dynamic adjustment of build parallelism!

Changed the "ps" external process starting code to wait indefinitely for "ps"
to finish. Previously the QProcess class was defaulting to a 30 second 
time-out, which ordinarily is plenty of time for "ps" to finish executing, 
but I guess if your system is heavily swamped, it might actually take longer. 
This might solve sibercat's report of getting "QProcess::start: Process is 
already running" warning messages.

Requirements
============
This program is written in Qt, uses the procps library, and executes the "ps" command internally to watch the portage user's processes. 

How to Use
==========
Run this program as root, or at least as the "portage" user, so that it will have permission to send stop and continue "kill" signals to the cc1plus processes spawned by your emerge build. You can freely start up, terminate, or re-start this program at any time and it will pick right up suspending and resuming the on-going portage build processes for you. 

If your builds aren't running as the "portage" user, you may need to use
the "-u" command line option to specify which processes to watch/manage.

Sometimes it can be handy to forcibly limit the number of concurrent processes,
or to even suspend the build entirely if you're trying to use your computer
for a bit and the builds are just taking up too much RAM. 

"-1" will limit running builds to one process at a time.
"-2" will limit builds to two or three processes at a time.
"-s" will limit existing builds to one process at a time, until they've all
finished, then DrainingTheSwamp will exit, with the build suspended. Probably 
only works on ninja builds right now.

Limitations
===========
Unfortunately, this program does not work against "rustc" builds, as rust seems to do some funny business with CPU usage reporting and the way it manages to carry out builds. But it should work great for programs compiled with gcc or clang.

When a large compilation process begins, there may be a window of time where we could theoretically run some small compilation processes that would complete before the "big one" gets far enough along to cause swapping. Unfortunately, there is no good way to predict which processes are going to be small vs big, so the program currently does not utilize as much CPU as it theoretically could. 

What is really needed, is smarter build tools (make/ninja), or a more memory efficient gcc. A database of how big each source file becomes during compilation might be useful, so that ninja/make could schedule one "big one" in parallel with known "small ones." As an external process, Draining The Swamp can only do so much.
