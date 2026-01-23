QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = password_manager_gui
TEMPLATE = app

# Исходные файлы
SOURCES += \
    gui_main.cpp \
    mainwindow.cpp \
    common_utils.cpp \
    hashTableUrers.cpp \
    userHashTable.cpp \
    register.cpp \
    log_in.cpp \
    client.cpp

# Заголовочные файлы
HEADERS += \
    mainwindow.h \
    common_utils.h \
    hashTableUrers.h \
    userHashTable.h \
    register.h \
    log_in.h \
    client.h \
    json.hpp

# Конфигурация libsodium
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
    LIBS += -lsodium
}

# Дополнительные файлы
DISTFILES += \
    README.md \
    english.txt \
    rockyou_1000k.txt

# Флаги компилятора
QMAKE_CXXFLAGS += -std=c++17

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
