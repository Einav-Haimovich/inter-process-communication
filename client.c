/**
 * CALCULATOR CLIENT - Inter-Process Communication System
 * 
 * This client implements a file-based IPC calculator client that:
 * 1. Validates command-line arguments for calculation request
 * 2. Creates request file with random delay to avoid conflicts
 * 3. Sends SIGUSR1 signal to server to notify of request
 * 4. Waits for server response via SIGUSR1 signal
 * 5. Reads result from server-created response file
 * 6. Cleans up response file and exits
 * 
 * USAGE: ./client <serverPID> <num1> <operation> <num2>
 * 
 * COMMUNICATION PROTOCOL:
 * - Client writes: "{clientPID} {num1} {operation} {num2}" to toServer.txt
 * - Client sends SIGUSR1 to server
 * - Server processes request and creates "{clientPID}_toClient.txt"
 * - Server sends SIGUSR1 back to client
 * - Client reads result and cleans up
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
#include <sys/random.h>

#define MAX_RETRIES 10                    // Maximum attempts to create request file
#define RESPONSE_TIMEOUT_SECONDS 30       // Timeout for server response

// Global variable for response file name (used in signal handler)
char responseFile[64];

// Function prototype
void intToStr(int num, char *str);

/**
 * FUNCTION: signalHandler
 * PURPOSE: Handle SIGUSR1 signal from server (response ready notification)
 * 
 * DETAILED PROCESS FLOW:
 * 1. Construct response file name using client PID
 * 2. Poll for response file existence (busy-wait with small delays)
 * 3. Cancel timeout alarm once file is found
 * 4. Read result from response file
 * 5. Display result to user
 * 6. Clean up response file
 * 7. Exit client process
 * 
 * SYNCHRONIZATION MECHANISM:
 * - Server creates response file then sends SIGUSR1
 * - Client receives signal but file might not be fully written
 * - Polling loop ensures file exists before reading
 * - 10ms sleep prevents excessive CPU usage during polling
 * 
 * ERROR HANDLING:
 * - File access errors cause immediate exit
 * - Read errors are handled with perror() and exit
 * - File cleanup errors are logged but don't prevent exit
 */
void signalHandler(int signal) {
    // Construct response file name: "{clientPID}_toClient.txt"
    int myPID = getpid();
    intToStr(myPID, responseFile);
    strcat(responseFile, "_toClient.txt");

    // Wait until response file is created by server
    // This polling is necessary because signal arrival doesn't guarantee
    // file is fully written by the server process
    while (access(responseFile, F_OK) == -1) {
        usleep(10000); // Sleep for 10 milliseconds to avoid busy-waiting
    }

    // Cancel timeout alarm - response received successfully
    alarm(0);

    // Open response file for reading
    int responseFD = open(responseFile, O_RDONLY);
    if (responseFD < 0) {
        perror("ERROR_FROM_EX2");
        exit(0);
    }

    // Read calculation result
    char responseBuffer[256];
    ssize_t bytesRead = read(responseFD, responseBuffer, sizeof(responseBuffer) - 1);
    if (bytesRead < 0) {
        perror("ERROR_FROM_EX2");
        exit(0);
    }
    responseBuffer[bytesRead] = '\0';  // Null-terminate string

    // Display result to user
    printf("Client - Received result from server: %s. end of stage j.\n", responseBuffer);

    // Clean up response file
    close(responseFD);
    if (remove(responseFile) != 0) {
        perror("ERROR_FROM_EX2");  // Log error but don't exit
    }

    exit(0); // Successful completion
}

/**
 * FUNCTION: timerHandler
 * PURPOSE: Handle timeout when server doesn't respond within expected time
 * 
 * TIMEOUT MECHANISM:
 * - 30-second timeout set after sending request to server
 * - If no SIGUSR1 received within timeout, client exits with error
 * - Prevents client from hanging indefinitely
 * - Ensures responsive user experience
 */
void timerHandler(int signal) {
    printf("ERROR_FROM_EX2\n");
    exit(0);
}

