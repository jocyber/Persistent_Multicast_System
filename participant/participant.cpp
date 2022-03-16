#include "part_functions.h"

std::unordered_map<std::string, int> codes = {
    {"register", 1}, {"deregister", 2}
};

void errexit(const std::string message) {
    std::cerr << message << '\n';
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    int sockfd[2];
    const std::string configFile(argv[1]);

    // parse config file
    std::ifstream file(configFile);
    int id;  
    std::string logFile;
    std::string ipCoordinator;
    int portCoordinator;
    file >> id >> logFile >> ipCoordinator >> portCoordinator;
    std::cout << "ID: " << id << " logFile: " << logFile << " ip: " << ipCoordinator << " port: " << portCoordinator << "\n";

    //sockfd[0] for thread A, sockfd[1] for thread B
    /*
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == -1)
        errexit("Could not create socket.");
    */
    sockfd[0] = socket(AF_INET, SOCK_STREAM, 0);
    sockfd[1] = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset((struct sockaddr*) &addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(portCoordinator);
    addr.sin_addr.s_addr = INADDR_ANY;

    struct Parameters args;
    args.sock = sockfd[1];
    args.file = logFile; // pass in logFile to write to

    pthread_t tid;//thread B, accepts oncoming messages from the coordinator

    char buffer[BUFFSIZE];
    memset(buffer, '\0', BUFFSIZE);

    //thread A - accept input from the user
    while(true) {
        std::string input = "";

        std::cout << "myMulticast$ ";
        std::getline(std::cin, input);
        
        // parse command
        int option = -1;
        std::string command = "";

        //parse the command sent in
        for(unsigned int i = 0; input[i] != ' ' && input[i] != '\0'; ++i)
            command += input[i];
        option = codes[command];

        // connect to the coordinator
        if(connect(sockfd[0], (struct sockaddr*) &addr, sizeof(addr)) == -1)
            errexit("Could not connect to remote host.");

        switch(option) {
            case 1: // register
                registerParticipant(input, sockfd[0], id, tid, args);
                break;
            case 2:
                break;
            default:
                std::cerr << "Command not known.\n";
        }
/*
        if(send(sockfd[0], input.c_str(), input.length(), 0) == -1) {
            std::cerr << "Failed to send data through the socket.\n";
            continue;
       }
*/
/*
        //receive acknowledgement from the coordinator
        recv(sockfd[0], buffer, BUFFSIZE, 0);
        std::string parse(buffer);

        if(parse.compare("N_ACK") == 0)
            std::cerr << "Unrecognized command.\n";

        memset(buffer, '\0', BUFFSIZE);
*/
        close(sockfd[0]);
    }

    return EXIT_SUCCESS;
}