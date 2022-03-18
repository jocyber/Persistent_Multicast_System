#include "part_functions.h"

std::unordered_map<std::string, int> codes = {
    {"register", 1}, {"deregister", 2}
};

void errexit(const std::string message) {
    std::cerr << message << '\n';
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {

    if(argc != 2) {
        std::cerr << "Incorrect executable format. Must be {./file [config_file]}\n";
        exit(EXIT_FAILURE);
    }

    const std::string configFile(argv[1]);

    // parse config file
    std::ifstream file(configFile);
    int id, portCoordinator, sockfd;  
    std::string logFile, ipCoordinator;

    file >> id >> logFile >> ipCoordinator >> portCoordinator;
    std::cout << "ID: " << id << " logFile: " << logFile << " ip: " << ipCoordinator << " port: " << portCoordinator << "\n";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset((struct sockaddr*) &addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(portCoordinator);
    addr.sin_addr.s_addr = INADDR_ANY;

    struct Parameters args;
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
        if(connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
            errexit("Could not connect to remote host.");

        switch(option) {
            case 1: // register
                registerParticipant(input, sockfd, id, tid, args);
                break;
            case 2: //deregister
                deregisterParticipant(input, sockfd, id,tid, args);
                break;
            case 3: //disconnect
                disconnectParticipant(input, sockfd, id,tid, args);
                break;
            case 4: //reconnect
                reconnectParticipant(input, sockfd, id,tid, args);
                break;
            case 5: //msend
                msendParticipant(input, sockfd, id,tid, args);
            default:
                std::cerr << "Command not known.\n";
        }

        close(sockfd);
    }

    return EXIT_SUCCESS;
}