/**
 * FUNCTION: intToStr
 * PURPOSE: Convert integer to string (identical to server implementation)
 * 
 * ALGORITHM:
 * 1. Handle special case of 0
 * 2. Extract digits in reverse order using modulo/division
 * 3. Store digits in temporary buffer
 * 4. Reverse digits to get correct order
 * 5. Null-terminate result string
 * 
 * DESIGN DECISION:
 * - Code duplication with server for simplicity
 * - Could be moved to shared header/library
 * - Demonstrates consistent implementation across processes
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

    // Reverse digits to get correct order
    for (j = 0; j < i; j++) {
        str[j] = temp[i - j - 1];
    }

    str[i] = '\0';
}

/**
 * MAIN FUNCTION: Client initialization and request processing
 * 
 * COMMAND LINE VALIDATION:
 * - Expects exactly 4 arguments: serverPID, num1, operation, num2
 * - No validation of argument content (assumes well-formed input)
 * 
 * SIGNAL SETUP:
 * - SIGUSR1: Handle server response
 * - SIGALRM: Handle response timeout
 * 
 * REQUEST CREATION PROCESS:
 * 1. Generate random delay (0-5 * 10ms) to avoid file conflicts
 * 2. Attempt to create "toServer.txt" with O_EXCL flag
 * 3. Retry up to MAX_RETRIES times if file creation fails
 * 4. Write request data to file in required format
 * 
 * RACE CONDITION HANDLING:
 * - O_EXCL flag ensures atomic file creation
 * - Random delays reduce probability of simultaneous attempts
 * - Retry mechanism handles temporary conflicts
 * - getrandom() provides cryptographically secure randomness
 */
int main(int argc, char* argv[]) {
    // Validate command line arguments
    if (argc != 5) {
        printf("ERROR_FROM_EX2\n");
        exit(-1);
    }

    // Set up signal handlers
    signal(SIGUSR1, signalHandler);  // Handle server response
    signal(SIGALRM, timerHandler);   // Handle response timeout
    alarm(RESPONSE_TIMEOUT_SECONDS); // Set response timeout

    // Generate cryptographically secure random delay
    // This helps avoid race conditions when multiple clients
    // attempt to create toServer.txt simultaneously
    unsigned int randomDelay;
    ssize_t bytesRead = getrandom(&randomDelay, sizeof(randomDelay), 0);
    if (bytesRead < 0) {
        perror("ERROR_FROM_EX2");
        exit(-1);
    }
    randomDelay %= 6; // Get number between 0 and 5

    // Retry mechanism for file creation
    int retries = 0;
    while (retries < MAX_RETRIES) {
        // Random delay to reduce collision probability
        usleep((randomDelay + 1) * 10000); // Sleep for (randomDelay+1) * 10ms

        // Construct request string: "{clientPID} {num1} {operation} {num2}"
        int myPID = getpid();
        intToStr(myPID, responseFile);  // Reuse global buffer
        strcat(responseFile, " ");
        strcat(responseFile, argv[2]);  // num1
        strcat(responseFile, " ");
        strcat(responseFile, argv[3]);  // operation
        strcat(responseFile, " ");
        strcat(responseFile, argv[4]);  // num2

        // Attempt atomic file creation
        // O_EXCL ensures file creation fails if file already exists
        // This prevents race conditions between multiple clients
        int toServer = open("./toServer.txt", O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (toServer == -1) {
            // File creation failed - likely another client is using it
            perror("ERROR_FROM_EX2");
            retries++;
        } else {
            // File created successfully - write request data
            ssize_t bytesWritten = write(toServer, responseFile, strlen(responseFile));
            if (bytesWritten == -1) {
                perror("ERROR_FROM_EX2");
            } else {
                close(toServer);
                break; // Success - exit retry loop
            }

            close(toServer);
            retries++;
        }
    }

    // Check if maximum retries exceeded
    if (retries == MAX_RETRIES) {
        printf("ERROR_FROM_EX2\n");
        exit(-1);
    }

    // Send notification signal to server
    int processID = atoi(argv[1]);  // Server PID from command line
    int result = kill(processID, SIGUSR1);

    if (result == 0) {
        printf("Client - Signal successfully sent to process with PID %d. end of stage d.\n", processID);
    } else {
        perror("ERROR_FROM_EX2");
    }

    // Wait for server response
    // pause() suspends process until signal received
    // Signal handler will process response and exit
    pause();

    return 0; // This line should never be reached
}