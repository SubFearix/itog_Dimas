#include "server.h"
#include "hashTableUrers.h"
#include "register.h"
#include "log_in.h"
#include "common_utils.h"
#include "userHashTable.h"

#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>

using namespace std;
using json = nlohmann::json;

Server::Server(int port, const string& usersFile, const string& vaultDir) 
    : port(port), usersFilePath(usersFile), vaultDirectory(vaultDir), serverSocket(-1) {
}

Server::~Server() {
    stop();
}

string Server::getUserVaultPath(const string& username) {
    return vaultDirectory + "/" + username + ".vault";
}

bool Server::createUserVault(const string& username, const vector<unsigned char>& encryptedData) {
    string vaultPath = getUserVaultPath(username);
    ofstream file(vaultPath, ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(encryptedData.data()), encryptedData.size());
    file.close();
    return true;
}

vector<unsigned char> Server::readUserVault(const string& username) {
    string vaultPath = getUserVaultPath(username);
    ifstream file(vaultPath, ios::binary | ios::ate);
    
    if (!file.is_open()) {
        throw runtime_error("Хранилище пользователя не найдено");
    }
    
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    
    vector<unsigned char> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        throw runtime_error("Ошибка чтения хранилища пользователя");
    }
    
    file.close();
    return data;
}

bool Server::updateUserVault(const string& username, const vector<unsigned char>& encryptedData) {
    string vaultPath = getUserVaultPath(username);
    ofstream file(vaultPath, ios::binary);
    
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(encryptedData.data()), encryptedData.size());
    file.close();
    return true;
}

json Server::handleRegister(const json& request) {
    json response;
    
    try {
        string username = request["username"];
        string password = request["password"];
        
        // Валидация
        string errorMessage;
        if (!validateUsername(username, errorMessage)) {
            response["status"] = "error";
            response["message"] = errorMessage;
            return response;
        }
        
        if (!validatePassword(password, errorMessage)) {
            response["status"] = "error";
            response["message"] = errorMessage;
            return response;
        }
        
        // Загружаем таблицу пользователей
        HashTableUsers users;
        users.loadFromFile(usersFilePath);
        
        // Регистрируем пользователя
        auto seedWords = loginExist(username, password, &users);
        
        if (seedWords.empty()) {
            response["status"] = "error";
            response["message"] = "Пользователь уже существует";
            return response;
        }
        
        // Сохраняем пользователей
        users.saveToFile(usersFilePath);
        
        // Получаем vaultSalt для возврата клиенту
        string vaultSaltHex = users.getVaultSalt(username);
        
        // НЕ создаем зашифрованное хранилище здесь - клиент сделает это с кодовым словом
        // Создаем пустой файл хранилища
        vector<unsigned char> emptyVault;
        createUserVault(username, emptyVault);
        
        // Возвращаем seed words и vaultSalt
        response["status"] = "success";
        response["seedWords"] = seedWords;
        response["vaultSalt"] = vaultSaltHex;
        
    } catch (const exception& e) {
        response["status"] = "error";
        response["message"] = string("Ошибка регистрации: ") + e.what();
    }
    
    return response;
}

json Server::handleLogin(const json& request) {
    json response;
    
    try {
        string username = request["username"];
        string password = request["password"];
        
        // Загружаем таблицу пользователей
        HashTableUsers users;
        users.loadFromFile(usersFilePath);
        
        // Проверяем существование пользователя
        if (!existUser(username, users)) {
            response["status"] = "error";
            response["message"] = "Пользователь не найден";
            return response;
        }
        
        // Проверяем пароль
        if (!checkPasswordUser(username, password, users)) {
            response["status"] = "error";
            response["message"] = "Неверный пароль";
            return response;
        }
        
        // Читаем зашифрованное хранилище пользователя
        auto vaultData = readUserVault(username);
        
        // Получаем vaultSalt для клиента
        string vaultSalt = users.getVaultSalt(username);
        
        // Возвращаем зашифрованные данные клиенту
        response["status"] = "success";
        response["message"] = "Вход выполнен успешно";
        
        // Конвертируем вектор в base64 или hex для передачи
        string vaultHex = toHex(vaultData);
        response["vaultData"] = vaultHex;
        response["vaultSalt"] = vaultSalt;
        
    } catch (const exception& e) {
        response["status"] = "error";
        response["message"] = string("Ошибка входа: ") + e.what();
    }
    
    return response;
}

