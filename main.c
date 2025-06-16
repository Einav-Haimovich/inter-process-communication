#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_PROCESSES 100

typedef struct {
    int arrivalTime;
    int computationTime;
    int remainingTime;
    int completionTime;
} Process;

Process processes[MAX_PROCESSES];
int numProcesses = 0;

void swap(Process* a, Process* b) {
    Process temp = *a;
    *a = *b;
    *b = temp;
}

void bSort(Process* arr, int n, int (*compare)(Process, Process)) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (compare(arr[j], arr[j + 1]) > 0) {
                swap(&arr[j], &arr[j + 1]);
            }
        }
    }
}

int compareArrival(Process a, Process b) {
    return a.arrivalTime - b.arrivalTime;
}

int compareShortest(Process a, Process b) {
    return a.computationTime - b.computationTime;
}

void readInputFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    fscanf(file, "%d", &numProcesses);

    for (int i = 0; i < numProcesses; i++) {
        fscanf(file, "%d,%d", &processes[i].arrivalTime, &processes[i].computationTime);
        processes[i].remainingTime = processes[i].computationTime;
        processes[i].completionTime = 0;
    }

    fclose(file);
}

float calculateAverageTurnaround(Process* arr, int n) {
    float totalTurnaround = 0;
    for (int i = 0; i < n; i++) {
        totalTurnaround += arr[i].completionTime - arr[i].arrivalTime;
    }
    return totalTurnaround / n;
}

