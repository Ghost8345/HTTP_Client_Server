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


string OK_RESPONSE = "HTTP/1.1 200 OK\\r\\n \n";
string NOT_FOUND_RESPONSE = "HTTP/1.1 404 Not Found\\r\\n \n";

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
            // cout << "Client Socket: " << clients[i]->clientSocket << " Duration: " << duration << "Threshold" << threshold << endl;
            // cout << flush;

            // Close Connection if it exceeded threshold time.
            if (duration >= threshold){
                close((clients[i]->clientSocket));
                clients.erase(clients.begin()+i);
                cout << "Client connection " << i <<" has timed out" << endl;
                cout << flush;
                i--;
            }
        }
    }
}


// Remove any unnecessary spaces.
string getCoreRequest(const string& request){
    size_t first = request.find_first_not_of(' ');
    if (string::npos == first)
        return request;
    size_t last = request.find_last_not_of(' ');
    return request.substr(first, (last - first + 1));
}

// Split request delimited by space.
vector<string> splitRequest(string request){
    request = getCoreRequest(request);
    vector<string> words;
    int counter = 0;
    string word = "";
    for (auto x : request){
        if (x == ' '){
            words.push_back(word);
            word = "";
            counter++;
            if (counter == 2) break;
        }
        else
            word = word + x;
    }
    return words;
}


// Read Content from file system.
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

// Send huge files using chunks
void sendChunks(int socket , string s) {
    const char *start = s.c_str();
    for (int i = 0; i < s.length(); i += 500)
        send(socket, start + i,min(500, (int)s.length() - i), 0);
}


// saveFile wehn getting post requests.
void saveFile (string fileName, string content){
    ofstream f_stream(fileName.c_str());
    f_stream.write(content.c_str(), content.length());
}

// Check if file exists or not.
bool checkFileExist(string fileName){
    ifstream ifile;
    ifile.open(fileName);
    if(ifile)
        return true;
    else
        return false;
}

void handleGet(string fileName, int socketClient){

    if (checkFileExist(fileName)){
        string ok = OK_RESPONSE;
        char * temp = &ok[0];
        write(socketClient, temp, strlen(temp));
        string content = readFileContent(fileName);
        int fileSize = content.size();
        write(socketClient, &fileSize, sizeof(int));
        sendChunks(socketClient, content);
    } else {
        string str = NOT_FOUND_RESPONSE;
        char * temp = &str[0];
        write(socketClient, temp, strlen(temp));
        // string content = readFileContent("not_found.txt");
        // int fileSize = content.size();
        // write(socketClient, &fileSize, sizeof(int));
        // sendChunks(socketClient, content);
    }

}

// Get type of external get request.
string getContentType(string fname){
    if (fname.find(".txt") != std::string::npos)
        return "text/plain";
    if (fname.find(".html") != std::string::npos)
        return "text/html";
}


// Get file for external get request.
string getRequestedFileContent(string fileName){
    string line;
    string content = "";
    ifstream myfile;
    myfile.open(fileName);
    while(getline(myfile, line))
        content += line;
    return content;
}

// Handle get requests from browser
void handleExternalGet(string fileName, int socketClient){

    string real = "";
    for (int i = 1; i < fileName.size() ; i++){
        real += fileName[i];
    }

    if (checkFileExist(real)){
        cout << endl << real << endl;
        string fContent =  getRequestedFileContent(real);
        int cLength = fContent.size();
        string conType = getContentType(real);
        string str = "HTTP/1.1 200 OK\nContent-Type: " + conType+ "\nContent-Length: " + to_string(cLength) + "\n\n" + getRequestedFileContent(real);
        char * temp = &str[0];
        write(socketClient, temp, strlen(temp));
    } else {
        string fContent =  getRequestedFileContent("not_found.txt");
        int cLength = fContent.size();
        string conType = getContentType(fileName);
        string str = "HTTP/1.1 404 Not Found\n";
        char * temp = &str[0];
        write(socketClient, temp, strlen(temp));
    }

}



