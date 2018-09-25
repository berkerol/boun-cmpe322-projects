### Scheduler using Round Robin Algorithm

Compile with `$ g++ -std=c++11 main.cpp` and run with `$ ./a.out`

#### Struct

**Process** contains name, program code file name, arrival time and last executed instruction.

#### Data structures

- Processes are stored in a list.
  - pop_front method is used to fetch the earliest instruction.

- Ready queue is represented as a list.
  - pop_front and push_back methods are used to simulate a FIFO queue.

- Instruction execution times are stored as integers in a vector.
  - Then program code file name is mapped to this vector.
  - Instruction names are not stored because they are not used.

#### Main Loop

- Each iteration of the while loop is a CPU cycle.
- In the beginning of the cycle, current ready queue is printed.

- If there are no processes waiting in the ready queue, then CPU begins to idle.
  - Otherwise next process is fetched and it is executed until there are no instructions left or until there is no quantum left for this CPU cycle.
  - If there is no quantum left then this process is pushed back into the ready queue.

- In both idle state and execution state, total time is incremented either by whole quantum or instruction execution time respectively.
  - After increasing total time, processes list is checked if there are any processes that is entered while CPU is busy.
