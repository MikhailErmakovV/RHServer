#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"Ws2_32.lib")

#define PORT 8080

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: client <host> <message>\n";
        return 1;
    }

    const char* host = argv[1];
    const char* msg = argv[2];

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != NO_ERROR) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return 1;
    }

    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo hints, * resultPtr = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Получение структуры с информацией о адресе
    result = getaddrinfo(host, "8080", &hints, &resultPtr);
    if (result != 0) {
        std::cerr << "getaddrinfo failed with error: " << result << std::endl;
        WSACleanup();
        return 1;
    }

    // Создание сокета
    connectSocket = socket(resultPtr->ai_family, resultPtr->ai_socktype, resultPtr->ai_protocol);
    if (connectSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(resultPtr);
        WSACleanup();
        return 1;
    }

    // Подключение к серверу
    result = connect(connectSocket, resultPtr->ai_addr, (int)resultPtr->ai_addrlen);
    if (result == SOCKET_ERROR) {
        std::cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(resultPtr);
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(resultPtr);

    // Отправка сообщения серверу
    send(connectSocket, msg, strlen(msg), 0);

    // Чтение ответа от сервера
    char recvbuf[1024];
    int recvlen = recv(connectSocket, recvbuf, 1024, 0);
    if (recvlen > 0) {
        std::string response(recvbuf, recvlen);
        std::cout << "Response from server: \"" << response << "\"" << std::endl;
    }

    closesocket(connectSocket);
    WSACleanup();

    return 0;
}
