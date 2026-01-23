#ifndef COURSEWORK_LOG_IN_H
#define COURSEWORK_LOG_IN_H

#include "hashTableUrers.h"
#include <string>
#include <vector>

bool existUser(const std::string& login, HashTableUsers& users);
bool checkPasswordUser(const std::string& login, const std::string& password, const HashTableUsers& users);
bool checkPhrase(const std::string& login, HashTableUsers& users, const std::string& words);
std::vector<std::string> switchDataUsers(const std::string& login, const std::string& newPass, HashTableUsers& users);


#endif //COURSEWORK_LOG_IN_H