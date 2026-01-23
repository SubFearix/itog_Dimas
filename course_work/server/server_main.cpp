#include "server.h"
#include <iostream>
#include <signal.h>

using namespace std;

Server* globalServer = nullptr;

void signalHandler(int signum) {
    cout << "\nПолучен сигнал прерывания (" << signum << "). Остановка сервера..." << endl;
    if (globalServer) {
        globalServer->stop();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    cout << "Запуск сервера на порту " << port << "..." << endl;
    
    // Настраиваем обработчик сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    Server server(port);
    globalServer = &server;
    
    if (!server.initialize()) {
        cerr << "Не удалось инициализировать сервер" << endl;
        return 1;
    }
    
    server.start();
    
    return 0;
}
