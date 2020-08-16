# OperatingSystems Exercise 4

This exercise is focused on Process Scheduling. Created a Round-Robin scheduler that runs on the user space. In order to synchronize the processes, scheduler sends SIGALRM signals. After, a SIGALRM signal, scheduler stops the process that runs and starts the next process from the "queue".
As a next step, Shell becomes a process to be scheduled. Scheduler is able to communicate with Shell in order to create and kill processes, as well as, set them to high/low priority.
