#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <map>
#include "json.hpp"

class Server {
private:
    std::string usersFilePath;
    std::string vaultDirectory;
    int port;
    int serverSocket;
    
    // Вспомогательные функции
    std::string getUserVaultPath(const std::string& username);
    bool createUserVault(const std::string& username, const std::vector<unsigned char>& encryptedData);
    std::vector<unsigned char> readUserVault(const std::string& username);
    bool updateUserVault(const std::string& username, const std::vector<unsigned char>& encryptedData);
    
    // Обработчики запросов
    nlohmann::json handleRegister(const nlohmann::json& request);
    nlohmann::json handleLogin(const nlohmann::json& request);
    nlohmann::json handleChangePassword(const nlohmann::json& request);
    nlohmann::json handleRecoverPassword(const nlohmann::json& request);
    nlohmann::json handleGetVault(const nlohmann::json& request);
    nlohmann::json handleGetVaultWithSeedPhrase(const nlohmann::json& request);
    nlohmann::json handleUpdateVault(const nlohmann::json& request);
    
    // Обработка клиентских соединений
    void handleClient(int clientSocket);
    nlohmann::json processRequest(const nlohmann::json& request);
    
public:
    Server(int port = 8080, const std::string& usersFile = "users.json", 
           const std::string& vaultDir = "server_vaults");
    ~Server();
    
    bool initialize();
    void start();
    void stop();
};

#endif // SERVER_H
