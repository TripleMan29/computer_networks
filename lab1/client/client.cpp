#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <iostream>
#include <string.h>

#define BUFFER_LENGTH 1024

// std::string message;
using namespace std;
char buffer[BUFFER_LENGTH];
int n;
struct hostent *server;

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
        n = read(sock, buffer, BUFFER_LENGTH);
        if (n < 0) {
            perror("Server disconnect");
            close(sock);
            exit(1);
        }
        printf("%s\n", buffer);

    while (true){
        // std::getline(std::cin, message);
        cin.getline(buffer, BUFFER_LENGTH, '\n');
        n = write(sock, buffer, BUFFER_LENGTH);
        if (n <= 0) {
            perror("Server disconnect");
            close(sock);
            exit(0);
        }
        n = read(sock, buffer, BUFFER_LENGTH);
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
