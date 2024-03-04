# Project 1
## Programming Language:
- C
## Problem Description:

**Part 1 of the Project:**

The goal of this project is to develop the IST "Event Management System" (IST-EMS), an event management system that allows the creation, reservation, and verification of ticket availability for events such as concerts and theatrical performances. IST-EMS explores parallelization techniques based on multiple processes and multiple tasks to accelerate request processing. While developing IST-EMS, students will also learn how to implement scalable synchronization mechanisms between tasks and inter-process communication mechanisms (FIFOs and signals). IST-EMS will also interact with the file system, offering the opportunity to learn how to use POSIX file system programming interfaces.

**Command Parameters:**

1. **CREATE <event_id> <num_rows> <num_columns>**
   - This command is used to create a new event with a hall.
   - 'event_id' is a unique identifier for the event, 'num_rows' is the number of rows, and 'num_columns' is the number of columns in the hall.
   - Syntax: `CREATE 1 10 20` creates an event with identifier 1, a hall with 10 rows, and 20 columns.

2. **RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]**
   - Allows reservation of one or more seats in an existing event hall.
   - 'event_id' identifies the event, and each coordinate pair (x, y) specifies a seat to reserve.
   - Reservations are identified by a strictly positive integer identifier (res_id > 0).
   - Syntax: `RESERVE 1 [(1,1) (1,2) (1,3)]` reserves seats (1,1), (1,2), (1,3) in event 1.

3. **SHOW <event_id>**
   - Prints the current state of all seats in an event. Available seats are marked with '0', and reserved seats are marked with the reservation identifier.
   - Syntax: `SHOW 1` displays the current state of seats for event 1.

4. **LIST**
   - Lists all events created by their identifier.
   - Syntax: `LIST`

5. **WAIT <delay_ms> [thread_id]**
   - Injects a wait of the specified duration for all tasks before processing the next command, unless the optional 'thread_id' parameter is used.
   - Examples: `WAIT 2000` makes all tasks wait for 2 seconds before executing the next command. `WAIT 3000 5` makes the task with thread_id = 5 wait for 3 seconds before executing the next command.

6. **BARRIER**
   - (Only used at the end) Forces all tasks to wait for the completion of commands preceding BARRIER before resuming the execution of subsequent commands.

7. **HELP**
   - Provides information about available commands and their usage.

**Input Comments:**
Lines starting with '#' are considered comments and are ignored by the command processor, useful for testing.

**First Part of the Project:**
The first part of the project consists of three exercises. IST-EMS should receive the path to a directory named "JOBS" as a command-line argument, where command files are stored. IST-EMS should retrieve the list of ".jobs" files in the "JOBS" directory, process all commands in each ".jobs" file, and create a corresponding output file with the same name and a ".out" extension, reporting the state of each event. File access and manipulation should be performed using the POSIX file interface based on file descriptors, without using the stdio.h library and FILE stream abstraction.

Example output from the test file `/jobs/test.jobs`:
```
1 0 2
0 1 0
0 0 0
```

After completing this part, the code should be extended so that each ".job" file is processed by a parallel child process. The program should ensure that the maximum number of active parallel child processes is limited by a constant, `MAX_PROC`, passed as a command-line argument. To ensure the correctness of this solution, ".jobs" files should contain requests related to different events, i.e., two ".jobs" files cannot contain requests related to the same event. The main process should wait for the completion of each child process and print the corresponding termination status to std-output.

**Second Part of the Project:**
In the second part, the program should take advantage of parallelizing the processing of each ".job" file using multiple threads. The number of threads to use for processing each ".job" file, `MAX_THREADS`, should be specified as a command-line argument. Solutions with synchronization mechanisms that maximize achievable system parallelism will be valued. The synchronization solution developed should ensure that any operation is executed atomically, preventing partially executed reservations, for example. The main process should observe the return value of the child processes using `pthread_join` and, if it detects that the BARRIER command was found, start a new round of parallel processing that resumes after the BARRIER command.

```bash
# compile
make

# program execution
# ./ems, jobs directory, max processes, max threads, delay(optional),example:
./ems jobs 4 4 
```