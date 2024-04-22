#include <stdio.h>          /*for printing*/
#include <stdlib.h>         /*for malloc*/
#include <string.h>         /*for strlen function*/
#include <signal.h>		    /*for signals functions*/
#include <fcntl.h>          /*for open function*/
#include <sys/socket.h>     /*protocol families for communication 
                            bind, socket, listen etc*/
#include <errno.h>          /*for errors printing in readable format*/
#include <arpa/inet.h>      /*definitions for IP addresses operations*/
#include <unistd.h>         /*for closing file descriptor*/
#include "http_server.h"    /*http_server header*/

#define FILENAME_SIZE 20
#define BUFFER_SIZE 1024
#define DEFAULT_PAGE "index.html"
#define RESPONSE_OK \
    "HTTP/1.0 200 OK\r\n" \
    "Server: webserver-c\r\n" \
    "Content-type: text/html\r\n\r\n"

#define RESPONSE_BAD \
    "HTTP/1.0 400 Bad Request\r\n" \
    "Server: webserver-c\r\n" \
    "Content-type: text/html\r\n\r\n" \
    "<html>Bad Request</html>"

#define RESPONSE_NOTFOUND \
    "HTTP/1.0 404 Not Found\r\n" \
    "Server: webserver-c\r\n" \
    "Content-type: text/html\r\n\r\n" \
    "<html>Not Found</html>"

/*global run flag 1 for while RunServer, 0 for gracefull exit*/
static sig_atomic_t g_srv_run = 1;

struct server_handle
{
    int sockfd;
    int srv_addrlen;
    struct sockaddr_in srv_addr_bind;
};

typedef struct srv_request
{
    char method[BUFFER_SIZE];
    char uri[BUFFER_SIZE];
    char version[BUFFER_SIZE];
} srv_request_t;

/*use signal handler to gracefull exit when ctrl^c*/
void Destroy(server_handle_t *srv_params)
{
    free(srv_params);
}

int ParseRequest(srv_request_t *srv_request, char *buffer)
{
    /*write from buffer into srv struct*/
    sscanf(buffer, "%s %s %s", srv_request->method, 
    srv_request->uri, srv_request->version);
    /*for example if connecting from nc, passing random method or passing
    GET but uri doesnt start with / it will throw <bad request error> in terminal.
    */
    if (strcmp(srv_request->method, "GET") != 0 || srv_request->uri[0] != '/')
    {
        return -1;
    }

    return 0;
}

/*used when pressed ctrl^c to send interrupt signal*/
void KeyboardSignalHandler()
{
    g_srv_run = 0;
	puts("Server interrupted by user");
}

int SendFile(int client_socket, char *file_name)
{
    /*
    1. if filename is [/] than render index.html although it is the default,
    if not specifying index.html explicilty its part of the reponse.

    2. else move one char forward in filename as we want to try open the file.

    on success, write the response ok into client socket,
    while reading fd into buffer, write buffer into client socket.
    */
    int fd;
    char name_no_html[FILENAME_SIZE];
    char buff[BUFFER_SIZE] = {0};

    if (strlen(file_name) == 1 && *file_name == '/')
    {
        file_name = DEFAULT_PAGE;
    }
    else
    {
        ++file_name;
        /*if no .html in filename, add .html to the file, and try open it
        so the user dont have to add .html each time when requesting web page*/
        if (strstr(file_name, ".html") == NULL)
        {
            strcpy(name_no_html, file_name);
            strcat(name_no_html, ".html");
            file_name = name_no_html;
        }
    }
    
    /*if file not found response_notfound will be returned*/
    fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        perror("webserver (open)");
        return -1;
    }

    write(client_socket, RESPONSE_OK, strlen(RESPONSE_OK));

    while (read(fd, buff, BUFFER_SIZE) > 0)
    {
        write(client_socket, buff, strlen(buff));
    }

    return 0;
}

