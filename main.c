/**
 * CPU SCHEDULING ALGORITHMS SIMULATOR
 * 
 * This program implements and compares five fundamental CPU scheduling algorithms:
 * 1. First-Come, First-Served (FCFS)
 * 2. Last-Come, First-Served Non-Preemptive (LCFS-NP)
 * 3. Last-Come, First-Served Preemptive (LCFS-P)
 * 4. Round Robin (RR) with time quantum = 2
 * 5. Shortest Job First (SJF) - Non-preemptive
 * 
 * PERFORMANCE METRIC:
 * - Average Turnaround Time = (Completion Time - Arrival Time)
 * - Lower values indicate better performance
 * 
 * INPUT FORMAT:
 * Line 1: Number of processes
 * Following lines: arrival_time,computation_time (one per process)
 * 
 * USAGE: ./scheduler input_file.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_PROCESSES 100

/**
 * PROCESS CONTROL BLOCK (PCB) STRUCTURE
 * 
 * This structure represents the essential information maintained
 * by the operating system for each process in the system.
 * 
 * FIELDS:
 * - arrivalTime: When process enters the ready queue
 * - computationTime: Total CPU time required (burst time)
 * - remainingTime: CPU time still needed (for preemptive algorithms)
 * - completionTime: When process finishes execution
 * 
 * DESIGN DECISIONS:
 * - Simple integer fields for educational clarity
 * - No priority field (not needed for implemented algorithms)
 * - No process ID (array index serves as implicit PID)
 */
typedef struct {
    int arrivalTime;      // Process arrival time
    int computationTime;  // Total CPU burst time required
    int remainingTime;    // Remaining CPU time (for preemptive scheduling)
    int completionTime;   // Time when process completes execution
} Process;

// Global process table - simulates OS process table
Process processes[MAX_PROCESSES];
int numProcesses = 0;

/**
 * UTILITY FUNCTION: swap
 * PURPOSE: Exchange two Process structures
 * 
 * Used by sorting algorithms to reorder processes based on different criteria.
 * Pass-by-reference ensures actual process data is swapped.
 */
void swap(Process* a, Process* b) {
    Process temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * GENERIC BUBBLE SORT IMPLEMENTATION
 * PURPOSE: Sort process array using provided comparison function
 * 
 * PARAMETERS:
 * - arr: Array of processes to sort
 * - n: Number of processes
 * - compare: Function pointer for comparison logic
 * 
 * ALGORITHM COMPLEXITY: O(n²)
 * - Suitable for small datasets (educational purposes)
 * - Could be optimized with quicksort or mergesort for production
 * 
 * DESIGN PATTERN: Strategy Pattern
 * - Different comparison functions enable sorting by different criteria
 * - Same sorting logic works for arrival time, burst time, etc.
 */
void bSort(Process* arr, int n, int (*compare)(Process, Process)) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (compare(arr[j], arr[j + 1]) > 0) {
                swap(&arr[j], &arr[j + 1]);
            }
        }
    }
}

/**
 * COMPARISON FUNCTION: compareArrival
 * PURPOSE: Compare processes by arrival time (ascending order)
 * 
 * RETURN VALUES:
 * - Negative: a arrives before b
 * - Zero: a and b arrive simultaneously
 * - Positive: a arrives after b
 * 
 * Used by FCFS and other algorithms that need arrival-time ordering.
 */
int compareArrival(Process a, Process b) {
    return a.arrivalTime - b.arrivalTime;
}

/**
 * COMPARISON FUNCTION: compareShortest
 * PURPOSE: Compare processes by computation time (ascending order)
 * 
 * Used by SJF algorithm to identify shortest jobs first.
 * Enables efficient sorting of processes by burst time.
 */
int compareShortest(Process a, Process b) {
    return a.computationTime - b.computationTime;
}

