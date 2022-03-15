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

#define PORT 2004
#define THREAD_COUNT 10
#define BUFFSIZE 1000

void errexit(const std::string message);
void handleRequest(int clientSock);
void* worker_thread(void* arg);

//information on the participants in the multicast system
typedef struct participant_info {
    int port;
    std::string ip_addr;
} participant_info;

//global data structures
std::queue<int> global_queue;
std::unordered_map<std::string, int> codes = {
    {"register", 1}, {"deregister", 2}
};
std::unordered_map<int, participant_info> client_table;

sem_t sem_full;
sem_t sem_empty;
pthread_mutex_t mutex;

int main(int argc, char* argv[]) 
{
    int sockfd;
    const std::string config_file(argv[1]);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        errexit("Failed to create the socket.");

    struct sockaddr_in addr;
    memset((struct sockaddr_in *) &addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        errexit("Could not bind the socket to the port " + PORT);

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
            if((clientfd = accept(sockfd, 0, 0)) == -1)
                throw "Could not accept the oncoming connection.";

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
void* thread_func(void* arg) {
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
    const std::string ack = "ACK", nack = "N_ACK";
    memset(message, '\0', BUFFSIZE);

    recv(clientSock, message, BUFFSIZE, 0);

    int option = -1;
    std::string command = "";

    //parse the command sent in
    for(unsigned int i = 0; message[i] != ' ' && message[i] != '\0'; ++i)
        command += message[i];

    option = codes[command];

    //acknowledge the client
    if(option != -1)
        send(clientSock, ack.c_str(), sizeof(ack), 0);
    else {
        send(clientSock, nack.c_str(), sizeof(nack), 0);

        if(close(clientSock) == -1)
            std::cerr << "Error in closing the socket file descriptor.\n";

        return;
    }

    //if command is defined
    try {
        if(close(clientSock) == -1)
            throw "Error in closing the socket file descriptor.";

        std::string input(message);

        switch(option) {
            case 1: {//register
                std::string parameter = "", ip_addr;
                unsigned int i = 8, id, port_num;

                unsigned short int turn = 1;

                //parse id first, then port, then ip_address
                while(i < input.length()) {
                    for(;input[i] != ' ' && i < input.length(); ++i)
                        parameter += input[i];

                    switch(turn) {
                        case 1:
                            id = std::stoi(parameter);
                            break;
                        case 2:
                            port_num = std::stoi(parameter);
                            break;
                        case 3:
                            ip_addr = parameter;
                    }

                    parameter.clear();
                    turn++;
                }

                client_table[id].port = port_num;
                client_table[id].ip_addr = ip_addr;
                break;
            }

            case 2: //deregister
                break;
        }
    }
    catch(const char *message) {
        std::cerr << message << '\n';
    }
    catch(...) {
        std::cerr << "Unknown exception was thrown.\n";
    }
}