#include "userHashTable.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <sodium.h>
#include <stdexcept>

using namespace std;
using json = nlohmann::json;

constexpr unsigned long base = 2166136261;
constexpr unsigned long prime = 16777619;
const int primes[] = {5, 7, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853};

int UserHashTable::hashFunction(const std::pair<std::string, std::string>& loginAndService) const {
    unsigned long hash = base;
    std::string combined = loginAndService.first + loginAndService.second;

    for (const auto& c : combined) {
        hash ^= static_cast<unsigned char>(c);
        hash *= prime;
    }
    return hash % capacity;
}

UserHashTable::UserHashTable(const int cap) : capacity(cap), size(0) {
    table = new UserHashTableNode[cap];
    for (size_t i = 0; i < cap; i++) {
        table[i] = UserHashTableNode();
    }
}

bool UserHashTable::rehash() {
    vector<tuple<string, string, string, string, string, string>> element;

    for (int i = 0; i < capacity; i++) {
        if (!table[i].isNull && !table[i].isDelete) {
            element.emplace_back(table[i]._service, table[i]._lastModifiedTime,
                table[i]._login, table[i]._password, table[i]._url, table[i]._note);
        }
    }

    const UserHashTableNode* oldTable = table;
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

    table = new UserHashTableNode[newCap];
    capacity = newCap;
    size = 0;

    for (int i = 0; i < newCap; i++) {
        table[i].isNull = true;
        table[i].isDelete = false;
    }

    bool success = true;
    for (auto& el : element) {
        if (!insert(get<0>(el), get<1>(el), get<2>(el), get<3>(el), get<4>(el), get<5>(el))) {
            success = false;
            break;
        }
    }

    delete[] oldTable;
    return success;
}

bool UserHashTable::insert(const std::string& service, const std::string& lastTime,
                           const std::string& login, const std::string& password,
                           const std::string& url, const std::string& note) {
    auto key = make_pair(login, service);
    int h = hashFunction(key);

    if (static_cast<double>(size) / capacity >= 0.75) {
        if (!rehash()) return false;
        h = hashFunction(key);
    }

    // Сначала проверяем, существует ли уже такая запись
    for (size_t i = 0; i < capacity; i++) {
        const size_t index = (h + i) % capacity;
        
        // Если нашли пустую ячейку, значит записи с таким ключом нет
        if (table[index].isNull && !table[index].isDelete) {
            break;
        }
        
        // Если нашли запись с таким же ключом, обновляем её
        if (!table[index].isNull && !table[index].isDelete &&
            table[index]._login == login && table[index]._service == service) {
            table[index]._lastModifiedTime = lastTime;
            table[index]._password = password;
            table[index]._url = url;
            table[index]._note = note;
            return true;  // Обновили существующую запись
        }
    }

    // Если не нашли существующую запись, добавляем новую
    for (size_t i = 0; i < capacity; i++) {
        const size_t index = (h + i) % capacity;
        if (table[index].isDelete || table[index].isNull) {
            table[index]._service = service;
            table[index]._lastModifiedTime = lastTime;
            table[index]._login = login;
            table[index]._password = password;
            table[index]._url = url;
            table[index]._note = note;
            table[index].isNull = table[index].isDelete = false;
            size++;
            return true;
        }
    }
    return false;
}

void UserHashTable::loadFromFile(const std::string& filename) {
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
        string service = doc["_service"];
        string lastTime = doc["_lastModifiedTime"];
        string login = doc["_login"];
        string password = doc["_password"];
        string url = doc["_url"];
        string note = doc["_note"];
        insert(service, lastTime, login, password, url, note);
    }
}

void UserHashTable::saveToFile(const std::string& filename) const {
    json data = json::array();
    for (size_t i = 0; i < capacity; i++) {
        if (!table[i].isNull && !table[i].isDelete) {
            json obj;
            obj["_service"] = table[i]._service;
            obj["_lastModifiedTime"] = table[i]._lastModifiedTime;
            obj["_login"] = table[i]._login;
            obj["_password"] = table[i]._password;
            obj["_url"] = table[i]._url;
            obj["_note"] = table[i]._note;
            data.push_back(obj);
        }
    }

    ofstream file(filename);
    if (file.is_open()) {
        file << data.dump(4);
        file.close();
    } else {
        cerr << "Не удалось открыть файл для записи: " << filename << endl;
    }
}

