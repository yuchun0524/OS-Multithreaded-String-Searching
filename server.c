#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "server.h"

struct Node
{
    char data[5100];
    struct Node *next;
    int socket;
};
typedef struct Node queue;
queue *head, *current, *tail;
int count = 0;
char *path;
int port;
int num_threads;
char *quote = "\"\n\0";
char *str_head = "String: \"\0";
char *file_head = "File: \0";
char *count_string = ", Count: \0";
char *no = "Not found\n\0";

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int count_word(char *path, char *str)
{
    FILE *fp;
    char data[1024];
    char *string = str;
    int j = 0, k = 0, count = 0;
    char ch;
    if ((fp = fopen(path, "r")) == NULL)
    {
        printf("open file error\n");
        return -1;
    }
    else
    {
        int i = 0;
        while ((ch = getc(fp)) != EOF)
        {
            data[i] = ch;
            i++;
        }
        data[i] = '\0';
        fclose(fp);
        for (j = 0; j < i; j++)
        {
            int index = 0;
            bool flag[strlen(string) - 1];
            if (*(string + index) == data[j])
            {
                index++;
                for (k = j + 1; k < j + (int)strlen(string); k++)
                {
                    if (*(string + index) == data[k])
                    {
                        flag[index - 1] = true;
                        index++;
                    }
                    else
                    {
                        flag[index - 1] = false;
                        index++;
                    }
                }
                int count_t = 0;
                for (k = 0; k < strlen(string) - 1; k++)
                {
                    if (flag[k] == true)
                        count_t++;
                }
                if (count_t == strlen(string) - 1)
                    count++;
            }
        }
    }
    return count;
}
void listFilesRecursively(char *basePath, char *str, char *each_string)
{
    char path[1000];
    struct dirent *dp;
    struct stat buf;
    int count = 0;
    DIR *dir = opendir(basePath);
    if (!dir)
        return;
    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);
            if (stat(path, &buf) < 0)
            {
                printf("error\n");
            }
            else
            {
                if (!S_ISDIR(buf.st_mode))
                {
                    count = count_word(path, str);
                    if (count > 0)
                    {
                        if (strlen(each_string) <= 20350)
                        {
                            strcat(each_string, file_head);
                            strcat(each_string, path);
                            strcat(each_string, count_string);
                            char count_str[64];
                            sprintf(count_str, "%d", count);
                            strcat(count_str, "\0");
                            strcat(each_string, count_str);
                            strcat(each_string, "\n\0");
                        }
                        else
                        {
                            strcat(each_string, "found in too many files.\n");
                            return;
                        }
                    }
                }
                listFilesRecursively(path, str, each_string);
            }
        }
    }
    closedir(dir);
}

void *pool_worker()
{
    char each_string[20480] = "";
    char return_string[40960] = "";
    while (1)
    {
        pthread_mutex_lock(&lock);
        if (head != NULL)
        {
            char *message = malloc(sizeof(char) * 5100);
            current = head;
            head = head->next;
            snprintf(message, strlen(current->data) + 1, "%s", current->data);
            char *copy = message;
            int len = 0;
            while (1)
            {
                char *first = strchr(message, '"') + 1;
                char *second = strchr(first, '"');
                if (first == second)
                {
                    message = second + 1;
                    if (strlen(second) <= 2)
                        break;
                    continue;
                }
                len = strlen(first) - strlen(second);
                message = second + 1;
                char *dest = (char *)malloc(sizeof(char) * (len + 1));
                snprintf(dest, len + 1, "%s", first);
                strcat(dest, "\0");
                strcat(each_string, str_head);
                strcat(each_string, dest);
                strcat(each_string, quote);
                int len_string = strlen(each_string);
                listFilesRecursively(path, dest, each_string);
                if (strlen(return_string) <= 40850)
                {
                    if (strlen(each_string) > 20350)
                    {
                        strcat(return_string, "too many string\n");
                    }
                    else if (strlen(each_string) > len_string)
                    {
                        strcat(return_string, each_string);
                        memset(each_string, 0, sizeof(each_string));
                    }
                    else
                    {
                        strcat(each_string, no);
                        strcat(return_string, each_string);
                        memset(each_string, 0, sizeof(each_string));
                    }
                }
                else
                {
                    memset(return_string, 0, sizeof(return_string));
                    strcat(return_string, "too many words. Please try again respectively.\n");
                }
                free(dest);
                if (strlen(second) <= 2)
                    break;
            }
            send(current->socket, return_string, sizeof(return_string), 0);
            message = copy;
            copy = NULL;
            free(message);
            memset(return_string, 0, sizeof(return_string));
            free(current);
        }
        pthread_mutex_unlock(&lock);
    }
}

void *add_request()
{
    char message[5100] = {};
    memset(message, '\0', sizeof(char)*strlen(message));
    int serverSocket, newSocket;
    int recv_msg;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (listen(serverSocket, num_threads) != 0)
        printf("Error\n");
    addr_size = sizeof(serverStorage);
    while (1)
    {
        newSocket = accept(serverSocket, (struct sockaddr *)&serverStorage, &addr_size);
        recv_msg = recv(newSocket, message, sizeof(message), 0);
        if (recv_msg < 0)
            printf("recv fail\n");
        current = (queue *)malloc(sizeof(queue));
        current->socket = newSocket;
        current->next = NULL;
        snprintf(current->data, strlen(message) + 1, "%s", message);
        memset(message, '\0', sizeof(char)*strlen(message));
        printf("%s\n", current->data);
        pthread_mutex_lock(&lock);
        if (head == NULL)
        {
            head = current;
            tail = current;
        }
        else
        {
            tail->next = current;
            tail = current;
        }
        pthread_mutex_unlock(&lock);
    }
}
int main(int argc, char *argv[])
{
    pthread_t main_thread;
    path = argv[2];
    port = atoi(argv[4]);
    num_threads = atoi(argv[6]);
    pthread_mutex_init(&lock, NULL);
    pthread_create(&main_thread, NULL, add_request, NULL);
    pthread_t thread_pool[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&thread_pool[i], NULL, pool_worker, NULL);
    }
    while (1)
    {
    }
    return 0;
}
