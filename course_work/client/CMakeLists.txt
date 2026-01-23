cmake_minimum_required(VERSION 3.10)
project(PasswordManagerClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск libsodium
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SODIUM libsodium)
    if(SODIUM_FOUND)
        message(STATUS "Found libsodium via pkg-config: ${SODIUM_VERSION}")
    endif()
endif()

# Если pkg-config не нашел libsodium, ищем вручную
if(NOT SODIUM_FOUND)
    message(STATUS "pkg-config не нашел libsodium, ищем вручную...")
    
    find_path(SODIUM_INCLUDE_DIR 
        NAMES sodium.h
        HINTS ${CMAKE_PREFIX_PATH}/include
        PATHS 
            /usr/include 
            /usr/local/include 
            /opt/homebrew/include 
            /opt/local/include
    )
    
    find_library(SODIUM_LIBRARY 
        NAMES sodium libsodium
        HINTS ${CMAKE_PREFIX_PATH}/lib ${CMAKE_PREFIX_PATH}/lib64
        PATHS 
            /usr/lib 
            /usr/lib64
            /usr/local/lib 
            /usr/local/lib64
            /opt/homebrew/lib 
            /opt/local/lib
    )
    
    if(SODIUM_INCLUDE_DIR AND SODIUM_LIBRARY)
        set(SODIUM_FOUND TRUE)
        set(SODIUM_INCLUDE_DIRS ${SODIUM_INCLUDE_DIR})
        set(SODIUM_LIBRARIES ${SODIUM_LIBRARY})
        message(STATUS "Найден libsodium вручную:")
        message(STATUS "  Заголовки: ${SODIUM_INCLUDE_DIR}")
        message(STATUS "  Библиотека: ${SODIUM_LIBRARY}")
    else()
        # Последняя попытка: проверяем стандартные пути напрямую для Arch Linux
        message(STATUS "find_path/find_library не сработали, проверяем стандартные пути напрямую...")
        
        if(EXISTS "/usr/include/sodium.h" AND EXISTS "/usr/lib/libsodium.so")
            set(SODIUM_FOUND TRUE)
            set(SODIUM_INCLUDE_DIRS "/usr/include")
            set(SODIUM_LIBRARIES "/usr/lib/libsodium.so")
            message(STATUS "Найден libsodium через прямую проверку:")
            message(STATUS "  Заголовки: /usr/include")
            message(STATUS "  Библиотека: /usr/lib/libsodium.so")
        elseif(EXISTS "/usr/local/include/sodium.h" AND EXISTS "/usr/local/lib/libsodium.so")
            set(SODIUM_FOUND TRUE)
            set(SODIUM_INCLUDE_DIRS "/usr/local/include")
            set(SODIUM_LIBRARIES "/usr/local/lib/libsodium.so")
            message(STATUS "Найден libsodium через прямую проверку:")
            message(STATUS "  Заголовки: /usr/local/include")
            message(STATUS "  Библиотека: /usr/local/lib/libsodium.so")
        else()
            message(STATUS "Результаты поиска:")
            message(STATUS "  SODIUM_INCLUDE_DIR: ${SODIUM_INCLUDE_DIR}")
            message(STATUS "  SODIUM_LIBRARY: ${SODIUM_LIBRARY}")
            message(FATAL_ERROR "libsodium не найдена!\n"
                    "Пожалуйста, установите libsodium:\n"
                    "  - Debian/Ubuntu: sudo apt-get install libsodium-dev\n"
                    "  - Fedora/RHEL: sudo dnf install libsodium-devel\n"
                    "  - Arch Linux: sudo pacman -S libsodium\n"
                    "  - macOS: brew install libsodium\n"
                    "\n"
                    "Если libsodium уже установлена, проверьте:\n"
                    "  ls -la /usr/include/sodium.h\n"
                    "  ls -la /usr/lib/libsodium.so\n"
                    "\n"
                    "Или постройте из исходников: https://github.com/jedisct1/libsodium")
        endif()
    endif()
endif()

