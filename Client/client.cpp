#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>

using namespace std;

vector<string> getCommands (string fileName){
    vector<string> commands;
    string line;
    string content = "";
    ifstream f;
    f.open(fileName);
    while(getline(f, line))
        commands.push_back(line);
    return commands;
}

vector<string> splitRequest(string str){
    vector<string> b;
    int count = 0;
    string word = "";
    for (auto x : str){
        if (x == ' '){
            b.push_back(word);
            word = "";
            count++;
            if (count == 2) break;
        }else {
            if (x != '/') word = word + x;
        }
    }
    return b;
}

string getPostFileContent(string fileName){
    string line;
    string c = "";
    ifstream myfile;
    myfile.open(fileName);
    while(getline(myfile, line))
        c += line;
    return c;
}

void saveFile (string fileName, string content){
    ofstream f_stream(fileName.c_str());
    f_stream.write(content.c_str(), content.length());
}



int main(int argc, char const *argv[]){

    // Handle Wrong Arguments.
    if (argc != 3){
        cout << "Please Enter valid inputs" << endl;
        exit(-1);
    }

    // cast Port Number from String to Integer.
    int portNumber = atoi(argv[2]);

    // Create our Client Socket and handle error if it exists.
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0){
        cout << "Error: couldn't create server socket";
        exit(-1);
    }

    // Construct our address struct to bind to our client.
    struct sockaddr_in serverAddress;
    memset(&serverAddress, '0', sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    
   
   // convert Address to binary and handle errors.
   int conversionState = inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);
    if( conversionState <= 0){
        cout << "Error: Invalid Address";
        exit(-1) ;
    }

    // Connect to Server and handle error.
    int connectState = connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if ( connectState < 0){
        cout << "Error: couldn't connect";
        exit(-1);
    }
    // Get Commands from text files
    vector<string> commands = getCommands("commands.txt");

    char buffer[1024] = {0};

    // Loop through each command and send it to server.
    for (int i = 0; i < commands.size() ; i++){
        string command = commands[i];
        vector<string> splits = splitRequest(command);
        bzero(buffer, 1024);
        if (splits[0] == "client_get"){
            string type = splits[0];
            char * tempBuffer = &command[0];
            cout << endl << command << endl;
            cout << flush;
            send(clientSocket, tempBuffer, strlen(tempBuffer), 0 );
            read(clientSocket, buffer, 1024);
            cout << buffer << endl;
            cout << flush;
            int size;
            read(clientSocket, &size, sizeof(int));
            cout << "String Size : " << size << endl;
            cout << flush;
            string content = "";
            while (true){
                char contentBuffer [1024];
                if (content.size() == size){
                    break;
                }
                long valRead = read(clientSocket, contentBuffer, 1024);
                if (valRead <= 0){
                    cout << "File Completed";
                    cout << flush;
                    break;
                }
                content += string(contentBuffer,valRead);
            }
            cout << content << endl;
            cout << flush;
            saveFile(splits[1] , content);
        } else if (splits[0] == "client_post"){
            string type = splits[0];
            char * tempBuffer = &command[0];
            cout << endl <<command << endl;
            cout << flush;
            send(clientSocket, tempBuffer, strlen(tempBuffer), 0 );
            read(clientSocket, buffer, 1024);
            cout << buffer << endl;
            cout << flush;
            string fileContent = getPostFileContent(splits[1]);
            char * tempFile = &fileContent[0];
            cout << fileContent << endl;
            cout << flush;
            send(clientSocket, tempFile, strlen(tempFile), 0 );
            bzero(buffer, 1024);
            read(clientSocket, buffer, 1024);
            cout << buffer << endl;
        }
    }


    // Read from command line.
    while (1){
        fgets(buffer, 1024,stdin);
        string req = buffer;
        send(clientSocket, buffer, strlen(buffer), 0 );
        cout << "Buffer Sent" << endl;
        cout << flush;
        bzero(buffer, 1024);
        read(clientSocket, buffer, 1024);
        cout << buffer << endl;
        cout << flush;
        bzero(buffer, 1024);
        vector<string> spl = splitRequest(req);
        if (spl[0] == "post" || spl[0] == "client_post"){
            string fileContent = getPostFileContent(spl[1]);
            char * tempFile = &fileContent[0];
            cout << fileContent << endl;
            cout << flush;
            send(clientSocket, tempFile, strlen(tempFile), 0 );
            read(clientSocket, buffer, 1024);
            cout << buffer << endl;
            cout << flush;
            bzero(buffer, 1024);
        } else if (spl[0] == "get" || spl[0] == "client_get"){
            int size;
            read(clientSocket, &size, sizeof(int));
            cout << "String Size : " << size << endl;
            cout << flush;
            string content = "";
            while (true){
                char contentBuffer [1024];
                if (content.size() == size){
                    break;
                }
                long valRead = read(clientSocket, contentBuffer, 1024);
                if (valRead <= 0){
                    cout << "File Completed";
                    cout << flush;
                    break;
                }
                content += string(contentBuffer,valRead);
            }
            cout << content << endl;
            saveFile(spl[1] , content);
        }
    }
    return 0;
}
