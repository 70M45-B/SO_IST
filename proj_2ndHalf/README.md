# Project 2
## Programming Language:
- C
## Problem Description:

**Part 2 of the Project:**

The second part of the project consists of 2 exercises aiming to:

i) Make IST-EMS accessible to client processes through named pipes.
ii) Allow interaction with IST-EMS through signals.

It will be divided into exercises for better understanding. The commands used in this part of the project are the same as in the first part, with the exception of BARRIER, which no longer exists in this delivery, and WAIT, which no longer receives the thread_id argument and is executed on the client side.

**Exercise 1: Interaction with client processes via named pipes**

IST-EMS should become an autonomous server process, launched as follows:

```
ems pipe_name
```

Upon initiation, the server should create a named pipe whose name (pathname) is the one indicated in the above argument. This pipe will be used by client processes to connect to the server and send login requests.

Any client process can connect to the server's pipe and send a message requesting the start of a session. This request contains the names of two named pipes, which the client previously created for the new session. It is through these named pipes that the client will send future requests to the server and receive corresponding responses.

Upon receiving a session request, the server assigns a unique identifier to the session (session_id) and associates the names of the named pipes indicated by the client with this session_id. It then responds to the client with the session_id of the new session.

The server accepts a maximum of S sessions simultaneously, each with a distinct session_id, where S is a constant defined in the server's code. This implies that when the server receives a new session request and has S active sessions, it should block, waiting for a session to end so that it can create the new one.

A session lasts until either i) the client sends an end-of-session message, or ii) the server detects that the client is unavailable.

In the following subsections, we describe the client API of IST-EMS in greater detail, as well as the content of the messages exchanged between clients and the server.

**Client API of IST-EMS**

To allow client processes to interact with IST-EMS, there is a programming interface (API) in C, which we call the IST-EMS client API. This API allows the client to have programs that establish a session with a server and, during that session, invoke operations to access and modify the state of events managed by IST-EMS. Below is the API:

**Operations for Establishing and Ending a Session:**

```c
int ems_setup(char const *req_pipe_path, char const *resp_pipe_path, char const *server_pipe_path)
```

Establishes a session using the specified named pipes. The named pipes used for the exchange of requests and responses (i.e., after the session has been established) must be created (using mkfifo) using the names passed in the 1st and 2nd arguments. The server's named pipe must already be created by the server and its name is passed in the 3rd argument.

Upon success, the session_id associated with the new session will have been stored in a variable of the client indicating the currently active session. Additionally, all pipes will have been opened by the client.

Returns 0 on success, 1 on error.

```c
int ems_quit()
```

Ends an active session identified in the client's respective variable, closing the pipes the client had open when the session was established, and deleting the client's named pipe.

Returns 0 on success, 1 on error.

**Operations Available During a Session:**

```c
int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols)
```

```c
int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys)
```

```c
int ems_show(int out_fd, unsigned int event_id)
```

```c
int ems_list_events(int out_fd)
```

`ems_show` and `ems_list_events` receive a file descriptor to where they should print their output, with the same format as the first part of the project.

Different client programs can exist, all invoking the above API concurrently. For simplification, the following assumptions are made:

- Client processes are single-threaded.
- Client processes are correct, meaning they comply with the specification described in the rest of this document. In particular, it is assumed that no client sends messages with a format outside the specified format.

**Protocol for Requests and Responses:**

The content of each message (request and response) must follow the following format:

- `ems_setup`:

```c
(char) OP_CODE=1 | (char[40]) client request pipe name | (char[40]) client response pipe name | (int) session_id
```

- `ems_quit`:

```c
(char) OP_CODE=2
```

- `ems_create`:

```c
(char) OP_CODE=3 | (unsigned int) event_id | (size_t) num_rows | (size_t) num_cols | (int) return (according to base code)
```

- `ems_reserve`:

```c
(char) OP_CODE=4 | (unsigned int) event_id | (size_t) num_seats | (size_t[num_seats]) xs content | (size_t[num_seats]) ys content | (int) return (according to base code)
```

- `ems_show` and `ems_list_events`:

```c
(char) OP_CODE=5 | (unsigned int) event_id | (int) return (according to base code) | (size_t) num_rows | (size_t) num_cols | (unsigned int[num_rows * num_cols]) seats
```

```c
(char) OP_CODE=6 | (int) return (according to base code) | (size_t) num_events | (unsigned int[num_events]) ids
```

Where:

- `|` denotes the concatenation of elements in a message.
- All request messages start with a code that identifies the requested operation (OP_CODE). Except for requests for `ems_setup`, OP_CODE is followed by the session_id of the client's current session (which should have been saved in a client variable when calling `ems_setup`).
- Strings carrying pipe names are of fixed size (40). In the case of names shorter than this, additional characters should be filled with '\0'.
- The buffer of seats returned by `ems_show` must follow the row-major order.
- In case of an error in `ems_show` or `ems_list_events`, the server should only send the error code.

**Implementation in Two Stages:**

Given the complexity of this requirement, it is recommended that the solution be developed gradually, in two stages, as described below:

**Stage 1.1: Single-session IST-EMS Server**

In this phase, the following simplifications should be assumed (to be eliminated in the next requirement):

- The server is single-threaded.
- The server accepts only one session at a time (i.e., S=1).

**Experiment:**

- Run the test provided in `jobs/test.jobs` on your client-server implementation of IST-EMS. Confirm that the test completes successfully.
- Build and try more elaborate tests that explore different functionalities offered by the IST-EMS server

.

**Stage 1.2: Support for Multiple Concurrent Sessions**

In this stage, the composite solution up to this point should be extended to support the following more advanced aspects.

On the one hand, the server must now support multiple active sessions simultaneously (i.e., S>1).

On the other hand, the server must be able to handle requests from different sessions (i.e., different clients) in parallel, using multiple tasks (pthreads). These tasks include:

- The initial task of the server should be responsible for receiving requests that arrive at the server through its pipe, hence called the host task.
- There should also be S worker tasks, each associated with a session_id and dedicated to serving the requests of the client corresponding to this session. The worker tasks should be created during the initialization of the server.

The host task coordinates with the worker tasks as follows:

- When the host task receives a session establishment request from a client, the host task inserts the request into a producer-consumer buffer. The worker tasks extract requests from this buffer and communicate with the respective client through the named pipes that the client will have previously created and communicated along with the session establishment request. Synchronization of the producer-consumer buffer should be based on condition variables (in addition to mutexes).

**Experiment:**

- Experiment by running the client-server tests you previously composed, but now launching them concurrently with 2 or more client processes.

**Exercise 2: Interaction via Signals**

Extend IST-EMS so that the server redefines the signal handling routine for SIGUSR1. Upon receiving this signal, the (server) IST-EMS should remember that, as soon as possible but outside the signal handling function, the main thread should print on std-output the identifier of each event, followed by the state of its seats, just like the SHOW command from the first exercise. Since only the main thread that receives client connections should listen to SIGUSR1, all threads serving a specific client should use the pthread_sigmask function to block (with the SIG_BLOCK option) the reception of SIGUSR1.

```bash
# compile
make

# program execution
# To run this programm, you will need at least two terminal windows, one for the server and the others for the clients.

# For the server window:
# ./server serverfifo, example:
./server /tmp/serverfifo

# For the client(s) window(s):(X should be different for different clients)
# ./client request_pipe answer_pipe server_pipe(same as above) jobs_directory, example:  
./client /tmp/req_pipe_X /tmp/resp_pipe_X /tmp/serverfifo ../jobs
```