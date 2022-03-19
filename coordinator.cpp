#include <iostream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fstream>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <list>

#define THREAD_COUNT 10
#define BUFFSIZE 1000

void errexit(const std::string message);
void handleRequest(int clientSock);
void* worker_thread(void* arg);
void printPartInfo();
void registerParticipant(const std::string &input);

//information on the participants in the multicast system
typedef struct participant_info {                                  // id number of participant
    int port, id;                                   // port of threadB on participant
    std::string ip_addr;                        // ip address of participant
    int threadBsocket;                          // file descriptor for threadB socket
    bool online = false;                         // boolean if participant is online or offline
    std::queue<int> participant_mailbox;        // mailbox to store messages when offline
} participant_info;


//global data structures
std::queue<int> global_queue;//for working threads
std::unordered_map<std::string, int> codes = {//for switching on command
    {"register", 1}, {"deregister", 2}, {"disconnect", 3}, {"reconnect", 4}, {"msend", 5}
};

std::unordered_map<int, participant_info> client_table;//for storing participant information

sem_t sem_full;
sem_t sem_empty;
pthread_mutex_t mutex;
pthread_mutex_t message_mutex;

int main(int argc, char* argv[]) 
{
    if(argc != 2) {
        std::cerr << "Incorrect executable format. Must be {./file [config_file]}\n";
        exit(EXIT_FAILURE);
    }

    const std::string configFile(argv[1]);

    // parse config file
    std::ifstream file(configFile);
    int port, sockfd, timeToLive;
    file >> port >> timeToLive;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        errexit("Failed to create the socket.");

    struct sockaddr_in addr;
    memset((struct sockaddr_in *) &addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        errexit("Could not bind the socket to the port " + port);

    listen(sockfd, SOMAXCONN);

    //thread pool
    pthread_t pool[THREAD_COUNT];
    sem_init(&sem_full, 0, THREAD_COUNT);
	sem_init(&sem_empty, 0, 0);

     //thread waits for connection, parses message, performs the necessary action, then closes the connection.
    for(unsigned int i = 0; i < THREAD_COUNT; ++i)
        pthread_create(&pool[i], NULL, worker_thread, NULL);
        
    //accept oncoming connections
    while(true) {  
        try {
            sem_wait(&sem_full);
            int clientfd;

            //for thread A
            if((clientfd = accept(sockfd, 0, 0)) == -1)
                throw "Could not accept the first oncoming connection.";

            //when connection is received, push the file descriptor to a global queue that the worker threads read from
            pthread_mutex_lock(&mutex);
            //critical section
            global_queue.push(clientfd);
            pthread_mutex_unlock(&mutex);

            sem_post(&sem_empty);
        }
        catch(const char* message) {
            std::cerr << message << '\n';
            sem_post(&sem_full);
        }
    }

    return EXIT_SUCCESS;
}

void errexit(const std::string message) {
    std::cerr << message << '\n';
    exit(EXIT_FAILURE);
}

//wait for work to be done on the global queue
//thread pool is a bounded buffer problem, so use 2 semaphores and a mutex lock to synchronize the work
void* worker_thread(void* arg) {
	while(true) {
		sem_wait(&sem_empty);
        int clientSock = -1;

		pthread_mutex_lock(&mutex);
        //critical section
        if(!global_queue.empty()) {
		    clientSock = global_queue.front();
            global_queue.pop();
        }
		pthread_mutex_unlock(&mutex);

		if(clientSock != -1)
			handleRequest(clientSock);

		sem_post(&sem_full);
	}
}

//function that parses the packets and performs the action
void handleRequest(int clientSock) {
    char message[BUFFSIZE]; 
    //const std::string ack = "ACK", nack = "N_ACK";
    memset(message, '\0', BUFFSIZE);

    recv(clientSock, message, BUFFSIZE, 0);
    std::cout << message << '\n';

    int option = -1;
    std::string command = "";

    //parse the command sent in
    for(unsigned int i = 0; message[i] != ' ' && message[i] != '\0'; ++i)
        command += message[i];

    option = codes[command];

    //if command is defined
    try {
        std::string input(message);

        switch(option) {
            case 1: {//register
                registerParticipant(input);
                printPartInfo();
                break;
            }

            case 2: { //deregister
                int id = std::stoi(input.substr(11, input.length()));//get id
                client_table.erase(id);//remove participant from table

                std::cout << "Participant with id " + std::to_string(id) + " has been removed.\n";

                break;
            }
            
            case 3: { //disconnect
                break;
            }
            
            case 4: { //reconnect
                break;      
            }
            case 5: { //msend
                break;
            }
        }

        if(close(clientSock) == -1)
            throw "Error in closing the socket file descriptor.";
    } // try
    catch(const char *message) {
        std::cerr << message << '\n';
    }
    catch(const std::exception &ex) {
        std::cerr << ex.what() << '\n';
    }
    catch(...) {
        std::cerr << "Unknown exception was thrown.\n";
    }
}

// function to check the contents of participant_info for testing
void printPartInfo() {
    int j = 1;
    std::cout << "\n";
    std::cout << "Client Table:\n";
    std::cout << "____________________________________________\n";

    for(const auto &i: client_table) {
        std::cout << "Client " << j << ":\n";
        std::cout << "\tIP: " << i.second.ip_addr << "\n";
        std::cout << "\tPort: " << i.second.port << "\n";
        std::cout << "\tOnline: " << i.second.online << "\n";
        std::cout << "\tMailbox Length: " << i.second.participant_mailbox.size() << "\n\n";
    }
    std::cout << "____________________________________________";
}

void registerParticipant(const std::string &input) {//return the id
    std::string parameter = "", ip_addr;
    unsigned int i = 9, id, port_num;

    unsigned short int turn = 1;

    //parse id first, then port, then ip_address
    while(i < input.length()) {
        for(;input[i] != ' ' && i < input.length(); ++i)
            parameter += input[i];

        switch(turn) {
            case 1:
                port_num = std::stoi(parameter);
                break;
            case 2:
                id = std::stoi(parameter);
                break;
            case 3:
                ip_addr = parameter;
        }

        parameter.clear();
        turn++;
        i++;
    }

    // set client_table values
    client_table[id].id = id;
    client_table[id].port = port_num;
    client_table[id].ip_addr = ip_addr;
    client_table[id].online = true;

    // connect to thread B in the participant
    int sockfd2;
    if((sockfd2 = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        errexit("Failed to create the socket for thread B.");

    struct sockaddr_in addr;
    memset((struct sockaddr_in *) &addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_num);
    addr.sin_addr.s_addr = INADDR_ANY;

    //send it back to the IP address it came from. If participants are on the same machine,
    //it will differentiate between them based on the port number
    if(connect(sockfd2, (struct sockaddr*) &addr, sizeof(addr)) == -1)
        errexit("Could not connect to remote host on thread B.");

    // store participant threadB socket to hash table
    client_table[id].threadBsocket = sockfd2;
}