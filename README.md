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

Requirements
============
This program is written in Qt, uses the procps library, and executes the "ps" command internally to watch the portage user's processes. 

How to Use
==========
Run this program as root, or at least as the "portage" user, so that it will have permission to send stop and continue "kill" signals to the cc1plus processes spawned by your emerge build. You can freely start up, terminate, or re-start this program at any time and it will pick right up suspending and resuming the on-going portage build processes for you. 

Unfortunately, this program does not work against "rustc" builds, as rust seems to do some funny business with CPU usage reporting and the way it manages to carry out builds. But it should work great for programs compiled with gcc or clang.
