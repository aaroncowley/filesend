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

char *md5_check(char *filename){
    unsigned char c[MD5_DIGEST_LENGTH];
    
    int i;
    char hash[33];
    FILE *inFile = fopen(filename, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    
    if (inFile == NULL){
        printf("%s can't be opened.\n", filename);
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
    return hash;

}

int main(int argc, char *argv[]){
    char *filename = argv[1];
    char *hash = md5_check(filename);
    printf("the hash %s \n", hash);
    return 0;
}


