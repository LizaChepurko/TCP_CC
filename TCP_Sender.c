#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <asm-generic/socket.h>


#define ReceiverS 1

#define BUFFER_SIZE (2 * 1024 *1024)


#define Reno "reno" 
#define Cubic "cubic" 

char *util_generate_random_data(unsigned) ;

int main(int argc, char **argv) {
    char *ACK = "ACK";
    char *SYN = 0;
    char *IP =  argv[1];
    int PORT = strtol(argv[2], NULL, 10);
    char message;
    char *file = util_generate_random_data(BUFFER_SIZE);

    socklen_t len;
    char buf[256];

    int sock = -1;
    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // If the socket creation failed, print an error message and return 1.
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    // Choose TCP congestion type
    if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0)
    {
        perror("getsockopt");
        return -1;
    }

    strcpy(buf, argv[3]);

    len = strlen(buf);

    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0)
    {
        perror("setsockopt");
        return -1;
    }

    len = sizeof(buf);

    if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0)
    {
        perror("getsockopt");
        return -1;
    }

    printf("TCP_Sender -ip %s -p %d -algo %s\n",IP, PORT,buf);


    struct sockaddr_in serverAddress, ReceiverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));// Clear server address structure

    socklen_t Receiver_size = sizeof(ReceiverAddress); // Size of the Receiver address structure
    memset(&ReceiverAddress, 0, sizeof(ReceiverAddress)); // Clear Receiver address structure
    // IPv4 address family
    serverAddress.sin_family = AF_INET;
    // Convert port number to network byte order
    serverAddress.sin_port = htons(PORT);
   // Convert IP address from text to binary form and store it in server address structure
    if (inet_pton(AF_INET, IP, &serverAddress.sin_addr) < 0) {
        perror("inet_pton");
        exit(1);
    }
    // bind the socket to the server address
    if (bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("bind");
        close(sock);
        exit(1);
    }
    // Start listening for Receiver connections
    if (listen(sock, ReceiverS) < 0) {
        perror("listen");
        exit(1);
    }

    printf("waiting for connection...\n");

    while (1) {
        // Accept a Receiver connection
        int Receiver = accept(sock, (struct sockaddr *)&ReceiverAddress, &Receiver_size);
       
        if (Receiver < 0) {
            close(sock);
            perror("accept");
            exit(1);
        }
       // Starting communication with receiver
        if(SYN == NULL){
          recv(Receiver, &SYN, sizeof(SYN), 0);
          fprintf(stdout, "Receiver sent SYN message.\n");
          send(Receiver,"SYN, ACK",30,0);
        }
       // Handle Receiver communication
        while(1) {
            int byte_recv = recv(Receiver, &message, sizeof(message), 0);
            if (byte_recv < 0) {
                close(Receiver);
                close(sock);
                perror("recv");
                exit(1);
            } else if (byte_recv == 0) {
                close(Receiver);
                break;
            }
            // Process Receiver's request
            if (message == '1') {
                printf("Receiver wants to receive the file.\n");
                // Send the file content
                int bytes_sent = send(Receiver, file, BUFFER_SIZE, 0);
                if (bytes_sent < 0) {
                    close(Receiver);
                    close(sock);
                    perror("send");
                    exit(1);
                }
            printf("sent %d bytes to receiver\n", bytes_sent);
            // Receiver indicates end of file transfer
            } else if (message == '0') {
                printf("Receiver does not want to receive the file again.\n");
                // Send acknowledgment to Receiver
                send(Receiver,ACK,sizeof(ACK),0);
                close(Receiver);
                break;
            }
        }
    } 
    // Free allocated memory
    free(file);
    return 0;
}


char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;
    // Argument check.
    if (size == 0)
    return NULL;
    buffer = (char *)calloc(size, sizeof(char));
    // Error checking.
    if (buffer == NULL)
    return NULL;
    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++){
     *(buffer + i) = (unsigned int)rand() % 256;
    }
    return buffer;
}