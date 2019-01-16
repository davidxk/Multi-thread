# Multi-thread
My pthread, POSIX and C++11 implementation of classic multi-thread models

* Producer-consumer problem
* Read-write lock
* Barrier Synchronization

## Interview Question: Mutex vs Semaphore
* A binary semaphore works the same way as a mutex
* Usually we use a counting semaphore to solve the producer-consumer problem
* Mutex manages one unit of shared resources while semaphore manages N unit of shared resources
* For a locked mutex it must be unlocked by the same thread, but a semaphore can be incremented by a different thread

Semaphore has two operations, P and V and can be implemented with a mutex and a conditional variable. Its logic can be easily expressed with a guarded command.
