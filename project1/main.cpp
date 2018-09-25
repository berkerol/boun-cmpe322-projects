#include <fstream>
#include <list>
#include <map>
#include <vector>

using namespace std;

const int CHARS = 50;     // max length of a process name and program code file name
const int QUANTUM = 100;  // quantum of the system

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

list<Process> processes;  // processes that are read from definition file
list<Process>ready_queue; // processes that are ready to enter the cpu (FIFO queue)

// checks processes list to determine which process is going to enter the ready queue
// which processes have an arrival time that is less than or equal to the current time
void check(int total_time) {
  while (!processes.empty()) {
    Process process = processes.front(); // fetch the earlier process
    if (process.arrival_time <= total_time) {
      // if process has arrived then push it into the ready queue
      processes.pop_front();
      ready_queue.push_back(process);
    } else {
      // arrival times of next proceses are later than the current time
      break;
    }
  }
}

int main() {
  FILE *file = fopen("definition.txt", "r"); // open the definiton file
  char name[CHARS], instruction_set[CHARS];  // variables to read
  int arrival_time;                          // variable to read
  // read processes in definiton file
  while (fscanf(file, "%s %s %d", name, instruction_set, &arrival_time) != EOF) {
    processes.push_back(Process(name, instruction_set, arrival_time, -1));
  }
  fclose(file); // close the definiton file
  map<string, vector<int>> instruction_sets; // map the program code file name to instructions
  for (int i = 1; i <= 4; i++) {                              // read program code files
    file = fopen((to_string(i) + ".code.txt").c_str(), "r");  // open the program code file
    int execution_time;       // variable to read
    vector<int> instructions; // list of instruction execution times
    // read the instructions of the process
    while (fscanf(file, "%s %d", name, &execution_time) != EOF) {
      instructions.push_back(execution_time);
    }
    // map the program code file name to these instructions
    instruction_sets.insert(pair<string, vector<int>>(to_string(i) + ".code.txt", instructions));
    fclose(file); // close the program code file
  }
  file = fopen("output.txt", "w"); // open the output file
  int total_time = 0;           // total time elapsed
  // check processes list to push the first arriving process into the ready queue
  check(total_time);
  while (true) {                                        // beginning of the cpu cycle
    string output = to_string(total_time) + "::HEAD-";  // prepare the output
    if (ready_queue.empty()) {
      output += "-";
    }
    // get all process names in the ready queue
    for (list<Process>::iterator it = ready_queue.begin(); it != ready_queue.end(); it++) {
      output += it->name + "-";
    }
    // print the current ready queue to the output file
    fprintf(file, "%s", (output + "TAIL\n").c_str());
    if (ready_queue.empty()) {
      if (processes.empty()) {
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
      ready_queue.pop_front();
      int quantum = QUANTUM; // set the remaining quantum to maximum
      // get the instructions of this process
      vector<int> instructions = instruction_sets[process.instruction_set];
      // if there are instructions left to execute for this process
      while (process.last != instructions.size() - 1) {
        int instruction = instructions[process.last + 1]; // fetch the next instruction time
        if (quantum > 0) { // if there is quantum left then continue to execute the next instruction
          process.last++;             // save the last executed instruction
          quantum -= instruction;     // decrease the remaining quantum by the amount of instruction
          total_time += instruction;  // increase total time by the amount of instruction
          // check whether there are any processes that entered the ready queue while executing this instruction
          check(total_time);
        } else { // if not then push this process back into the ready queue
          ready_queue.push_back(process);
          break;
        }
      }
    }
  }
  fclose(file); // close the output file
  return 0;
}
