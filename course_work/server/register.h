#ifndef COURSEWORK_REGISTER_H
#define COURSEWORK_REGISTER_H

#include <string>
#include <vector>

#include "hashTableUrers.h"
#include "common_utils.h"

std::vector<std::string> loginExist(const std::string &login, const std::string& password, HashTableUsers* table);

#endif