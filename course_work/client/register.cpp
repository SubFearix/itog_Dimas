#include "register.h"
#include "hashTableUrers.h"
#include "common_utils.h"

#include <fstream>
#include <iostream>

using namespace std;

//
vector<string> loginExist(const string &login, const string& password, HashTableUsers* table) {
    vector<string> seedWordsVec;
    if (table->searchLogin(login)) return seedWordsVec;

    auto salt = generateSaltRaw(16); //Генерируем соль
    auto hash = hashPasswordArgon2id(password, salt); //Хэшируем пароль
    auto vaultSalt = generateSaltRaw(32);

    //Преобразуем соль и хэшированный пароль в hex
    auto saltStr = toHex(salt);
    auto passStr = toHex(hash);
    auto vaultSaltStr = toHex(vaultSalt);

    //Генерируем слова для восстановления
    vector<int> indexSeedWord = genIndexSeedWord();
    vector<string> allSeedWord;
    ifstream file("english.txt");
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            if (!line.empty()) {  // Skip empty lines
                allSeedWord.push_back(line);
            }
        }
        file.close();
    } else {
        throw std::runtime_error("Failed to open english.txt dictionary file");
    }

    // Verify we have enough words
    if (allSeedWord.size() < 2048) {
        throw std::runtime_error("Dictionary file must contain at least 2048 words");
    }

    string seedWords;
    for (auto word : indexSeedWord) {
        if (word < 0 || word >= static_cast<int>(allSeedWord.size())) {
            throw std::runtime_error("Invalid word index: " + std::to_string(word));
        }
        seedWordsVec.push_back(allSeedWord[word]);
        seedWords += allSeedWord[word] + " ";
    }

    //Хэшируем слова
    if (!seedWords.empty()) seedWords.pop_back();
    auto hashSeedVec = hashSHA512(seedWords);
    auto hashSeedStr = toHex(hashSeedVec);

    table->insert(login, passStr, saltStr, hashSeedStr, vaultSaltStr);

    return seedWordsVec;
}
