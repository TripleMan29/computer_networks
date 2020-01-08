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
#include <sstream>
#include <cstring>

using namespace std;

int socket_descriptor;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
list<string> threads;
list<thread> threads_list;
list<int> descriptors;

string list_to_string() {
    struct sockaddr_in sai;
    socklen_t len = sizeof(sai);
    string threads_list = "list of threads: ";
    for (const string &v : threads) {
        threads_list += v;
        threads_list += " ";
    }
    threads_list += "\n";
    return threads_list;
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

bool contains_thread(const string &id) {
    for (const string &elem : threads) {
        if (strcmp(elem.c_str(), id.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

void fun(int new_socket_descriptor) {
    string first_string = "Hi ";
    first_string += to_str(this_thread::get_id());
    write(new_socket_descriptor, first_string.c_str(), 1024);
    while (true) {
        string message;
        char buf[1024];
        int bytes_read;
        if (!contains_thread(to_str(this_thread::get_id()))) {
            break;
        }
        bytes_read = readn(new_socket_descriptor, buf, 1024);
        if (bytes_read <= 0) {
            break;
        }
        if (!contains_thread(to_str(this_thread::get_id()))) {
            break;
        }
        write(new_socket_descriptor, buf, 1024);
    }
    close(new_socket_descriptor);
    descriptors.remove(new_socket_descriptor);
    pthread_mutex_lock(&mutex);
    threads.remove(to_str(this_thread::get_id()));
    pthread_mutex_unlock(&mutex);
}

void kill(const string &id) {
    if (contains_thread(id)) {
        pthread_mutex_lock(&mutex);
        threads.remove(id);
        pthread_mutex_unlock(&mutex);
        printf("Killed %s\n", id.c_str());
    } else {
        printf("Can not kill this\n");
    }
}

void exit() {
    shutdown(socket_descriptor, 2);
    close(socket_descriptor);
    for (int &it: descriptors){
        shutdown(it,2);
        close(it);
    }
}

void admin_fun(){
    char buffer[1024];
    while (true){
        cin.getline(buffer, 1024, '\n');
        if (strcmp(buffer, string("list").c_str()) == 0) {
            printf(list_to_string().c_str());
        } else if (strcmp(buffer, string("exit").c_str()) == 0) {
            exit();
            break;
        } else {
            list<string> listSplit = split(buffer, ' ');
            if (strcmp(listSplit.front().c_str(), string("kill").c_str()) == 0) {
                listSplit.pop_front();
                kill(listSplit.back());
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int new_socket_descriptor;
    struct sockaddr_in addr{};
    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor < 0) {
        perror("socket");
        exit(1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;
    const int on = 1;
    setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    if (bind(socket_descriptor, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    listen(socket_descriptor, 1);

    thread t0(admin_fun);


    while (true) {
        new_socket_descriptor = accept(socket_descriptor, nullptr, nullptr);
        if (new_socket_descriptor < 0) {
            break;
        }
        pthread_mutex_lock(&mutex);
        threads_list.emplace_back([&] { fun(new_socket_descriptor);});
        threads.push_back(to_str(threads_list.back().get_id()));
        descriptors.push_back(new_socket_descriptor);
        pthread_mutex_unlock(&mutex);
    }
    for (thread &t: threads_list){
        t.join();
    }
    t0.join();
    return 0;
}
