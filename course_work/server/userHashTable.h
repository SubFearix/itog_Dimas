
#ifndef COURSE_WORK_DIMAS_COPILOT_UPDATE_REGISTRATION_FLOW_USERHASHTABLE_H
#define COURSE_WORK_DIMAS_COPILOT_UPDATE_REGISTRATION_FLOW_USERHASHTABLE_H


#include <string>
#include "json.hpp"


class UserHashTable {
private:
    struct UserHashTableNode {
        std::string _service;
        std::string _lastModifiedTime;
        std::string _login;
        std::string _password;
        std::string _url;
        std::string _note;

        bool isDelete;
        bool isNull;

        UserHashTableNode() : isDelete(false), isNull(true) {}
    };

    UserHashTableNode* table;
    size_t capacity;
    size_t size;
    [[nodiscard]] int hashFunction(const std::pair<std::string, std::string>& loginAndService) const;
    bool rehash();
public:
    explicit UserHashTable (int cap = 101);
    ~UserHashTable() {delete[] table;}

    bool insert(const std::string& service, const std::string& lastTime, const std::string& login,
                const std::string& password, const std::string& url, const std::string& note);

    void loadFromFile(const std::string& filename);
    void saveToFile(const std::string& filename) const;

    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& j);
};

std::vector<unsigned char> encrypt_aes_gcm(
    const std::string& plaintext,
    const std::vector<unsigned char>& key);

std::string decrypt_aes_gcm(
    const std::vector<unsigned char>& blob,
    const std::vector<unsigned char>& key);

#endif //COURSE_WORK_DIMAS_COPILOT_UPDATE_REGISTRATION_FLOW_USERHASHTABLE_H