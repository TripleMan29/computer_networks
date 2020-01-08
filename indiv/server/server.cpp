#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string.h>
#include <pthread.h>
#include <thread>
#include <time.h>
#include <sstream>

using namespace std;

//1) ники-запрет символов
//2) ==""
//3) ники-вне accept
//4)

#define true  1
#define false 0
#define NUMBER_OF_CLIENTS 1000


int socket_descriptor;
list<int> clients;
list<thread> threads_list;
struct sockaddr_in server_address;

pthread_mutex_t cs_mutex;
int port;

struct vacancy {
    int id;
    string company;
    string position;
    int lower_age_limit;
    int upper_age_limit;
    int wages;
};

multimap <string, vacancy> specialties;


void check_argc(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage %s port\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
}

string to_str(int id) {
    stringstream ss;
    ss << id;
    return ss.str();
}




void initialization_socket_descriptor() {
    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
}

void initialization_socket_structure() {
    uint16_t port_number = htons((uint16_t) port);

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = port_number;
}

void bind_host_address() {
    if (bind(socket_descriptor, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }
}

void communicating(int new_socket_descriptor);

void accept_connection() {
    struct sockaddr_in client_address;
    unsigned int client_length = sizeof(client_address);
    int nsd = accept(socket_descriptor, (struct sockaddr *) &client_address, &client_length);
    if (nsd <= 0) {
        perror("ERROR opening socket");
        return;
    }
    threads_list.emplace_back([&] { communicating(nsd); });
}

int read_int(int new_socket_descriptor){
    int message;
    int temp = read(new_socket_descriptor, &message, 4);
    if (temp <= 0) {
        perror("ERROR reading from socket");
        pthread_mutex_lock(&cs_mutex);
        clients.remove(new_socket_descriptor);
        pthread_mutex_unlock(&cs_mutex);
        close(new_socket_descriptor);
        pthread_exit(0);
    }
    return message;
}

string read_message(int new_socket_descriptor){
    int message_size;
    int temp = read(new_socket_descriptor, &message_size, 4);
    if (temp <= 0) {
        perror("ERROR reading from socket");
        pthread_mutex_lock(&cs_mutex);
        clients.remove(new_socket_descriptor);
        pthread_mutex_unlock(&cs_mutex);
        close(new_socket_descriptor);
        pthread_exit(0);
    }
    auto buffer = new char[message_size];
    bzero(buffer, sizeof(buffer));
    temp = read(new_socket_descriptor, buffer, message_size);
    if (temp <= 0) {
        perror("ERROR reading from socket");
        pthread_mutex_lock(&cs_mutex);
        clients.remove(new_socket_descriptor);
        pthread_mutex_unlock(&cs_mutex);
        close(new_socket_descriptor);
        pthread_exit(0);
    }
    return string(buffer);
}

void write_message(int new_socket_descriptor, string message){
    int message_size = message.length();
    int temp = write(new_socket_descriptor, &message_size, 4);
    if (temp <= 0) {
        perror("ERROR writing in socket");
        return;
    }
    temp = write(new_socket_descriptor, message.c_str(), message_size);
    if (temp <= 0) {
        perror("ERROR writing in socket");
        return; 
    }
}    


void add_specialty(string specialty){
    vacancy vac = {0, "", "", 0, 0, 0};
    specialties.insert(pair<string, vacancy>{specialty, vac});
}

void delete_specialty(string specialty){
    for (int i = 0; i < specialties.count(specialty); i++){
        multimap<string, vacancy>::iterator it = specialties.find(specialty);
        specialties.erase(it);
    }
}

void add_vacancy(string specialty, vacancy const& vac){
    bool flag = false;
    for (multimap<string, vacancy>::iterator it = specialties.begin(); it != specialties.end(); it++){
        if (it->first == specialty){
            flag = true;
            break;
        }
    }
    if (flag && vac.id != 0) specialties.insert(pair<string, vacancy> {specialty, vac});
}
 
void delete_vacancy(int id){
    for (multimap<string,vacancy>::iterator it = specialties.begin(); it != specialties.end(); it++){
        if (it->second.id == id) {
            specialties.erase(it);
            return;
        }
    }
}

string get_full_specialty_list(){
    if (specialties.size() == 0) {
        return string("is empty");
    }
    set <string> spec_set;
    for (multimap<string, vacancy>::iterator it = specialties.begin(); it != specialties.end(); it++){
        spec_set.insert(it->first);
    }
    string result = " ";
    for (set<string>::iterator its = spec_set.begin(); its != spec_set.end(); its++){
        result = result + *its;
        result = result + " ";
    }
    spec_set.clear();
    return result;
}

string get_pattern(multimap<string, vacancy>::iterator it){
    if (it->second.id == 0)
        return "";
    string result = "Специальность: " + it->first + ". ID: " + to_str(it->second.id) + ". Компания: " +
             it->second.company + ". Должность: " + it->second.position + "Возраст от " +
            to_str(it->second.lower_age_limit) + " до " + to_str(it->second.upper_age_limit) + ". ЗП: " +
            to_str(it->second.wages) + ".\n";
    return result;
}

string get_specialty_list(string specialty){
    string result = " ";
    for (multimap<string, vacancy>::iterator it = specialties.begin(); it != specialties.end(); it++){
        if (it->first == specialty) {
            result = result + get_pattern(it);
        }
    }
    return result;
}

string get_position_list(string position){
    string result = " ";
    for (multimap<string, vacancy>::iterator it = specialties.begin(); it != specialties.end(); it++){
        if (it->second.position == position) {
            result = result + get_pattern(it);
        }
    }
    return result;
}

string get_age_limit_list(int age){
    string result = " ";
    for (multimap<string, vacancy>::iterator it = specialties.begin(); it != specialties.end(); it++){
        if (age >= it->second.lower_age_limit && age <= it->second.upper_age_limit) {
            result = result + get_pattern(it);
        }
    }
    return result;
}

string get_wages_list(int wages){
    string result = " ";
    for (multimap<string, vacancy>::iterator it = specialties.begin(); it != specialties.end(); it++){
        if (it->second.wages == wages) {
            result = result + get_pattern(it);
        }
    }
    return result;
}


//Режимы:
//101-Добавление новой специальности
//102-Удаление специальности
//103-Добавление вакансии
//104-Список специальностей
//105-Список вакансий по условиям поиска (специальность)
//106-Список вакансий по условиям поиска (должность)
//107-Список вакансий по условиям поиска (возраст)
//108-Список вакансий по условиям поиска (заработная плата)
//

void communicating(int new_socket_descriptor) {

    while (true) {
        int mode;
        int temp = read(new_socket_descriptor, &mode, 4);
        if (temp <= 0) {
            perror("ERROR reading from socket");
            pthread_mutex_lock(&cs_mutex);
            clients.remove(new_socket_descriptor);
            pthread_mutex_unlock(&cs_mutex);
            close(new_socket_descriptor);
            break;
        }                                                                                                                                                                                                                  

        switch(mode){
            case 101:{
                string temp = read_message(new_socket_descriptor);
                printf("101 HERE\n");
                add_specialty(temp);
                break;
            }
            case 102:{
                string temp = read_message(new_socket_descriptor);
                delete_specialty(temp);
                printf("102 HERE\n");
                break;
            }
            case 103:{
                vacancy vac;
                string temp = read_message(new_socket_descriptor);
                vac.id = read_int(new_socket_descriptor);
                vac.company = read_message(new_socket_descriptor);
                vac.position = read_message(new_socket_descriptor);
                vac.lower_age_limit = read_int(new_socket_descriptor);
                vac.upper_age_limit = read_int(new_socket_descriptor);
                vac.wages = read_int(new_socket_descriptor);
                add_vacancy(temp, vac);
                break;
            }
            case 104:{
                write_message(new_socket_descriptor, get_full_specialty_list());
                break;
            }
            case 105:{
                string temp = read_message(new_socket_descriptor);
                write_message(new_socket_descriptor, get_specialty_list(temp));
                break;
            }
            case 106:{
                string temp = read_message(new_socket_descriptor);
                write_message(new_socket_descriptor, get_position_list(temp));
                break;
            }
            case 107:{
                int tempi = read_int(new_socket_descriptor);
                write_message(new_socket_descriptor, get_age_limit_list(tempi));
                break;
            }
            case 108:{
                int tempi = read_int(new_socket_descriptor);
                write_message(new_socket_descriptor, get_wages_list(tempi));
                break;
            }
        }
    }
    pthread_exit(0);
}


int main(int argc, char *argv[]) {
    check_argc(argc, argv);
    initialization_socket_descriptor();
    initialization_socket_structure();
    bind_host_address();
    listen(socket_descriptor, 5);
    while (true) {
        accept_connection();
    }
    for (thread &t: threads_list) {
        t.join();
    }
}
