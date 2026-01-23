#include "common_utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <sodium.h>
#include <cctype>
#include <stdexcept>

using namespace std;

// Генерация соли
vector<unsigned char> generateSaltRaw(size_t size) {
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (!urandom) {
        throw std::runtime_error("Cannot open /dev/urandom");
    }

    std::vector<unsigned char> salt(size);
    urandom.read(reinterpret_cast<char*>(salt.data()), size);

    if (urandom.gcount() != static_cast<std::streamsize>(size)) {
        throw std::runtime_error("Failed to read enough random bytes");
    }
    return salt;
}

std::vector<unsigned char> generate128bitLibsodium() {
    if (sodium_init() < 0) {
        throw std::runtime_error("Libsodium initialization failed");
    }
    // Генерируем 16 байт (128 бит)
    std::vector<unsigned char> randomData(16);
    randombytes_buf(randomData.data(), randomData.size());

    return randomData;
}

// Алгоритм Argon2id
std::vector<unsigned char> hashPasswordArgon2id(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    size_t hashLength,
    unsigned long long opsLimit,
    size_t memLimit) {

    if (sodium_init() < 0) {
        throw std::runtime_error("Libsodium initialization failed");
    }

    std::vector<unsigned char> hash(hashLength);

    if (crypto_pwhash_argon2id(
            hash.data(), hashLength,
            password.c_str(), password.length(),
            salt.data(),
            opsLimit,
            memLimit,
            crypto_pwhash_argon2id_ALG_ARGON2ID13) != 0) {
        throw std::runtime_error("Argon2 hashing failed - out of memory");
    }

    return hash;
}

std::vector<unsigned char> hashSHA512(const std::string& data) {
    if (sodium_init() < 0) {
        throw std::runtime_error("Libsodium initialization failed");
    }

    std::vector<unsigned char> hash(crypto_hash_sha512_BYTES);

    crypto_hash_sha512(
        hash.data(),
        reinterpret_cast<const unsigned char*>(data.c_str()),
        data.length()
    );

    return hash;
}

