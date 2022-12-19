#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <bits/stdc++.h>
using namespace std;

static const int MAXPENDING = 5; // Max number of concurrent connections for server.

// Struct for measuring client's time on server for implementing timeout functionality.
struct client {
    int clientSocket;
    time_t timeEntered;
    };


// List of client Connections.
vector <client*> clients;

// Handle TimeOut for client connections.
void * clientTimeOut (void *){
    while (1){
        time_t timeNow;
        for (int i = 0; i < clients.size() ; i++){

            time(&timeNow);

            // Implement Heurisitic.
            double duration = difftime(timeNow, clients[i]->timeEntered);
            double threshold = 75 - 10*clients.size();

            // Close Connection if it exceeded threshold time.
            if (duration >= threshold){
                close((clients[i]->clientSocket));
                clients.erase(clients.begin()+i);
                i--;
                cout << "Client connection " << i <<" has timed out" << endl;
            }
        }
    }
}

string trim(const string& str){
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first)
        return str;
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

vector<string> splitRequest(string str){
    str = trim(str);
    vector<string> vec;
    int counter = 0;
    string word = "";
    for (auto x : str){
        if (x == ' '){
            vec.push_back(word);
            word = "";
            counter++;
            if (counter == 2) break;
        }
        else
            word = word + x;
    }
    return vec;
}

bool checkFileExist(string fileName){
    ifstream ifile;
    ifile.open(fileName);
    if(ifile)
        return true;
    else
        return false;
}

string readFileContent(string fileName){
    ifstream fin(fileName);
    size_t buffer_size = 1024;
    char buffer[buffer_size];
    size_t len = 0;
    ostringstream streamer;
    while((len = fin.readsome(buffer, buffer_size)) > 0)
        streamer.write(buffer, len);
    return streamer.str();
}

void sendChunks(int socket , string s) {
    const char *beginner = s.c_str();
    for (int i = 0; i < s.length(); i += 500)
        send(socket, beginner + i,min(500, (int)s.length() - i), 0);
}

void saveFile (string fileName, string content){
    ofstream f_stream(fileName.c_str());
    f_stream.write(content.c_str(), content.length());
}


string getRequestedFileContent(string fileName){
    string line;
    string content = "";
    ifstream myfile;
    myfile.open(fileName);
    while(getline(myfile, line))
        content += line;
    return content;
}

string getContentType(string fname){
    if (fname.find(".txt") != std::string::npos)
        return "text/plain";
    if (fname.find(".html") != std::string::npos)
        return "text/html";
}


