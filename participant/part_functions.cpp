#include "part_functions.h"

void* acceptMessages(void* args) {
    int sockfd = ((struct Parameters*)args)->sock;
    const std::string configFile = ((struct Parameters*)args)->file;

    char buffer[BUFFSIZE];
    std::fstream file(configFile, std::ios::app);//append to the file

    if(file.is_open()) {
        while(true) {
            memset(buffer, '\0', BUFFSIZE);
            //wait for messages from the coordinator
            //should also parse to know when it needs to terminate
            recv(sockfd, buffer, BUFFSIZE, 0);

            //output the multicast message to the file
            file << buffer;
        }
    }
}