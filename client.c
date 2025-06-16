#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/random.h>

#define MAX_RETRIES 10
#define RESPONSE_TIMEOUT_SECONDS 30

char responseFile[64];  // Declare responseFile globally

void intToStr(int num, char *str); // Function prototype for intToStr

void signalHandler(int signal) {
    // Read the response file
    int myPID = getpid();
    intToStr(myPID, responseFile);
    strcat(responseFile, "_toClient.txt");

    // Wait until the response file is created by the server
    while (access(responseFile, F_OK) == -1) {
        usleep(10000); // Sleep for 10 milliseconds
    }

    // Reset the alarm timer
    alarm(0);

    // Open the response file for reading
    int responseFD = open(responseFile, O_RDONLY);
    if (responseFD < 0) {
        perror("ERROR_FROM_EX2");
        exit(0);
    }

    // Read the result from the response file
    char responseBuffer[256];
    ssize_t bytesRead = read(responseFD, responseBuffer, sizeof(responseBuffer) - 1);
    if (bytesRead < 0) {
        perror("ERROR_FROM_EX2");
        exit(0);
    }
    responseBuffer[bytesRead] = '\0';

    // Print the received result
    printf("Client - Received result from server: %s. end of stage j.\n", responseBuffer);

    // Close the response file
    close(responseFD);

    // Delete the response file
    if (remove(responseFile) != 0) {
        perror("ERROR_FROM_EX2");
    }

    exit(0); // Add this line to exit the client program after receiving the result and deleting the file
}

void timerHandler(int signal) {
    printf("ERROR_FROM_EX2\n");
    exit(0);
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

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("ERROR_FROM_EX2\n");
        exit(-1);
    }

    // Set up signal handler for SIGUSR1 to receive the result
    signal(SIGUSR1, signalHandler);

    // Set up timer handler for response timeout
    signal(SIGALRM, timerHandler);
    alarm(RESPONSE_TIMEOUT_SECONDS);

    // Generate the random delay
    unsigned int randomDelay;
    ssize_t bytesRead = getrandom(&randomDelay, sizeof(randomDelay), 0);
    if (bytesRead < 0) {
        perror("ERROR_FROM_EX2");
        exit(-1);
    }
    randomDelay %= 6; // Get a number between 0 and 5

    // Retry generating the file for a maximum number of times
    int retries = 0;
    while (retries < MAX_RETRIES) {
        usleep((randomDelay + 1) * 1000000); // Sleep for randomDelay seconds

        // Write to toServer
        int myPID = getpid();
        intToStr(myPID, responseFile);
        strcat(responseFile, " ");
        strcat(responseFile, argv[2]);
        strcat(responseFile, " ");
        strcat(responseFile, argv[3]);
        strcat(responseFile, " ");
        strcat(responseFile, argv[4]);

        int toServer = open("./toServer.txt", O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (toServer == -1) {
            perror("ERROR_FROM_EX2");
            retries++;
        } else {
            ssize_t bytesWritten = write(toServer, responseFile, strlen(responseFile));
            if (bytesWritten == -1) {
                perror("ERROR_FROM_EX2");
            } else {
                close(toServer);
                break; // Exit the loop on successful write
            }

            close(toServer);
            retries++;
        }
    }

    // Check if the maximum number of retries was reached
    if (retries == MAX_RETRIES) {
        printf("ERROR_FROM_EX2\n");
        exit(-1);
    }

    // Send signal to the server
    int processID = atoi(argv[1]);
    int result = kill(processID, SIGUSR1);

    if (result == 0) {
        printf("Client - Signal successfully sent to process with PID %d. end of stage d.\n", processID);
    } else {
        perror("ERROR_FROM_EX2");
    }

    // Wait for the response from the server
    pause();

    return 0;
}
