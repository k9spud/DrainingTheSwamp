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

What's New in v1.0.8
====================

This release provides support for throttling Rust projects. It turns out,
Rust makes use of multiple CPU cores through threads rather than forking
separate processes. Top and "ps" by default report at the process level, thus
hiding all the threads from Draining The Swamp. We now call "ps" with the
right command line options to report each thread, so now Rust should work
nicely. 

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
