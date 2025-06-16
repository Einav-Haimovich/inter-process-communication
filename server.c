/**
 * CALCULATOR SERVER - Inter-Process Communication System
 * 
 * This server implements a file-based IPC calculator that:
 * 1. Waits for SIGUSR1 signals from clients
 * 2. Reads calculation requests from "toServer.txt"
 * 3. Forks child processes to handle calculations
 * 4. Writes results to client-specific response files
 * 5. Signals clients when results are ready
 * 
 * COMMUNICATION PROTOCOL:
 * - Client writes: "{clientPID} {num1} {operation} {num2}" to toServer.txt
 * - Server reads request, deletes toServer.txt
 * - Server forks child to calculate result
 * - Child writes result to "{clientPID}_toClient.txt"
 * - Child sends SIGUSR1 to client
 * 
 * OPERATIONS SUPPORTED:
 * 1 = Addition, 2 = Subtraction, 3 = Multiplication, 4 = Division
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define REQUEST_TIMEOUT_SECONDS 60  // Server timeout for client requests

// Global flag to track if any request was received (for timeout handling)
int isRequestReceived = 0;

/**
 * FUNCTION: parseInput
 * PURPOSE: Parse space-delimited input string into calculation parameters
 * 
 * PARAMETERS:
 * - buffer: Input string containing "{clientPID} {num1} {operation} {num2}"
 * - clientPID: Output pointer for client process ID
 * - num1: Output pointer for first operand
 * - operation: Output pointer for operation code (1-4)
 * - num2: Output pointer for second operand
 * 
 * ALGORITHM:
 * 1. Use strtok() to tokenize space-separated values
 * 2. Convert string tokens to integers using atoi()
 * 3. Store parsed values in output parameters
 * 
 * ERROR HANDLING: None (assumes well-formed input)
 */
void parseInput(char *buffer, int *clientPID, int *num1, int *operation, int *num2) {
    char *token;

    // Extract client PID
    token = strtok(buffer, " ");
    *clientPID = atoi(token);

    // Extract first operand
    token = strtok(NULL, " ");
    *num1 = atoi(token);

    // Extract operation code
    token = strtok(NULL, " ");
    *operation = atoi(token);

    // Extract second operand
    token = strtok(NULL, " ");
    *num2 = atoi(token);
}

/**
 * FUNCTION: intToStr
 * PURPOSE: Convert integer to string (custom implementation)
 * 
 * ALGORITHM:
 * 1. Handle special case of 0
 * 2. Extract digits in reverse order using modulo/division
 * 3. Store digits in temporary buffer
 * 4. Reverse the digits to get correct order
 * 5. Null-terminate the result string
 * 
 * WHY CUSTOM IMPLEMENTATION:
 * - Avoids dependency on sprintf/snprintf
 * - Demonstrates low-level string manipulation
 * - Educational value for understanding number-to-string conversion
 */
void intToStr(int num, char *str) {
    int i = 0, j = 0;
    char temp[10];

    // Handle zero case
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    // Extract digits in reverse order
    while (num > 0) {
        temp[i++] = (num % 10) + '0';  // Convert digit to ASCII
        num /= 10;
    }

    temp[i] = '\0';

    // Reverse the digits to get correct order
    for (j = 0; j < i; j++) {
        str[j] = temp[i - j - 1];
    }

    str[i] = '\0';
}

/**
 * FUNCTION: performCalculation
 * PURPOSE: Execute calculation and create response file for client
 * 
 * PROCESS FLOW:
 * 1. Perform arithmetic operation based on operation code
 * 2. Create client-specific response file "{clientPID}_toClient.txt"
 * 3. Write result to response file
 * 4. Send SIGUSR1 signal to client to notify completion
 * 
 * ERROR HANDLING:
 * - Division by zero: Print error and exit
 * - Invalid operation: Print error and exit
 * - File operations: Check return values, exit on failure
 * 
 * SECURITY CONSIDERATIONS:
 * - Response file created with restricted permissions (S_IRUSR | S_IWUSR)
 * - Only owner can read/write the response file
 */
