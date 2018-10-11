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

#define DIE(x) { perror(x); exit(EXIT_FAILURE);}

char *md5_check(char *file){
    unsigned char c[MD5_DIGEST_LENGTH];
    
    int i;
    char hash[33];
    FILE *inFile = fopen(file, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    
    if (inFile == NULL){
        printf("%s can't be opened.\n", file);
        return 0;
    }

    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, 1024, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);
    
    MD5_Final(c, &mdContext);
    for(i = 0; i < MD5_DIGEST_LENGTH; i++){
        printf("%02x", c[i]);
        sprintf(&hash[i*2], "%02x", (unsigned int)c[i]);
    }

    fclose(inFile);
    printf("Hash: %s\n", hash);
    return *hash;
}

void *server(char *file, int port){
    int fd, sock, sent_bytes, offset, peer_sock;
    ssize_t len;
    struct sockaddr_in sin;
    struct sockaddr_in peer;
    int opt;
    struct stat file_stat;
    char *filesize;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        DIE("socket failure");
    
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    inet_pton(AF_INET, host, &sin.sin_addr.s_addr);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1)
        DIE("ERROR: socket()");

    if (bind(sock, (struct sock_addr *)&sin, sizeof(struct sockaddr_in)) == -1)
        DIE("ERROR: bind()");
    
    if (listen(sock, 5) == -1)
        DIE("ERROR: listen()");

    fd = open(file, O_RDONLY);
    if (fd == -1)
        DIE("ERROR: opening file");
    
    if (fstat(fd, &file_stat) < 0)
        DIE("ERROR: fstat");
    printf("File Size: %d bytes\n", file_stat.st_size);

    peer_sock = accept(sock, (struct sockaddr *)&peer, sizeof(struct sockaddr));
    if (peer == -1)
        DIE("ERROR: accept()");
    printf("Peer accpeted: %s\n", inet_ntop(peer));
    
    sprintf(filesize, "%d", file_stat.st_size);
    /*send size of file*/
    len = send(peer_sock, filesize, 256, 0);
    if (len < 0)
        DIE("ERROR: On handshake");
    printf("Sent %d bytes as filesize\n", len);

    /* Sending file data */
    while (((sent_bytes = sendfile(peer_sock, fd, &offset, BUFSIZ)) > 0) && (remain_data > 0)){
        fprintf(stdout, "1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
        remain_data -= sent_bytes;
        fprintf(stdout, "2. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
    }

    close(peer_sock);
    close(sock);

    return 0;
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
    return 0;
}

int main(int argc, char *argv[]){
    char *filename = argv[1];
    char *hash = md5_check(filename);
    printf("the hash %s \n", hash);
    return 0;
}


