#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <iostream>
#include <cstring>
#include <sstream>

#define BUFFER_LENGTH 1024

using namespace std;
char buffer[BUFFER_LENGTH];
int n;
struct hostent *server;

string to_str(thread::id id) {
    stringstream ss;
    ss << id;
    return ss.str();
}

int readn(int fd, char *bp, size_t len){
    int cnt;
    int rc;

    cnt = len;
    while(cnt > 0){
        rc = recv(fd, bp, cnt, 0);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (rc == 0)
            return len - cnt;
        bp += rc;
        cnt -= rc;
    }
    return len;
}

void Event(int port) {
    int sock;
    struct sockaddr_in addr{};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror(" Failed create socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    bcopy(server->h_addr, (char *) &addr.sin_addr.s_addr, (size_t) server->h_length);
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Failed connect to server");
        exit(1);
    }
        n = readn(sock, buffer, BUFFER_LENGTH);
        if (n < 0) {
            perror("Server disconnect");
            close(sock);
            exit(1);
        }
        printf("%s\n", buffer);

    while (true){
        cin.getline(buffer, BUFFER_LENGTH, '\n');
        if (strcmp(buffer, string("/exit").c_str()) == 0){
            close(sock);
            exit(0);
        }
        n = write(sock, buffer, BUFFER_LENGTH);
        if (n <= 0) {
            perror("Server disconnect");
            close(sock);
            exit(0);
        }
        n = readn(sock, buffer, BUFFER_LENGTH);
        if (n <= 0) {
            perror("Server disconnect");
            close(sock);
            exit(0);
        }
        printf("%s\n", buffer);
    }

    close(sock);
}

int main(int argc, char *argv[]) {
    server = gethostbyname(argv[1]);
    if (server == NULL){
        perror("No such host");
        exit(1);
    }
    std::thread t1(Event,atoi(argv[2]));
    t1.join();
    return 0;
}