float fcfs() {
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);
    bSort(tempProcesses, numProcesses, compareArrival);

    int currentTime = 0;
    for (int i = 0; i < numProcesses; i++) {
        if (currentTime < tempProcesses[i].arrivalTime) {
            currentTime = tempProcesses[i].arrivalTime;
        }
        currentTime += tempProcesses[i].computationTime;
        tempProcesses[i].completionTime = currentTime;
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

float lcfsNonPreemptive() {
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);
    bSort(tempProcesses, numProcesses, compareArrival);

    int currentTime = 0;
    int completed = 0;
    int stack[MAX_PROCESSES];
    int top = -1;

    while (completed < numProcesses) {
        // Add newly arrived processes to the stack
        for (int i = 0; i < numProcesses; i++) {
            if (tempProcesses[i].arrivalTime <= currentTime && tempProcesses[i].remainingTime > 0) {
                stack[++top] = i;
            }
        }

        if (top >= 0) {
            int i = stack[top--];
            if (currentTime < tempProcesses[i].arrivalTime) {
                currentTime = tempProcesses[i].arrivalTime;
            }
            currentTime += tempProcesses[i].computationTime;
            tempProcesses[i].completionTime = currentTime;
            tempProcesses[i].remainingTime = 0;
            completed++;
        }
        else {
            // If no process is available, move to the next arrival time
            int nextArrival = INT_MAX;
            for (int i = 0; i < numProcesses; i++) {
                if (tempProcesses[i].arrivalTime > currentTime && tempProcesses[i].arrivalTime < nextArrival) {
                    nextArrival = tempProcesses[i].arrivalTime;
                }
            }
            if (nextArrival != INT_MAX) {
                currentTime = nextArrival;
            }
            else {
                break;  // All processes have been scheduled
            }
        }
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

float lcfsPreemptive() {
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);
    bSort(tempProcesses, numProcesses, compareArrival);

    int currentTime = 0;
    int completed = 0;
    int stack[MAX_PROCESSES];
    int top = -1;

    while (completed < numProcesses) {
        while (top >= 0) {
            int i = stack[top];
            if (tempProcesses[i].remainingTime > 0) {
                tempProcesses[i].remainingTime--;
                currentTime++;
                if (tempProcesses[i].remainingTime == 0) {
                    tempProcesses[i].completionTime = currentTime;
                    completed++;
                    top--;
                }
                break;
            }
            else {
                top--;
            }
        }

        if (top < 0) {
            int nextArrival = INT_MAX;
            for (int i = 0; i < numProcesses; i++) {
                if (tempProcesses[i].arrivalTime > currentTime && tempProcesses[i].arrivalTime < nextArrival) {
                    nextArrival = tempProcesses[i].arrivalTime;
                }
            }
            if (nextArrival != INT_MAX) {
                currentTime = nextArrival;
            }
            else {
                break;  // All processes have been scheduled
            }
        }

        for (int i = 0; i < numProcesses; i++) {
            if (tempProcesses[i].arrivalTime <= currentTime && tempProcesses[i].remainingTime > 0) {
                stack[++top] = i;
            }
        }
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

float roundRobin() {
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);

    int currentTime = 0;
    int completed = 0;
    int queue[MAX_PROCESSES];
    int front = 0, rear = -1;
    int timeQuantum = 2;

    while (completed < numProcesses) {
        // Add newly arrived processes to the queue
        for (int i = 0; i < numProcesses; i++) {
            if (tempProcesses[i].arrivalTime <= currentTime && tempProcesses[i].remainingTime > 0) {
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
            int i = queue[front++];
            if (tempProcesses[i].remainingTime <= timeQuantum) {
                currentTime += tempProcesses[i].remainingTime;
                tempProcesses[i].completionTime = currentTime;
                tempProcesses[i].remainingTime = 0;
                completed++;
            }
            else {
                currentTime += timeQuantum;
                tempProcesses[i].remainingTime -= timeQuantum;

                // Add newly arrived processes before re-adding the current process
                for (int j = 0; j < numProcesses; j++) {
                    if (tempProcesses[j].arrivalTime <= currentTime && tempProcesses[j].remainingTime > 0) {
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
                queue[++rear] = i;
            }
        }
        else {
            // Find the next arrival time if the queue is empty
            int nextArrival = INT_MAX;
            for (int i = 0; i < numProcesses; i++) {
                if (tempProcesses[i].arrivalTime > currentTime && tempProcesses[i].arrivalTime < nextArrival) {
                    nextArrival = tempProcesses[i].arrivalTime;
                }
            }
            if (nextArrival != INT_MAX) {
                currentTime = nextArrival;
            }
            else {
                break;  // All processes have been scheduled
            }
        }
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

float sjf() {
    Process tempProcesses[MAX_PROCESSES];
    memcpy(tempProcesses, processes, sizeof(Process) * numProcesses);
    bSort(tempProcesses, numProcesses, compareArrival);

    int currentTime = 0;
    int completed = 0;

    while (completed < numProcesses) {
        int shortestIndex = -1;
        int shortestTime = INT_MAX;

        for (int i = 0; i < numProcesses; i++) {
            if (tempProcesses[i].arrivalTime <= currentTime && tempProcesses[i].remainingTime > 0) {
                if (tempProcesses[i].remainingTime < shortestTime) {
                    shortestTime = tempProcesses[i].remainingTime;
                    shortestIndex = i;
                }
            }
        }

        if (shortestIndex == -1) {
            int nextArrival = INT_MAX;
            for (int i = 0; i < numProcesses; i++) {
                if (tempProcesses[i].arrivalTime > currentTime && tempProcesses[i].arrivalTime < nextArrival) {
                    nextArrival = tempProcesses[i].arrivalTime;
                }
            }
            if (nextArrival != INT_MAX) {
                currentTime = nextArrival;
            }
            else {
                break;  // All processes have been scheduled
            }
        }
        else {
            currentTime += tempProcesses[shortestIndex].remainingTime;
            tempProcesses[shortestIndex].completionTime = currentTime;
            tempProcesses[shortestIndex].remainingTime = 0;
            completed++;
        }
    }

    return calculateAverageTurnaround(tempProcesses, numProcesses);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    readInputFile(argv[1]);

    printf("FCFS: mean turnaround = %.2f\n", fcfs());
    printf("LCFS (NP): mean turnaround = %.2f\n", lcfsNonPreemptive());
    printf("LCFS (P): mean turnaround = %.2f\n", lcfsPreemptive());
    printf("RR: mean turnaround = %.2f\n", roundRobin());
    printf("SJF: mean turnaround = %.2f\n", sjf());

    return 0;
}
