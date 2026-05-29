# Unix Text Processor

A Unix-based text processing application written in C that supports multiple text buffers, file operations, process execution, and signal management.

## Features

- Create and manage multiple text buffers
- Open and save text files
- Execute external programs using fork() and exec()
- Background and foreground process management
- Signal handling with SIGINT, SIGTSTP, and SIGCHLD
- Pause, resume, and cancel running processes
- Pipe-based interprocess communication

## Technologies

- C
- Linux
- Unix System Calls
- Signals
- Pipes
- Process Management

## Concepts Demonstrated

- Systems Programming
- Operating Systems
- Dynamic Memory Management
- Interprocess Communication
- Concurrent Process Control

## Note

Core functionality was implemented in `textmngr.c`. Some supporting framework files provided as part of the course infrastructure are not included in this repository.
