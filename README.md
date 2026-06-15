# mini_webserv

`mini_webserv` is a small C web server that was built as a chat-like server. The goal was to keep the implementation simple and readable: a small server, a polling loop, and message broadcasting between connected clients.

The project is intentionally inspired by the idea of an IRC-style room. Clients join, receive a welcome message, and can broadcast messages to everyone else connected to the server. The code favors stack allocation for the runtime data structures and avoids dynamic allocation for the main server state.

## Project goals

- Practice low-level socket programming in C.
- Build a chat-oriented server with a minimal amount of state.
- Understand event-driven I/O with `poll`.
- Learn how to manage clients, broadcasts, disconnects, and shutdown cleanly.
- Explore how server logic behaves under debugging and memory checking tools.

## Relation to IRC and the 42 exam

This project is close in spirit to IRC:

- clients join a shared room,
- messages are broadcast to other clients,
- disconnects are announced to the remaining users,
- the server keeps a small, central notion of presence.

It also fits the final 42 exam style because it stays compact and focused. In contrast to the 42 final exam, this version uses memset instead of bzero. But each time memset was used, there is also a bzero-version commented out one line above. 

## Build

```sh
make
```

Build a debug version:

```sh
make debug
```

## Run

The server expects a port number.

```sh
./miniserv 8080
```

The provided Makefile also includes convenience targets:

```sh
make run
make debugrun
make test
```

## Test

`make test` runs the Python integration test suite against the local binary and prints the server's Valgrind log to stdout at the end of the run.

```sh
make test
```

The test target uses the same Valgrind flags as `make debugrun`, so both paths report memory and file descriptor issues consistently.

## Implementation notes

- The server uses `poll` to manage the listening socket and all connected clients.
- Message buffers are kept in fixed-size arrays.
- Client state is updated with simple array management instead of heap-backed containers.
- Signal handling is used so the server can stop cleanly when interrupted.

## Why this project matters

This is a useful exercise for both learning and hiring relevance.

For learning, it forces you to deal with topics that appear everywhere in systems work:

- sockets,
- event loops,
- dynamic and static memory allocation
- buffering,
- partial reads and writes,
- resource cleanup,
- signal handling,
- debugging with Valgrind.

For the job market, it demonstrates practical skills that are easy to evaluate in interviews and useful in real backend or infrastructure work:

- writing correct C code under constraints,
- reasoning about stateful network services,
- avoiding leaks and descriptor problems,
- building predictable server behavior,
- keeping implementations small and maintainable.

## Current status

The repository includes:

- `miniserv` as the compiled binary,
- a `Makefile` with build, debug, run, test, and cleanup targets,
- a Python integration test script,
- the server implementation split across a few C files.

If you are using this as an exam-style reference, the main focus is not feature completeness but control over the server loop, client lifecycle, and clean shutdown behavior.

## About AI usage
AI (Copilot) was used to create a testscript (100%) and served as assictance to create this README-file.
