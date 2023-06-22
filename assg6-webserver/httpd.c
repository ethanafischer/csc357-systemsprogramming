#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 10

void send_response(int client_socket, int status_code, const char *content_type, const char *content, int content_length) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.1 %d OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", status_code, content_type, content_length);
    send(client_socket, response, strlen(response), 0);
    if (content != NULL && content_length > 0) {
        send(client_socket, content, content_length, 0);
    }
}

void handle_client_request(int client_socket, char *kvstore_fifo) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("recv");
        close(client_socket);
        exit(1);
    }
    buffer[bytes_received] = '\0';

    char method[8], endpoint[BUFFER_SIZE], version[16];
    sscanf(buffer, "%s %s %s", method, endpoint, version);

    if (strcmp(method, "GET") == 0 || strcmp(method, "HEAD") == 0) {
        if (strncmp(endpoint, "/kv/", 4) == 0) {
            // Handle GET /kv/<key>
            char *key = endpoint + 4;

            // Open the FIFO for writing
            int fifo = open(kvstore_fifo, O_WRONLY);
            if (fifo == -1) {
                perror("open");
                send_response(client_socket, 500, "text/html", "Internal Server Error", 21);
                close(client_socket);
                exit(1);
            }

            // Write the GET request to the FIFO
            char kvstore_request[BUFFER_SIZE];
            snprintf(kvstore_request, sizeof(kvstore_request), "GET %s", key);
            write(fifo, kvstore_request, strlen(kvstore_request));

            // Read the response from the FIFO
            char kvstore_response[BUFFER_SIZE];
            ssize_t bytes_read = read(fifo, kvstore_response, sizeof(kvstore_response) - 1);
            close(fifo);

            if (bytes_read == -1) {
                perror("read");
                send_response(client_socket, 500, "text/html", "Internal Server Error", 21);
                close(client_socket);
                exit(1);
            } else if (bytes_read == 0) {
                send_response(client_socket, 404, "text/html", "Not Found", 9);
            } else {
                kvstore_response[bytes_read] = '\0';
                send_response(client_socket, 200, "text/plain", kvstore_response, bytes_read);
            }
        } else {
            // Handle GET /file
            int fd = open(endpoint + 1, O_RDONLY);
            if (fd < 0) {
                send_response(client_socket, 404, "text/html", "Not Found", 9);
            } else {
                struct stat st;
                fstat(fd, &st);
                int content_length = st.st_size;
                if (strcmp(method, "HEAD") == 0) {
                    send_response(client_socket, 200, "text/html", NULL, content_length);
                } else {
                    char *content = malloc(content_length);
                    read(fd, content, content_length);
                    send_response(client_socket, 200, "text/html", content, content_length);
                    free(content);
                }
                close(fd);
            }
        }
    } else if (strcmp(method, "PUT") == 0) {
        if (strncmp(endpoint, "/kv/", 4) == 0) {
            // Handle PUT /kv/<key>
            char *key = endpoint + 4;
            char *value = strstr(buffer, "\r\n\r\n") + 4;

            // Open the FIFO for writing
            int fifo = open(kvstore_fifo, O_WRONLY);
            if (fifo == -1) {
                perror("open");
                send_response(client_socket, 500, "text/html", "Internal Server Error", 21);
                close(client_socket);
                exit(1);
            }

            // Write the PUT request to the FIFO
            char kvstore_request[BUFFER_SIZE];
            snprintf(kvstore_request, sizeof(kvstore_request), "PUT %s %s", key, value);
            write(fifo, kvstore_request, strlen(kvstore_request));

            close(fifo);

            send_response(client_socket, 200, "text/html", "OK", 2);
        } else {
            send_response(client_socket, 403, "text/html", "Forbidden", 9);
        }
    } else {
        send_response(client_socket, 501, "text/html", "Not Implemented", 15);
    }

    close(client_socket);
    exit(0);
}

int setup_server_socket(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(1);
    }

    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_socket, MAX_CONNECTIONS) < 0) {
        perror("listen");
        exit(1);
    }

    return server_socket;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <kvstore_fifo> <port>\n", argv[0]);
        exit(1);
    }

    char *kvstore_fifo = argv[1];
    int port = atoi(argv[2]);

    int server_socket = setup_server_socket(port);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // Child process
        char *args[] = {"../../assg5/csc357-assignment5-ethanafischer/kvstore", kvstore_fifo, NULL};
		execvp("./kvstore", args);
        perror("execvp");
        exit(1);
    } else {
        // Parent process
        while (1) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_socket < 0) {
                perror("accept");
                continue;
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                close(client_socket);
                continue;
            }

            if (pid == 0) {
                close(server_socket);
                handle_client_request(client_socket, kvstore_fifo);
                exit(0);
            } else {
                close(client_socket);
                waitpid(pid, NULL, 0);
            }
        }
    }

    close(server_socket);
    return 0;
}
