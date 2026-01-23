#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include "json.hpp"
#include "userHashTable.h"

class Client {
private:
    std::string serverHost;
    int serverPort;
    std::string username;
    std::string password;
    std::string codeWord;  // Кодовое слово для шифрования хранилища (не сохраняется)
    bool isLoggedIn;
    
    UserHashTable* vault;
    std::vector<unsigned char> vaultKey;
    
    // Сетевые функции
    nlohmann::json sendRequest(const nlohmann::json& request);
    
    // Криптография
    std::vector<unsigned char> deriveVaultKey(const std::string& codeWord, const std::string& vaultSaltHex);
    void decryptAndLoadVault(const std::string& encryptedVaultHex);
    std::string encryptVault();
    
    // Вспомогательная функция для валидации кодового слова путем попытки расшифровки хранилища
    bool validateCodeWordWithVault(const std::string& codeWord, const std::string& vaultSaltHex, const std::string& encryptedVaultHex, std::string& decryptedJson);
    
public:
    Client(const std::string& host = "127.0.0.1", int port = 8080);
    ~Client();
    
    // Основные операции
    bool checkUserExists(const std::string& username);
    bool validateCodeWord(const std::string& codeWord, std::string& errorMessage);
    
    // Legacy registration methods (for backward compatibility with old clients)
    // New clients should use registerUser with codeWord parameter
    bool registerUser(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password, std::vector<std::string>& seedWords);
    
    // New registration with code word encryption
    bool registerUser(const std::string& username, const std::string& password, const std::string& codeWord, std::vector<std::string>& seedWords);
    
    // Login with code word for vault decryption
    bool login(const std::string& username, const std::string& password, const std::string& codeWord);
    
    // Change password (logged in users)
    // Note: Code word CANNOT be changed - it remains the same
    bool changePassword(const std::string& seedPhrase, const std::string& newPassword, const std::string& codeWord);
    
    // Legacy password recovery methods (for backward compatibility)
    bool recoverPassword(const std::string& username, const std::string& seedPhrase, const std::string& newPassword);
    bool recoverPassword(const std::string& username, const std::string& seedPhrase, const std::string& newPassword, std::vector<std::string>& newSeedWords);
    
    // Password recovery with code word for vault re-encryption
    // Note: Code word CANNOT be changed - same code word is used with new salt
    bool recoverPassword(const std::string& username, const std::string& seedPhrase, const std::string& newPassword, const std::string& codeWord, std::vector<std::string>& newSeedWords);
    void logout();
    
    // Работа с данными
    void displayAllEntries();
    bool addEntry(const std::string& service, const std::string& login, 
                  const std::string& password, const std::string& url = "", 
                  const std::string& note = "");
    bool updateEntry(const std::string& service, const std::string& login,
                     const std::string& newPassword);
    bool updateEntryFull(const std::string& service, const std::string& login,
                         const std::string& newPassword, const std::string& newUrl, 
                         const std::string& newNote);
    bool deleteEntry(const std::string& service, const std::string& login);
    
    // Синхронизация с сервером
    bool syncToServer();
    bool syncFromServer();
    
    // Утилиты
    bool isAuthenticated() const { return isLoggedIn; }
    std::string getUsername() const { return username; }
    nlohmann::json getVaultEntries() const;
};

#endif // CLIENT_H
