#include "client.h"
#include "common_utils.h"
#include <iostream>
#include <limits>

using namespace std;

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void showMainMenu() {
    cout << "\n=== Менеджер паролей ===" << endl;
    cout << "1. Регистрация" << endl;
    cout << "2. Вход" << endl;
    cout << "3. Восстановить пароль (забыл пароль)" << endl;
    cout << "4. Оценить сложность пароля" << endl;
    cout << "0. Выход" << endl;
    cout << "Выберите действие: ";
}

void showUserMenu() {
    cout << "\n=== Меню пользователя ===" << endl;
    cout << "1. Просмотреть все записи" << endl;
    cout << "2. Добавить новую запись" << endl;
    cout << "3. Обновить данные записи" << endl;
    cout << "4. Синхронизировать с сервером" << endl;
    cout << "5. Загрузить данные с сервера" << endl;
    cout << "6. Сменить пароль" << endl;
    cout << "7. Генерировать пароль" << endl;
    cout << "8. Оценить сложность пароля" << endl;
    cout << "0. Выход" << endl;
    cout << "Выберите действие: ";
}

void evaluatePasswordUI() {
    string password;
    cout << "\nВведите пароль для оценки: ";
    clearInput();
    getline(cin, password);
    
    int strength = evaluatePasswordStrength(password);
    string description = getPasswordStrengthDescription(strength);
    string timeToCrack = estimateTimeToCrack(password);
    
    cout << "\n=== Оценка сложности пароля ===" << endl;
    cout << "Пароль: " << password << endl;
    cout << "Оценка: " << strength << "/100" << endl;
    cout << "Уровень: " << description << endl;
    cout << "Время взлома (брутфорс): ~" << timeToCrack << endl;
    
    if (isWeakPassword(password)) {
        cout << "\n⚠️  ВНИМАНИЕ: Этот пароль найден в списке распространенных паролей!" << endl;
        cout << "Настоятельно рекомендуется использовать другой пароль." << endl;
    }
}

// Функция для безопасного чтения ответа y/n
bool getYesNoChoice() {
    string choice;
    getline(cin, choice);
    return !choice.empty() && (choice[0] == 'y' || choice[0] == 'Y');
}


// Функция для безопасного ввода нового пароля с валидацией
string getValidatedNewPassword() {
    string newPassword;
    bool validPassword = false;
    
    while (!validPassword) {
        cout << "Введите новый пароль: ";
        getline(cin, newPassword);
        
        // Проверка на слабый пароль
        if (isWeakPassword(newPassword)) {
            cout << "\n⚠️  ОШИБКА: Пароль слишком распространенный!" << endl;
            cout << "Пожалуйста, выберите более безопасный пароль.\n" << endl;
            continue;
        }
        
        // Проверка валидности пароля
        string errorMessage;
        if (!validatePassword(newPassword, errorMessage)) {
            cout << "\n⚠️  ОШИБКА: " << errorMessage << endl;
            cout << "Попробуйте снова.\n" << endl;
            continue;
        }
        
        validPassword = true;
    }
    
    return newPassword;
}

// Функция для процедуры восстановления пароля
bool performPasswordRecovery(Client& client, const string& username) {
    string seedPhrase;
    
    cout << "Введите 12 слов восстановления (через пробел):" << endl;
    clearInput();
    getline(cin, seedPhrase);
    
    string newPassword = getValidatedNewPassword();
    
    if (client.recoverPassword(username, seedPhrase, newPassword)) {
        cout << "Теперь вы можете войти с новым паролем." << endl;
        return true;
    }
    
    cout << "Не удалось восстановить пароль. Проверьте правильность кодовых слов." << endl;
    return false;
}

// Функция для выполнения входа с ограничением на 3 попытки
bool performLoginWithAttempts(Client& client, const string& username) {
    const int maxAttempts = 3;
    int attempts = 0;
    string password;
    
    // Очищаем буфер перед началом
    clearInput();
    
    while (attempts < maxAttempts) {
        cout << "Введите пароль (попытка " << (attempts + 1) << " из " << maxAttempts << "): ";
        getline(cin, password);
        
        if (client.login(username, password)) {
            client.syncFromServer();
            return true;
        }
        
        attempts++;
        if (attempts < maxAttempts) {
            cout << "Неверный пароль. Осталось попыток: " << (maxAttempts - attempts) << endl;
        }
    }
    
    // Если все попытки исчерпаны, предлагаем восстановление пароля
    cout << "\n⚠️  Исчерпаны все попытки входа." << endl;
    cout << "Хотите восстановить пароль с помощью кодовых фраз? (y/n): ";
    
    if (getYesNoChoice()) {
        return performPasswordRecovery(client, username);
    }
    
    return false;
}