/**
 * INPUT PROCESSING FUNCTION
 * PURPOSE: Read process data from input file
 * 
 * FILE FORMAT:
 * Line 1: Number of processes
 * Subsequent lines: arrival_time,computation_time
 * 
 * INITIALIZATION:
 * - Sets remainingTime = computationTime (for preemptive algorithms)
 * - Initializes completionTime = 0
 * 
 * ERROR HANDLING:
 * - Exits program if file cannot be opened
 * - Assumes well-formed input data
 */
void readInputFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    // Read number of processes
    fscanf(file, "%d", &numProcesses);

    // Read process data
    for (int i = 0; i < numProcesses; i++) {
        fscanf(file, "%d,%d", &processes[i].arrivalTime, &processes[i].computationTime);
        processes[i].remainingTime = processes[i].computationTime;
        processes[i].completionTime = 0;
    }

    fclose(file);
}

/**
 * PERFORMANCE METRIC CALCULATION
 * PURPOSE: Calculate average turnaround time for given process set
 * 
 * TURNAROUND TIME = Completion Time - Arrival Time
 * 
 * This metric measures the total time a process spends in the system,
 * from arrival until completion. Lower values indicate better performance.
 * 
 * ALTERNATIVE METRICS (not implemented):
 * - Waiting Time = Turnaround Time - Burst Time
 * - Response Time = First CPU allocation - Arrival Time
 */
float calculateAverageTurnaround(Process* arr, int n) {
    float totalTurnaround = 0;
    for (int i = 0; i < n; i++) {
        totalTurnaround += arr[i].completionTime - arr[i].arrivalTime;
    }
    return totalTurnaround / n;
}

/**
 * FIRST-COME, FIRST-SERVED (FCFS) SCHEDULING
 * 
 * ALGORITHM CHARACTERISTICS:
 * - Non-preemptive
 * - Processes executed in arrival order
 * - Simple to implement and understand
 * - Can cause "convoy effect" with long processes
 * 
 * IMPLEMENTATION DETAILS:
 * 1. Sort processes by arrival time
 * 2. Execute each process to completion in order
 * 3. Handle CPU idle time when no processes are ready
 * 
 * TIME COMPLEXITY: O(n log n) due to sorting
 * SPACE COMPLEXITY: O(n) for temporary process array
 * 
 * CONVOY EFFECT:
 * - Short processes wait behind long processes
 * - Can lead to poor average turnaround time
 * - Real-world example: Supermarket checkout with one slow customer
 */
