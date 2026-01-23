#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QListWidget>
#include "client.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Main menu slots
    void onRegisterClicked();
    void onLoginClicked();
    void onPasswordEvalClicked();
    void onRecoverPasswordClicked();
    
    // Registration slots
    void onRegisterSubmitClicked();
    void onRegisterBackClicked();
    
    // Login slots
    void onLoginSubmitClicked();
    void onLoginBackClicked();
    void onLoginRecoverClicked();
    
    // Password evaluation slots
    void onEvaluateSubmitClicked();
    void onEvaluateBackClicked();
    
    // Recovery slots
    void onRecoverySubmitClicked();
    void onRecoveryBackClicked();
    
    // User menu slots
    void onViewEntriesClicked();
    void onAddEntryClicked();
    void onLogoutClicked();
    
    // Password detail actions
    void onShowPasswordClicked();
    void onCopyPasswordClicked();
    void onEditEntryClicked();
    void onDeleteEntryClicked();
    void onSearchTextChanged(const QString& text);
    
    // Add/Edit entry page actions
    void onAddEntryPageSubmitClicked();
    void onAddEntryPageCancelClicked();
    
    // Password tools
    void onGeneratePasswordClicked();
    void onCheckStrengthClicked();
    
    // Helper for password generation
    std::string generateCustomPassword(int length, bool useUpper, bool useLower, bool useDigits, bool useSpecial);
    
private:
    void setupUI();
    void createMainMenuPage();
    void createRegisterPage();
    void createLoginPage();
    void createPasswordEvalPage();
    void createRecoveryPage();
    void createUserMenuPage();
    void createAddEntryPage();
    
    void showMainMenu();
    void showUserMenu();
    void performLogin(const QString& username, const QString& password);
    bool attemptLogin(const QString& username, const QString& password);
    
    QStackedWidget* stackedWidget;
    Client* client;
    
    // Main menu page
    QWidget* mainMenuPage;
    
    // Register page
    QWidget* registerPage;
    QLineEdit* regUsernameEdit;
    QLineEdit* regPasswordEdit;
    QTextEdit* regResultText;
    
    // Login page
    QWidget* loginPage;
    QLineEdit* loginUsernameEdit;
    QLineEdit* loginPasswordEdit;
    QLabel* loginAttemptsLabel;
    QPushButton* loginRecoverBtn;
    QTextEdit* loginResultText;
    int loginAttempts;
    QString currentLoginUsername;
    
    // Password evaluation page
    QWidget* passwordEvalPage;
    QLineEdit* evalPasswordEdit;
    QTextEdit* evalResultText;
    
    // Recovery page
    QWidget* recoveryPage;
    QLineEdit* recoveryUsernameEdit;
    QTextEdit* recoverySeedEdit;
    QLineEdit* recoveryPasswordEdit;
    QTextEdit* recoveryResultText;
    
    // User menu page
    QWidget* userMenuPage;
    QListWidget* entriesList;
    QWidget* entryDetailWidget;
    QLabel* detailServiceLabel;
    QLabel* detailLoginLabel;
    QLabel* detailPasswordLabel;
    QLabel* detailUrlLabel;
    QLabel* detailNoteLabel;
    QPushButton* showPasswordBtn;
    QPushButton* copyPasswordBtn;
    QPushButton* editEntryBtn;
    QPushButton* deleteEntryBtn;
    QLineEdit* searchBox;
    
    // Add/Edit entry page
    QWidget* addEntryPage;
    QLineEdit* addServiceEdit;
    QLineEdit* addLoginEdit;
    QLineEdit* addPasswordEdit;
    QLineEdit* addUrlEdit;
    QTextEdit* addNoteEdit;
    bool isEditingEntry;
    QString currentEditService;
    QString currentEditLogin;
    
    void updateEntriesList();
    void showEntryDetails(const QString& service);
};

#endif // MAINWINDOW_H
