#ifndef COURSEWORK_COMMON_UTILS_H
#define COURSEWORK_COMMON_UTILS_H

#include <string>
#include <vector>
#include <sodium.h>

// Генерация случайных данных
std::vector<unsigned char> generateSaltRaw(size_t size = 16);
std::vector<unsigned char> generate128bitLibsodium();

// Хеширование
std::vector<unsigned char> hashPasswordArgon2id(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t hashLength = 32,
    unsigned long long opsLimit = crypto_pwhash_OPSLIMIT_SENSITIVE,
    size_t memLimit = crypto_pwhash_MEMLIMIT_SENSITIVE);
std::vector<unsigned char> hashSHA512(const std::string& data);

// Преобразование данных
std::string toHex(const std::vector<unsigned char>& data);
std::vector<unsigned char> hexToBytes(const std::string& hex);
std::vector<unsigned char> hexStringToVector(const std::string& hexStr);

// Генерация мнемонических фраз
std::vector<int> genIndexSeedWord();
std::vector<int> split132bitsTo11bitChunks(const std::vector<unsigned char>& data);

// Генерация паролей
std::string generate_password(int length = 16, bool use_uppercase = true,
                              bool use_digits = true, bool use_special = true);

// Валидация
bool validatePassword(const std::string& password, std::string& errorMessage);
bool validateUsername(const std::string& username, std::string& errorMessage);

// Оценка сложности пароля
int evaluatePasswordStrength(const std::string& password);
std::string getPasswordStrengthDescription(int strength);
std::string estimateTimeToCrack(const std::string& password);

// Проверка на слабый/распространенный пароль
bool isWeakPassword(const std::string& password);

#endif // COURSEWORK_COMMON_UTILS_H
