//
// Created by yuval on 12/22/19.
//

#include "OpenServerCommand.h"
#include <thread>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include "Interpreter.h"
#include <regex>
#include "SymbolTable.h"
#include <mutex>

void updateValuesInSymbolTable(string stringOfValuesFromSim);
vector <string> splitByComma(string stringOfValuesFromSim);

// constructor
OpenServerCommand :: OpenServerCommand(string strPort, SymbolTable* symbolTable,pthread_mutex_t* mutex) {
    // change the port to int
    this->mutex = mutex;
    auto *stringToInterpretForPort= new Interpreter();
    stringToInterpretForPort->setVariablesByMapOfVars(symbolTable->getMap());
    Expression *expressionToPrint = stringToInterpretForPort->interpret(strPort);
    int intPort = (int) expressionToPrint->calculate();
    this->port = intPort;
    this->symbolTable = symbolTable;
}

// parameters for open the data server function
struct parameters {
    SymbolTable* symbolTableToConnect;
    int clientSocket;
    int portToconnect;
    pthread_mutex_t *mutex;
};

// execute to opening of the server
void OpenServerCommand:: execute(int* index) {
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        //error
        std::cerr << "Could not create a socket" << std::endl;
        exit(1);
    }

    //bind socket to IP address
    // we first need to create the sockaddr obj.
    sockaddr_in address; //in means IP4
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; //give me any IP allocated for my machine
    address.sin_port = htons(this->port);
    //we need to convert our number
    // to a number that the network understands.

    //the actual bind command
    if (bind(socketfd, (struct sockaddr *) &address, sizeof(address)) == -1) {
        std::cerr << "Could not bind the socket to an IP" << std::endl;
        exit(1);
    }

    //making socket listen to the port
    if (listen(socketfd, 5) == -1) { //can also set to SOMAXCON (max connections)
        std::cerr << "Error during listening command" << std::endl;
        exit(1);
    } else {
        std::cout << "Server is now listening ..." << std::endl;
    }

    // accepting a client
    int client_socket = accept(socketfd, (struct sockaddr *) &address, (socklen_t * ) & address);

    if (client_socket == -1) {
        std::cerr << "Error accepting client" << std::endl;
        exit(1);
    } else {
        cout << "server connected" << endl;
    }

    close(socketfd); //closing the listening socket

    // parameters contains the parameters to the createConnectString
    struct parameters *parametersToOpenDataServer;
    parametersToOpenDataServer = new parameters();
    parametersToOpenDataServer->portToconnect = this->port;
    parametersToOpenDataServer->clientSocket = client_socket;
    parametersToOpenDataServer->symbolTableToConnect = this->symbolTable;
    parametersToOpenDataServer->mutex = this->mutex;
    pthread_t thread;
    pthread_create(&thread, nullptr, OpenServerCommand::openDataServer, parametersToOpenDataServer);
    *index += 3;
}

// open the data server
void* OpenServerCommand::openDataServer(void* arguments) {
    struct parameters* parametersToOpenDataServer = (struct parameters*) arguments;
    //reading from client
    while (true) {
        string stringOfValuesFromSim = "";
        char buffer[1024] = {0};
        //valread is 0 if there is no data to read. -1 is there was an erroe
        int valread = read(parametersToOpenDataServer->clientSocket, buffer, 1024);
        if (valread > 0) {
            for (int i = 0; i < valread; i++) {
                char c = buffer[i];
                // if this is the end of the values (36 values)
                if (c == '\n') {
                    if (stringOfValuesFromSim.length() > 0) {
                        vector<string> splitValues = splitByComma(stringOfValuesFromSim);
                        pthread_mutex_lock(parametersToOpenDataServer->mutex);
                        parametersToOpenDataServer->symbolTableToConnect->addValuesFromSimToSymbolTable(splitValues);
                        pthread_mutex_unlock(parametersToOpenDataServer->mutex);
                        cout << stringOfValuesFromSim << endl;
                        stringOfValuesFromSim = "";
                    }
                } else stringOfValuesFromSim += c;
            }
        }
    }
}

// split the values from the xml by comma
vector <string> splitByComma(string stringOfValuesFromSim) {
    vector <string> splitValues;
    regex r("[^,]+");
    std::smatch match;
    while (regex_search(stringOfValuesFromSim, match, r)) {
        string str = match[0];
        splitValues.push_back(str);
        // the index of the matching
        int indexOfMatching = match.position();
        // search again on the continue of the string
        stringOfValuesFromSim = stringOfValuesFromSim.substr(indexOfMatching + match.length());
    }
    return splitValues;
}
