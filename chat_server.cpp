#include <iostream>
#include <map>
#include <queue>
#include <vector>
#include <cstring>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

constexpr int MAX_CONNECTIONS = 10000;

typedef struct userinfo{
    string password;
    string status;
    int chatting;
    int sockid;
}userinfo_t;

typedef struct chatroom{
    int c;
    string owner;
    vector<int> chatters;
    string history[13];
    int pingm;
    string last;
}chatroom_t;

chatroom_t rooms[100];

map<string, userinfo_t> users;
map<int, string> state;

void handel_message(int r, string t, int s){
    for(auto i = rooms[r].chatters.begin(); i != rooms[r].chatters.end(); ++i){
        int id = *i;
        auto it = state.find(id);
        if(it != state.end()){
            int sockid = it->first;
            send(sockid, t.c_str(), t.size(), 0);
        }
    }
    if(s != 0){
        for(int i=3; i<13; ++i){
            if(rooms[r].history[i] == ""){
                if(s == 2){
                    if(rooms[r].pingm == -1){
                        rooms[r].pingm = i;
                        rooms[r].history[i] = t;
                    }
                    else{
                        int p = rooms[r].pingm;
                        for(int j=p; j<i-1; ++j){
                            rooms[r].history[j] = rooms[r].history[j+1];
                        }
                        rooms[r].history[i-1] = t;
                        rooms[r].pingm = i-1;
                    }
                }
                else{
                    rooms[r].history[i] = t;
                }
                break;
            }
            if(i == 12){
                int j;
                if(rooms[r].pingm != -1){
                    j = rooms[r].pingm;
                }
                else{
                    j = 3;
                    rooms[r].last = rooms[r].history[j];
                }
                for(j; j<12; ++j){
                    rooms[r].history[j] = rooms[r].history[j+1];
                }
                rooms[r].history[12] = t;
                if(s == 2){
                    rooms[r].pingm = 12;
                }
            }
        }
    }
}

int handle_register(queue<string> q){
    if(q.size() != 3){
        return 1;
    }
    else{
        q.pop();
        string username = q.front();
        auto it = users.find(username);
        if(it != users.end()){
            return 2;
        }
        else{
            userinfo_t add;
            add.password = q.back();
            add.sockid = -1;
            add.status = "offline";
            add.chatting = -1;
            users[username] = add;
            return 0;
        }
    }
}

int handle_login(queue<string> q, int id){
    if(q.size() != 3){
        return 1;
    }
    else{
        q.pop();
        auto it = state.find(id);
        if(it == state.end()){
            string name = q.front();
            string password = q.back();
            auto i = users.find(name);
            if(i != users.end()){
                if(i->second.sockid != -1){
                    return 3;
                }
                string test = i->second.password;
                if(test == password){
                    i->second.sockid = id;
                    state[id] = name;
                    i->second.status = "online";
                    return 0;
                }
                return 3;
            }
            else{
                return 3;
            }
        }
        else{
            return 2;
        }
    }
}

int handle_logout(queue<string> q, int id){
    if(q.size() != 1){
        return 1;
    }
    else{
        auto it = state.find(id);
        if(it == state.end()){
            return 2;
        }
        else{
            return 0;
        }
    }
}

int handle_whoami(queue<string> q, int id){
    if(q.size() != 1){
        return 1;
    }
    else{
        auto it = state.find(id);
        if(it == state.end()){
            return 2;
        }
        else{
            return 0;
        }
    }
}

int handle_setstatus(queue<string> q, int id){
    if(q.size() != 2){
        return 1;
    }
    else{
        auto it = state.find(id);
        if(it == state.end()){
            return 2;
        }
        else{
            string st = q.back();
            st.pop_back();
            if(st == "online" || st == "offline" || st == "busy"){
                string name = it->second;
                auto i = users.find(name);
                i->second.status = st;
                return 0;
            }
            return 3;
        }
    }
}

