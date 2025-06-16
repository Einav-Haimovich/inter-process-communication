#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define REQUEST_TIMEOUT_SECONDS 60

int isRequestReceived = 0;

void parseInput(char *buffer, int *clientPID, int *num1, int *operation, int *num2) {
    // Parse the input buffer and extract the values
    char *token;
    token = strtok(buffer, " ");
    *clientPID = atoi(token);

    token = strtok(NULL, " ");
    *num1 = atoi(token);

    token = strtok(NULL, " ");
    *operation = atoi(token);

    token = strtok(NULL, " ");
    *num2 = atoi(token);
}

void intToStr(int num, char *str) {
    int i = 0, j = 0;
    char temp[10];

    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    while (num > 0) {
        temp[i++] = (num % 10) + '0';
        num /= 10;
    }

    temp[i] = '\0';

    for (j = 0; j < i; j++) {
        str[j] = temp[i - j - 1];
    }

    str[i] = '\0';
}

void performCalculation(int clientPID, int num1, int operation, int num2) {
    // Perform calculation
    int result;
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
            if (num2 != 0)
                result = num1 / num2;
            else {
                printf("ERROR_FROM_EX2\n");
                exit(0);
            }
            break;
        default:
            printf("ERROR_FROM_EX2\n");
            exit(0);
    }

    // Create a response file
    char responseFile[64];
    intToStr(clientPID, responseFile);
    strcat(responseFile, "_toClient.txt");
    int responseFD = open(responseFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (responseFD < 0) {
        perror("ERROR_FROM_EX2\n");
        exit(0);
    }

    // Write the result to the response file
    char buffer[256];
    intToStr(result, buffer);
    ssize_t bytesWritten = write(responseFD, buffer, strlen(buffer));
    if (bytesWritten < 0) {
        perror("ERROR_FROM_EX2\n");
        exit(0);
    }

    close(responseFD);  // Close the response file

    // Send a kill signal back to the client process
    kill(clientPID, SIGUSR1);
    printf("Server - Created response file '%s' for client with PID %d. end of stage g.\n", responseFile, clientPID);
}

void signalHandler(int signal) {
    if (signal == SIGUSR1) {
        // Read the file content
        int fd = open("toServer.txt", O_RDONLY);
        if (fd < 0) {
            perror("ERROR_FROM_EX2\n");
            exit(0);
        }

        struct stat fileStat;
        if (fstat(fd, &fileStat) < 0) {
            perror("ERROR_FROM_EX2\n");
            close(fd);
            exit(0);
        }

        off_t fileSize = fileStat.st_size;
        char *buffer = malloc(fileSize + 1);
        if (buffer == NULL) {
            perror("ERROR_FROM_EX2\n");
            close(fd);
            exit(0);
        }

        ssize_t bytesRead = read(fd, buffer, fileSize);
        if (bytesRead < 0) {
            perror("ERROR_FROM_EX2\n");
            close(fd);
            free(buffer);
            exit(0);
        }
        buffer[bytesRead] = '\0';

        close(fd);  // Close the file

        // Remove the "toServer.txt" file
        if (remove("toServer.txt") != 0) {
            perror("ERROR_FROM_EX2\n");
            free(buffer);
            exit(0);
        }

        // Parse the input
        int clientPID, num1, operation, num2;
        parseInput(buffer, &clientPID, &num1, &operation, &num2);

        // Reset the request received flag
        isRequestReceived = 1;

        // Fork a child process to perform the calculation
        pid_t pid = fork();
        if (pid == -1) {
            perror("ERROR_FROM_EX2\n");
            free(buffer);
            exit(1);
        } else if (pid == 0) {
            // Child process
            // Perform calculation and write result to the response file
            performCalculation(clientPID, num1, operation, num2);

            free(buffer);
            printf("Server - performed calculation, sent the result to toClient.txt file. end of stage i.");
            exit(0);
        } else {
            // Parent process
            printf("Server - Child process created with PID: %d. end of stage f.\n", pid);
            wait(NULL);  // Wait for the child process to finish
            free(buffer);
        }
    }
}

void timerHandler(int signal) {
    if (!isRequestReceived) {
        printf("ERROR_FROM_EX2 - no signal was given in the last 60 seconds\n");
        exit(0);
    }
}

int main() {
    // Set up signal handler for SIGUSR1
    signal(SIGUSR1, signalHandler);

    // Set up timer handler for request timeout
    signal(SIGALRM, timerHandler);
    alarm(REQUEST_TIMEOUT_SECONDS);

    while (1) {
        pause();
    }

    return 0;
}
