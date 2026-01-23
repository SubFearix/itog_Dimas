#include "client.h"
#include "hashTableUrers.h"
#include "common_utils.h"
#include "userHashTable.h"

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;
using json = nlohmann::json;

Client::Client(const string& host, int port) 
    : serverHost(host), serverPort(port), isLoggedIn(false), vault(nullptr), codeWord("") {
}

Client::~Client() {
    if (vault) {
        delete vault;
    }
}

json Client::sendRequest(const json& request) {
    // Создаем сокет
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw runtime_error("Ошибка создания сокета");
    }
    
    // Настраиваем адрес сервера
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    
    if (inet_pton(AF_INET, serverHost.c_str(), &serverAddr.sin_addr) <= 0) {
        close(sock);
        throw runtime_error("Неверный адрес сервера");
    }
    
    // Подключаемся к серверу
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(sock);
        throw runtime_error("Не удалось подключиться к серверу");
    }
    
    // Отправляем запрос
    string requestStr = request.dump();
    send(sock, requestStr.c_str(), requestStr.length(), 0);
    
    // Получаем ответ
    char buffer[8192];
    memset(buffer, 0, sizeof(buffer));
    int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    close(sock);
    
    if (bytesRead <= 0) {
        throw runtime_error("Не получен ответ от сервера");
    }
    
    // Парсим ответ
    json response = json::parse(buffer);
    return response;
}

vector<unsigned char> Client::deriveVaultKey(const string& codeWord, const string& vaultSaltHex) {
    auto vaultSalt = hexToBytes(vaultSaltHex);
    return hashPasswordArgon2id(codeWord, vaultSalt);
}

void Client::decryptAndLoadVault(const string& encryptedVaultHex) {
    if (encryptedVaultHex.empty()) {
        // Пустое хранилище - создаем новое
        if (vault) {
            delete vault;
        }
        vault = new UserHashTable();
        return;
    }
    
    auto encryptedVault = hexToBytes(encryptedVaultHex);
    string decryptedJson = decrypt_aes_gcm(encryptedVault, vaultKey);
    
    if (vault) {
        delete vault;
    }
    vault = new UserHashTable();
    
    json vaultJson = json::parse(decryptedJson);
    vault->fromJson(vaultJson);
}

string Client::encryptVault() {
    json vaultJson = vault->toJson();
    string vaultStr = vaultJson.dump();
    
    auto encryptedVault = encrypt_aes_gcm(vaultStr, vaultKey);
    return toHex(encryptedVault);
}

bool Client::validateCodeWordWithVault(const string& codeWord, const string& vaultSaltHex, const string& encryptedVaultHex, string& decryptedJson) {
    // Проверяем, есть ли данные в хранилище
    if (encryptedVaultHex.empty()) {
        // Хранилище пустое
        decryptedJson = "[]";
        return true;
    }
    
    try {
        // Пытаемся расшифровать с предоставленным кодовым словом
        auto vaultKey = deriveVaultKey(codeWord, vaultSaltHex);
        auto encryptedVault = hexToBytes(encryptedVaultHex);
        decryptedJson = decrypt_aes_gcm(encryptedVault, vaultKey);
        return true;
    } catch (const exception& e) {
        // Неверное кодовое слово
        return false;
    }
}

bool Client::validateCodeWord(const string& codeWord, string& errorMessage) {
    // Проверка минимальной длины (не менее 3 символов)
    if (codeWord.length() < 3) {
        errorMessage = "Кодовое слово должно содержать не менее 3 символов";
        return false;
    }
    
    // Проверка максимальной длины (не более 30 символов)
    if (codeWord.length() > 30) {
        errorMessage = "Кодовое слово должно содержать не более 30 символов";
        return false;
    }
    
    return true;
}

bool Client::checkUserExists(const string& user) {
    try {
        json request;
        request["action"] = "checkUser";
        request["username"] = user;
        
        json response = sendRequest(request);
        
        if (response["status"] == "success") {
            return response["exists"];
        } else {
            // Если сервер вернул ошибку, считаем что пользователь не существует
            return false;
        }
    } catch (const exception& e) {
        return false;
    }
}

bool Client::registerUser(const string& user, const string& pass) {
    vector<string> seedWords;
    return registerUser(user, pass, seedWords);
}