void performCalculation(int clientPID, int num1, int operation, int num2) {
    int result;

    // Perform calculation based on operation code
    switch (operation) {
        case 1: // Addition
            result = num1 + num2;
            break;
        case 2: // Subtraction
            result = num1 - num2;
            break;
        case 3: // Multiplication
            result = num1 * num2;
            break;
        case 4: // Division
            if (num2 != 0) {
                result = num1 / num2;  // Integer division
            } else {
                printf("ERROR_FROM_EX2\n");
                exit(0);
            }
            break;
        default:
            printf("ERROR_FROM_EX2\n");
            exit(0);
    }

    // Create response file name: "{clientPID}_toClient.txt"
    char responseFile[64];
    intToStr(clientPID, responseFile);
    strcat(responseFile, "_toClient.txt");

    // Create response file with restricted permissions
    int responseFD = open(responseFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (responseFD < 0) {
        perror("ERROR_FROM_EX2\n");
        exit(0);
    }

    // Convert result to string and write to file
    char buffer[256];
    intToStr(result, buffer);
    ssize_t bytesWritten = write(responseFD, buffer, strlen(buffer));
    if (bytesWritten < 0) {
        perror("ERROR_FROM_EX2\n");
        exit(0);
    }

    close(responseFD);

    // Notify client that result is ready
    kill(clientPID, SIGUSR1);
    printf("Server - Created response file '%s' for client with PID %d. end of stage g.\n", 
           responseFile, clientPID);
}

/**
 * FUNCTION: signalHandler
 * PURPOSE: Handle SIGUSR1 signals from clients (main request processing)
 * 
 * DETAILED PROCESS FLOW:
 * 1. Open and read "toServer.txt" file
 * 2. Get file size using fstat() for proper buffer allocation
 * 3. Allocate dynamic buffer based on file size
 * 4. Read entire file content into buffer
 * 5. Delete "toServer.txt" to prevent conflicts with other clients
 * 6. Parse the request string to extract parameters
 * 7. Fork a child process to handle the calculation
 * 8. Child process performs calculation and creates response
 * 9. Parent process waits for child completion
 * 
 * CONCURRENCY HANDLING:
 * - Each client request is handled by a separate child process
 * - Parent server remains available for new requests
 * - Child processes handle file I/O and calculations independently
 * 
 * MEMORY MANAGEMENT:
 * - Dynamic allocation based on actual file size
 * - Proper cleanup with free() in all code paths
 * - Buffer null-termination for string safety
 */
void signalHandler(int signal) {
    if (signal == SIGUSR1) {
        // Open request file
        int fd = open("toServer.txt", O_RDONLY);
        if (fd < 0) {
            perror("ERROR_FROM_EX2\n");
            exit(0);
        }

        // Get file size for proper buffer allocation
        struct stat fileStat;
        if (fstat(fd, &fileStat) < 0) {
            perror("ERROR_FROM_EX2\n");
            close(fd);
            exit(0);
        }

        off_t fileSize = fileStat.st_size;

        // Allocate buffer based on actual file size
        char *buffer = malloc(fileSize + 1);
        if (buffer == NULL) {
            perror("ERROR_FROM_EX2\n");
            close(fd);
            exit(0);
        }

        // Read file content
        ssize_t bytesRead = read(fd, buffer, fileSize);
        if (bytesRead < 0) {
            perror("ERROR_FROM_EX2\n");
            close(fd);
            free(buffer);
            exit(0);
        }
        buffer[bytesRead] = '\0';  // Null-terminate for string operations

        close(fd);

        // Remove request file to prevent conflicts
        if (remove("toServer.txt") != 0) {
            perror("ERROR_FROM_EX2\n");
            free(buffer);
            exit(0);
        }

        // Parse request parameters
        int clientPID, num1, operation, num2;
        parseInput(buffer, &clientPID, &num1, &operation, &num2);

        // Mark that a request was received (for timeout handling)
        isRequestReceived = 1;

        // Fork child process to handle calculation
        pid_t pid = fork();
        if (pid == -1) {
            perror("ERROR_FROM_EX2\n");
            free(buffer);
            exit(1);
        } else if (pid == 0) {
            // CHILD PROCESS: Handle calculation
            performCalculation(clientPID, num1, operation, num2);
            free(buffer);
            printf("Server - performed calculation, sent the result to toClient.txt file. end of stage i.");
            exit(0);
        } else {
            // PARENT PROCESS: Continue serving other clients
            printf("Server - Child process created with PID: %d. end of stage f.\n", pid);
            wait(NULL);  // Wait for child completion
            free(buffer);
        }
    }
}

/**
 * FUNCTION: timerHandler
 * PURPOSE: Handle timeout when no client requests are received
 * 
 * TIMEOUT MECHANISM:
 * - Server sets 60-second alarm using alarm()
 * - If no SIGUSR1 received within timeout, server exits
 * - Prevents server from running indefinitely without clients
 * - Ensures clean resource cleanup
 */
void timerHandler(int signal) {
    if (!isRequestReceived) {
        printf("ERROR_FROM_EX2 - no signal was given in the last 60 seconds\n");
        exit(0);
    }
}

/**
 * MAIN FUNCTION: Server initialization and event loop
 * 
 * INITIALIZATION:
 * 1. Register signal handlers for SIGUSR1 and SIGALRM
 * 2. Set 60-second timeout alarm
 * 3. Enter infinite event loop using pause()
 * 
 * EVENT LOOP:
 * - pause() suspends process until signal received
 * - Signal handlers process events asynchronously
 * - Server remains responsive to multiple clients
 * 
 * SIGNAL HANDLING ARCHITECTURE:
 * - SIGUSR1: Client request processing
 * - SIGALRM: Timeout handling
 * - Asynchronous, event-driven design
 */
int main() {
    // Register signal handlers
    signal(SIGUSR1, signalHandler);  // Handle client requests
    signal(SIGALRM, timerHandler);   // Handle timeout

    // Set timeout alarm
    alarm(REQUEST_TIMEOUT_SECONDS);

    // Enter event loop - wait for signals
    while (1) {
        pause();  // Suspend until signal received
    }

    return 0;
}