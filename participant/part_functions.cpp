#include "part_functions.h"
#include <unistd.h>
#include <netdb.h>
#include <string.h>

void* acceptMessages(void* args) {
    int sockfd;
    const std::string logFile = ((struct Parameters*)args)->file;
    int port = ((struct Parameters*)args)->port;

    char buffer[BUFFSIZE];
    std::fstream file(logFile, std::ios::app);//append to the file

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "Could not create socket for thread B.\n";
        pthread_exit(0);
    }

    struct sockaddr_in addr;
    memset((struct sockaddr_in *) &addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        std::cerr << "Could not bind the socket to the port " << port;
        pthread_exit(0);
    }

    listen(sockfd, 1);

    // accept connection from server
    int sockCoordinator;
    if((sockCoordinator = accept(sockfd, 0, 0)) == -1) {
        std::cerr << "Could not accept the first oncoming connection.\n";
        pthread_exit(0);
    }

    if(file.is_open()) {
        while(true) {
            memset(buffer, '\0', BUFFSIZE);
            //wait for messages from the coordinator
            //should also parse to know when it needs to terminate
            recv(sockCoordinator, buffer, BUFFSIZE, 0);

            if(strcmp(buffer, "CLOSE") == 0) {
                close(sockCoordinator);
                file.close();

                pthread_exit(0);
            }

            //output the multicast message to the file
            file << buffer;
            break; //only here for testing
        }
    }

    //here for testing
    pthread_exit(0);
}

void registerParticipant(std::string &input, int sock, int id, pthread_t &tid, struct Parameters args) {
    // get port number
    int port = std::stoi(input.substr(8, input.length()));
    args.port = port;

    // create threadB
    int status = pthread_create(&tid, NULL, acceptMessages, (void*) &args);
    if(status != 0) {
        std::cerr << "Failed to create thread B.\n";
        return;
    }
    
    std::string ip = getIP();
    // create data to send
    std::cout << "Id: " << id << " IP: " << ip << " Port: " << port << "\n";

    std::string message = input + " " + std::to_string(id) + " " + ip;
    send(sock, message.c_str(), message.length(), 0);
    // send data
}

std::string getIP(void) {
    FILE *fpntr = popen("hostname -I", "r");

    char buffer[20];
    //get first row
    fgets(buffer, 20, fpntr);
    std::string output(buffer), IP = "";

    for(unsigned int i = 0; output[i] != '\n' && output[i] != '\0'; ++i)
        IP += output[i];

    fclose(fpntr);
    return IP;
}

void disconnectParticipant(std::string &input, int sock, int id, pthread_t &tid, struct Parameters args) {
    ;
}
void reconnectParticipant(std::string &input, int sock, int id, pthread_t &tid, struct Parameters args) {
    ;
}
void msendParticipant(std::string &input, int sock, int id, pthread_t &tid, struct Parameters args) {
    ;
}