#include "hashTableUrers.h"
#include <vector>
#include <fstream>
#include <iostream>


using namespace std;
using json = nlohmann::json;

int HashTableUsers::hashFunction(const string &str) const {
    unsigned long hash = base;

    for (const auto& c : str) {
        hash ^= static_cast<unsigned char>(c);
        hash *= prime;
    }
    return hash % capacity;
}

HashTableUsers::HashTableUsers(const int cap) : capacity(cap), size(0) {
    table = new HashTableNodeUsers[cap];
    for (size_t i = 0; i < cap; i++) {
        table[i] = HashTableNodeUsers();
    }
}

bool HashTableUsers::rehash() {
    vector<tuple<string, string, string, string, string>> element;

    for (int i = 0; i < capacity; i++) {
        if (!table[i].isNull && !table[i].isDelete) {
            element.emplace_back(table[i]._login, table[i]._passwordHash
                , table[i]._salt, table[i]._seedPhraseHash, table[i]._vaultSalt);
        }
    }

    const HashTableNodeUsers* oldTable = table;
    const int oldCapacity = capacity;

    int newCap = 0;
    for (const int prime1 : primes) {
        if (prime1 > capacity) {
            newCap = prime1;
            break;
        }
    }

    if (newCap == 0) {
        newCap = oldCapacity * 2;
    }

    table = new HashTableNodeUsers[newCap];
    capacity = newCap;
    size = 0;

    for (int i = 0; i < newCap; i++) {
        table[i].isNull = true;
        table[i].isDelete = false;
    }

    bool success = true;
    for (auto& el : element) {
        if (!insert(get<0>(el), get<1>(el), get<2>(el), get<3>(el), get<4>(el))) {
            success = false;
            break;
        }
    }

    delete[] oldTable;
    return success;
}

bool HashTableUsers::insert(const std::string &login, const std::string &password
                                , const std::string &salt, const std::string &seed, const string& vaultSalt) {
    int h = hashFunction(login);

    if (static_cast<double>(size) / capacity >= 0.75) {
        if (!rehash()) return false;
        h = hashFunction(login);
    }

    for (size_t i = 0; i < capacity; i++) {
        const size_t index = (h + i) % capacity;
        if (table[index].isDelete || table[index].isNull) {
            table[index]._login = login;
            table[index]._passwordHash = password;
            table[index]._salt = salt;
            table[index]._seedPhraseHash = seed;
            table[index]._vaultSalt = vaultSalt;
            table[index].isNull = table[index].isDelete = false;
            size++;
            return true;
        }
    }
    return false;
}

bool HashTableUsers::deleteKey(const std::string &login) {
    const int h = hashFunction(login);
    for (size_t i = 0; i < capacity; i++) {
        const int index = (h + i) % capacity;
        if (!table[index].isDelete && !table[index].isNull && table[index]._login == login) {
            table[index].isDelete = true;
            size--;
            return true;
        }
    }
    return false;
}

void HashTableUsers::loadFromFile(const std::string &filename) {
    json docs = json::array();
    ifstream file(filename);
    if (file.is_open()) {
        try {
            file >> docs;
        } catch (...) {
            cerr << "Файл повреждён — начинаем с нуля." << endl;
        }
        file.close();
    }
    for (const auto& doc : docs) {
        string login = doc["_login"];
        string password = doc["_passwordHash"];
        string salt = doc["_salt"];
        string seed = doc["_seedPhraseHash"];
        string vaultSalt = doc["_vaultSalt"];
        insert(login, password, salt, seed, vaultSalt);
    }
}

std::vector<std::tuple<std::string, std::string, std::string, std::string, std::string> > HashTableUsers::items() const {
    vector<std::tuple<std::string, std::string, std::string, std::string, std::string>> result;
    for (size_t i = 0; i < capacity; i++) {
        if (!table[i].isNull && !table[i].isDelete) { // ДОБАВИТЬ ПРОВЕРКУ
            result.push_back(make_tuple(
                table[i]._login,
                table[i]._passwordHash,
                table[i]._salt,
                table[i]._seedPhraseHash,
                table[i]._vaultSalt
            ));
        }
    }
    return result;
}


void HashTableUsers::saveToFile(const std::string &filename) const{
    json data = json::array();
    auto allItems = items();
    for (const auto& item : allItems) {
        json obj;
        obj["_login"] = std::get<0>(item);
        obj["_passwordHash"] = std::get<1>(item);
        obj["_salt"] = std::get<2>(item);
        obj["_seedPhraseHash"] = std::get<3>(item);
        obj["_vaultSalt"] = std::get<4>(item);
        data.push_back(obj);
    }

    ofstream file(filename);
    if (file.is_open()) {
        file << data.dump(4);
        file.close();
    } else {
        cerr << "Не удалось открыть файл для записи: " << filename << endl;
    }
}

bool HashTableUsers::searchLogin(const std::string &login) {
    const int h = hashFunction(login);
    for (size_t i = 0; i < capacity ;i++) {
        const int index = (h + i) % capacity;
        if (table[index].isDelete || table[index].isNull) continue;
        if (!table[index].isDelete && !table[index].isNull && table[index]._login == login) {
            return true;
        }
    }
    return false;
}

pair<string, string> HashTableUsers::getHashPassword(const std::string &login) const {
    const int h = hashFunction(login);
    for (size_t i = 0; i < capacity ;i++) {
        const int index = (h + i) % capacity;
        if (table[index].isDelete || table[index].isNull) continue;
        if (!table[index].isDelete && !table[index].isNull && table[index]._login == login) {
            return make_pair(table[index]._passwordHash, table[index]._salt);
        }
    }
    return make_pair("", "");
}

std::string HashTableUsers::getSeedPhraseHash(const std::string &login) const {
    const int h = hashFunction(login);
    for (size_t i = 0; i < capacity ;i++) {
        const int index = (h + i) % capacity;
        if (table[index].isDelete || table[index].isNull) continue;
        if (!table[index].isDelete && !table[index].isNull && table[index]._login == login) {
            return table[index]._seedPhraseHash;
        }
    }
    return "";
}

std::string HashTableUsers::getVaultSalt(const std::string &login) const {
    const int h = hashFunction(login);
    for (size_t i = 0; i < capacity ;i++) {
        const int index = (h + i) % capacity;
        if (table[index].isDelete || table[index].isNull) continue;
        if (!table[index].isDelete && !table[index].isNull && table[index]._login == login) {
            return table[index]._vaultSalt;
        }
    }
    return "";
}

void HashTableUsers::switchUsersData(const string& login, const string& newPass
                                    , const string& newSalt, const string& newPhrase
                                    , const string& newVaultSalt) {
    const int h = hashFunction(login);
    for (size_t i = 0; i < capacity ;i++) {
        const int index = (h + i) % capacity;
        if (table[index].isDelete || table[index].isNull) continue;
        if (!table[index].isDelete && !table[index].isNull && table[index]._login == login) {
            table[index]._passwordHash = newPass;
            table[index]._salt = newSalt;
            table[index]._seedPhraseHash = newPhrase;
            table[index]._vaultSalt = newVaultSalt;
            return;
        }
    }
}