json Server::handleChangePassword(const json& request) {
    json response;
    
    try {
        string username = request["username"];
        string seedPhrase = request["seedPhrase"];
        string newPassword = request["newPassword"];
        
        // Валидация нового пароля
        string errorMessage;
        if (!validatePassword(newPassword, errorMessage)) {
            response["status"] = "error";
            response["message"] = errorMessage;
            return response;
        }
        
        // Загружаем таблицу пользователей
        HashTableUsers users;
        users.loadFromFile(usersFilePath);
        
        // Проверяем существование пользователя
        if (!existUser(username, users)) {
            response["status"] = "error";
            response["message"] = "Пользователь не найден";
            return response;
        }
        
        // Проверяем seed phrase
        if (!checkPhrase(username, users, seedPhrase)) {
            response["status"] = "error";
            response["message"] = "Неверная фраза восстановления";
            return response;
        }
        
        // Получаем старую vaultSalt перед изменением
        string oldVaultSalt = users.getVaultSalt(username);
        
        // Получаем зашифрованные данные хранилища
        vector<unsigned char> vaultData;
        try {
            vaultData = readUserVault(username);
        } catch (const exception& e) {
            // Если хранилища нет, создаем пустое
            vaultData.clear();
        }
        string vaultDataHex = toHex(vaultData);
        
        // Меняем пароль и получаем новую vaultSalt
        auto newSeedWords = switchDataUsers(username, newPassword, users);
        string newVaultSalt = users.getVaultSalt(username);
        
        // Сохраняем изменения
        users.saveToFile(usersFilePath);
        
        response["status"] = "success";
        response["message"] = "Пароль успешно изменен";
        response["newSeedWords"] = newSeedWords;
        response["oldVaultSalt"] = oldVaultSalt;
        response["newVaultSalt"] = newVaultSalt;
        response["vaultData"] = vaultDataHex;
        
    } catch (const exception& e) {
        response["status"] = "error";
        response["message"] = string("Ошибка смены пароля: ") + e.what();
    }
    
    return response;
}

json Server::handleRecoverPassword(const json& request) {
    json response;
    
    try {
        string username = request["username"];
        string seedPhrase = request["seedPhrase"];
        string newPassword = request["newPassword"];
        
        // Валидация нового пароля
        string errorMessage;
        if (!validatePassword(newPassword, errorMessage)) {
            response["status"] = "error";
            response["message"] = errorMessage;
            return response;
        }
        
        // Загружаем таблицу пользователей
        HashTableUsers users;
        users.loadFromFile(usersFilePath);
        
        // Проверяем существование пользователя
        if (!existUser(username, users)) {
            response["status"] = "error";
            response["message"] = "Пользователь не найден";
            return response;
        }
        
        // Проверяем seed phrase
        if (!checkPhrase(username, users, seedPhrase)) {
            response["status"] = "error";
            response["message"] = "Неверная фраза восстановления";
            return response;
        }
        
        // Получаем старую vaultSalt перед изменением
        string oldVaultSalt = users.getVaultSalt(username);
        
        // Получаем зашифрованные данные хранилища
        vector<unsigned char> vaultData;
        try {
            vaultData = readUserVault(username);
        } catch (const exception& e) {
            // Если хранилища нет, создаем пустое
            vaultData.clear();
        }
        string vaultDataHex = toHex(vaultData);
        
        // Меняем пароль и получаем новую vaultSalt
        auto newSeedWords = switchDataUsers(username, newPassword, users);
        string newVaultSalt = users.getVaultSalt(username);
        
        // Сохраняем изменения
        users.saveToFile(usersFilePath);
        
        response["status"] = "success";
        response["message"] = "Пароль успешно восстановлен";
        response["newSeedWords"] = newSeedWords;
        response["oldVaultSalt"] = oldVaultSalt;
        response["newVaultSalt"] = newVaultSalt;
        response["vaultData"] = vaultDataHex;
        
    } catch (const exception& e) {
        response["status"] = "error";
        response["message"] = string("Ошибка восстановления пароля: ") + e.what();
    }
    
    return response;
}

json Server::handleGetVault(const json& request) {
    json response;
    
    try {
        string username = request["username"];
        string password = request["password"];
        
        // Аутентификация
        HashTableUsers users;
        users.loadFromFile(usersFilePath);
        
        if (!existUser(username, users) || !checkPasswordUser(username, password, users)) {
            response["status"] = "error";
            response["message"] = "Ошибка аутентификации";
            return response;
        }
        
        // Читаем зашифрованное хранилище
        auto vaultData = readUserVault(username);
        string vaultHex = toHex(vaultData);
        
        // Получаем vaultSalt для клиента
        string vaultSalt = users.getVaultSalt(username);
        
        response["status"] = "success";
        response["vaultData"] = vaultHex;
        response["vaultSalt"] = vaultSalt;
        
    } catch (const exception& e) {
        response["status"] = "error";
        response["message"] = string("Ошибка получения данных: ") + e.what();
    }
    
    return response;
}