bool Client::registerUser(const string& user, const string& pass, vector<string>& seedWords) {
    try {
        json request;
        request["action"] = "register";
        request["username"] = user;
        request["password"] = pass;
        
        json response = sendRequest(request);
        
        if (response["status"] == "success") {
            auto seedWordsJson = response["seedWords"];
            seedWords.clear();
            for (size_t i = 0; i < seedWordsJson.size(); i++) {
                string word = seedWordsJson[i];
                seedWords.push_back(word);
            }
            
            return true;
        } else {
            return false;
        }
    } catch (const exception& e) {
        return false;
    }
}

bool Client::registerUser(const string& user, const string& pass, const string& codeWord, vector<string>& seedWords) {
    try {
        // Validate code word
        string errorMessage;
        if (!validateCodeWord(codeWord, errorMessage)) {
            return false;
        }
        
        json request;
        request["action"] = "register";
        request["username"] = user;
        request["password"] = pass;
        
        json response = sendRequest(request);
        
        if (response["status"] == "success") {
            auto seedWordsJson = response["seedWords"];
            seedWords.clear();
            for (size_t i = 0; i < seedWordsJson.size(); i++) {
                string word = seedWordsJson[i];
                seedWords.push_back(word);
            }
            
            // Сохраняем кодовое слово в памяти для шифрования хранилища
            this->codeWord = codeWord;
            this->username = user;
            this->password = pass;
            
            // Получаем vaultSalt из ответа
            string vaultSaltHex = response["vaultSalt"];
            
            // Создаем пустое хранилище
            if (vault) {
                delete vault;
            }
            vault = new UserHashTable();
            
            // Вычисляем ключ для шифрования хранилища с кодовым словом
            vaultKey = deriveVaultKey(codeWord, vaultSaltHex);
            
            // Шифруем пустое хранилище с кодовым словом
            json emptyVault = json::array();
            string vaultJson = emptyVault.dump();
            auto encryptedVault = encrypt_aes_gcm(vaultJson, vaultKey);
            string encryptedVaultHex = toHex(encryptedVault);
            
            // Отправляем зашифрованное хранилище на сервер
            json updateRequest;
            updateRequest["action"] = "updateVault";
            updateRequest["username"] = user;
            updateRequest["password"] = pass;
            updateRequest["vaultData"] = encryptedVaultHex;
            
            json updateResponse = sendRequest(updateRequest);
            
            if (updateResponse["status"] != "success") {
                return false;
            }
            
            return true;
        } else {
            return false;
        }
    } catch (const exception& e) {
        return false;
    }
}

bool Client::login(const string& user, const string& pass, const string& codeWord) {
    try {
        // Validate code word
        string errorMessage;
        if (!validateCodeWord(codeWord, errorMessage)) {
            return false;
        }
        
        json request;
        request["action"] = "login";
        request["username"] = user;
        request["password"] = pass;
        
        json response = sendRequest(request);
        
        if (response["status"] == "success") {
            username = user;
            password = pass;
            this->codeWord = codeWord;
            isLoggedIn = true;
            
            // Получаем vaultSalt из ответа сервера
            string vaultSaltHex = response["vaultSalt"];
            
            // Вычисляем ключ для расшифровки хранилища используя кодовое слово
            vaultKey = deriveVaultKey(codeWord, vaultSaltHex);
            
            // Расшифровываем и загружаем хранилище
            string encryptedVaultHex = response["vaultData"];
            decryptAndLoadVault(encryptedVaultHex);
            
            return true;
        } else {
            return false;
        }
    } catch (const exception& e) {
        return false;
    }
}

