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

struct Semaphore {
  bool locked;              // state of semaphore
  list<Process> wait_queue; // wait queue of this semaphore
  vector<string> lines;     // output file lines
  // constructor for semaphore
  Semaphore() {
    this->locked = false;
  }
};

list<Process> wait_queue;     // processes that are read from definition file
list<Process> ready_queue;    // processes that are ready to enter the cpu (FIFO queue)
vector<Semaphore> semaphores; // all semaphores

// checks processes list to determine which process is going to enter the ready queue
// which processes have an arrival time that is less than or equal to the current time
void check(const int &total_time) {
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

int main() {
  FILE *file = fopen((base + "definition.txt").c_str(), "r"); // open the definiton file
  char name[CHARS], instruction_set[CHARS];  // variables to read
  int arrival_time;                          // variable to read
  // read processes in definiton file
  while (fscanf(file, "%s %s %d", name, instruction_set, &arrival_time) != EOF) {
    wait_queue.push_back(Process(name, instruction_set, arrival_time, -1));
  }
  fclose(file); // close the definiton file
  map<string, vector<Instruction> > instruction_sets; // map the program code file name to instructions
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
  for (int i = 0; i <= 9; i++) { // initialize semaphores
    semaphores.push_back(Semaphore());
  }
  int total_time = 0; // total time elapsed
  check(total_time); // check processes list to push the first arriving process into the ready queue
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
        check(total_time);
      }
    } else {
      Process process = ready_queue.front(); // fetch the next process from the ready queue
      int quantum = QUANTUM; // set the remaining quantum to maximum
      // get the instructions of this process
      vector<Instruction> instructions = instruction_sets[process.instruction_set];
      // if there are instructions left to execute for this process
      while (process.last != instructions.size() - 1) {
        Instruction instruction = instructions[process.last + 1]; // fetch the next instruction
        if (quantum > 0) { // if there is quantum left then continue to execute the next instruction
          process.last++;  // save the last executed instruction
          quantum -= instruction.execution_time;    // decrease the remaining quantum by the amount of instruction
          total_time += instruction.execution_time; // increase total time by the amount of instruction
          if (instruction.name.compare(0, 5, "waitS") == 0) { // waitS instruction
            int s_no = instruction.name[6] - '0'; // number of semaphore to be locked
            Semaphore semaphore = semaphores[s_no];
            if (semaphore.locked) { // semaphore was locked by another process
              // send this process to the wait queue of semaphore
              semaphore.wait_queue.push_back(process);
              // append the current status of the wait queue
              semaphore.lines.push_back(print_queue(total_time, semaphore.wait_queue));
              semaphores[s_no] = semaphore; // save semaphore
              break; // schedule another process
            }
            semaphore.locked = true; // lock semaphore
            semaphores[s_no] = semaphore; // save semaphore
          } else if (instruction.name.compare(0, 5, "signS") == 0) { // signS instruction
            int s_no = instruction.name[6] - '0'; // number of semaphore to be released
            Semaphore semaphore = semaphores[s_no];
            if (!semaphore.wait_queue.empty()) { // another process is waiting for this semaphore
              // send the next waiting process to the ready queue
              ready_queue.push_back(semaphore.wait_queue.front());
              // print the current ready queue to the output file
              fprintf(file, "%s", print_queue(total_time, ready_queue).c_str());
              semaphore.wait_queue.pop_front(); // remove the next waiting process from semaphore
              // append the current status of the wait queue
              semaphore.lines.push_back(print_queue(total_time, semaphore.wait_queue));
            } else {
              semaphore.locked = false; // release semaphore
            }
            semaphores[s_no] = semaphore; // save semaphore
          }
        } else { // if not then push this process back into the ready queue
          // check whether there are any processes that entered the ready queue while executing this instruction
          check(total_time);
          ready_queue.push_back(process);
          break;
        }
      }
      // check whether there are any processes that entered the ready queue while executing this instruction
      check(total_time);
      ready_queue.pop_front();
    }
  }
  fclose(file); // close the main output file
  for (int i = 0; i <= 9; i++) { // print semaphore queues
    Semaphore semaphore = semaphores[i];
    if (!semaphore.lines.empty()) { // blank files are not needed
      file = fopen((base + "output_" + to_string(i) + ".txt").c_str(), "w"); // open the semaphore output file
      for (string line : semaphore.lines) {
        fprintf(file, "%s", line.c_str());
      }
      fclose(file); // close the semaphore output file
    }
  }
  return 0;
}
