cmake_minimum_required(VERSION 3.10)
project(PasswordManagerServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходные файлы сервера
set(SERVER_SOURCES
    common_utils.cpp
    hashTableUrers.cpp
    userHashTable.cpp
    register.cpp
    log_in.cpp
    server.cpp
    server_main.cpp
)

# Сборка сервера
add_executable(password_server ${SERVER_SOURCES})
target_link_libraries(password_server sodium)

# Копирование необходимых файлов в build директорию
configure_file(${CMAKE_SOURCE_DIR}/english.txt ${CMAKE_BINARY_DIR}/english.txt COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/rockyou_1000k.txt ${CMAKE_BINARY_DIR}/rockyou_1000k.txt COPYONLY)