# Исходные файлы клиента
set(CLIENT_SOURCES
    common_utils.cpp
    hashTableUrers.cpp
    userHashTable.cpp
    register.cpp
    log_in.cpp
    client.cpp
    client_main.cpp
)

# Сборка клиента
add_executable(password_client ${CLIENT_SOURCES})
target_include_directories(password_client PRIVATE ${SODIUM_INCLUDE_DIRS})
target_link_libraries(password_client ${SODIUM_LIBRARIES})

# Копирование необходимых файлов в build директорию
configure_file(${CMAKE_SOURCE_DIR}/english.txt ${CMAKE_BINARY_DIR}/english.txt COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/rockyou_1000k.txt ${CMAKE_BINARY_DIR}/rockyou_1000k.txt COPYONLY)

# Попытка найти Qt для GUI версии
find_package(Qt5 COMPONENTS Core Widgets QUIET)
if(Qt5_FOUND)
    message(STATUS "Найден Qt5: ${Qt5_VERSION}")
    message(STATUS "Сборка GUI версии включена")
    
    set(GUI_SOURCES
        common_utils.cpp
        hashTableUrers.cpp
        userHashTable.cpp
        register.cpp
        log_in.cpp
        client.cpp
        gui_main.cpp
        mainwindow.cpp
    )
    
    set(GUI_HEADERS
        mainwindow.h
    )
    
    # Включаем автоматическую обработку MOC
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    
    # Сборка GUI клиента
    add_executable(password_manager_gui ${GUI_SOURCES} ${GUI_HEADERS})
    target_include_directories(password_manager_gui PRIVATE ${SODIUM_INCLUDE_DIRS})
    target_link_libraries(password_manager_gui ${SODIUM_LIBRARIES} Qt5::Core Qt5::Widgets)
    
    # Копирование файлов для GUI версии
    configure_file(${CMAKE_SOURCE_DIR}/english.txt ${CMAKE_BINARY_DIR}/english.txt COPYONLY)
    configure_file(${CMAKE_SOURCE_DIR}/rockyou_1000k.txt ${CMAKE_BINARY_DIR}/rockyou_1000k.txt COPYONLY)
    
    # Install targets для GUI версии
    install(TARGETS password_manager_gui
        RUNTIME DESTINATION bin
    )
    
    install(FILES ${CMAKE_SOURCE_DIR}/english.txt ${CMAKE_SOURCE_DIR}/rockyou_1000k.txt
        DESTINATION share/password-manager
    )
    
    install(FILES ${CMAKE_SOURCE_DIR}/password-manager.desktop
        DESTINATION share/applications
    )
    
    # Создание символических ссылок на файлы данных
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/share/password-manager/english.txt \
        ${CMAKE_INSTALL_PREFIX}/bin/english.txt)")
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \
        ${CMAKE_INSTALL_PREFIX}/share/password-manager/rockyou_1000k.txt \
        ${CMAKE_INSTALL_PREFIX}/bin/rockyou_1000k.txt)")
else()
    message(STATUS "Qt5 не найден. GUI версия не будет собрана.")
    message(STATUS "Для сборки GUI версии установите Qt5:")
    message(STATUS "  - Debian/Ubuntu: sudo apt-get install qtbase5-dev")
    message(STATUS "  - Fedora/RHEL: sudo dnf install qt5-qtbase-devel")
    message(STATUS "  - macOS: brew install qt@5")
endif()

# Install targets для CLI версии
install(TARGETS password_client
    RUNTIME DESTINATION bin
)

# CPack configuration для создания пакетов
set(CPACK_PACKAGE_NAME "PasswordManagerClient")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Secure Password Manager with GUI")
set(CPACK_PACKAGE_CONTACT "SubFearix <200814727+SubFearix@users.noreply.github.com>")
set(CPACK_PACKAGE_VENDOR "SubFearix")

# Настройки для DEB пакета
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsodium23, libqt5widgets5, libqt5core5a")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")

# Настройки для RPM пакета
set(CPACK_RPM_PACKAGE_REQUIRES "libsodium, qt5-qtbase")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")

include(CPack)
