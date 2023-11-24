// nfs client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 12345
#define MAX_FILENAME 100
#define MAX_CONTENT 1000

void send_file_request(const char *filename, int clientSocket) {

    ssize_t bytes_sent = send(clientSocket, filename, strlen(filename), 0);
    if (bytes_sent != strlen(filename)) {
        perror("Error sending file request");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
}

void send_file_content(const char *content, int clientSocket) {

    ssize_t bytes_sent = send(clientSocket, content, strlen(content), 0);
    if (bytes_sent != strlen(content)) {
        perror("Error sending file content");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
}

void receive_file_content(char *buffer, size_t size, int clientSocket) {

    ssize_t bytes_received = recv(clientSocket, buffer, size, 0);
    if (bytes_received <= 0) {
        perror("Error receiving file content");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0'; 
}

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;


    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }


    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 


    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error connecting to server");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    char option;
    char filename[MAX_FILENAME];
    char content[MAX_CONTENT];



        printf("\nMenu:\n");
        printf("1. Create a new file\n");
        printf("2. Read an existing file\n");
        printf("3. Exit\n");
        printf("Enter your choice (1, 2, or 3): ");
        scanf(" %c", &option);


        while (getchar() != '\n');

        switch (option) {
            case '1':

                printf("Enter the filename to create: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = '\0'; 

                send_file_request(filename, clientSocket);

                printf("Enter the content for the file:\n");
                fgets(content, sizeof(content), stdin);
                content[strcspn(content, "\n")] = '\0'; 


                send_file_content(content, clientSocket);

                break;

            case '2':

                printf("Enter the filename to request: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = '\0'; 

                send_file_request(filename, clientSocket);


                receive_file_content(content, sizeof(content), clientSocket);
                printf("File content:\n%s\n", content);

                break;

            case '3':

                close(clientSocket);
                printf("Exiting...\n");
                exit(EXIT_SUCCESS);

            default:
                printf("Invalid option. Please enter 1, 2, or 3.\n");
        }


    return 0;
}
