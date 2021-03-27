# OperatingSystems

Code written by Markos Baratsas for the exercises of course [Operating Systems (ECE NTUA)](http://www.cslab.ece.ntua.gr/courses/os/).

This repository consists of the following 4 exercises.

## Exercise 1

This exercise aims to introduce some useful tools for coding (i.e. Makefile). Also, it is an introduction to file descriptors and how operating systems use them to read/write to files.

## Exercise 2

This exercise is focused on process handling and inter-process communication.

## Exercise 3

This exercise is focused on POSIX Thread Synchronization. In order to deal with competing threads, two approaches are used: a) **POSIX Mutexes** and b) **GCC Atomic Operations**.
Also, **semaphores** are used in order to establish a synchronization among the various threads.

## Exercise 4

This exercise is focused on Process Scheduling. Created a **Round-Robin scheduler** that runs on the user space. In order to synchronize the processes, scheduler sends SIGALRM signals. After, a SIGALRM signal, scheduler stops the process that runs and starts the next process from the "queue".
As a next step, Shell becomes a process to be scheduled. Scheduler is able to communicate with Shell in order to create and kill processes, as well as, set them to high/low priority.