json Server::handleGetVaultWithSeedPhrase(const json& request) {
    json response;
    
    try {
        // Проверяем наличие обязательных полей
        if (!request.contains("username") || !request.contains("seedPhrase")) {
            response["status"] = "error";
            response["message"] = "Отсутствуют обязательные поля";
            return response;
        }
        
        string username = request["username"];
        string seedPhrase = request["seedPhrase"];
        
        // Валидация имени пользователя
        string errorMessage;
        if (!validateUsername(username, errorMessage)) {
            response["status"] = "error";
            response["message"] = errorMessage;
            return response;
        }
        
        // Загружаем таблицу пользователей
        HashTableUsers users;
        users.loadFromFile(usersFilePath);
        
        // Проверяем существование пользователя
        if (!existUser(username, users)) {
            response["status"] = "error";
            response["message"] = "Пользователь не найден";
            return response;
        }
        
        // Аутентификация по seed phrase
        if (!checkPhrase(username, users, seedPhrase)) {
            response["status"] = "error";
            response["message"] = "Неверная фраза восстановления";
            return response;
        }
        
        // Читаем зашифрованное хранилище
        auto vaultData = readUserVault(username);
        string vaultHex = toHex(vaultData);
        
        // Получаем vaultSalt для клиента
        string vaultSalt = users.getVaultSalt(username);
        
        response["status"] = "success";
        response["vaultData"] = vaultHex;
        response["vaultSalt"] = vaultSalt;
        
    } catch (const exception& e) {
        response["status"] = "error";
        response["message"] = string("Ошибка получения данных: ") + e.what();
    }
    
    return response;
}

json Server::handleUpdateVault(const json& request) {
    json response;
    
    try {
        string username = request["username"];
        string password = request["password"];
        string vaultHex = request["vaultData"];
        
        // Аутентификация
        HashTableUsers users;
        users.loadFromFile(usersFilePath);
        
        if (!existUser(username, users) || !checkPasswordUser(username, password, users)) {
            response["status"] = "error";
            response["message"] = "Ошибка аутентификации";
            return response;
        }
        
        // Конвертируем hex обратно в вектор
        auto vaultData = hexToBytes(vaultHex);
        
        // Обновляем хранилище
        if (!updateUserVault(username, vaultData)) {
            response["status"] = "error";
            response["message"] = "Не удалось обновить хранилище";
            return response;
        }
        
        response["status"] = "success";
        response["message"] = "Данные успешно обновлены";
        
    } catch (const exception& e) {
        response["status"] = "error";
        response["message"] = string("Ошибка обновления данных: ") + e.what();
    }
    
    return response;
}

json Server::processRequest(const json& request) {
    string action = request["action"];
    
    if (action == "register") {
        return handleRegister(request);
    } else if (action == "login") {
        return handleLogin(request);
    } else if (action == "changePassword") {
        return handleChangePassword(request);
    } else if (action == "recoverPassword") {
        return handleRecoverPassword(request);
    } else if (action == "getVault") {
        return handleGetVault(request);
    } else if (action == "getVaultWithSeedPhrase") {
        return handleGetVaultWithSeedPhrase(request);
    } else if (action == "updateVault") {
        return handleUpdateVault(request);
    } else {
        json response;
        response["status"] = "error";
        response["message"] = "Неизвестное действие";
        return response;
    }
}

void Server::handleClient(int clientSocket) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    // Читаем запрос
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }
    
    try {
        // Парсим JSON запрос
        json request = json::parse(buffer);
        
        // Обрабатываем запрос
        json response = processRequest(request);
        
        // Отправляем ответ
        string responseStr = response.dump();
        send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
        
    } catch (const exception& e) {
        json errorResponse;
        errorResponse["status"] = "error";
        errorResponse["message"] = string("Ошибка обработки запроса: ") + e.what();
        
        string responseStr = errorResponse.dump();
        send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
    }
    
    close(clientSocket);
}

bool Server::initialize() {
    // Создаем директорию для хранилищ, если она не существует
    mkdir(vaultDirectory.c_str(), 0755);
    
    // Создаем файл пользователей, если он не существует
    ifstream testFile(usersFilePath);
    if (!testFile.good()) {
        ofstream createFile(usersFilePath);
        createFile << "[]";
        createFile.close();
    }
    testFile.close();
    
    // Создаем сокет
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "Ошибка: не удалось создать сокет" << endl;
        return false;
    }
    
    // Настраиваем опции сокета
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "Ошибка: не удалось установить опции сокета" << endl;
        return false;
    }
    
    // Привязываем сокет к порту
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Ошибка: не удалось привязать сокет к порту " << port << endl;
        cerr << "Возможно, порт уже используется другим процессом" << endl;
        return false;
    }
    
    // Начинаем прослушивание
    if (listen(serverSocket, 5) < 0) {
        cerr << "Ошибка: не удалось начать прослушивание сокета" << endl;
        return false;
    }
    
    cout << "Сервер инициализирован на порту " << port << endl;
    return true;
}

void Server::start() {
    cout << "Сервер запущен и ожидает подключений..." << endl;
    
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            continue;
        }
        
        // Обрабатываем клиента
        handleClient(clientSocket);
    }
}

void Server::stop() {
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
}
