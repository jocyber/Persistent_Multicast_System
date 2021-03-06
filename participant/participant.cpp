#include "part_functions.h"

std::unordered_map<std::string, int> codes = {
    {"register", 1}, {"deregister", 2}, {"disconnect", 3}, {"reconnect", 4}, {"msend", 5}
};

void errexit(const std::string message) {
    std::cerr << message << '\n';
    exit(EXIT_FAILURE);
}

int connectToCoor(int portCoordinator, std::string coorIP);

int main(int argc, char* argv[]) {

    if(argc != 2) {
        std::cerr << "Incorrect executable format. Must be {./file [config_file]}\n";
        exit(EXIT_FAILURE);
    }

    const std::string configFile(argv[1]);

    // parse config file
    std::ifstream file(configFile);
    int id, portCoordinator;  
    std::string logFile, ipCoordinator;

    file >> id >> logFile >> ipCoordinator >> portCoordinator;
    std::cout << "ID: " << id << " logFile: " << logFile << " ip: " << ipCoordinator << " port: " << portCoordinator << "\n";

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
        for(unsigned int i = 0; input[i] != ' ' && i < input.length(); ++i)
            command += input[i];
        option = codes[command];

        int sockfd = connectToCoor(portCoordinator, ipCoordinator);//connect to remote host

        switch(option) {
            case 1:// register
                registerParticipant(input, sockfd, id, tid, args);
                break;
            case 2: { //deregister
                //send id to make operation on coordinator more efficient
                const std::string message = input + " " + std::to_string(id);

                if(send(sockfd, message.c_str(), message.length(), 0) == -1)
                    std::cerr << "Unable to send command 'deregister' to the coordinator.\n";

                break;
            }
            case 3: { //disconnect
                const std::string message = input + " " + std::to_string(id);

                if(send(sockfd, message.c_str(), message.length(), 0) == -1)
                    std::cerr << "Unable to send command 'disconnect' to the coordinator.\n";

                break;
            }
            case 4: { //reconnect
                registerParticipant(input, sockfd, id, tid, args);
                break;
            }
            case 5: { //msend
                // get message and setup command to send to coordinator
                std::string message = input.substr(6, input.length());
                std::string coordMessage = command + " " + std::to_string(id) + " " + message;

                // write to file
                std::fstream file(logFile, std::ios::app);//append to the file
                file << message << "\n";
                file.close();

                // send command to server
                if(send(sockfd, coordMessage.c_str(), coordMessage.length(), 0) == -1)
                    std::cerr << "Unable to send command 'deregister' to the coordinator.\n";

                break;
            }
            default:
                std::cerr << "Command not known.\n";
                close(sockfd);
                continue;
        }
        // block for ACK
        char ack[3];
        recv(sockfd, ack, sizeof(ack), 0);
        if(strcmp(ack, "ACK") != 0) {
            std::cerr << "Command error on coordinator.\n";
        }

        close(sockfd);
    }

    return EXIT_SUCCESS;
}

int connectToCoor(int portCoordinator, std::string coorIP) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset((struct sockaddr*) &addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(portCoordinator);
    addr.sin_addr.s_addr = inet_addr(coorIP.c_str());

    // connect to the coordinator
    if(connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
        errexit("Could not connect to remote host.");

    return sockfd;
}