nlohmann::json UserHashTable::toJson() const {
    json data = json::array();
    for (size_t i = 0; i < capacity; i++) {
        if (!table[i].isNull && !table[i].isDelete) {
            json obj;
            obj["_service"] = table[i]._service;
            obj["_lastModifiedTime"] = table[i]._lastModifiedTime;
            obj["_login"] = table[i]._login;
            obj["_password"] = table[i]._password;
            obj["_url"] = table[i]._url;
            obj["_note"] = table[i]._note;
            data.push_back(obj);
        }
    }
    return data;
}

bool UserHashTable::fromJson(const nlohmann::json& j) {
    if (!j.is_array()) {
        return false;
    }
    
    for (const auto& item : j) {
        string service = item["_service"];
        string lastTime = item["_lastModifiedTime"];
        string login = item["_login"];
        string password = item["_password"];
        string url = item["_url"];
        string note = item["_note"];
        if (!insert(service, lastTime, login, password, url, note)) {
            return false;
        }
    }
    return true;
}

// AES-256-GCM шифрование
std::vector<unsigned char> encrypt_aes_gcm(
    const std::string& plaintext,
    const std::vector<unsigned char>& key) {
    
    if (sodium_init() < 0) {
        throw std::runtime_error("Libsodium initialization failed");
    }

    if (key.size() != crypto_aead_aes256gcm_KEYBYTES) {
        throw std::invalid_argument("Invalid key size for AES-256-GCM");
    }

    // Генерируем nonce
    std::vector<unsigned char> nonce(crypto_aead_aes256gcm_NPUBBYTES);
    randombytes_buf(nonce.data(), nonce.size());

    // Выделяем память для зашифрованного текста
    std::vector<unsigned char> ciphertext(plaintext.size() + crypto_aead_aes256gcm_ABYTES);
    unsigned long long ciphertext_len;

    // Шифруем
    if (crypto_aead_aes256gcm_encrypt(
            ciphertext.data(), &ciphertext_len,
            reinterpret_cast<const unsigned char*>(plaintext.c_str()), plaintext.size(),
            nullptr, 0,
            nullptr,
            nonce.data(),
            key.data()) != 0) {
        throw std::runtime_error("Encryption failed");
    }

    ciphertext.resize(ciphertext_len);

    // Объединяем nonce и ciphertext в один вектор
    std::vector<unsigned char> result;
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

// AES-256-GCM расшифровка
std::string decrypt_aes_gcm(
    const std::vector<unsigned char>& blob,
    const std::vector<unsigned char>& key) {
    
    if (sodium_init() < 0) {
        throw std::runtime_error("Libsodium initialization failed");
    }

    if (key.size() != crypto_aead_aes256gcm_KEYBYTES) {
        throw std::invalid_argument("Invalid key size for AES-256-GCM");
    }

    if (blob.size() < crypto_aead_aes256gcm_NPUBBYTES) {
        throw std::invalid_argument("Blob too small to contain nonce");
    }

    // Извлекаем nonce
    std::vector<unsigned char> nonce(blob.begin(), blob.begin() + crypto_aead_aes256gcm_NPUBBYTES);
    
    // Извлекаем ciphertext
    std::vector<unsigned char> ciphertext(blob.begin() + crypto_aead_aes256gcm_NPUBBYTES, blob.end());

    // Выделяем память для расшифрованного текста
    std::vector<unsigned char> decrypted(ciphertext.size());
    unsigned long long decrypted_len;

    // Расшифровываем
    if (crypto_aead_aes256gcm_decrypt(
            decrypted.data(), &decrypted_len,
            nullptr,
            ciphertext.data(), ciphertext.size(),
            nullptr, 0,
            nonce.data(),
            key.data()) != 0) {
        throw std::runtime_error("Decryption failed");
    }

    decrypted.resize(decrypted_len);

    return std::string(decrypted.begin(), decrypted.end());
}
