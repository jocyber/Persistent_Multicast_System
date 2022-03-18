#ifndef PART
#define PART

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fstream>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <fstream>
#include <unordered_map>

#define BUFFSIZE 1000

struct Parameters {
    int sock;
    std::string file;
    int port;
};

//prototypes
void* acceptMessages(void* args);
void registerParticipant(std::string &input, int sock, int id, pthread_t &tid, struct Parameters args);
std::string getIP(void);
#endif