#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <netdb.h> 
#include <sys/stat.h>
#include <arpa/inet.h> 

#define DIE(x) { perror(x); exit(EXIT_FAILURE);}


/* ooh colors */

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

void md5_check(char *hash, char *file){
    unsigned char c[MD5_DIGEST_LENGTH];
    
    int i;
    FILE *inFile = fopen(file, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    
    if (inFile == NULL)
        DIE("ERROR: file cannot be opened for hashing");

    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, 1024, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);
    
    MD5_Final(c, &mdContext);
    for(i = 0; i < MD5_DIGEST_LENGTH; i++){
        sprintf(&hash[i*2], "%02x", (unsigned int)c[i]);
    }

    fclose(inFile); 
}

void *server(char *file, int port, char *server_ip){
    int fd, sock, sent_bytes, offset, peer_sock;
    ssize_t len;
    FILE *serveFile;
    struct sockaddr_in sin;
    struct sockaddr_in peer;
    int opt;
    struct stat file_stat;
    char *filesize;
    int remaining_bytes;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        DIE("socket failure");
    
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &sin.sin_addr.s_addr);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1)
        DIE("ERROR: socket()");

    if (bind(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) == -1)
        DIE("ERROR: bind()");
    
    if (listen(sock, 5) == -1)
        DIE("ERROR: listen()");

    serveFile = fopen(file, "r");
    if (fd == -1)
        DIE("ERROR: opening file");
    
    if (fstat(serveFile, &file_stat) < 0)
        DIE("ERROR: fstat");
    printf("File Size: %d bytes\n", file_stat.st_size);

    peer_sock = accept(sock, (struct sockaddr *)&peer, sizeof(struct sockaddr));
    if (peer_sock == -1)
        DIE("ERROR: accept()");
    printf("Peer accpeted: %s\n", inet_ntoa(peer.sin_addr));;
    
    sprintf(filesize, "%d", file_stat.st_size);
    /*send size of file*/
    len = send(peer_sock, filesize, 256, 0);
    if (len < 0)
        DIE("ERROR: On handshake");
    printf("Sent %d bytes as filesize\n", len);
    
    /* Sending file data */
    while (((sent_bytes = send(peer_sock, serveFile, &offset, BUFSIZ)) > 0) && (remaining_bytes> 0)){
        printf("1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remaining_bytes);
        remaining_bytes -= sent_bytes;
        printf("2. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remaining_bytes);
    }

    close(peer_sock);
    close(sock); 
    printf("File sent, socket closed.");
}

void *client(char *file, int port, char *server_ip){
    int sock;
    ssize_t len;
    struct sockaddr_in remote_addr;
    char buffer[BUFSIZ];
    int file_size;
    FILE *received_file;
    int remain_data = 0;

    /* Zeroing remote_addr struct */
    memset(&remote_addr, 0, sizeof(remote_addr));

    /* Construct remote_addr struct */
    remote_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &(remote_addr.sin_addr));
    remote_addr.sin_port = htons(port);

    /* Create client socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        DIE("ERROR: client socket");
    
    /* Connect to the server */
    if (connect(sock, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
        DIE("ERROR: client connect");

    /* Receiving file size */
    recv(sock, buffer, BUFSIZ, 0);
    file_size = atoi(buffer);
    printf("\nFile size : %d\n", file_size);

    received_file = fopen(file, "w");
    if (received_file == NULL)
        DIE("ERROR: file not received");
    

    remain_data = file_size;
    while (((len = recv(sock, buffer, BUFSIZ, 0)) > 0) && (remain_data > 0)){
            fwrite(buffer, sizeof(char), len, received_file);
            remain_data -= len;
            fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", len, remain_data);
    }
    fclose(received_file);
    close(sock);
}

int main(int argc, char *argv[]){
    char filename[32];
    char port_ascii[6];
    int port;
    char *hash;
    int command, hostname;
    char *IPbuf;
    char hostbuffer[256];
    struct hostent *host_entry;

    printf(RED"\n\n----------------pick an option----------------\n"RESET);
    printf(CYAN"option s: send a file\n"RESET);
    printf(CYAN"option h: hash a file\n"RESET);
    printf(CYAN"option r: receive a file\n"RESET);
    printf(RED"----------------------------------------------\n\n"RESET);

    do {
        printf(GREEN"Enter your Command >>> "RESET);
        command = getchar();
        switch(command){
            case 's':
                hostname = gethostname(hostbuffer, sizeof(hostbuffer));
                if (hostname == -1)
                    DIE("ERROR: Could Not get hostname");

                host_entry = gethostbyname(hostbuffer);
                if (host_entry == -1)
                    DIE("ERROR: Could not retrieve host info");
                
                IPbuf = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
                printf("using IP: %s\n", IPbuf);

                printf("enter a filename:");
                fflush(stdin);
                fgets(filename, 32, stdin);
                printf("\n");
                
                printf("enter a port: ");
                fflush(stdin);
                fgets(port_ascii, 6, stdin);
                port = atoi(port_ascii);
                printf("\n");
                
                printf("starting server on socket %s:%d \n", IPbuf, port);
                server(filename, port, IPbuf); 
                break;
            case 'h':
                printf("enter a filename:");
                fflush(stdin);
                fgets(filename, 32, stdin);
                printf("\n"); 
                md5_check(hash, filename);
                printf("Computed MD5 hash is: %s \n", hash);
                break;
            default:
                printf("invalid input entered, learn to read \n");
                continue;
        }
    } while(1);

    //md5_check(hash, filename);
    //printf("the hash %s \n", hash);
        
    return 0;
}