server_handle_t *Init(int port)
{
    int var = 1;
    server_handle_t *srv_params = (server_handle_t*)malloc(sizeof(server_handle_t));
    /*init signal to keyboard interrupt*/
	signal(SIGINT, KeyboardSignalHandler);

    /*
    socket() creates TCP socket and returns fd for that socket
    (IPv4 protocols, TCP, IP protocol in header sent, or received)
    */
    srv_params->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_params->sockfd == -1)
    {
        perror("webserver (socket)");
        Destroy(srv_params);
        return NULL;
    }

    /*each time we close connection there will be waiting timeout,
    disable this to make address available again when closing, without waiting
    for timeout*/
    setsockopt(srv_params->sockfd, SOL_SOCKET, SO_REUSEADDR, &var, sizeof(var));

    /*
    bind the socket to an address, meaning accept connections from any
    interface - 0.0.0.0 on [PORT].
    AF_INET - port in network byte order
    htons - convert host byte (port) order to network byte order
    sin_addr - srv interface address in network byte order
    */
    srv_params->srv_addrlen = sizeof(srv_params->srv_addr_bind);
    srv_params->srv_addr_bind.sin_family = AF_INET;
    srv_params->srv_addr_bind.sin_port = htons(port);
    srv_params->srv_addr_bind.sin_addr.s_addr = htonl(INADDR_ANY);

    /*sockaddr used to cast structure pointer passed in addr to avoid compiler
    warnings, whatever addr points to, act like sockaddr.*/
    if (bind(
        srv_params->sockfd, 
        (struct sockaddr*)&srv_params->srv_addr_bind, 
        srv_params->srv_addrlen) != 0)
    {
        perror("webserver (bind)");
        Destroy(srv_params);
        return NULL;
    }

    return srv_params;
}

int RunServer(server_handle_t *srv_params)
{
    /*
    listen passively for active connections, backlog is for max queued incoming
    connections.
    */
    if (listen(srv_params->sockfd, SOMAXCONN) != 0)
    {
        perror("webserver (listen)");
        return 1;
    }
    printf("Server listening for connections\n");

    /*
    accept connections in a loop, get first connection from the queue of the 
    listening socket. accept() creates new connected socket for each incoming 
    connection and returns fd for that socket, then that socket fd is closed.
    */
    while (g_srv_run)
    {
        srv_request_t srv_request = {0};
        char buffer[BUFFER_SIZE] = {0};
        
        /*accept incoming connection*/
        int new_sock_fd = accept(
            srv_params->sockfd, 
            (struct sockaddr*)&srv_params->srv_addr_bind, 
            (socklen_t*)&srv_params->srv_addrlen);
        if (new_sock_fd < 0)
        {
            perror("webserver (accept)");
            continue;
        }
        printf("Connection accepted\n");

        /*read from new connection socket, into buffer.
        the buffer contains method, uri, protocol, user-agent, HTTP body etc
        */
        if (read(new_sock_fd, buffer, BUFFER_SIZE) < 0)
        {
            perror("webserver (read)");
            continue;
        }

        /*write the request method, uri and protocol into srv struct
        and check the requested method is correct, else bad_response*/
        if (ParseRequest(&srv_request, &buffer[0]) == -1)
        {
            write(new_sock_fd, RESPONSE_BAD, strlen(RESPONSE_BAD));
            close(new_sock_fd);
            continue;
        }

        /*convert to string representation and print. use srv struct
        inet_ntoa - IP, in network byte order to string in IPv4 dotted decimal notation.
        ntohs - network byte order to short integer byte order.
        [127.0.0.1:46322] GET / HTTP/1.1
        */
        printf("[%s:%u] %s %s %s\n", 
        inet_ntoa(srv_params->srv_addr_bind.sin_addr), 
        ntohs(srv_params->srv_addr_bind.sin_port), srv_request.method, 
        srv_request.uri, srv_request.version);

        /*write the correct file back to client*/
        if (SendFile(new_sock_fd, srv_request.uri) == -1)
        {
            write(new_sock_fd, RESPONSE_NOTFOUND, strlen(RESPONSE_NOTFOUND));
            close(new_sock_fd);
            continue;
        }

        close(new_sock_fd);
    }

    return 0;
}
