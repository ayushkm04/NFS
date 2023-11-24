// nfs server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#define PORT 12345
#define MAX_FILENAME 100
#define MAX_CONTENT 1000
#define FILES_FOLDER_PATH "/home/ayush/Desktop/nfs_files/"

typedef struct {
    int clientSocket;
    struct sockaddr_in clientAddr;
} ThreadArgs;



void write_file(const char *filename, const char *content) {
    char filepath[MAX_FILENAME + sizeof(FILES_FOLDER_PATH)];
    snprintf(filepath, sizeof(filepath), "%s%s", FILES_FOLDER_PATH, filename);

    int file = open(filepath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (file == -1) {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    ssize_t bytesWritten = write(file, content, strlen(content));
    if (bytesWritten == -1) {
        perror("Error writing to file");
        close(file);
        exit(EXIT_FAILURE);
    }

    close(file);
    printf("Wrote %zd bytes to file: %s\n", bytesWritten, filename);
}

void read_file(const char *filename, char *buffer, size_t size) {
    char filepath[MAX_FILENAME + sizeof(FILES_FOLDER_PATH)];
    snprintf(filepath, sizeof(filepath), "%s%s", FILES_FOLDER_PATH, filename);

    printf("Trying to read file from: %s\n", filepath);

    int file = open(filepath, O_RDONLY);
    if (file == -1) {
        perror("Error opening file");
        snprintf(buffer, size, "File not found: %s", filename);
        return;
    }

    ssize_t bytesRead = read(file, buffer, size);
    if (bytesRead == -1) {
        perror("Error reading file");
        close(file);
        exit(EXIT_FAILURE);
    }

    close(file);
    printf("Read %zd bytes from file: %s\n", bytesRead, filename);
}

void *handle_connection(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    int clientSocket = threadArgs->clientSocket;
    struct sockaddr_in clientAddr = threadArgs->clientAddr;

    char filename[MAX_FILENAME];
    char content[MAX_CONTENT];

    ssize_t bytes_received = recv(clientSocket, filename, sizeof(filename) - 1, 0);
    if (bytes_received <= 0) {
        perror("Error receiving file request");
        close(clientSocket);
        free(args);
        return NULL;
    }

    filename[bytes_received] = '\0';


    char filepath[MAX_FILENAME + sizeof(FILES_FOLDER_PATH)];
    snprintf(filepath, sizeof(filepath), "%s%s", FILES_FOLDER_PATH, filename);


    int file = open(filepath, O_RDONLY);
    if (file == -1) {
        if (errno == ENOENT) {

            printf("File not found, creating and writing content...\n");


            ssize_t content_received = recv(clientSocket, content, sizeof(content) - 1, 0);
            if (content_received <= 0) {
                perror("Error receiving file content");
                close(clientSocket);
                free(args);
                return NULL;
            }

            content[content_received] = '\0';


            write_file(filename, content);


            send(clientSocket, "File created successfully", sizeof("File created successfully"), 0);
        } else {

            perror("Error opening file");
        }
    } else {

        close(file);

        char buffer[MAX_CONTENT];
        read_file(filename, buffer, sizeof(buffer));
        send(clientSocket, buffer, sizeof(buffer), 0);
    }


    close(clientSocket);
    free(args);

    printf("Response sent to %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    return NULL;
}

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr;
    socklen_t addrSize = sizeof(struct sockaddr_in);


    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }


    int enable = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }


    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;


    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }


    if (listen(serverSocket, 5) == -1) {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

    printf("NFS Server listening on port %d...\n", PORT);


    char folderPath[MAX_FILENAME + sizeof(FILES_FOLDER_PATH)];
    snprintf(folderPath, sizeof(folderPath), "%s", FILES_FOLDER_PATH);

    if (access(folderPath, F_OK) != 0) {
        if (mkdir(folderPath, 0777) == -1) {
            perror("Error creating folder");
            exit(EXIT_FAILURE);
        }
    }


    while (1) {
        int clientSocket = accept(serverSocket, (struct sockaddr *)&serverAddr, &addrSize);
        if (clientSocket == -1) {
            perror("Error accepting connection");
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));


        pthread_t tid;
        ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        args->clientSocket = clientSocket;
        args->clientAddr = serverAddr;

        if (pthread_create(&tid, NULL, handle_connection, (void *)args) != 0) {
            perror("Error creating thread");
            close(clientSocket);
            free(args);
        }


        pthread_detach(tid);
    }


    close(serverSocket);

    return 0;
}