int handle_enter(queue<string> q, int id){
    if(q.size() != 2){
        return 1;
    }
    else{
        int r = stoi(q.back());
        if(r < 0 || r > 99){
            return 2;
        }
        auto it = state.find(id);
        if(it == state.end()){
            return 3;
        }
        else{
            // If the chat room does not exist, create a new room and enter this room:
            if(rooms[r].c == 0){
                rooms[r].c++;
                rooms[r].owner = it->second;
                rooms[r].chatters.push_back(id);
                rooms[r].history[0] = "Welcome to the public chat room.\n";
                rooms[r].history[1] = "Room number: " + to_string(r) + "\n";
                rooms[r].history[2] = "Owner: " + rooms[r].owner + "\n";
                for(int i=3; i<13; ++i){
                    rooms[r].history[i] = "";
                }
                rooms[r].pingm = -1;
                rooms[r].last = "";
                users[it->second].chatting = r;
                return 0;
            }
            // If the chat room exists, enter this room:
            else{
                string t = it->second + " had enter the chat room.\n";
                handel_message(r, t, 0);
                rooms[r].chatters.push_back(id);
                users[it->second].chatting = r;
                return 0;
            }
        }
    }
}

int handle_close(queue<string> q, int id){
    if(q.size() != 2){
        return 1;
    }
    else{
        int r = stoi(q.back());
        auto it = state.find(id);
        if(it == state.end()){
            return 2;
        }
        string name = it->second;
        auto i = users.find(name);
        if(r < 0 || r > 99){
            return 4;
        }
        if(rooms[r].c == 0){
            return 4;
        }
        if(i->first != rooms[r].owner){
            return 3;
        }
        string t = "Chat room " + to_string(r) + " was closed.\n";
        handel_message(r, t, 0);
        for(auto j = rooms[r].chatters.begin(); j != rooms[r].chatters.end(); ++j){
            int sockid = *j;
            send(sockid, "% ", 2, 0);
            auto k = state.find(sockid);
            string n = k->second;
            users[n].chatting = -1;
        }
        rooms[r].c = 0;
        rooms[r].owner = "";
        rooms[r].last = "";
        rooms[r].chatters.clear();
        for(int i=0; i<13; ++i){
            rooms[r].history[i] = "";
        }
        rooms[r].pingm = -1;
        return 0;
    }
}


