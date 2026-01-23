#include "mainwindow.h"
#include <QApplication>
#include <iostream>
#include <sodium.h>

int main(int argc, char *argv[])
{
    // Initialize libsodium
    if (sodium_init() < 0) {
        std::cerr << "Ошибка инициализации libsodium" << std::endl;
        return 1;
    }
    
    QApplication app(argc, argv);
    
    // Set application style
    app.setStyle("Fusion");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
