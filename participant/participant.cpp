#include "part_functions.h"

void errexit(const std::string message) {
    std::cerr << message << '\n';
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    int port = 2004;
    int sockfd[2];
    const std::string configFile(argv[1]);

    //sockfd[0] for thread A, sockfd[1] for thread B
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == -1)
        errexit("Could not create socket.");

    struct sockaddr_in addr;
    memset((struct sockaddr*) &addr, '\0', sizeof(addr));

    addr.sin_family = AF_UNIX;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(connect(sockfd[0], (struct sockaddr*) &addr, sizeof(addr)) == -1)
        errexit("Could not connect to remote host.");

    if(connect(sockfd[1], (struct sockaddr*) &addr, sizeof(addr)) == -1)
        errexit("Could not connect to remote host.");

    struct Parameters args;
    args.sock = sockfd[1];
    args.file = configFile;

    pthread_t tid;//thread B, accepts oncoming messages from the coordinator
    pthread_create(&tid, NULL, acceptMessages, (void*) &args);

    char buffer[BUFFSIZE];
    memset(buffer, '\0', BUFFSIZE);

    //thread A - accept input from the user
    while(true) {
        std::string input = "";

        std::cout << "myMulticast$ ";
        std::getline(std::cin, input);
        
        if(send(sockfd[0], input.c_str(), input.length(), 0) == -1) {
            std::cerr << "Failed to send data through the socket.\n";
            continue;
        }

        //receive acknowledgement from the coordinator
        recv(sockfd[0], buffer, BUFFSIZE, 0);
        std::string parse(buffer);

        if(parse.compare("N_ACK") == 0)
            std::cerr << "Unrecognized command.\n";

        memset(buffer, '\0', BUFFSIZE);
        //close(sockfd[0]);
    }

    return EXIT_SUCCESS;
}