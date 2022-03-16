#include "part_functions.h"
#include <unistd.h>
#include <netdb.h>

void* acceptMessages(void* args) {
    int sockfd = ((struct Parameters*)args)->sock;
    const std::string logFile = ((struct Parameters*)args)->file;
    int port = ((struct Parameters*)args)->port;

    char buffer[BUFFSIZE];
    std::fstream file(logFile, std::ios::app);//append to the file

    struct sockaddr_in addr;
    memset((struct sockaddr_in *) &addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        std::cerr << "Could not bind the socket to the port " << port;

    listen(sockfd, SOMAXCONN);

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

            //output the multicast message to the file
            file << buffer;
            file.close();// dont close the file, should be open always listening until the client dissconects with the disconect method
            close(sockCoordinator);
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
    pthread_create(&tid, NULL, acceptMessages, (void*) &args);
    
    // get host IP
    char host[256];
    struct hostent *host_entry;
    gethostname(host, sizeof(host));
    host_entry = gethostbyname(host);
    std::string ip(inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])));

    // create data to send
    struct RegisterData data;
    data.ID = id;
    data.IP = ip;
    data.port = port;
    std::cout << "Id: " << id << " IP: " << ip << " Port: " << port << "\n";
    // send data

}