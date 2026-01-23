#ifndef COURSEWORK_HASHTABLE_H
#define COURSEWORK_HASHTABLE_H

#include <string>
#include "json.hpp"

constexpr unsigned long base = 2166136261;
constexpr unsigned long prime = 16777619;
const int primes[] = {5, 7, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853};

class HashTableUsers {
    struct HashTableNodeUsers {
        std::string _login;
        std::string _passwordHash;
        std::string _salt;
        std::string _seedPhraseHash;
        std::string _vaultSalt;
        bool isDelete;
        bool isNull;

        HashTableNodeUsers() : isDelete(false), isNull(true) {}
    };

    HashTableNodeUsers* table;
    size_t capacity;
    size_t size;
    [[nodiscard]] int hashFunction(const std::string& str) const;
    bool rehash();
public:
    explicit HashTableUsers (int cap = 101);
    ~HashTableUsers() {delete[] table;}


    bool insert(const std::string& login, const std::string& password,
        const std::string& salt, const std::string& seed, const std::string& vaultSalt);
    bool deleteKey(const std::string& login);

    void loadFromFile(const std::string &filename);
    void saveToFile(const std::string& filename) const;

    [[nodiscard]] std::vector<std::tuple<std::string,
                            std::string,
                            std::string,
                            std::string, std::string>> items() const;

    bool searchLogin(const std::string& login);

    [[nodiscard]] std::pair<std::string, std::string> getHashPassword(const std::string& login) const;

    std::string getSeedPhraseHash(const std::string& login) const;

    std::string getVaultSalt(const std::string& login) const;

    void switchUsersData(const std::string &login, const std::string &newPass,
        const std::string &newSalt, const std::string &newPhrase,
        const std::string& newVaultSalt);

};


#endif