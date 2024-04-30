#include <stdio.h> 
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <asm-generic/socket.h>


/*
 The TCP's server IP address to connect to.
*/
#define SERVER_IP "127.0.0.2"


/*
 The buffer size to store the received message.
*/

#define BUFFER_SIZE (2 * 1024 *1024) // 2 MB buffer size
#define Reno "reno" 
#define Cubic "cubic"

double MBPS(float delta, long bytes);
  
int main(int argc, char **argv) {
    struct timeval start, end;
    float firstTotaltime;
    float delta;
    static float averagetime;
    static float averagespeed;
    int PORT = strtol(argv[1], NULL, 10);
    char buf[256];
    float runs[20];
    double speed;
    
    printf("Starting Receiver...\n");
    char message;
    int sock = -1;
    struct sockaddr_in server;
    socklen_t len;


    // Create a buffer to store the received message.
    char buffer[BUFFER_SIZE] = {0};

    // Reset the server structure to zeros.
    memset(&server, 0, sizeof(server));

    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // If the socket creation failed, print an error message and return 1.
    if (sock == -1)
    {
        perror("socket(2)");
        return 1;
    }
    
    // Choose TCP congestion type

    if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0)
    {
        perror("getsockopt");
        return -1;
    }

    strcpy(buf, argv[2]);

    len = strlen(buf);

    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0)
    {
        perror("setsockopt");
        return -1;
    }

    len = sizeof(buf);

    printf("TCP_Receiver -p %d -algo %s\n", PORT, buf);


    // Convert the server's address from text to binary form and store it in the server structure.
    if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) <= 0)
    {
        perror("inet_pton(3)");
        close(sock);
        return 1;
    }

    // Set the server's address family to AF_INET (IPv4).
    server.sin_family = AF_INET;

    // Set the server's port to the defined port.
    server.sin_port = htons(5060);

    printf("Waiting for TCP connection...\n");

    // Try to connect to the server using the socket and the server structure.
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect(2)");
        close(sock);
        return 1;
    }
    // Start to calculate the time
    gettimeofday(&start, NULL);
    // sending syn
    if (send(sock, "SYN", 24, 0) < 0) {
        close(sock);
        perror("send");
        exit(1);
    }

    // If the message receiving failed, print an error message and return 1.
    // If no data was received, print an error message and return 1. Only occurs if the connection was closed.
    if(recv(sock, buffer, sizeof(buffer), 0)<=0){
        perror("recv(2)");
        close(sock);
        return 1;
    }

    //Set message 1 by defult

    message = '1';
    // Sending massege to sender
    if(send(sock, (char *)&message, 2, 0)<0){
        close(sock);
        perror("send");
        exit(1);
    }

    fprintf(stdout,"received %s from sender\n",buffer);
    // Reset buffer
    memset(buffer, 0, sizeof(buffer));
    // Receive file and store it in the buffer, return the number of bytes received.
    // if no bytes received print error
    int bytes_received = recv(sock, buffer, sizeof(buffer), 0);

    if (bytes_received <= 0)
    {
        perror("recv(2)");
        close(sock);
        return 1;
    }
    //End time counter
    gettimeofday(&end, NULL);

    // Print the received message.
    fprintf(stdout, "Got %d bytes from %s:%d\n", bytes_received, inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    // Set the counter to calculate the average
    int i = 1;
    firstTotaltime = ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec)/100.0F;
    averagetime = firstTotaltime;
    delta = firstTotaltime;
    speed = MBPS(delta,bytes_received);
    averagespeed = speed;
    printf("Run #%d time= %.2f milliseconds, speed= %lf MBps\n", i, firstTotaltime, speed);
    runs[i] = firstTotaltime;
    while(1){

      // Create a message to send to the server.
        printf("Enter 1 to receive the file again, enter 0 for exit\n");
        scanf("%c",&message);
        getchar();
        
        if(message != '1' && message != '0'){
            continue;
        }

        printf("Sending message to the server: %c\n", message);

        //Start time counter
        gettimeofday(&start, NULL);

        // Try to send the message to the server using the socket.
        int bytes_sent = send(sock, (char *)&message, 2, 0);

        // If the message sending failed, print an error message and return 1.
        // If no data was sent, print an error message and return 1. Only occurs if the connection was closed.
        if (bytes_sent <= 0)
        {
            perror("send(2)");
            close(sock);
            return 1;
        }
        

        fprintf(stdout, "Sent %d bytes to the sender!\n"
                        "Waiting for the server to respond...\n", bytes_sent);
        
        // Try to receive a message from the server using the socket and store it in the buffer.
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        // If the message receiving failed, print an error message and return 1.
        // If no data was received, print an error message and return 1. Only occurs if the connection was closed.
        if (bytes_received <= 0)
        {
            perror("recv(2)");
            close(sock);
            return 1;
        }
        
        // Reset buffer
        memset(buffer, 0, sizeof(buffer));
        // End time counter
        gettimeofday(&end, NULL);
        // put the time difference to averagetime and convert it into millseconds
        averagetime += ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec)/100.0F;
        // put the time difference in seconds and convert buytes to MB.
        delta = ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec)/100.0F;
        averagespeed += MBPS(delta,bytes_received);;
        i++;
        
        // send FIN to sender
        if(message == '0'){
            // Receiving ack message
            recv(sock, buffer, sizeof(buffer), 0);
            fprintf(stdout,"sender closed the connection\n");
            break;
        }

        printf("Run #%d time= %.2f milliseconds, speed= %lf MBbs\n", i, delta,MBPS(delta,bytes_received));
        runs[i] = delta;
        fprintf(stdout, "Got %d bytes from %s:%d\n", bytes_received, inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    }
    printf("\n");
    printf("-----------------STATISTICS-----------------\n");
    printf("\n");
    printf("CC Algorithm: %s \n",buf);
    printf("Totals runs: %d \n",i-1);
    printf("Total Time %.2f ms\n",averagetime);
    printf("Average time = %.2f millseconds\n", averagetime/i-1);
    printf("Average speed = %.2f MB/s\n", averagespeed/i);
        for(int j = 1; j<i; j++){
        printf("Run #%d took %.2f ms\n",j,runs[j]);
    }
    // Close the socket with the sender.
    close(sock);

    return 0;
}
//Converts bytes to MB
double MBPS(float delta, long bytes) {
    return (bytes/8/1024) / (delta);
}