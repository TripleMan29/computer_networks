#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <thread>
#include <iostream>
#include <locale.h>
#include <string>

#define true  1
#define false 0
#define BUFFER_LENGTH 1024

using namespace std;


int socket_descriptor;
uint16_t port_number;
struct sockaddr_in server_address;
struct hostent *server;
char temp_string[4];
string output_buffer;

pthread_mutex_t cs_mutex;

time_t seconds;


void check_condition(int cond, char *str, int sig){
    if (cond){
        perror(str);
        exit(sig);
    }
}

void check_argc(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage %s hostname port\n", argv[0]);
        exit(0);
    }
    server = gethostbyname(argv[1]);
    check_condition(server == NULL, "ERROR, no such host\n", 0);

    // convert to network-short (to big-endian)
    port_number = htons((uint16_t) atoi(argv[2]));
}


void initialization_socket_descriptor(){
    //  AF_INET     - IPv4
    //  SOCK_STREAM - TCP
    //  0           - Default
    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
}

void initialization_server_address(){
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy(server->h_addr, (char *) &server_address.sin_addr.s_addr, (size_t) server->h_length);
    server_address.sin_port = port_number;
}

void server_connect(){
    if (connect(socket_descriptor, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }
}

int read_int(){
    int message;
    int temp = read(socket_descriptor, &message, 4);
    if (temp <= 0) {
        perror("ERROR reading from socket");
        shutdown(socket_descriptor, 2);
        close(socket_descriptor);
        exit(0);
    }
    return message;
}

void write_int(int number){
    int temp = write(socket_descriptor, &number, 4);
    if (temp <= 0) {
        perror("ERROR reading from socket");
        shutdown(socket_descriptor, 2);
        close(socket_descriptor);
        exit(0);
    }
}

string read_message(){
    int message_size;
    int temp = read(socket_descriptor, &message_size, 4);
    if (temp <= 0) {
        perror("ERROR reading from socket");
        shutdown(socket_descriptor, 2);
        close(socket_descriptor);
        exit(0);
    }
    auto buffer = new char[message_size];
    bzero(buffer, sizeof(buffer));
    temp = read(socket_descriptor, buffer, message_size);
    if (temp <= 0) {
        perror("ERROR reading from socket");
        shutdown(socket_descriptor, 2);
        close(socket_descriptor);
        pthread_exit(0);
    }
    return string(buffer);
}

void write_message(string message){
    int message_size = message.length();
    int temp = write(socket_descriptor, &message_size, 4);
    if (temp <= 0) {
        perror("ERROR writing in socket");
        shutdown(socket_descriptor, 2);
        close(socket_descriptor);
        exit(0);
    }
    temp = write(socket_descriptor, message.c_str(), message_size);
    if (temp <= 0) {
        perror("ERROR writing in socket");
        shutdown(socket_descriptor, 2);
        close(socket_descriptor);
        exit(0);
    }
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

void * output_thread_fun(){
    while(true){
        char buffer[BUFFER_LENGTH] = {0};
        printf("Введите режим:\n");
        cin.getline(buffer, BUFFER_LENGTH, '\n');
        switch (atoi(buffer)) {
            case 101: {
                bzero(buffer, sizeof(buffer));
                printf("Введите специальность:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(101);
                write_message(string(buffer));
                break;
            }
            case 102: {
                bzero(buffer, sizeof(buffer));
                printf("Введите специальность:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(102);
                write_message(string(buffer));
                break;
            }
            case 103: {
                bzero(buffer, sizeof(buffer));
                write_int(103);
                printf("Введите специальность:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_message(string(buffer));

                bzero(buffer, sizeof(buffer));
                printf("Введите id:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(atoi(buffer));

                bzero(buffer, sizeof(buffer));
                printf("Введите компанию:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_message(string(buffer));

                bzero(buffer, sizeof(buffer));
                printf("Введите должность:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_message(string(buffer));

                bzero(buffer, sizeof(buffer));
                printf("Введите нижний порог возраста:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(atoi(buffer));

                bzero(buffer, sizeof(buffer));
                printf("Введите верхний порог возраста:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(atoi(buffer));

                bzero(buffer, sizeof(buffer));
                printf("Введите уровень зарплаты:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(atoi(buffer));

                break;
            }
            case 104: {
                write_int(104);
                string temp = read_message();
                printf("Список специальностей: %s\n", temp.c_str());
                break;
            }
            case 105: {
                bzero(buffer, sizeof(buffer));
                printf("Введите специальность:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(105);
                write_message(string(buffer));
                string temp = read_message();
                printf("Список: \n%s\n", temp.c_str());
                break;
            }
            case 106: {
                bzero(buffer, sizeof(buffer));
                printf("Введите должность:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(106);
                write_message(string(buffer));
                string temp = read_message();
                printf("Список: \n%s\n", temp.c_str());
                break;
            }
            case 107: {
                bzero(buffer, sizeof(buffer));
                printf("Введите возраст:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(107);
                write_int(atoi(buffer));
                string temp = read_message();
                printf("Список: \n%s\n", temp.c_str());
                break;
            }
            case 108: {
                bzero(buffer, sizeof(buffer));
                printf("Введите ЗП:\n");
                cin.getline(buffer, BUFFER_LENGTH, '\n');
                write_int(108);
                write_int(atoi(buffer));
                string temp = read_message();
                printf("Список: \n%s\n", temp.c_str());
                break;
            }
        }
    }
}


int main(int argc, char *argv[]) {
    check_argc(argc, argv);
    initialization_socket_descriptor();
    initialization_server_address();
    server_connect();

//    thread input_thread(input_thread_fun);
    thread output_thread(output_thread_fun);

    output_thread.join();
//    input_thread.join();
}

