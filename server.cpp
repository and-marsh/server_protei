//server.cpp -  эхо TCP(UDP)/IP сервер
//порт - 1234

//подключаем необходимые заголовчные файлы
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unistd.h>

//проверка, задан ли шаблон INADDR_NONE, который обозначает сразу все доступные сетевые интерфейсы
#ifndef INADDR_NONE
#define INADDR_NONE 0xfffffffff
#endif
#define BUFFSIZE 100 //максимальный размер сообщения, 100 байт
#define QUEUE 5      //длина очереди подключения к серверу

using namespace std;

//функция создания и связывания сокета, объявление
//аргументы:
//const char port - порт, с которым связывается сервер
//const char transport - протокол, по которому будет работать сервер (tcp или udp)
//const char qlen - длина  очереди
//struct sockaddr_in &sin - структура IP-адреса сервера
//возвращаемое значение:
//int - дескриптор сокета
int sock(const char *port, const char *transport, int qlen, struct sockaddr_in &sin);

//фунция обработки сообщения, объявление
//аргументы:
//const char* msg - адрес массива сообщения
//vector<int>& - ссылка не вектор, куда будет записываться результат
void messageProcessing(const char* msg, vector<int>& numbers);

//функция для сортировки int по убыванию функцией sort(int a, int b, comp)
bool comp (int a,int b) { return (a>b); }

//главная функция
int main()
{
    int udp_sock, tcp_sock, client_sock;                //дескрипторы сокетов
    struct sockaddr_in sin;                             //структура IP-адреса сервера
    struct sockaddr_in  client_sin;                     //структура IP-адреса клиента
    unsigned int client_sin_size = sizeof(client_sin);	//размер структуры адреса
    int length_msg;                                     //длина отправляемого/принимаемого сообщения
    char msg[BUFFSIZE];                                 //буфер для принимаемого сообщения
    memset(&msg, 0, sizeof(msg));                       //обнуляем буфер
    char *dyn_msg;                                      //динамический буфер для отправляемого сообщения
    vector<int> numbers;                                //вектор для чисел из сообщения

    udp_sock = sock("1234", "udp", QUEUE, sin);         //создаем udp сокет и привязываем его к порту 1234, задав очередь 5
    if(udp_sock < 0)                                    //проверяем значение дескриптора сокета udp
        return -1;                                      //завершаем программу в случае неудачи
    tcp_sock = sock("1234", "tcp", QUEUE, sin);         //создаем tcp сокет и привязываем его к порту 1234(порты udp и tcp не пересекаются), задав очередь 5
    if(tcp_sock < 0)                                    //проверяем значение дескриптора сокета tcp
        return -1;                                      //завершаем программу в случае неудачи

    fd_set fd,cur_fd;                       //структуры, для работы с дескрипторами
    FD_ZERO( &fd );                         //инициализация нулями
    FD_SET( udp_sock, &fd );                //добавление дескриптора udp_sock в структуру
    FD_SET( tcp_sock, &fd );                //добавление дескриптора tcp_sock в структуру

    struct timeval tv;                      //структура для времени ожидания
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    while(1)
    {
        cur_fd = fd;
        if( select( tcp_sock + 1, &cur_fd, NULL, NULL, &tv ) <0 )       //ожидание входящих udp/tcp соединений на дескрипторы udp_sock/tcp_sock
        {
            cout<<"Ошибка сокета: "<<strerror(errno)<<endl;
        }

        if(FD_ISSET(udp_sock, &cur_fd))     //если входящее подключение -  udp
        {
            cout<<"UDP!!!"<<endl;
            //принимаем сообщение, адрес клиента кладется в стуктуру client_sin
            if((recvfrom( udp_sock, ( void* )msg, BUFFSIZE, 0, ( struct sockaddr* )&client_sin, &client_sin_size))<0)
            {
                cout<<"Ошибка: "<<strerror( errno );
                continue;
            }
            else
            {
                length_msg = strlen(msg) + 1;               //длина строки + '\0'
                cout<<"Байт получено: "<<length_msg<<endl;
                cout<<"Сообщение: "<<msg<<endl;
                dyn_msg = new char[length_msg];             //выделяем память
                strcpy(dyn_msg,msg);                        //формируем сообщение
                if(sendto(udp_sock, dyn_msg, length_msg, 0, ( struct sockaddr* )&client_sin, sizeof( struct sockaddr_in )) < 0)	//отсылаем клиенту ответ
                    cout<<"Не удалось отправить данные клиенту: "<<strerror(errno)<<endl;
                memset(&msg, 0, length_msg);                //обнуляем буфер
                messageProcessing(dyn_msg,numbers);
                delete dyn_msg;                             //очищаем память
            }
        }

        if(FD_ISSET(tcp_sock, &cur_fd))     //если входящее подключение -  tcp
        {
            cout<<"TCP!!!"<<endl;
            client_sock = accept(tcp_sock, (struct sockaddr*) &client_sin, &client_sin_size);	//принимаем входящее подключение, адрес клиента в client_sin
            if(client_sock < 0)                                                                 //проверяем результат
            {
                cout<<"Ошибка принятия подключения: "<<strerror(errno)<<endl;                   //в случае неудачи выводим сообщение ошибки
                continue;
            }
            if(read(client_sock, &msg, sizeof(msg)) <0)                                         //пробуем читать данные от клиента
            {
                cout<<"Ошибка: "<<strerror( errno );
                close(client_sock);
                continue;
            }
            else
            {
                length_msg = strlen(msg) + 1;               //длина строки + '\0'
                cout<<"Байт получено: "<<length_msg<<endl;
                cout<<"Сообщение: "<<msg<<endl;
                dyn_msg = new char[length_msg];             //выделяем память
                strcpy(dyn_msg,msg);                        //формируем сообщение
                write(client_sock, dyn_msg, length_msg);    //отсылаем ответ
                memset(&msg, 0, length_msg);                //обнуляем буфер
                messageProcessing(dyn_msg,numbers);
                delete dyn_msg;                             //очищаем память
            }
            close(client_sock);                             //закрываем сокет клиента
        }
    }
}