// Перевод в HEX
std::string toHex(const std::vector<unsigned char>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

vector<unsigned char> hexToBytes(const std::string& hex) {
    vector<unsigned char> bytes;
    bytes.reserve(hex.length() / 2);

    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::vector<unsigned char> hexStringToVector(const std::string& hexStr) {
    // Проверка на четность длины
    if (hexStr.length() % 2 != 0) {
        throw std::invalid_argument("Hex string must have even length");
    }

    std::vector<unsigned char> result;
    result.reserve(hexStr.length() / 2);

    for (size_t i = 0; i < hexStr.length(); i += 2) {
        std::string byteString = hexStr.substr(i, 2);

        // Преобразуем два символа в число
        unsigned char byte = static_cast<unsigned char>(
            std::stoi(byteString, nullptr, 16));

        result.push_back(byte);
    }

    return result;
}

std::vector<int> split132bitsTo11bitChunks(const std::vector<unsigned char>& data) {
    // Проверяем, что у нас ровно 132 бита (132 / 8 = 16.5 байт)
    size_t totalBits = data.size() * 8;
    if (totalBits < 132) {
        throw std::invalid_argument("Not enough bits in vector");
    }

    // 132 бита / 11 бит = 12 блоков
    const size_t BLOCK_SIZE_BITS = 11;
    const size_t NUM_BLOCKS = 12;

    std::vector<int> result;
    result.reserve(NUM_BLOCKS);

    // Индекс текущего бита в data
    size_t currentBit = 0;

    for (int block = 0; block < NUM_BLOCKS; block++) {
        int value = 0;

        // Собираем 11 бит
        for (int bitInBlock = 0; bitInBlock < BLOCK_SIZE_BITS; bitInBlock++) {
            if (currentBit >= totalBits) {
                throw std::runtime_error("Unexpected end of data");
            }

            // Определяем байт и позицию бита в нем
            size_t byteIndex = currentBit / 8;
            size_t bitIndexInByte = 7 - (currentBit % 8); // Бит 7 - старший

            // Извлекаем бит
            bool bitValue = (data[byteIndex] & (1 << bitIndexInByte)) != 0;

            // Добавляем бит к значению
            value = (value << 1) | (bitValue ? 1 : 0);

            currentBit++;
        }

        result.push_back(value % 2048);
    }

    return result;
}

vector<int> genIndexSeedWord() {
    auto seed = generate128bitLibsodium();
    const auto seedString = toHex(seed);
    const auto saltSeed = hashSHA512(seedString);
    unsigned char checksum = saltSeed[0] >> 4;
    seed.push_back(checksum);
    return split132bitsTo11bitChunks(seed);
}

std::string generate_password(int length, bool use_uppercase,
                              bool use_digits, bool use_special) {
    std::string chars = "abcdefghijklmnopqrstuvwxyz";
    std::string uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string digits = "0123456789";
    std::string special = "!@#$%^&*()_+-=[]{}|;:,.<>?";

    if (use_uppercase) chars += uppercase;
    if (use_digits) chars += digits;
    if (use_special) chars += special;

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);

    std::string password;
    for (int i = 0; i < length; ++i) {
        password += chars[distribution(generator)];
    }

    return password;
}

bool validatePassword(const std::string& password, std::string& errorMessage) {
    // Проверка минимальной длины (не менее 8 символов)
    if (password.length() < 8) {
        errorMessage = "Пароль должен содержать не менее 8 символов";
        return false;
    }

    // Флаги для проверки наличия различных типов символов
    bool hasLower = false;
    bool hasUpper = false;
    bool hasDigit = false;
    bool hasSpecial = false;

    // Специальные символы
    const std::string specialChars = "!@#$%^&*()_+-=[]{}|;:,.<>?/~`'\"\\";

    for (char c : password) {
        if (islower(c)) hasLower = true;
        else if (isupper(c)) hasUpper = true;
        else if (isdigit(c)) hasDigit = true;
        else if (specialChars.find(c) != std::string::npos) hasSpecial = true;
    }

    // Проверка наличия всех типов символов
    if (!hasLower) {
        errorMessage = "Пароль должен содержать хотя бы одну строчную букву";
        return false;
    }
    if (!hasUpper) {
        errorMessage = "Пароль должен содержать хотя бы одну заглавную букву";
        return false;
    }
    if (!hasDigit) {
        errorMessage = "Пароль должен содержать хотя бы одну цифру";
        return false;
    }
    if (!hasSpecial) {
        errorMessage = "Пароль должен содержать хотя бы один специальный символ (!@#$%^&* и т.д.)";
        return false;
    }

    return true;
}

bool validateUsername(const std::string& username, std::string& errorMessage) {
    // Проверка минимальной длины
    if (username.length() < 3) {
        errorMessage = "Логин должен содержать не менее 3 символов";
        return false;
    }

    // Проверка максимальной длины
    if (username.length() > 30) {
        errorMessage = "Логин должен содержать не более 30 символов";
        return false;
    }

    return true;
}

// Оценка сложности пароля (возвращает баллы от 0 до 100)
int evaluatePasswordStrength(const std::string& password) {
    int strength = 0;
    
    // Длина пароля
    if (password.length() >= 8) strength += 20;
    if (password.length() >= 12) strength += 10;
    if (password.length() >= 16) strength += 10;
    
    // Наличие разных типов символов
    bool hasLower = false;
    bool hasUpper = false;
    bool hasDigit = false;
    bool hasSpecial = false;
    
    const std::string specialChars = "!@#$%^&*()_+-=[]{}|;:,.<>?/~`'\"\\";
    
    for (char c : password) {
        if (islower(c)) hasLower = true;
        else if (isupper(c)) hasUpper = true;
        else if (isdigit(c)) hasDigit = true;
        else if (specialChars.find(c) != std::string::npos) hasSpecial = true;
    }
    
    if (hasLower) strength += 10;
    if (hasUpper) strength += 10;
    if (hasDigit) strength += 10;
    if (hasSpecial) strength += 15;
    
    // Бонус за разнообразие
    int diversity = (hasLower ? 1 : 0) + (hasUpper ? 1 : 0) + 
                    (hasDigit ? 1 : 0) + (hasSpecial ? 1 : 0);
    if (diversity >= 3) strength += 10;
    if (diversity == 4) strength += 5;
    
    return std::min(strength, 100);
}

std::string getPasswordStrengthDescription(int strength) {
    if (strength < 30) return "Очень слабый";
    if (strength < 50) return "Слабый";
    if (strength < 70) return "Средний";
    if (strength < 90) return "Сильный";
    return "Очень сильный";
}

// Оценка времени взлома пароля
std::string estimateTimeToCrack(const std::string& password) {
    // Определяем размер алфавита
    bool hasLower = false;
    bool hasUpper = false;
    bool hasDigit = false;
    bool hasSpecial = false;
    
    const std::string specialChars = "!@#$%^&*()_+-=[]{}|;:,.<>?/~`'\"\\";
    
    for (char c : password) {
        if (islower(c)) hasLower = true;
        else if (isupper(c)) hasUpper = true;
        else if (isdigit(c)) hasDigit = true;
        else if (specialChars.find(c) != std::string::npos) hasSpecial = true;
    }
    
    int alphabetSize = 0;
    if (hasLower) alphabetSize += 26;
    if (hasUpper) alphabetSize += 26;
    if (hasDigit) alphabetSize += 10;
    if (hasSpecial) alphabetSize += specialChars.length();
    
    if (alphabetSize == 0) return "Неизвестно";
    
    // Вычисляем энтропию: длина_пароля * log2(размер_алфавита)
    double entropy = password.length() * (log(alphabetSize) / log(2));
    
    // Количество возможных комбинаций: alphabetSize^length
    // Используем энтропию для вычисления: 2^entropy
    
    // Предполагаем скорость перебора: 10 миллиардов попыток в секунду (современный GPU)
    const double attemptsPerSecond = 10e9;
    
    // Среднее время взлома = (2^entropy / 2) / attemptsPerSecond
    // Делим на 2, так как в среднем пароль находится на половине перебора
    double totalCombinations = pow(2, entropy);
    double secondsToHalfCrack = (totalCombinations / 2) / attemptsPerSecond;
    
    // Форматируем время
    if (secondsToHalfCrack < 1) {
        return "Мгновенно";
    } else if (secondsToHalfCrack < 60) {
        return std::to_string(static_cast<int>(secondsToHalfCrack)) + " секунд";
    } else if (secondsToHalfCrack < 3600) {
        int minutes = static_cast<int>(secondsToHalfCrack / 60);
        return std::to_string(minutes) + " минут";
    } else if (secondsToHalfCrack < 86400) {
        int hours = static_cast<int>(secondsToHalfCrack / 3600);
        return std::to_string(hours) + " часов";
    } else if (secondsToHalfCrack < 31536000) {
        int days = static_cast<int>(secondsToHalfCrack / 86400);
        return std::to_string(days) + " дней";
    } else if (secondsToHalfCrack < 31536000.0 * 1000) {
        int years = static_cast<int>(secondsToHalfCrack / 31536000);
        return std::to_string(years) + " лет";
    } else if (secondsToHalfCrack < 31536000.0 * 1000000) {
        long long thousands = static_cast<long long>(secondsToHalfCrack / (31536000.0 * 1000));
        return std::to_string(thousands) + " тысяч лет";
    } else if (secondsToHalfCrack < 31536000.0 * 1e9) {
        long long millions = static_cast<long long>(secondsToHalfCrack / (31536000.0 * 1000000));
        return std::to_string(millions) + " миллионов лет";
    } else {
        return "Более миллиарда лет";
    }
}

// Проверка на слабый/распространенный пароль
bool isWeakPassword(const std::string& password) {
    // Загружаем часто используемые пароли из файла
    std::ifstream file("rockyou_1000k.txt");
    if (!file.is_open()) {
        // Если файл не открылся, выполняем базовую проверку
        
        // Простые проверки на очевидно слабые пароли
        std::vector<std::string> commonPasswords = {
            "password", "123456", "12345678", "qwerty", "abc123",
            "monkey", "1234567", "letmein", "trustno1", "dragon",
            "baseball", "iloveyou", "master", "sunshine", "ashley"
        };
        
        std::string lowerPassword = password;
        std::transform(lowerPassword.begin(), lowerPassword.end(), 
                      lowerPassword.begin(), ::tolower);
        
        for (const auto& weak : commonPasswords) {
            if (lowerPassword == weak) {
                return true;
            }
        }
        return false;
    }
    
    std::string line;
    std::string lowerPassword = password;
    std::transform(lowerPassword.begin(), lowerPassword.end(), 
                  lowerPassword.begin(), ::tolower);
    
    while (std::getline(file, line)) {
        // Приводим к нижнему регистру для сравнения
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), 
                      lowerLine.begin(), ::tolower);
        
        if (lowerPassword == lowerLine) {
            file.close();
            return true;
        }
    }
    
    file.close();
    return false;
}