bool Client::changePassword(const string& seedPhrase, const string& newPassword, const string& codeWord) {
    try {
        // Validate code word
        string errorMessage;
        if (!validateCodeWord(codeWord, errorMessage)) {
            return false;
        }
        
        // Сначала получаем текущее хранилище БЕЗ изменения пароля, чтобы проверить кодовое слово
        json getVaultRequest;
        getVaultRequest["action"] = "getVault";
        getVaultRequest["username"] = username;
        getVaultRequest["password"] = password;  // Используем СТАРЫЙ пароль
        
        json getVaultResponse = sendRequest(getVaultRequest);
        
        if (getVaultResponse["status"] != "success") {
            return false;
        }
        
        // Проверяем кодовое слово, пытаясь расшифровать текущее хранилище
        string currentVaultSaltHex = getVaultResponse["vaultSalt"];
        string currentEncryptedVaultHex = getVaultResponse["vaultData"];
        
        string decryptedJson;
        if (!validateCodeWordWithVault(codeWord, currentVaultSaltHex, currentEncryptedVaultHex, decryptedJson)) {
            // Неверное кодовое слово - возвращаем ошибку ДО изменения пароля
            return false;
        }
        
        // Кодовое слово верное! Теперь можем менять пароль
        json request;
        request["action"] = "changePassword";
        request["username"] = username;
        request["seedPhrase"] = seedPhrase;
        request["newPassword"] = newPassword;
        
        json response = sendRequest(request);
        
        if (response["status"] == "success") {
            // Получаем новую vaultSalt
            string newVaultSaltHex = response["newVaultSalt"];
            
            // Шифруем данные с новым ключом
            // ВАЖНО: Используем то же кодовое слово, но с новой солью
            auto newVaultKey = deriveVaultKey(codeWord, newVaultSaltHex);
            auto reEncryptedVault = encrypt_aes_gcm(decryptedJson, newVaultKey);
            string reEncryptedVaultHex = toHex(reEncryptedVault);
            
            // Отправляем обратно зашифрованные данные на сервер
            json updateRequest;
            updateRequest["action"] = "updateVault";
            updateRequest["username"] = username;
            updateRequest["password"] = newPassword;
            updateRequest["vaultData"] = reEncryptedVaultHex;
            
            json updateResponse = sendRequest(updateRequest);
            
            if (updateResponse["status"] != "success") {
                return false;
            }
            
            // Обновляем локальные данные
            password = newPassword;
            this->codeWord = codeWord;
            vaultKey = newVaultKey;
            
            // Перезагружаем vault с новым ключом
            decryptAndLoadVault(reEncryptedVaultHex);
            
            return true;
        } else {
            return false;
        }
    } catch (const exception& e) {
        return false;
    }
}

bool Client::recoverPassword(const string& user, const string& seedPhrase, const string& newPassword) {
    vector<string> newSeedWords;
    return recoverPassword(user, seedPhrase, newPassword, newSeedWords);
}

bool Client::recoverPassword(const string& user, const string& seedPhrase, const string& newPassword, vector<string>& newSeedWords) {
    try {
        json request;
        request["action"] = "recoverPassword";
        request["username"] = user;
        request["seedPhrase"] = seedPhrase;
        request["newPassword"] = newPassword;
        
        json response = sendRequest(request);
        
        if (response["status"] == "success") {
            auto seeds = response["newSeedWords"];
            newSeedWords.clear();
            for (size_t i = 0; i < seeds.size(); i++) {
                string word = seeds[i];
                newSeedWords.push_back(word);
            }
            
            return true;
        } else {
            return false;
        }
    } catch (const exception& e) {
        return false;
    }
}

bool Client::recoverPassword(const string& user, const string& seedPhrase, const string& newPassword, const string& codeWord, vector<string>& newSeedWords) {
    try {
        // Validate code word
        string errorMessage;
        if (!validateCodeWord(codeWord, errorMessage)) {
            return false;
        }
        
        // Сначала получаем текущее хранилище с seed phrase, чтобы проверить кодовое слово
        // ДО изменения пароля
        json getVaultRequest;
        getVaultRequest["action"] = "getVaultWithSeedPhrase";
        getVaultRequest["username"] = user;
        getVaultRequest["seedPhrase"] = seedPhrase;
        
        json getVaultResponse = sendRequest(getVaultRequest);
        
        if (getVaultResponse["status"] != "success") {
            // Неверная seed phrase или другая ошибка
            return false;
        }
        
        // Проверяем кодовое слово, пытаясь расшифровать текущее хранилище
        string currentVaultSaltHex = getVaultResponse["vaultSalt"];
        string currentEncryptedVaultHex = getVaultResponse["vaultData"];
        
        string decryptedJson;
        if (!validateCodeWordWithVault(codeWord, currentVaultSaltHex, currentEncryptedVaultHex, decryptedJson)) {
            // Неверное кодовое слово - возвращаем ошибку ДО изменения пароля
            return false;
        }
        
        // Кодовое слово верное! Теперь можем восстанавливать пароль
        json request;
        request["action"] = "recoverPassword";
        request["username"] = user;
        request["seedPhrase"] = seedPhrase;
        request["newPassword"] = newPassword;
        
        json response = sendRequest(request);
        
        if (response["status"] == "success") {
            auto seeds = response["newSeedWords"];
            newSeedWords.clear();
            for (size_t i = 0; i < seeds.size(); i++) {
                string word = seeds[i];
                newSeedWords.push_back(word);
            }
            
            // Получаем новую vaultSalt
            string newVaultSaltHex = response["newVaultSalt"];
            
            // Шифруем данные с новым ключом
            // ВАЖНО: Используем то же кодовое слово, но с новой солью
            auto newVaultKey = deriveVaultKey(codeWord, newVaultSaltHex);
            auto reEncryptedVault = encrypt_aes_gcm(decryptedJson, newVaultKey);
            string reEncryptedVaultHex = toHex(reEncryptedVault);
            
            // Отправляем обратно зашифрованные данные на сервер
            json updateRequest;
            updateRequest["action"] = "updateVault";
            updateRequest["username"] = user;
            updateRequest["password"] = newPassword;
            updateRequest["vaultData"] = reEncryptedVaultHex;
            
            json updateResponse = sendRequest(updateRequest);
            
            if (updateResponse["status"] != "success") {
                return false;
            }
            
            return true;
        } else {
            return false;
        }
    } catch (const exception& e) {
        return false;
    }
}

