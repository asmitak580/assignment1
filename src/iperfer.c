#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PORT_MAX (1<<16)-1 // 65535
#define BUFFER_SIZE 1000
#define MAX_CLIENT 10

/* get_time function */
/* Input: None */
/* Output: current time in seconds */
/* (double data type and ns precision) */
double
get_time(void) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec + (now.tv_nsec * 1e-9);
}

/* TODO: Implement server mode operation here */
void
handle_server(int port) {
    double startTime = get_time();
    /* 1. Create a TCP/IP socket with `socket` system call */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0)
    /* 2. `bind` socket to the given port number */
    struct sockaddr_in sin;
    bzero((char*)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    bind(sockfd, (struct sockaddr *)&sin, sizeof(sin));
    /* 3. `listen` for TCP connections */
    // MAX_CLIENT can be pending
    listen(sockfd, MAX_CLIENT);
    /* 4. Wait for the client connection with `accept` system call */
    int n_sock;
    int len = 0;
    int bytesRead = 0;
    
    while(1) {
        if ((n_sock = accept(sockfd, (struct sockaddr *)&sin, &len)) < 0) {
            perror("error in accept");
            exit(EXIT_FAILURE);
        }

        /* 5. After the connection is established, received data in chunks of 1000 bytes */
        char buf[BUFFER_SIZE];
        while (len = recv(n_sock, buf, sizeof(buf), 0)) {
            if(len==0) {
                break;
            } else if(len==-1) {
                exit(EXIT_FAILURE);
            }
            bytesRead += len;
            fputs(buf, stdout);
        }
        close(n_sock);
        break;
    }
    close(sockfd);
    
    
    /* 6. When the connection is closed, the program should print out the elapsed time, */
    double totalTimeNano = get_time() - startTime;
    double totalTimeSec = totalTimeNano / 1000000000.0;
    fprintf(stdout, "Time elapsed (seconds): %f\n", totalTimeSec);
    /*    the total number of bytes received (in kilobytes), and the rate */ 
    double totalKBRead = bytesRead / 1000.0;
    fprintf(stdout, "Total Number of kilobytes received: %f\n", totalKBRead);
    /*    at which the program received data (in Mbps) */
    double bitsRead = bytesRead * 8.0;
    double mbRead = bitsRead / 1000000.0;
    double rateReceived = mbRead / totalTimeSec;
    fprintf(stdout, "Rate of retrieval (Mbps): %f\n", rateReceived);

    return;
}

/* TODO: Implement client mode operation here */
void
handle_client(const char *addr, int port, int duration) {
    /* 1. Create a TCP/IP socket with socket system call */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    /* 2. `connect` to the server specified by arguments (`addr`, `port`) */
    struct sockaddr_in sin;
    bzero((char*)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    inet_pton(AF_INET, addr, &sin.sin_addr);
    connect(sockfd, (struct sockaddr *)&sin, sizeof(sin));
    /* 3. Send data to the connected server in chunks of 1000bytes */
    char buf[BUFFER_SIZE];
    double startSendTime = get_time(); // in nanosec
    int durationNano = duration * 1000000000;
    double lastPossibleSendTime = startSendTime + durationNano;
    int bytesSent = 0;
    while(get_time() <= lastPossibleSendTime && fgets(buf, sizeof(buf), stdin)) {
        buf[BUFFER_SIZE-1] = '\0';
        int len = strlen(buf) + 1;
        bytesSent += len;
        send(sockfd, buf, len, 0);
    }
    /* 4. Close the connection after `duration` seconds */
    close(sockfd);
    /* 5. When the connection is closed, the program should print out the elapsed time, */
    /*    the total number of bytes sent (in kilobytes), and the rate */ 
    /*    at which the program sent data (in Mbps) */
    fprintf(stdout, "Time elapsed (seconds): %f\n", duration);
    double totalKBSent = bytesSent / 1000.0;
    fprintf(stdout, "Total Number of kilobytes sent: %f\n", totalKBSent);
    double bitsSent = bytesSent * 8.0;
    double mbSent = bitsSent / 1000000.0;
    double rateSent = mbSent / duration;
    fprintf(stdout, "Rate of sending (Mbps): %f\n", rateSent);
    return;
}

int
main(int argc, char *argv[]) {
    /* argument parsing */
    int mode = 0, server_tcp_port = 0, duration = 0;
    char *server_host_ipaddr = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "csh:p:t:")) != -1) {
        switch (opt) {
            case 'c':
                mode = 1;
                break;
            case 's':
                mode = 2;
                break;
            case 'h':
                server_host_ipaddr = optarg;
                break;
            case 'p':
                server_tcp_port = atoi(optarg);
                break;
            case 't':
                duration = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -c -h <server_host_ipaddr> -p <server_tcp_port> -t <duration_in_sec>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (mode == 0) {
        fprintf(stderr, "Please specify either -c (client mode) or -s (server mode).\n");
        exit(EXIT_FAILURE);
    }

    if (mode == 1) {
        if (server_host_ipaddr == NULL || duration == 0 || server_tcp_port == 0) {
            fprintf(stderr, "Client mode requires -h, -p, and -t options.\n");
            exit(EXIT_FAILURE);
        }

        /* 1. Check server_tcp_port is within the port number range */
        /* 2. Check the duration is a positive number */
        if(server_tcp_port < 1 || server_tcp_port > PORT_MAX) {
            fprintf(stderr, "server_tcp_port not in range.\n");
            exit(EXIT_FAILURE);
        }
        if(duration<0) {
            fprintf(stderr, "duration must be a positive number.\n");
            exit(EXIT_FAILURE);
        }

        printf("Client mode: Server IP = %s, Port = %d, Time Window = %d\n", server_host_ipaddr, server_tcp_port, duration);
        handle_client (server_host_ipaddr, server_tcp_port, duration);

    } else if (mode == 2) {
        // Server mode logic goes here
        if (server_tcp_port == 0) {
            fprintf(stderr, "Client mode requires -p option.\n");
            exit(EXIT_FAILURE);
        }

        /* Check server_tcp_port is within the port number range */
        if(server_tcp_port < 1 || server_tcp_port > PORT_MAX) {
            fprintf(stderr, "server_tcp_port not in range.\n");
            exit(EXIT_FAILURE);
        }
        
        printf("Server mode, Port = %d\n", server_tcp_port);
        handle_server(server_tcp_port);
    }

    return 0;
}