void * handleConnection(void* S_clientStruct)
{
    
    client clientStruct = *((client*) S_clientStruct);
    int socketClient = clientStruct.clientSocket;

    string total_buffer = "";
    while (1){

        char buffer[4096] = {0};
        long long val = read(socketClient, buffer, 4096);
        
        if (val <= 0) break;
        cout << buffer << endl;
        cout << flush;
        buffer[strlen(buffer) - 1] = '\0';
        total_buffer += buffer;
        vector<string> splits = splitRequest(total_buffer);
        if (splits[0] == "get" || splits[0] == "client_get") {
            string fileName = splits[1];
            if (checkFileExist(fileName)){
                string ok = "HTTP/1.1 200 OK\\r\\n \n";
                char * tab2 = &ok[0];
                write(socketClient, tab2, strlen(tab2));
                string content = readFileContent(fileName);
                int fileSize = content.size();
                write(socketClient, &fileSize, sizeof(int));
                sendChunks(socketClient, content);
            } else {
                string str = "HTTP/1.1 404 Not Found\\r\\n \n";
                char * tab2 = &str[0];
                write(socketClient, tab2, strlen(tab2));
                string content = readFileContent("not_found.txt");
                int fileSize = content.size();
                write(socketClient, &fileSize, sizeof(int));
                sendChunks(socketClient, content);
            }

        } else if (splits[0] == "post" || splits[0] == "client_post"){
            string fileName = splits[1];
            string str = "HTTP/1.1 200 OK\\r\\n \n";
            char * tab2 = &str[0];
            write(socketClient, tab2, strlen(tab2));
            bzero(buffer, 4096);
            long long val2 = read(socketClient, buffer, 4096);
            if (val2 <= 0) break;
            cout << buffer << endl;
            cout << flush;
            string fileToSave = string(buffer);
            saveFile(fileName, fileToSave);
            string res = "File is saved successfully \n";
            char * msg = &res[0];
            write(socketClient, msg, strlen(msg));
        } else if (splits[0] == "GET") {
            string fileName = splits[1];
            string real = "";
            for (int i = 1; i < fileName.size() ; i++){real += fileName[i];}
            if (checkFileExist(real)){
                cout << endl << real << endl;
                string fContent =  getRequestedFileContent(real);
                int cLength = fContent.size();
                string conType = getContentType(real);
                string str = "HTTP/1.1 200 OK\nContent-Type: " + conType+ "\nContent-Length: " + to_string(cLength) + "\n\n" + getRequestedFileContent(real);
                char * tab2 = &str[0];
                write(socketClient, tab2, strlen(tab2));
            } else {
                string fContent =  getRequestedFileContent("not_found.txt");
                int cLength = fContent.size();
                string conType = getContentType(fileName);
                string str = "HTTP/1.1 404 Not Found\n";
                char * tab2 = &str[0];
                write(socketClient, tab2, strlen(tab2));
            }
        } else if (splits[0] == "close") {
            break;
        } else {
            string str = "HTTP/1.1 404 Not Found\\r\\n \n";
            char * tab2 = new char [str.length()+1];
            strcpy (tab2, str.c_str());
            write(socketClient, tab2, strlen(tab2));
        }
        bzero(buffer, 4096);
        total_buffer = "";
    }
    close(socketClient);
    return NULL;
}



int main(int argc, char **argv){

    // Handle Wrong Arguments.
    if (argc < 2){
        cout << "Please enter port number";
        exit(-1);
    }
    else if (argc > 2){
        cout << "Invalid Arguments, please enter only one argumen (Port Number)";
        exit(-1);
    }

    // cast Port Number from String to Integer.
    int portNumber = atoi(argv[1]);
    
    // Create our Listening Socket and handle error if it exists.
    int serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0){
        cout << "Error: couldn't create server socket";
        exit(-1);
    }

    // Construct our address struct to bind to our server.
    struct sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(portNumber);
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    memset(socketAddress.sin_zero, '\0', sizeof socketAddress.sin_zero);
    int socketAddressLength = sizeof(socketAddress);

    // Bind the server and the address and handle error if it exists.
    int bindState = bind(serverSocket, (struct sockaddr*) &socketAddress, socketAddressLength);
    if (bindState < 0 ){
        cout << "Error: couldn't bind the server";
        exit(-1);
    }

    // Initalize Server for listening and assigning maximum concurrent connections and handle error if it exists.
    int listenState = listen(serverSocket, MAXPENDING);
    if (listenState < 0){
        cout << "Error: server couldn't listen";
        exit(-1);
    }

    cout << "Listening at port: " << portNumber << endl;

    // Create timeout running thread.
    pthread_t timeout;
    pthread_create(&timeout, NULL, clientTimeOut, NULL);

    // main loop.
    while(1){
        cout << flush;

        int newSocket = accept(serverSocket, (struct sockaddr*) &socketAddress, (socklen_t*) &socketAddressLength);
        if (newSocket < 0){
            cout << "Error: couldn't accept connection";
            exit(-1);
        }
        pthread_t t;
        client c;
        c.clientSocket = newSocket;
        time(&c.timeEntered); 
        
        clients.push_back(&c);
        pthread_create(&t, NULL, handleConnection, &c);
    }

    return 0;
}