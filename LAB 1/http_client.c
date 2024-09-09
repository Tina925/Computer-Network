/* The code is subject to Purdue University copyright policies.
 * DO NOT SHARE, DISTRIBUTE, OR POST ONLINE
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

// #define SERVPORT 80
#define buff_size 4096

int main(int argc, char *argv[])
{
    // printf("hello\n");
    int sockfd, numbytes;
    struct hostent *he;
    struct sockaddr_in their_addr; /* client's address information */
    if (argc != 4)
    {
        fprintf(stderr, "usage: ./http_client [host] [port number] [filepath]\n");
        exit(1);
    }

    const char *host = argv[1];
    char *port = argv[2];
    const char *path = argv[3];

    if ((he = gethostbyname(argv[1])) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(1);
    }
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(atoi(port));
    their_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    bzero(&(their_addr.sin_zero), 8);

    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("connect");
        exit(1);
    }
    // printf("string\n");

    char buffer[buff_size];
    char bufferSend[buff_size];
    char buffer2[buff_size];
    char buffer3[buff_size];
    strcpy(buffer, "GET ");
    strcat(buffer, path);
    strcat(buffer, " HTTP/1.0\r\n");
    strcat(buffer, "Host: ");
    strcat(buffer, host);
    strcat(buffer, ":");
    strcat(buffer, port);
    strcat(buffer, "\r\n\r\n");
    //printf("HTTP request%s\n", buffer);
    numbytes = send(sockfd, buffer, strlen(buffer), 0);
    if (numbytes < 0)
    {
        perror("send");
        exit(1);
    }
    
    int num = recv(sockfd, buffer2, 4095, 0);
    if (num < 0)
    {
        perror("recv");
        exit(1);
    }
    //printf("%s %d\n", buffer2, num);
    // get rid of the header
    char *skipHeader = strstr(buffer2, "\r\n\r\n");
    //printf("\n\nHeader is  %s\n\n", skipHeader);
    int headerLength;
    if (skipHeader != NULL)
    {
        headerLength = skipHeader - buffer2 + 4;
        //printf("Header length is %d\n\n", headerLength);
    }

    // get the download file name
    char *slash = strrchr(path, '/');
    FILE *file;
    char filename[buff_size];
    strcpy(filename, slash + 1);

    // get the content length store in new_content
    char *content_length = strstr(buffer2, "Content-Length: ");
    char new_content[buff_size];

    if (content_length != NULL)
    {
        // printf("\n\n\n\n\n\ncontent length is%s\n", content_length);
        content_length += strlen("Content-Length: ");
        //printf("\n\nnew content %d\n\n", atoi(content_length));
        int i = 0;
        while ((content_length[i] >= '0') && (content_length[i] <= '9'))
        {
            new_content[i] = content_length[i];
            i++;
        }
        //printf("the content length is %d\n\n, and content is %s\n\n", strlen(new_content), new_content);
    }
    else
    {
        fprintf(stdout, "Error: could not download the requested file (file length unknown)");
        exit(1);
    }

    // check if there's 200 in the first line of buffer2
    if (strncmp(buffer2, "HTTP/1.", 7) == 0)
    {
        char *code = buffer2 + 9;
        // printf("code is %s\n\n", code);
        if (strncmp(code, "200", 3) != 0)
        {
            char *firstLine = strchr(buffer2, '\n');
            if (firstLine)
            {
                *firstLine = '\0';
            }
            fprintf(stdout, "%s\n", buffer2);
            exit(1);
        }
    }
    else
    {
        fprintf(stdout, "Invalid reponse received");
        exit(1);
    }

    // new connection to get the full content of recv

    //fprintf(stderr, "set up connection 2\n");
    int sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd1 < 0)
    {
        perror("socket");
        exit(1);
    }
    if (connect(sockfd1, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("connect");
        exit(1);
    }
    //fprintf(stderr, "after connet\n");
    int numbytes1 = send(sockfd1, buffer, strlen(buffer), 0);
    if (numbytes1 < 0)
    {
        perror("send");
        exit(1);
    }
    //fprintf(stderr, "after send\n");
    int num1 = recv(sockfd1, buffer3, headerLength, 0);
    if (num1 < 0)
    {
        fprintf(stderr, "error of reveive\n");
        perror("recv");
        exit(1);
    }
    //printf("\n\n\n\n header received %s %d\n", buffer3, num1);
    //fprintf(stderr, "after recv\n");
    file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("could not open file for writing");
    }
    //fprintf(stderr, "before write, i is %d headerlength is %d  content length is %d\n", i, headerLength, atoi(new_content));
    char finalbuff[buff_size * 1000];
    finalbuff[0] = '\0';
    //fprintf(stderr, "headerlength is %d content length is %d, rest is %d", headerLength, atoi(new_content), atoi(new_content) - headerLength);
    int i = 0;
    char buffer4[buff_size];
    int numGet = 0;
    while (i < (atoi(new_content))){
        numGet = recv(sockfd1, buffer4, 1, 0);
        memcpy ( finalbuff+i, buffer4, strlen(buffer4)+1 );
        //strcat(finalbuff, buffer4);
        i += numGet;
        //fprintf(stderr, "\n\nbuffer4 is %s", buffer4);
        //fprintf(stderr, "\n\nnumGet is %d", numGet);
    }
    //fprintf(stderr, "\n\nall length: %d", atoi(new_content));
    //fprintf(stderr, "\n\ncontent length: %d", atoi(new_content) - headerLength);
    //fprintf(stderr, "\n\nheader length: %d", headerLength);
    //fprintf(stderr, "\n\nfinal content length %d\n\n", i);
    //fprintf(file, "%s", finalbuff);
    fwrite(finalbuff, 1, i, file);
    fclose(file);
    close(sockfd);
    close(sockfd1);

    return 0;
}