int main(int argc, char* argv[]) {

    int port = stoi(argv[1]);

    for(int i = 0; i < 100; ++i){
        rooms[i].c = 0;
    }
    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Error creating server socket" << endl;
        return EXIT_FAILURE;
    }

    // Configure server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    // Bind the socket
    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        cerr << "Error binding server socket" << endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }

    // Listen for incoming connections
    if (listen(serverSocket, MAX_CONNECTIONS) == -1) {
        cerr << "Error listening for connections" << endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }

    cout << "Server listening on port " << port << endl;

    // Set of file descriptors for select
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(serverSocket, &readSet);

    int maxFd = serverSocket;

    vector<int> clientSockets;

    // Accept and handle incoming connections using select
    while (true) {
        fd_set tmpSet = readSet;  // Make a copy of the original set

        // Use select to wait for activity on the sockets
        if (select(maxFd + 1, &tmpSet, nullptr, nullptr, nullptr) == -1) {
            cerr << "Error in select" << endl;
            break;
        }

        // Check if there is a new connection
        if (FD_ISSET(serverSocket, &tmpSet)) {
            sockaddr_in clientAddress{};
            socklen_t clientAddressLength = sizeof(clientAddress);
            int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddressLength);

            if (clientSocket == -1) {
                cerr << "Error accepting client connection" << endl;
            } else {
                cout << "Accepted connection" << endl;

                // Add the new client socket to the set
                FD_SET(clientSocket, &readSet);
                clientSockets.push_back(clientSocket);

                // send something to client
                const char* welcomeMessage = "*********************************\n** Welcome to the Chat server. **\n*********************************\n% ";
                send(clientSocket, welcomeMessage, strlen(welcomeMessage), 0);

                // Update the maximum file descriptor
                maxFd = max(maxFd, clientSocket);
            }
        }

        // Check data from existing clients
        for (auto it = clientSockets.begin(); it != clientSockets.end(); ) {
            int clientSocket = *it;

            if (FD_ISSET(clientSocket, &tmpSet)) {
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));

                // Read data from the client
                int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

                if (bytesRead <= 0) {
                    cout << "Connection closed by client" << endl;
                    close(clientSocket);

                    string u = state[clientSocket];
                    state.erase(clientSocket);
                    auto j = users.find(u);
                    j->second.sockid = -1;

                    clientSockets.erase(it);
                    FD_CLR(clientSocket, &readSet);
                } else {
                    auto j = state.find(clientSocket);
                    if(j != state.end() && users[j->second].chatting != -1){
                        if(buffer[0] == '/'){
                            string s = string(strtok(buffer, " "));
                            if(s.back() == '\n'){
                                s.pop_back();
                            }
                            if(s == "/exit-chat-room"){
                                string t = j->second + " had left the chat room.\n";
                                int r = users[j->second].chatting;
                                for(auto i = rooms[r].chatters.begin(); i != rooms[r].chatters.end(); ++i){
                                    if(*i == clientSocket){
                                        rooms[r].chatters.erase(i);
                                        break;
                                    }
                                }
                                handel_message(r, t, 0);
                                users[j->second].chatting = -1;
                                send(clientSocket, "% ", 2, 0);
                            }
                            else if(s == "/delete-pin"){
                                int r = users[j->second].chatting;
                                if(rooms[r].pingm == -1){
                                    string t = "No pin message in chat room " + to_string(r) + "\n";
                                    send(clientSocket, t.c_str(), t.size(), 0);
                                }
                                else{
                                    int p = rooms[r].pingm;
                                    rooms[r].history[p] = "";
                                    for(int i=3; i<13; i++){
                                        if(rooms[r].history[i] == ""){
                                            for(int j=p; j<12; ++j){
                                                rooms[r].history[j] = rooms[r].history[j+1];
                                            }
                                            rooms[r].history[12] = "";
                                            rooms[r].pingm = -1;
                                            break;
                                        }
                                        else if(i == 12){
                                            for(int j=p; j>3; j--){
                                                rooms[r].history[j] = rooms[r].history[j-1];
                                                cout << rooms[r].history[j] << endl;
                                            }
                                            rooms[r].history[3] = rooms[r].last;
                                            rooms[r].last = "";
                                        }
                                    }
                                }
                            }
                            else if(s == "/pin"){
                                int r = users[j->second].chatting;
                                s = string(strtok(nullptr, "\n"));
                                string temp = s;
                                for(auto t : temp){
                                    t = tolower(t);
                                }
                                if(s.find("==") != string::npos){
                                    int pos = s.find("==");
                                    s.replace(pos, 2, "**");
                                }
                                if(temp.find("superpie") != string::npos){
                                    int pos = temp.find("superpie");
                                    s.replace(pos, 8, "********");
                                }
                                if(temp.find("hello") != string::npos){
                                    int pos = temp.find("hello");
                                    s.replace(pos, 5, "*****");
                                }
                                if(temp.find("starburst stream") != string::npos){
                                    int pos = temp.find("starburst stream");
                                    s.replace(pos, 16, "****************");
                                }
                                if(temp.find("domain expansion") != string::npos){
                                    int pos = temp.find("domain expansion");
                                    s.replace(pos, 16, "****************");
                                }
                                s = "Pin -> [" + j->second + "]: " + s + "\n";
                                handel_message(r, s, 2);
                            }
                            else if(s == "/list-user"){
                                // first sort user alphabetically
                                vector<string> v;
                                for(auto i = users.begin(); i != users.end(); ++i){
                                    v.push_back(i->first);
                                }
                                sort(v.begin(), v.end());
                                for(auto i = v.begin(); i != v.end(); ++i){
                                    string n = *i;
                                    char m[100];
                                    snprintf(m, sizeof(m), "%s %s\n", n.c_str(), users[n].status.c_str());
                                    send(clientSocket, m, strlen(m), 0);
                                }
                            }
                            else{
                                string s = "Error: Unknown command\n";
                                send(clientSocket, s.c_str(), s.size(), 0);
                            }                      
                        }
                        else{
                            /* mask the part of messge with '*' when below words appear, Keyword matching is case insensitive
                            ==, Superpie, hello, Starburst Stream, Domain Expansion
                            */
                            string temp = buffer;
                            string s = temp;
                            for(auto t : temp){
                                t = tolower(t);
                            }
                            if(s.find("==") != string::npos){
                                int pos = s.find("==");
                                s.replace(pos, 2, "**");
                            }
                            if(temp.find("superpie") != string::npos){
                                int pos = temp.find("superpie");
                                s.replace(pos, 8, "********");
                            }
                            if(temp.find("hello") != string::npos){
                                int pos = temp.find("hello");
                                s.replace(pos, 5, "*****");
                            }
                            if(temp.find("starburst stream") != string::npos){
                                int pos = temp.find("starburst stream");
                                s.replace(pos, 16, "****************");
                            }
                            if(temp.find("domain expansion") != string::npos){
                                int pos = temp.find("domain expansion");
                                s.replace(pos, 16, "****************");
                            }
                            string t = "[" + j->second + "]: " + s;
                            int r = users[j->second].chatting;
                            handel_message(r, t, 1);
                        }
                    }
                    else{
                        char *token = strtok(const_cast<char*>(buffer), " ");
                        queue<string> tokens;
                        while(token != nullptr){
                            tokens.push(token);
                            token = strtok(nullptr," ");
                        }

                        int in = -1;
                        string c = tokens.front();
                        if(tokens.size() == 1){
                            c.pop_back();
                        }
                        const char *message;
                        if(c == "register"){
                            in = handle_register(tokens);
                            if(in == 1){
                                message = "Usage: register <username> <password>\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else if(in == 2){
                                message = "Username is already used.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else{
                                message = "Register successfully.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(c == "login"){
                            in = handle_login(tokens, clientSocket);
                            if(in == 1){
                                message = "Usage: login <username> <password>\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else if(in == 2){
                                message = "Please logout first.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else if(in == 3){
                                message = "Login failed.\n";
                                send(clientSocket, message, strlen(message), 0);

                            }
                            else{
                                char m[100];
                                snprintf(m, sizeof(m), "Welcome, %s.\n", state[clientSocket].c_str());
                                send(clientSocket, m, strlen(m), 0);
                            }
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(c == "logout"){
                            in = handle_logout(tokens, clientSocket);
                            if(in == 1){
                                message = "Usage: logout\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else if(in == 2){
                                message = "Please login first.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else{
                                char m[100];
                                snprintf(m, sizeof(m), "Bye, %s.\n", state[clientSocket].c_str());
                                send(clientSocket, m, strlen(m), 0);
                                auto t = state.find(clientSocket);
                                string name = t->second;
                                auto i = users.find(name);
                                i->second.sockid = -1;
                                i->second.status = "offline";
                                state.erase(clientSocket);
                            }
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(c == "exit"){
                            // if user is logged in, logout first
                            auto cs = state.find(clientSocket);
                            if(cs != state.end()){
                                char m[100];
                                snprintf(m, sizeof(m), "Bye, %s.\n", state[clientSocket].c_str());
                                send(clientSocket, m, strlen(m), 0);
                                auto t = state.find(clientSocket);
                                string name = t->second;
                                auto i = users.find(name);
                                i->second.sockid = -1;
                                state.erase(clientSocket);
                            }
                            close(clientSocket);
                            clientSockets.erase(it);
                            FD_CLR(clientSocket, &readSet);
                            if(clientSockets.empty()){
                                break;
                            }
                            if(it == clientSockets.end()){
                                it = clientSockets.begin();
                            }
                            else{
                                ++it;
                            }
                            continue;
                        }
                        else if(c == "whoami"){
                            in = handle_whoami(tokens, clientSocket);
                            if(in == 1){
                                message = "Usage: whoami\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else if(in == 2){
                                message = "Please login first.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else{
                                char m[100];
                                snprintf(m, sizeof(m), "%s\n", state[clientSocket].c_str());
                                send(clientSocket, m, strlen(m), 0);
                            }
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(c == "set-status"){
                            in = handle_setstatus(tokens, clientSocket);
                            if(in == 1){
                                message = "Usage: set-status <status>\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else if(in == 2){
                                message = "Please login first.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else if(in == 3){
                                message = "set-status failed\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else{
                                char m[100];
                                snprintf(m, sizeof(m), "%s %s\n", state[clientSocket].c_str(), users[state[clientSocket]].status.c_str());
                                send(clientSocket, m, strlen(m), 0);
                            }
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(c == "list-user"){
                            if(tokens.size() != 1){
                                message = "Usage: list-user\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            auto k = state.find(clientSocket);
                            if(k == state.end()){
                                message = "Please login first.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else{
                                char m[100];
                                for(auto i = users.begin(); i != users.end(); ++i){
                                    snprintf(m, sizeof(m), "%s %s\n", i->first.c_str(), i->second.status.c_str());
                                    send(clientSocket, m, strlen(m), 0);
                                }
                            }
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(c == "enter-chat-room"){
                        in = handle_enter(tokens, clientSocket);
                        if(in == 1){
                            message = "Usage: enter-chat-room <number>\n";
                            send(clientSocket, message, strlen(message), 0);
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(in == 2){
                            char m[100];
                            snprintf(m, sizeof(m), "Number %d is not valid.\n", stoi(tokens.back()));
                            send(clientSocket, m, strlen(m), 0);
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(in == 3){
                            message = "Please login first.\n";
                            send(clientSocket, message, strlen(message), 0);
                            send(clientSocket, "% ", 2, 0);
                        }
                        else{
                            int r = stoi(tokens.back());
                            for(int i=0; i<13; ++i){
                                if(rooms[r].history[i] != ""){
                                    send(clientSocket, rooms[r].history[i].c_str(), rooms[r].history[i].size(), 0);
                                }
                            }
                        }
                    }
                        else if(c == "list-chat-room"){
                            if(tokens.size() != 1){
                                message = "Usage: list-chat-room\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            auto k = state.find(clientSocket);
                            if(k == state.end()){
                                message = "Please login first.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else{
                                char m[100];
                                for(int i=0; i<100; ++i){
                                    if(rooms[i].c != 0){
                                        snprintf(m, sizeof(m), "%s %d\n", rooms[i].owner.c_str(), i);
                                        send(clientSocket, m, strlen(m), 0);
                                    }
                                }
                            }
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(c == "create-chat-room"){
                            if(tokens.size() != 1){
                                message = "Usage: create-chat-room\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            auto k = state.find(clientSocket);
                            if(k == state.end()){
                                message = "Please login first.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else{
                                int r = 0;
                                for(int i=0; i<100; ++i){
                                    if(rooms[i].c == 0){
                                        r = i;
                                        break;
                                    }
                                }
                                rooms[r].c++;
                                rooms[r].owner = k->second;
                                rooms[r].chatters.push_back(clientSocket);
                                rooms[r].history[0] = "Welcome to the public chat room.\n";
                                rooms[r].history[1] = "Room number: " + to_string(r) + "\n";
                                rooms[r].history[2] = "Owner: " + rooms[r].owner + "\n";
                                for(int i=3; i<13; ++i){
                                    rooms[r].history[i] = "";
                                }
                                users[k->second].chatting = r;
                                char m[100];
                                snprintf(m, sizeof(m), "Create chat room successfully.\n");
                                send(clientSocket, m, strlen(m), 0);
                            }
                            send(clientSocket, "% ", 2, 0);
                        }
                        else if(c == "close-chat-room"){
                            in = handle_close(tokens, clientSocket);
                            if(in == 1){
                                message = "Usage: close-chat-room <number>\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else if(in == 2){
                                char m[100];
                                message = "Please login first.\n";
                                send(clientSocket, message, strlen(message), 0);
                                send(clientSocket, "% ", 2, 0);
                            }
                            else if(in == 3){
                                char m[100];
                                message = "Only the owner can close this chat room.\n";
                                send(clientSocket, message, strlen(message), 0);
                            }
                            else if(in == 4){
                                char m[100];
                                snprintf(m, sizeof(m), "Chat room %d does not exist.\n", stoi(tokens.back()));
                                send(clientSocket, m, strlen(m), 0);
                            }
                            else{
                                char m[100];
                                snprintf(m, sizeof(m), "Chat room %d was closed.\n", stoi(tokens.back()));
                                send(clientSocket, m, strlen(m), 0);
                            }
                            send(clientSocket, "% ", 2, 0);
                        }
                        else{
                            message = "Error: Unknown command\n";
                            send(clientSocket, message, strlen(message), 0);
                            send(clientSocket, "% ", 2, 0);
                        }
                    }
                    ++it;
                }
            } 
            else {
                ++it;
            }
        }
    }

    // Close all client sockets
    for (int clientSocket : clientSockets) {
        close(clientSocket);
    }

    // Close the server socket
    close(serverSocket);

    return EXIT_SUCCESS;
}