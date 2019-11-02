#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <list>
#include <thread>
#include <iostream>
#include <c++/8/sstream>
#include <cstring>

using namespace std;

int listener;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
list<string> threads;

string list_to_string() {
    char buf[15] = "";
    struct sockaddr_in sai;
    socklen_t len = sizeof(sai);


    string threads_list = "list of threads: ";
    for (const string &v : threads) {
        threads_list += v;
        threads_list += " ";
    }
    return threads_list;
}

list<string> split(const string &s, char delimiter) {
    list<string> tokens;
    string token;
    istringstream token_stream(s);
    while (getline(token_stream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

string to_str(thread::id id) {
    stringstream ss;
    ss << id;
    return ss.str();
}

void exit() {
    close(listener);
    exit(0);
}


bool contains_thread(const string &id) {
    for (const string &elem : threads) {
        if (strcmp(elem.c_str(), id.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

void kill(const string &id, string &pString) {
    if (contains_thread(id)) {
        threads.remove(id);
        pString = "killed ";
        pString += id;
    } else {
        pString = "Can not kill this";
    }
}

void fun(int sock) {
    string hello = "Hello ";
    hello += to_str(this_thread::get_id());
    write(sock, hello.c_str(), 1024);
    while (true) {
        string message;
        char buf[1024];
        int bytes_read;


        if (!contains_thread(to_str(this_thread::get_id()))) {
            break;
        }
        bytes_read = read(sock, buf, 1024);
        if (bytes_read <= 0) {
            break;
        }
        if (!contains_thread(to_str(this_thread::get_id()))) {
            break;
        }
        if (strcmp(buf, string("list").c_str()) == 0) {
            message = list_to_string();
        } else if (strcmp(buf, string("exit").c_str()) == 0) {
            exit();
        } else {
            list<string> listSplit = split(buf, ' ');
            if (strcmp(listSplit.front().c_str(), string("kill").c_str()) == 0) {
                listSplit.pop_front();
                kill(listSplit.back(), message);
            } else {
                if (!contains_thread(to_str(this_thread::get_id()))) {
                    break;
                }
                message = "Server recived a message:" + string(buf);
            }
        }
        if (!contains_thread(to_str(this_thread::get_id()))) {
            break;
        }
        write(sock, message.c_str(), 1024);
        if (!contains_thread(to_str(this_thread::get_id()))) {
            break;
        }
    }
    close(sock);
    pthread_mutex_lock(&mutex);
    threads.remove(to_str(this_thread::get_id()));
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in addr{};
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;
    const int on = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    if (bind(listener, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(2);
    }

    listen(listener, 1);

    while (true) {
        sock = accept(listener, nullptr, nullptr);

        if (sock < 0) {
            perror("accept");
            exit(3);
        }

        thread t1(fun, sock);
        pthread_mutex_lock(&mutex);
        threads.push_back(to_str(t1.get_id()));
        pthread_mutex_unlock(&mutex);
        t1.detach();
    }
}