void * handleConnection(void* socket)
{
    

    //Initalize Variables
    string overAllBuffer = "";
    client clientStruct = *((client*) socket);
    int socketClient = clientStruct.clientSocket;

    cout << "HandleConnection: " << socketClient << endl;
    cout << flush;
    
    // Main loop
    while (1){
       
        char buffer[4096] = {0};

        // Read until end of request
        long long val = read(socketClient, buffer, 4096);
        if (val <= 0) break;

        cout << buffer << endl;
        cout << flush;

        // Add terminating char at the end of filled buffer.
        buffer[strlen(buffer) - 1] = '\0';

        overAllBuffer += buffer;

        // Parse Request and split it by space.
        vector<string> words = splitRequest(overAllBuffer);

        if (words[0] == "client_get") {
            string fileName = words[1];
            if (checkFileExist(fileName)){
                string ok = OK_RESPONSE;
                char * temp = &ok[0];
                // Send Response to CLient
                write(socketClient, temp, strlen(temp));
                string content = readFileContent(fileName);
                int fileSize = content.size();
                // Send Size to Client
                write(socketClient, &fileSize, sizeof(int));
                // Send file in chunks to CLient
                sendChunks(socketClient, content);
            } else {
                string str = NOT_FOUND_RESPONSE;
                char * temp = &str[0];
                // Send Response to Client
                write(socketClient, temp, strlen(temp));
            }

        } 
        else if (words[0] == "client_post"){
            string fileName = words[1];
            string str = OK_RESPONSE;
            char * temp = &str[0];
            // Send Response to Client.
            write(socketClient, temp, strlen(temp));
            bzero(buffer, 4096);
            // Read File from Client.
            long long val2 = read(socketClient, buffer, 4096);
            if (val2 <= 0)
             break;
            cout << buffer << endl;
            cout << flush;
            // Save File to System.
            string fileToSave = string(buffer);
            saveFile(fileName, fileToSave);
            string res = "File is saved successfully \n";
            char * msg = &res[0];
            // Write confirmation message to Client
            write(socketClient, msg, strlen(msg));
        }
        else if (words[0] == "GET") {
            string fileName = words[1];
            string real = "";
            for (int i = 1; i < fileName.size() ; i++){
                real += fileName[i];
            }

            if (checkFileExist(real)){
                cout << endl << real << endl;
                string fContent =  getRequestedFileContent(real);
                int cLength = fContent.size();
                string conType = getContentType(real);
                string str = "HTTP/1.1 200 OK\nContent-Type: " + conType+ "\nContent-Length: " + to_string(cLength) + "\n\n" + getRequestedFileContent(real);
                char * temp = &str[0];
                write(socketClient, temp, strlen(temp));
            } else {
                string fContent =  getRequestedFileContent("not_found.txt");
                int cLength = fContent.size();
                string conType = getContentType(fileName);
                string str = "HTTP/1.1 404 Not Found\n";
                char * temp = &str[0];
                write(socketClient, temp, strlen(temp));
            }
            
        } 
        else if (words[0] == "close") {
            break;
        } 
        else {
            string str = NOT_FOUND_RESPONSE;
            char * temp = new char [str.length()+1];
            strcpy (temp, str.c_str());
            write(socketClient, temp, strlen(temp));
        }
        bzero(buffer, 4096);
        overAllBuffer = "";
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
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
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
    // pthread_t timeout;
    // pthread_create(&timeout, NULL, clientTimeOut, NULL);

    
    // main loop.
    while(1){
        cout << "Waiting for a connection" << endl;
        cout << flush;

        // Accept Connection
        int newSocket = accept(serverSocket, (struct sockaddr*) &socketAddress, (socklen_t*) &socketAddressLength);
        cout << "NewSocket: " << newSocket << endl;
        cout << flush;

        if (newSocket < 0){
            cout << "Error: couldn't accept connection";
            exit(-1);
        }

        pthread_t thread;
        

        // Create Client Struct for new Connection
        client c;
        c.clientSocket = newSocket;
        time(&c.timeEntered); 
        
        clients.push_back(&c);

         // Create Thread
        int threadState = pthread_create(&thread, NULL, handleConnection, &c);
        if (threadState < 0) {
            cout << "Error: couldn't create thread";
            exit(-1);
        }
    }
    return 0;
}