float fcfs() {
    // Create working copy to avoid modifying original data
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);

    // Sort by arrival time
    bSort(tempProcesses, numProcesses, compareArrival);

    int currentTime = 0;
    for (int i = 0; i < numProcesses; i++) {
        // Handle CPU idle time
        if (currentTime < tempProcesses[i].arrivalTime) {
            currentTime = tempProcesses[i].arrivalTime;
        }

        // Execute process to completion
        currentTime += tempProcesses[i].computationTime;
        tempProcesses[i].completionTime = currentTime;
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

/**
 * LAST-COME, FIRST-SERVED NON-PREEMPTIVE (LCFS-NP) SCHEDULING
 * 
 * ALGORITHM CHARACTERISTICS:
 * - Non-preemptive (once started, process runs to completion)
 * - Uses stack (LIFO) data structure
 * - Most recently arrived process gets priority
 * - Can cause starvation of early-arriving processes
 * 
 * IMPLEMENTATION STRATEGY:
 * 1. Maintain a stack of ready processes
 * 2. When CPU becomes free, execute top of stack
 * 3. Add newly arrived processes to stack top
 * 
 * STACK SIMULATION:
 * - Array-based stack with top pointer
 * - Push: stack[++top] = process_index
 * - Pop: process_index = stack[top--]
 * 
 * STARVATION PROBLEM:
 * - Early processes may never execute if new processes keep arriving
 * - Not suitable for interactive systems
 * - Useful in batch processing scenarios
 */
float lcfsNonPreemptive() {
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);
    bSort(tempProcesses, numProcesses, compareArrival);

    int currentTime = 0;
    int completed = 0;
    int stack[MAX_PROCESSES];  // Stack for ready processes
    int top = -1;              // Stack top pointer

    while (completed < numProcesses) {
        // Add newly arrived processes to stack
        for (int i = 0; i < numProcesses; i++) {
            if (tempProcesses[i].arrivalTime <= currentTime && 
                tempProcesses[i].remainingTime > 0) {
                stack[++top] = i;
            }
        }

        if (top >= 0) {
            // Execute most recently arrived process (top of stack)
            int i = stack[top--];
            if (currentTime < tempProcesses[i].arrivalTime) {
                currentTime = tempProcesses[i].arrivalTime;
            }
            currentTime += tempProcesses[i].computationTime;
            tempProcesses[i].completionTime = currentTime;
            tempProcesses[i].remainingTime = 0;
            completed++;
        } else {
            // CPU idle - advance to next arrival time
            int nextArrival = INT_MAX;
            for (int i = 0; i < numProcesses; i++) {
                if (tempProcesses[i].arrivalTime > currentTime && 
                    tempProcesses[i].arrivalTime < nextArrival) {
                    nextArrival = tempProcesses[i].arrivalTime;
                }
            }
            if (nextArrival != INT_MAX) {
                currentTime = nextArrival;
            } else {
                break;  // All processes completed
            }
        }
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

/**
 * LAST-COME, FIRST-SERVED PREEMPTIVE (LCFS-P) SCHEDULING
 * 
 * ALGORITHM CHARACTERISTICS:
 * - Preemptive (running process can be interrupted)
 * - Most recently arrived process preempts currently running process
 * - Uses stack with time-slicing (1 time unit per iteration)
 * - High context switching overhead
 * 
 * PREEMPTION MECHANISM:
 * 1. Execute top of stack for 1 time unit
 * 2. Check for newly arrived processes
 * 3. If new process arrives, it goes to stack top (preempts current)
 * 4. Continue until all processes complete
 * 
 * CONTEXT SWITCHING:
 * - Each time unit represents potential context switch
 * - Real systems have context switch overhead (not modeled here)
 * - High overhead makes this algorithm impractical for real systems
 * 
 * IMPLEMENTATION CHALLENGES:
 * - Managing stack state during preemption
 * - Ensuring newly arrived processes get immediate priority
 * - Handling completion detection correctly
 */
float lcfsPreemptive() {
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);
    bSort(tempProcesses, numProcesses, compareArrival);

    int currentTime = 0;
    int completed = 0;
    int stack[MAX_PROCESSES];
    int top = -1;

    while (completed < numProcesses) {
        // Execute current process for 1 time unit
        while (top >= 0) {
            int i = stack[top];
            if (tempProcesses[i].remainingTime > 0) {
                tempProcesses[i].remainingTime--;
                currentTime++;

                // Check if process completed
                if (tempProcesses[i].remainingTime == 0) {
                    tempProcesses[i].completionTime = currentTime;
                    completed++;
                    top--;  // Remove completed process from stack
                }
                break;  // Execute only 1 time unit
            } else {
                top--;  // Remove already completed process
            }
        }

        // Handle CPU idle time
        if (top < 0) {
            int nextArrival = INT_MAX;
            for (int i = 0; i < numProcesses; i++) {
                if (tempProcesses[i].arrivalTime > currentTime && 
                    tempProcesses[i].arrivalTime < nextArrival) {
                    nextArrival = tempProcesses[i].arrivalTime;
                }
            }
            if (nextArrival != INT_MAX) {
                currentTime = nextArrival;
            } else {
                break;  // All processes completed
            }
        }

        // Add newly arrived processes to stack (they preempt current process)
        for (int i = 0; i < numProcesses; i++) {
            if (tempProcesses[i].arrivalTime <= currentTime && 
                tempProcesses[i].remainingTime > 0) {
                stack[++top] = i;
            }
        }
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

/**
 * ROUND ROBIN (RR) SCHEDULING
 * 
 * ALGORITHM CHARACTERISTICS:
 * - Preemptive with fixed time quantum (2 time units)
 * - Fair allocation of CPU time among processes
 * - Uses circular queue (FIFO) data structure
 * - Good for interactive systems
 * 
 * TIME QUANTUM SELECTION:
 * - Too small: High context switching overhead
 * - Too large: Approaches FCFS behavior
 * - Typical values: 10-100ms in real systems
 * - This implementation uses quantum = 2 for demonstration
 * 
 * QUEUE MANAGEMENT:
 * 1. Processes enter queue upon arrival
 * 2. Front process gets CPU for time quantum
 * 3. If not completed, process goes to rear of queue
 * 4. Newly arrived processes enter queue during execution
 * 
 * FAIRNESS PROPERTY:
 * - Each process gets equal share of CPU time
 * - No process waits more than (n-1) * quantum time units
 * - Prevents starvation unlike LCFS
 * 
 * IMPLEMENTATION DETAILS:
 * - Array-based circular queue simulation
 * - front/rear pointers manage queue state
 * - Duplicate prevention ensures processes appear only once in queue
 */
float roundRobin() {
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);

    int currentTime = 0;
    int completed = 0;
    int queue[MAX_PROCESSES];  // Ready queue
    int front = 0, rear = -1;  // Queue pointers
    int timeQuantum = 2;       // Fixed time slice

    while (completed < numProcesses) {
        // Add newly arrived processes to queue
        for (int i = 0; i < numProcesses; i++) {
            if (tempProcesses[i].arrivalTime <= currentTime && 
                tempProcesses[i].remainingTime > 0) {

                // Check if process already in queue (avoid duplicates)
                int alreadyInQueue = 0;
                for (int j = front; j <= rear; j++) {
                    if (queue[j] == i) {
                        alreadyInQueue = 1;
                        break;
                    }
                }
                if (!alreadyInQueue) {
                    queue[++rear] = i;
                }
            }
        }

        if (front <= rear) {
            // Get next process from front of queue
            int i = queue[front++];

            if (tempProcesses[i].remainingTime <= timeQuantum) {
                // Process completes within time quantum
                currentTime += tempProcesses[i].remainingTime;
                tempProcesses[i].completionTime = currentTime;
                tempProcesses[i].remainingTime = 0;
                completed++;
            } else {
                // Process uses full time quantum
                currentTime += timeQuantum;
                tempProcesses[i].remainingTime -= timeQuantum;

                // Add newly arrived processes before re-queuing current process
                for (int j = 0; j < numProcesses; j++) {
                    if (tempProcesses[j].arrivalTime <= currentTime && 
                        tempProcesses[j].remainingTime > 0) {

                        int alreadyInQueue = 0;
                        for (int k = front; k <= rear; k++) {
                            if (queue[k] == j) {
                                alreadyInQueue = 1;
                                break;
                            }
                        }
                        if (!alreadyInQueue && j != i) {
                            queue[++rear] = j;
                        }
                    }
                }
                // Re-queue current process at rear
                queue[++rear] = i;
            }
        } else {
            // Queue empty - advance to next arrival
            int nextArrival = INT_MAX;
            for (int i = 0; i < numProcesses; i++) {
                if (tempProcesses[i].arrivalTime > currentTime && 
                    tempProcesses[i].arrivalTime < nextArrival) {
                    nextArrival = tempProcesses[i].arrivalTime;
                }
            }
            if (nextArrival != INT_MAX) {
                currentTime = nextArrival;
            } else {
                break;  // All processes completed
            }
        }
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

/**
 * SHORTEST JOB FIRST (SJF) SCHEDULING
 * 
 * ALGORITHM CHARACTERISTICS:
 * - Non-preemptive
 * - Selects process with shortest remaining burst time
 * - Optimal for minimizing average turnaround time
 * - Requires knowledge of future CPU burst times
 * 
 * OPTIMALITY PROOF:
 * - SJF minimizes average waiting time (and thus turnaround time)
 * - Mathematical proof based on exchange argument
 * - Any deviation from shortest-first order increases average wait time
 * 
 * PRACTICAL LIMITATIONS:
 * - CPU burst times unknown in advance
 * - Real systems use prediction techniques (exponential averaging)
 * - Can cause starvation of long processes
 * 
 * STARVATION PROBLEM:
 * - Long processes may never execute if short processes keep arriving
 * - Solution: Aging (gradually increase priority of waiting processes)
 * - Not implemented in this basic version
 * 
 * IMPLEMENTATION STRATEGY:
 * 1. At each scheduling decision, scan all ready processes
 * 2. Select process with minimum remaining time
 * 3. Execute selected process to completion
 * 4. Repeat until all processes complete
 * 
 * TIME COMPLEXITY: O(n²) due to repeated scanning
 * - Could be optimized with priority queue (heap) to O(n log n)
 */
float sjf() {
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);
    bSort(tempProcesses, numProcesses, compareArrival);

    int currentTime = 0;
    int completed = 0;

    while (completed < numProcesses) {
        int shortestIndex = -1;
        int shortestTime = INT_MAX;

        // Find shortest job among ready processes
        for (int i = 0; i < numProcesses; i++) {
            if (tempProcesses[i].arrivalTime <= currentTime && 
                tempProcesses[i].remainingTime > 0) {
                if (tempProcesses[i].remainingTime < shortestTime) {
                    shortestTime = tempProcesses[i].remainingTime;
                    shortestIndex = i;
                }
            }
        }

        if (shortestIndex == -1) {
            // No ready processes - advance to next arrival
            int nextArrival = INT_MAX;
            for (int i = 0; i < numProcesses; i++) {
                if (tempProcesses[i].arrivalTime > currentTime && 
                    tempProcesses[i].arrivalTime < nextArrival) {
                    nextArrival = tempProcesses[i].arrivalTime;
                }
            }
            if (nextArrival != INT_MAX) {
                currentTime = nextArrival;
            } else {
                break;  // All processes completed
            }
        } else {
            // Execute shortest job to completion
            currentTime += tempProcesses[shortestIndex].remainingTime;
            tempProcesses[shortestIndex].completionTime = currentTime;
            tempProcesses[shortestIndex].remainingTime = 0;
            completed++;
        }
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

/**
 * MAIN FUNCTION: Program entry point and algorithm comparison
 * 
 * PROGRAM FLOW:
 * 1. Validate command line arguments
 * 2. Read process data from input file
 * 3. Execute all five scheduling algorithms
 * 4. Display average turnaround times for comparison
 * 
 * ALGORITHM COMPARISON:
 * - FCFS: Simple but can have poor performance
 * - LCFS-NP: Stack-based, can cause starvation
 * - LCFS-P: High overhead due to frequent preemption
 * - RR: Fair, good for interactive systems
 * - SJF: Optimal average turnaround time
 * 
 * EDUCATIONAL VALUE:
 * - Demonstrates trade-offs between different scheduling approaches
 * - Shows impact of preemption vs non-preemption
 * - Illustrates importance of algorithm selection based on system goals
 */
int main(int argc, char* argv[]) {
    // Validate command line arguments
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    // Load process data
    readInputFile(argv[1]);

    // Execute and compare all scheduling algorithms
    printf("FCFS: mean turnaround = %.2f\n", fcfs());
    printf("LCFS (NP): mean turnaround = %.2f\n", lcfsNonPreemptive());
    printf("LCFS (P): mean turnaround = %.2f\n", lcfsPreemptive());
    printf("RR: mean turnaround = %.2f\n", roundRobin());
    printf("SJF: mean turnaround = %.2f\n", sjf());

    return 0;
}