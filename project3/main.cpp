#include <fstream>
#include <list>
#include <map>
#include <vector>

using namespace std;

const int CHARS = 50;     // max length of a process name and program code file name
const int QUANTUM = 100;  // quantum of the system
const string base = "";

struct Process {
  string name;            // name of process
  string instruction_set; // name of program code file
  int arrival_time;       // arrival time of process
  int last;               // last executed instruction
  // constructor for process
  Process(const string &name, const string &instruction_set,
          const int &arrival_time, const int &last) {
    this->name = name;
    this->instruction_set = instruction_set;
    this->arrival_time = arrival_time;
    this->last = last;
  }
};

struct Instruction {
  string name;        // name of instruction
  int execution_time; // execution time of instruction
  // constructor for instruction
  Instruction(const string &name, const int &execution_time) {
    this->name = name;
    this->execution_time = execution_time;
  }
};

struct IO {
  int finish;               // finishing time
  list<Process> wait_queue; // wait queue of this io device
  vector<string> lines;     // output file lines
  // constructor for io device
  IO() {
    this->finish = 0;
  }
};

int total_time = 0;           // total time elapsed
vector<IO> io_devices;        // io devices
list<Process> wait_queue;     // processes that are read from definition file
list<Process> ready_queue;    // processes that are ready to enter the cpu (FIFO queue)
map<string, vector<Instruction> > instruction_sets; // map the program code file name to instructions

// creates a one-line string that corresponds to the current status of the given queue
string print_queue(int total_time, list<Process> queue) {
  string output = to_string(total_time) + "::HEAD-";  // prepare the output
  if (queue.empty()) {
    output += "-";
  }
  // get all process names in the ready queue
  for (list<Process>::iterator it = queue.begin(); it != queue.end(); it++) {
    output += it->name + "-";
  }
  return output + "TAIL\n";
}

// put process in io device
void do_io(int io_no, Process p) {
  IO io = io_devices[io_no];
  if (io.wait_queue.empty()) { // update finishing time of io device
    io.finish = total_time + instruction_sets[p.instruction_set][p.last].execution_time;
  }
  io.wait_queue.push_back(p); // io device is busy, put process into the wait queue
  io.lines.push_back(print_queue(total_time, io.wait_queue));
  io_devices[io_no] = io; // save state of io device
}

// first checks completed processes in io devices and finishing time of io device is updated
// then checks unarrived processes to determine which process is going to enter the ready queue
void check() {
  for (int i = 0; i < 3; i++) { // traverse all io devices
    IO io = io_devices[i];
    while (io.finish <= total_time && io.finish != 0 && !io.wait_queue.empty()) {
      // if process is completed its io then take it from io and push it into the ready queue
      Process p = io.wait_queue.front();
      io.wait_queue.pop_front();
      io.lines.push_back(print_queue(total_time, io.wait_queue));
      ready_queue.push_back(p);
      if (!io.wait_queue.empty()) { // update finishing time of io device
        p = io.wait_queue.front();
        io.finish = total_time + instruction_sets[p.instruction_set][p.last].execution_time;
      }
      io_devices[i] = io; // save state of io device
    }
  }
  while (!wait_queue.empty()) {
    Process process = wait_queue.front(); // fetch the earlier process
    if (process.arrival_time <= total_time) {
      // if process has arrived then push it into the ready queue
      wait_queue.pop_front();
      ready_queue.push_back(process);
    } else {
      // arrival times of next proceses are later than the current time
      break;
    }
  }
}

int main() {
  FILE *file = fopen((base + "definition.txt").c_str(), "r"); // open the definiton file
  char name[CHARS], instruction_set[CHARS];  // variables to read
  int arrival_time;                          // variable to read
  // read processes in definiton file
  while (fscanf(file, "%s %s %d", name, instruction_set, &arrival_time) != EOF) {
    wait_queue.push_back(Process(name, instruction_set, arrival_time, -1));
  }
  fclose(file); // close the definiton file
  for (int i = 1; i <= 4; i++) {                      // read program code files
    file = fopen((base + to_string(i) + ".code.txt").c_str(), "r"); // open the program code file
    int execution_time;               // variable to read
    vector<Instruction> instructions; // list of instruction execution times
    // read the instructions of the process
    while (fscanf(file, "%s %d", name, &execution_time) != EOF) {
      instructions.push_back(Instruction(name, execution_time));
    }
    // map the program code file name to these instructions
    instruction_sets.insert(pair<string, vector<Instruction> >(to_string(i) + ".code.txt", instructions));
    fclose(file); // close the program code file
  }
  for (int i = 0; i < 3; i++) { // initialize io devices
    io_devices.push_back(IO());
  }
  int block1 = -1, block2 = -1; // initialize cache blocks
  bool lru = false; // false if block 1 is lru, true if block 2 is lru
  check(); // check processes list to push the first arriving process into the ready queue
  file = fopen((base + "output.txt").c_str(), "w"); // open the main output file
  while (true) {                                // beginning of the cpu cycle
    // print the current ready queue to the output file
    fprintf(file, "%s", print_queue(total_time, ready_queue).c_str());
    if (ready_queue.empty()) {
      if (wait_queue.empty()) {
        // there is no process left to execute then exit
        break;
      } else {
        // if there are processes left but they are later than the current time, so cpu is idle
        total_time += QUANTUM;
        // check whether there are any processes that entered the ready queue when cpu is idle
        check();
      }
    } else {
      Process process = ready_queue.front(); // fetch the next process from the ready queue
      ready_queue.pop_front();
      int quantum = QUANTUM; // set the remaining quantum to maximum
      // get the instructions of this process
      vector<Instruction> instructions = instruction_sets[process.instruction_set];
      // if there are instructions left to execute for this process
      while (process.last != instructions.size() - 1) {
        Instruction instruction = instructions[process.last + 1]; // fetch the next instruction
        if (quantum > 0) { // if there is quantum left then continue to execute the next instruction
          process.last++;  // save the last executed instruction
          if (instruction.name.compare(0, 5, "dispM") == 0) { // dispM instruction
            do_io(instruction.name[6] - '0', process); // send process to printer
            break;
          } else if (instruction.name.compare(0, 5, "readM") == 0) { // readM instruction
            int block_no = instruction.name[6] - '0'; // number of block to be read
            if (block1 != block_no && block2 != block_no) { // cache miss, read from harddisk
              do_io(2, process); // send process to hard disk
              // update cache
              if (!lru) {
                block1 = block_no;
              }
              else {
                block2 = block_no;
              }
              lru = !lru;
              break;
            } else { // cache hit
              lru = block1 == block_no; // update lru block
            }
          } else {
            quantum -= instruction.execution_time;    // decrease the remaining quantum by the amount of instruction
            total_time += instruction.execution_time; // increase total time by the amount of instruction
          }
          // check whether there are any processes that entered the ready queue while executing this instruction
          check();
        } else { // if not then push this process back into the ready queue
          // check whether there are any processes that entered the ready queue while executing this instruction
          check();
          ready_queue.push_back(process);
          break;
        }
      }
    }
  }
  fclose(file); // close the main output file
  for (int i = 0; i < 3; i++) { // print io queues
    IO io = io_devices[i];
    if (!io.lines.empty()) { // blank files are not needed
      file = fopen((base + "output_1" + to_string(i) + ".txt").c_str(), "w"); // open the io output file
      for (string line : io.lines) {
        fprintf(file, "%s", line.c_str());
      }
      fclose(file); // close the io output file
    }
  }
  return 0;
}
