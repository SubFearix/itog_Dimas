#include "register.h"
#include "hashTableUrers.h"
#include "log_in.h"
#include "common_utils.h"

#include <fstream>
#include <iostream>

using namespace std;

bool existUser(const string& login, HashTableUsers& users) {
    return users.searchLogin(login);
}

bool checkPasswordUser(const string& login, const string& password, const HashTableUsers& users) {
    auto usersPassAndSalt = users.getHashPassword(login);
    string _pass = usersPassAndSalt.first;
    string _salt = usersPassAndSalt.second;
    auto salt = hexToBytes(_salt);
    auto verificationPass = toHex(hashPasswordArgon2id(password, salt));
    return _pass == verificationPass;
}

bool checkPhrase(const string& login, HashTableUsers& users, const string& words) {
    auto _Phrase = users.getSeedPhraseHash(login);
    auto hashWords = toHex(hashSHA512(words));
    cout << "Слова юзера: " << words << endl;
    cout << "Хэш в таблице: " << _Phrase << endl;
    cout << "Полученный хэш: " << hashWords << endl;
    return _Phrase == hashWords;
}

vector<string> switchDataUsers(const string& login, const string& newPass,  HashTableUsers& users) {
    vector<string> seedWordsVec;

    auto salt = generateSaltRaw(16); //Генерируем соль
    auto hash = hashPasswordArgon2id(newPass, salt); //Хэшируем пароль
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

    users.switchUsersData(login, passStr, saltStr, hashSeedStr, vaultSaltStr);

    return seedWordsVec;
}
