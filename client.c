#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
int j = 0;
int port;
char host[64];
char message[5100] = {};
void *clientThread()
{
    int clientSocket, err;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        printf("socket error:%d\n", errno);
    }
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(host);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    addr_size = sizeof serverAddr;
    err = connect(clientSocket, (struct sockaddr *)&serverAddr, addr_size);
    if (err < 0)
    {
        printf("connect error: %d\n", errno);
    }
    char buffer[10240];
    if (send(clientSocket, message, strlen(message), 0) < 0)
    {
        printf("Send failed\n");
    }
    if (recv(clientSocket, buffer, sizeof(buffer), 0) < 0)
    {
        printf("Receive failed\n");
    }
    printf("%s", buffer);
    close(clientSocket);
}
bool count_len(char *message)
{
    int len = 0;
    char *string = malloc(sizeof(char) * 5100);
    snprintf(string, strlen(message) + 1, "%s", message);
    char *copy = string;
    while (1)
    {
        char *first = strchr(string, '"') + 1;
        char *second = strchr(first, '"');
        if (first == second)
        {
            string = second + 1;
            if (strlen(second) <= 2)
                break;
            continue;
        }
        len = strlen(first) - strlen(second);
        if (len > 128)
            return false;
        string = second + 1;
        char *dest = (char *)malloc(sizeof(char) * (len + 1));
        snprintf(dest, len + 1, "%s", first);
        if (strlen(second) <= 2)
            break;
    }
    string = copy;
    copy = NULL;
    free(string);
    return true;
}
int main(int argc, char *argv[])
{
    memset(message, '\0', sizeof(char)*strlen(message));
    strcpy(host, argv[2]);
    port = atoi(argv[4]);
    pthread_t client;
    size_t end;
    while (1)
    {
        fgets(message, sizeof(message), stdin);
        if (strlen(message) > 5100)
        {
            printf("The maximum size of message is 5100 bytes\n");
            scanf("%*[^\n]%*c");
            continue;
        }
        int count = 0;
        char *no_query;
        char *str = message;
        char query[5100];
        char query_array[5100];
        snprintf(query, strlen(message) + 1, "%s", message);
        strtok_r(query, " ", &no_query);
        strcpy(query_array, no_query);
        query_array[strlen(query_array) - 1] = '\0';
        if (strcmp(message, "\n") == 0 || strstr(str, "\"\"\"") != NULL)
        {
            printf("The strings format is not correct\n");
            continue;
        }
        if (strcmp(query, "Query") != 0 || strchr(str, '\"') == NULL || (query_array[0] != '\"'))
        {
            printf("The strings format is not correct\n");
            continue;
        }
        while (str != NULL)
        {
            if (strchr(str, '\"') != NULL)
            {
                str = strchr(str, '\"');
                count++;
                str++;
            }
            else
            {
                break;
            }
        }
        if (count % 2 != 0)
        {
            printf("The strings format is not correct\n");
            continue;
        }
        if (!count_len(query_array))
        {
            printf("The strings format is not correct. The maximum size of query string is 128 bytes\n");
            continue;
        }
        if (message[strlen(message) - 1] == '\n')
        {
            end = strlen(message);
            --end;
            message[end] = '\0';
        }
        if (pthread_create(&client, NULL, clientThread, NULL) != 0)
        {
            printf("Failed to create thread\n");
        }
    }
    return 0;
}