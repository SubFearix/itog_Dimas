TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

TARGET = password_client

# Исходные файлы
SOURCES += \
    common_utils.cpp \
    hashTableUrers.cpp \
    userHashTable.cpp \
    register.cpp \
    log_in.cpp \
    client.cpp \
    client_main.cpp

# Заголовочные файлы
HEADERS += \
    common_utils.h \
    hashTableUrers.h \
    userHashTable.h \
    register.h \
    log_in.h \
    client.h \
    json.hpp

# Конфигурация libsodium
# Попытка использовать pkg-config для автоматического определения путей
CONFIG += link_pkgconfig
PKGCONFIG += libsodium

# Если pkg-config недоступен или не работает, используем стандартные пути
!packagesExist(libsodium) {
    message("pkg-config для libsodium не найден, используем стандартные пути")
    
    # Пути для поиска заголовочных файлов
    INCLUDEPATH += /usr/local/include \
                   /opt/homebrew/include \
                   /opt/local/include
    
    # Пути для поиска библиотек
    LIBS += -L/usr/lib \
            -L/usr/local/lib \
            -L/opt/homebrew/lib \
            -L/opt/local/lib \
            -lsodium
} else {
    # pkg-config найден, но явно добавляем -lsodium для надежности
    # (некоторые конфигурации pkg-config могут не включать флаг линковки)
    LIBS += -lsodium
}

# Дополнительные файлы для отображения в проекте
DISTFILES += \
    CMakeLists.txt \
    README.md \
    english.txt \
    rockyou_1000k.txt

# Флаги компилятора
QMAKE_CXXFLAGS += -std=c++17