//функция создания и связывания сокета, реализация
int sock(const char *port, const char *transport, int qlen, sockaddr_in &sin)
{
    struct protoent *ppe;       //указатель на запись с информацией о протоколе
    int s, type;                //nдескриптор и тип сокета

    memset(&sin, 0, sizeof(sin));                       //обнуляем структуру адреса
    sin.sin_family = AF_INET;                           //указываем тип адреса - IPv4
    sin.sin_addr.s_addr = INADDR_ANY;                   //указываем, в качестве адреса, шаблон INADDR_ANY - все сетевые интерфейсы
    sin.sin_port = htons((unsigned short)atoi(port));   //задаем порт

    if((ppe = getprotobyname(transport)) == 0)          //преобразовываем имя транспортного протокола в номер протокола
        {
            cout<<"Ошибка преобразования имени транспортного протокола: "<<strerror(errno)<<endl;	//в случае неудачи выводим сообщение ошибки
            return -1;
        }
    if(strcmp(transport, "udp") == 0)           //используем имя протокола для определения типа сокета
        type = SOCK_DGRAM;                      //для udp
    else
        type = SOCK_STREAM;                     //для tcp

    s = socket(PF_INET, type, ppe->p_proto);    //создаем сокет
    if(s < 0)
        {
            cout<<"Ошибка создания сокета: "<<strerror(errno)<<endl;        //в случае неудачи выводим сообщение ошибки
            return -1;
        }
    if(bind(s, (struct sockaddr *)&sin, sizeof( struct sockaddr ) ) < 0)    //привязка сокета с проверкой результата
        {
            cout<<"Ошибка связывания сокета: "<<strerror(errno)<<endl;      //в случае неудачи выводим сообщение ошибки
            return -1;
        }
    if(type == SOCK_STREAM && listen(s, qlen) <0)                           //запуск прослушивания с проверкой результата
        {
            cout<<"Ошибка прослушивания сокета: "<<strerror(errno)<<endl;	//в случае неудачи выводим сообщение ошибки
            return -1;
        }
    return s;	//возвращаем дескриптор сокета
}

//фунция обработки сообщения, реализация
void messageProcessing(const char* msg, vector<int>& numbers)
{
    int sum=0;
    numbers.clear();
    for(unsigned int i=0; i<strlen(msg); i++)
    {
        if(msg[i]>=48 && msg[i]<=57)
        {
            numbers.push_back(msg[i] - '0');
        }
    }
    sort(numbers.begin(),numbers.end(),comp);
    cout<<"Числа в порядке убывания:"<<endl;
    for(unsigned int i=0; i<numbers.size();i++)
    {
        sum+=numbers[i];
        cout<<numbers[i]<<" ";
    }
    cout<<"\nСумма чисел: "<<sum<<endl;
    cout<<"Минимальное число: "<<numbers.back()<<endl;
    cout<<"Максимальное число: "<<numbers.front()<<endl<<endl;
}