void Client::logout() {
    if (isLoggedIn && vault) {
        // Синхронизируем данные перед выходом
        syncToServer();
    }
    
    isLoggedIn = false;
    username.clear();
    password.clear();
    codeWord.clear();  // Очищаем кодовое слово
    vaultKey.clear();
    
    if (vault) {
        delete vault;
        vault = nullptr;
    }
}

void Client::displayAllEntries() {
    if (!isLoggedIn || !vault) {
        return;
    }
    
    json vaultJson = vault->toJson();
}

bool Client::addEntry(const string& service, const string& login, 
                     const string& password, const string& url, 
                     const string& note) {
    if (!isLoggedIn || !vault) {
        return false;
    }
    
    // Получаем текущее время
    auto now = chrono::system_clock::now();
    auto now_c = chrono::system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    string lastModified = ss.str();
    
    if (vault->insert(service, lastModified, login, password, url, note)) {
        return true;
    } else {
        return false;
    }
}

bool Client::updateEntryFull(const string& service, const string& login,
                             const string& newPassword, const string& newUrl, 
                             const string& newNote) {
    if (!isLoggedIn || !vault) {
        return false;
    }
    
    // Для обновления в UserHashTable нужно удалить и снова добавить
    // Получаем текущее время
    auto now = chrono::system_clock::now();
    auto now_c = chrono::system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    string lastModified = ss.str();
    
    // Удаляем старую запись (если есть функция deleteEntry в UserHashTable)
    // Затем добавляем обновленную
    if (vault->insert(service, lastModified, login, newPassword, newUrl, newNote)) {
        return true;
    } else {
        return false;
    }
}

bool Client::deleteEntry(const string& service, const string& login) {
    if (!isLoggedIn || !vault) {
        return false;
    }
    
    // Используем метод remove из UserHashTable
    if (vault->remove(service, login)) {
        return true;
    } else {
        return false;
    }
}

bool Client::syncToServer() {
    if (!isLoggedIn || !vault) {
        return false;
    }
    
    try {
        string encryptedVault = encryptVault();
        
        json request;
        request["action"] = "updateVault";
        request["username"] = username;
        request["password"] = password;
        request["vaultData"] = encryptedVault;
        
        json response = sendRequest(request);
        
        if (response["status"] == "success") {
            return true;
        } else {
            return false;
        }
    } catch (const exception& e) {
        return false;
    }
}

bool Client::syncFromServer() {
    if (!isLoggedIn) {
        return false;
    }
    
    try {
        json request;
        request["action"] = "getVault";
        request["username"] = username;
        request["password"] = password;
        
        json response = sendRequest(request);
        
        if (response["status"] == "success") {
            // Обновляем vaultKey если нужно
            if (response.contains("vaultSalt")) {
                string vaultSaltHex = response["vaultSalt"];
                vaultKey = deriveVaultKey(codeWord, vaultSaltHex);
            }
            
            string encryptedVaultHex = response["vaultData"];
            decryptAndLoadVault(encryptedVaultHex);
            return true;
        } else {
            return false;
        }
    } catch (const exception& e) {
        return false;
    }
}

json Client::getVaultEntries() const {
    if (!vault) {
        return json::array();
    }
    return vault->toJson();
}