int main(int argc, char* argv[]) {
    string serverHost = "127.0.0.1";
    int serverPort = 8080;
    
    if (argc > 1) {
        serverHost = argv[1];
    }
    if (argc > 2) {
        serverPort = atoi(argv[2]);
    }
    
    cout << "Подключение к серверу " << serverHost << ":" << serverPort << endl;
    
    Client client(serverHost, serverPort);
    
    bool running = true;
    while (running) {
        if (!client.isAuthenticated()) {
            showMainMenu();
            int choice;
            cin >> choice;
            
            switch (choice) {
                case 1: {
                    // Регистрация с проверкой существования пользователя и слабых паролей
                    string username, password;
                    
                    cout << "Введите логин: ";
                    cin >> username;
                    
                    // Проверяем, существует ли пользователь
                    if (client.checkUserExists(username)) {
                        cout << "\n⚠️  Пользователь с таким логином уже существует!" << endl;
                        cout << "Хотите войти в существующий аккаунт? (y/n): ";
                        clearInput();
                        
                        if (getYesNoChoice()) {
                            performLoginWithAttempts(client, username);
                        }
                        break;
                    }
                    
                    // Если пользователь не существует, продолжаем регистрацию
                    bool validPassword = false;
                    clearInput();
                    while (!validPassword) {
                        cout << "Введите пароль: ";
                        getline(cin, password);
                        
                        // Проверка на слабый пароль
                        if (isWeakPassword(password)) {
                            cout << "\n⚠️  ОШИБКА: Пароль слишком распространенный!" << endl;
                            cout << "Пожалуйста, выберите более безопасный пароль." << endl;
                            cout << "Не используйте имена, логины или простые комбинации.\n" << endl;
                            continue;
                        }
                        
                        // Проверка валидности пароля
                        string errorMessage;
                        if (!validatePassword(password, errorMessage)) {
                            cout << "\n⚠️  ОШИБКА: " << errorMessage << endl;
                            cout << "Попробуйте снова.\n" << endl;
                            continue;
                        }
                        
                        validPassword = true;
                    }
                    
                    client.registerUser(username, password);
                    break;
                }
                case 2: {
                    // Вход с 3 попытками
                    string username;
                    cout << "Введите логин: ";
                    cin >> username;
                    
                    performLoginWithAttempts(client, username);
                    break;
                }
                case 3: {
                    // Восстановление пароля
                    string username;
                    
                    cout << "Введите логин: ";
                    clearInput();
                    getline(cin, username);
                    
                    performPasswordRecovery(client, username);
                    break;
                }
                case 4: {
                    // Оценить сложность пароля
                    evaluatePasswordUI();
                    break;
                }
                case 0: {
                    running = false;
                    break;
                }
                default:
                    cout << "Неверный выбор" << endl;
                    break;
            }
        } else {
            showUserMenu();
            int choice;
            cin >> choice;
            
            switch (choice) {
                case 1: {
                    // Просмотр записей
                    client.displayAllEntries();
                    break;
                }
                case 2: {
                    // Добавление записи
                    string service, login, password, url, note;
                    
                    cout << "Сервис/приложение: ";
                    clearInput();
                    getline(cin, service);
                    
                    cout << "Логин: ";
                    getline(cin, login);
                    
                    cout << "Пароль: ";
                    getline(cin, password);
                    
                    cout << "URL (опционально): ";
                    getline(cin, url);
                    
                    cout << "Заметка (опционально): ";
                    getline(cin, note);
                    
                    client.addEntry(service, login, password, url, note);
                    break;
                }
                case 3: {
                    // Обновление данных записи
                    string service, login, newPassword, newUrl, newNote;
                    
                    cout << "Сервис/приложение для обновления: ";
                    clearInput();
                    getline(cin, service);
                    
                    cout << "Логин записи: ";
                    getline(cin, login);
                    
                    cout << "Новый пароль: ";
                    getline(cin, newPassword);
                    
                    cout << "Новый URL (опционально): ";
                    getline(cin, newUrl);
                    
                    cout << "Новая заметка (опционально): ";
                    getline(cin, newNote);
                    
                    client.updateEntryFull(service, login, newPassword, newUrl, newNote);
                    break;
                }
                case 4: {
                    // Синхронизация с сервером
                    client.syncToServer();
                    break;
                }
                case 5: {
                    // Загрузка с сервера
                    client.syncFromServer();
                    break;
                }
                case 6: {
                    // Смена пароля
                    string seedPhrase;
                    
                    cout << "Введите 12 слов восстановления (через пробел):" << endl;
                    clearInput();
                    getline(cin, seedPhrase);
                    
                    string newPassword = getValidatedNewPassword();
                    client.changePassword(seedPhrase, newPassword);
                    break;
                }
                case 7: {
                    // Генерация пароля
                    int length;
                    char useUpper, useDigits, useSpecial;
                    
                    cout << "Длина пароля: ";
                    cin >> length;
                    
                    cout << "Использовать заглавные буквы? (y/n): ";
                    cin >> useUpper;
                    
                    cout << "Использовать цифры? (y/n): ";
                    cin >> useDigits;
                    
                    cout << "Использовать специальные символы? (y/n): ";
                    cin >> useSpecial;
                    
                    string password = generate_password(
                        length,
                        useUpper == 'y' || useUpper == 'Y',
                        useDigits == 'y' || useDigits == 'Y',
                        useSpecial == 'y' || useSpecial == 'Y'
                    );
                    
                    cout << "\nСгенерированный пароль: " << password << endl;
                    
                    // Показать оценку сложности
                    int strength = evaluatePasswordStrength(password);
                    string description = getPasswordStrengthDescription(strength);
                    string timeToCrack = estimateTimeToCrack(password);
                    cout << "Сложность: " << strength << "/100 (" << description << ")" << endl;
                    cout << "Время взлома: ~" << timeToCrack << endl;
                    break;
                }
                case 8: {
                    // Оценить сложность пароля
                    evaluatePasswordUI();
                    break;
                }
                case 0: {
                    client.logout();
                    break;
                }
                default:
                    cout << "Неверный выбор" << endl;
                    break;
            }
        }
    }
    
    cout << "Программа завершена" << endl;
    return 0;
}
