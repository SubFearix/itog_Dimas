#include "mainwindow.h"
#include "common_utils.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QTimer>
#include <QListWidget>
#include <QSplitter>
#include <QInputDialog>
#include <QClipboard>
#include <QDate>
#include <QCheckBox>
#include <QSlider>
#include <QDialog>
#include <QProgressDialog>
#include <sodium.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), loginAttempts(0), isEditingEntry(false)
{
    client = new Client("127.0.0.1", 8080);
    setupUI();
    showMainMenu();
}

MainWindow::~MainWindow()
{
    delete client;
}

void MainWindow::setupUI()
{
    setWindowTitle("Менеджер паролей");
    setMinimumSize(900, 650);
    resize(1000, 700);
    
    // Apple-inspired stylesheet (light, clean, minimal)
    QString appStyle = R"(
        QMainWindow {
            background-color: #f5f5f7;
        }
        QWidget#mainMenuPage {
            background-color: white;
        }
        QWidget#registerPage, QWidget#loginPage, 
        QWidget#passwordEvalPage, QWidget#recoveryPage {
            background-color: #f5f5f7;
        }
        QWidget#userMenuPage {
            background-color: #ffffff;
        }
        QMessageBox {
            background-color: white;
        }
        QMessageBox QLabel {
            color: #1d1d1f;
            font-size: 14px;
        }
        QMessageBox QPushButton {
            background-color: #007aff;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 6px;
            min-width: 80px;
            font-weight: 600;
            font-size: 13px;
        }
        QMessageBox QPushButton:hover {
            background-color: #0051d5;
        }
        QMessageBox QPushButton:pressed {
            background-color: #003d99;
        }
        QLabel {
            color: #1d1d1f;
            font-size: 13px;
            background-color: transparent;
        }
        QPushButton {
            background-color: #007aff;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 10px 20px;
            font-size: 13px;
            font-weight: 500;
            min-width: 100px;
        }
        QPushButton:hover {
            background-color: #0051d5;
        }
        QPushButton:pressed {
            background-color: #004bb8;
        }
        QPushButton:disabled {
            background-color: #d1d1d6;
        }
        QLineEdit, QTextEdit {
            border: 1px solid #d1d1d6;
            border-radius: 6px;
            padding: 8px 12px;
            background-color: white;
            font-size: 13px;
            color: #1d1d1f;
        }
        QLineEdit:focus, QTextEdit:focus {
            border: 2px solid #007aff;
        }
        QTextEdit[readOnly="true"] {
            background-color: #f5f5f7;
        }
        QListWidget {
            background-color: #f5f5f7;
            border: none;
            border-radius: 8px;
            padding: 4px;
        }
        QListWidget::item {
            background-color: white;
            border-radius: 6px;
            padding: 12px;
            margin: 2px;
            color: #1d1d1f;
        }
        QListWidget::item:selected {
            background-color: #007aff;
            color: white;
        }
        QListWidget::item:hover {
            background-color: #e8e8ed;
        }
    )";
    setStyleSheet(appStyle);
    
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);
    
    createMainMenuPage();
    createRegisterPage();
    createLoginPage();
    createPasswordEvalPage();
    createRecoveryPage();
    createUserMenuPage();
    createAddEntryPage();
}

void MainWindow::createMainMenuPage()
{
    mainMenuPage = new QWidget();
    mainMenuPage->setObjectName("mainMenuPage");
    
    QVBoxLayout* layout = new QVBoxLayout(mainMenuPage);
    layout->setSpacing(15);
    layout->setContentsMargins(60, 40, 60, 40);
    
    // Icon and title
    QLabel* titleLabel = new QLabel("Менеджер паролей");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(32);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #007aff; margin-bottom: 10px;");
    
    QLabel* subtitleLabel = new QLabel("Безопасное хранение ваших паролей");
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPointSize(12);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("color: #7f8c8d; margin-bottom: 20px;");
    
    QPushButton* registerBtn = new QPushButton("Регистрация");
    QPushButton* loginBtn = new QPushButton("Вход");
    QPushButton* recoverBtn = new QPushButton("Восстановить пароль");
    QPushButton* evalBtn = new QPushButton("Оценить сложность пароля");
    QPushButton* exitBtn = new QPushButton("Выход");
    
    // Style specific buttons
    exitBtn->setStyleSheet(
        "QPushButton { background-color: #e74c3c; }"
        "QPushButton:hover { background-color: #c0392b; }"
        "QPushButton:pressed { background-color: #a93226; }"
    );
    
    connect(registerBtn, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
    connect(loginBtn, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(recoverBtn, &QPushButton::clicked, this, &MainWindow::onRecoverPasswordClicked);
    connect(evalBtn, &QPushButton::clicked, this, &MainWindow::onPasswordEvalClicked);
    connect(exitBtn, &QPushButton::clicked, this, &QApplication::quit);
    
    layout->addStretch();
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addSpacing(20);
    layout->addWidget(registerBtn);
    layout->addWidget(loginBtn);
    layout->addWidget(recoverBtn);
    layout->addWidget(evalBtn);
    layout->addSpacing(10);
    layout->addWidget(exitBtn);
    layout->addStretch();
    
    stackedWidget->addWidget(mainMenuPage);
}

void MainWindow::createRegisterPage()
{
    registerPage = new QWidget();
    registerPage->setObjectName("registerPage");
    QVBoxLayout* layout = new QVBoxLayout(registerPage);
    layout->setContentsMargins(50, 30, 50, 30);
    
    QLabel* titleLabel = new QLabel("Регистрация");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    QLabel* usernameLabel = new QLabel("Логин:");
    regUsernameEdit = new QLineEdit();
    regUsernameEdit->setMaxLength(50); // Limit username to 50 characters
    
    QLabel* passwordLabel = new QLabel("Пароль:");
    regPasswordEdit = new QLineEdit();
    regPasswordEdit->setEchoMode(QLineEdit::Password);
    regPasswordEdit->setMaxLength(128); // Limit password to 128 characters
    
    // Add show/hide password button for registration
    QPushButton* regShowPasswordBtn = new QPushButton("👁");
    regShowPasswordBtn->setMaximumWidth(40);
    regShowPasswordBtn->setCheckable(true);
    regShowPasswordBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #f5f5f7; "
        "color: #1d1d1f; "
        "border: 1px solid #d1d1d6; "
        "border-radius: 4px; "
        "padding: 6px; "
        "min-width: 0; "
        "}"
        "QPushButton:hover { background-color: #e8e8ed; }"
        "QPushButton:checked { background-color: #007aff; color: white; }"
    );
    connect(regShowPasswordBtn, &QPushButton::toggled, [this](bool checked) {
        regPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
    
    QHBoxLayout* regPasswordLayout = new QHBoxLayout();
    regPasswordLayout->addWidget(regPasswordEdit);
    regPasswordLayout->addWidget(regShowPasswordBtn);
    
    regResultText = new QTextEdit();
    regResultText->setReadOnly(true);
    regResultText->setMaximumHeight(150);
    
    QPushButton* submitBtn = new QPushButton("Зарегистрироваться");
    QPushButton* backBtn = new QPushButton("Назад");
    
    connect(submitBtn, &QPushButton::clicked, this, &MainWindow::onRegisterSubmitClicked);
    connect(backBtn, &QPushButton::clicked, this, &MainWindow::onRegisterBackClicked);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(backBtn);
    btnLayout->addWidget(submitBtn);
    
    layout->addWidget(titleLabel);
    layout->addSpacing(20);
    layout->addWidget(usernameLabel);
    layout->addWidget(regUsernameEdit);
    layout->addWidget(passwordLabel);
    layout->addLayout(regPasswordLayout);
    layout->addSpacing(10);
    layout->addWidget(regResultText);
    layout->addSpacing(10);
    layout->addLayout(btnLayout);
    layout->addStretch();
    
    stackedWidget->addWidget(registerPage);
}

void MainWindow::createLoginPage()
{
    loginPage = new QWidget();
    loginPage->setObjectName("loginPage");
    QVBoxLayout* layout = new QVBoxLayout(loginPage);
    layout->setContentsMargins(50, 30, 50, 30);
    
    QLabel* titleLabel = new QLabel("Вход");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    QLabel* usernameLabel = new QLabel("Логин:");
    loginUsernameEdit = new QLineEdit();
    loginUsernameEdit->setMaxLength(50); // Limit username to 50 characters
    
    QLabel* passwordLabel = new QLabel("Пароль:");
    loginPasswordEdit = new QLineEdit();
    loginPasswordEdit->setEchoMode(QLineEdit::Password);
    loginPasswordEdit->setMaxLength(128); // Limit password to 128 characters
    
    // Add show/hide password button for login
    QPushButton* loginShowPasswordBtn = new QPushButton("👁");
    loginShowPasswordBtn->setMaximumWidth(40);
    loginShowPasswordBtn->setCheckable(true);
    loginShowPasswordBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #f5f5f7; "
        "color: #1d1d1f; "
        "border: 1px solid #d1d1d6; "
        "border-radius: 4px; "
        "padding: 6px; "
        "min-width: 0; "
        "}"
        "QPushButton:hover { background-color: #e8e8ed; }"
        "QPushButton:checked { background-color: #007aff; color: white; }"
    );
    connect(loginShowPasswordBtn, &QPushButton::toggled, [this](bool checked) {
        loginPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
    
    QHBoxLayout* loginPasswordLayout = new QHBoxLayout();
    loginPasswordLayout->addWidget(loginPasswordEdit);
    loginPasswordLayout->addWidget(loginShowPasswordBtn);
    
    loginAttemptsLabel = new QLabel("Попыток осталось: 3");
    loginAttemptsLabel->setStyleSheet("color: blue; font-weight: bold;");
    
    loginResultText = new QTextEdit();
    loginResultText->setReadOnly(true);
    loginResultText->setMaximumHeight(100);
    
    QPushButton* submitBtn = new QPushButton("Войти");
    loginRecoverBtn = new QPushButton("Восстановить пароль");
    loginRecoverBtn->setVisible(false);
    QPushButton* backBtn = new QPushButton("Назад");
    
    connect(submitBtn, &QPushButton::clicked, this, &MainWindow::onLoginSubmitClicked);
    connect(loginRecoverBtn, &QPushButton::clicked, this, &MainWindow::onLoginRecoverClicked);
    connect(backBtn, &QPushButton::clicked, this, &MainWindow::onLoginBackClicked);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(backBtn);
    btnLayout->addWidget(submitBtn);
    
    layout->addWidget(titleLabel);
    layout->addSpacing(20);
    layout->addWidget(usernameLabel);
    layout->addWidget(loginUsernameEdit);
    layout->addWidget(passwordLabel);
    layout->addLayout(loginPasswordLayout);
    layout->addWidget(loginAttemptsLabel);
    layout->addSpacing(10);
    layout->addWidget(loginResultText);
    layout->addSpacing(10);
    layout->addWidget(loginRecoverBtn);
    layout->addLayout(btnLayout);
    layout->addStretch();
    
    stackedWidget->addWidget(loginPage);
}

void MainWindow::createPasswordEvalPage()
{
    passwordEvalPage = new QWidget();
    passwordEvalPage->setObjectName("passwordEvalPage");
    QVBoxLayout* layout = new QVBoxLayout(passwordEvalPage);
    layout->setContentsMargins(50, 30, 50, 30);
    
    QLabel* titleLabel = new QLabel("Оценка сложности пароля");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    QLabel* passwordLabel = new QLabel("Введите пароль для оценки:");
    evalPasswordEdit = new QLineEdit();
    evalPasswordEdit->setEchoMode(QLineEdit::Normal);
    
    evalResultText = new QTextEdit();
    evalResultText->setReadOnly(true);
    
    QPushButton* evalBtn = new QPushButton("Оценить");
    QPushButton* backBtn = new QPushButton("Назад");
    
    connect(evalBtn, &QPushButton::clicked, this, &MainWindow::onEvaluateSubmitClicked);
    connect(backBtn, &QPushButton::clicked, this, &MainWindow::onEvaluateBackClicked);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(backBtn);
    btnLayout->addWidget(evalBtn);
    
    layout->addWidget(titleLabel);
    layout->addSpacing(20);
    layout->addWidget(passwordLabel);
    layout->addWidget(evalPasswordEdit);
    layout->addSpacing(10);
    layout->addWidget(evalResultText);
    layout->addSpacing(10);
    layout->addLayout(btnLayout);
    
    stackedWidget->addWidget(passwordEvalPage);
}

void MainWindow::createRecoveryPage()
{
    recoveryPage = new QWidget();
    recoveryPage->setObjectName("recoveryPage");
    QVBoxLayout* layout = new QVBoxLayout(recoveryPage);
    layout->setContentsMargins(50, 30, 50, 30);
    
    QLabel* titleLabel = new QLabel("Восстановление пароля");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    QLabel* usernameLabel = new QLabel("Логин:");
    recoveryUsernameEdit = new QLineEdit();
    recoveryUsernameEdit->setMaxLength(50); // Limit username to 50 characters
    
    QLabel* seedLabel = new QLabel("12 слов восстановления (через пробел):");
    recoverySeedEdit = new QTextEdit();
    recoverySeedEdit->setMaximumHeight(80);
    
    QLabel* passwordLabel = new QLabel("Новый пароль:");
    recoveryPasswordEdit = new QLineEdit();
    recoveryPasswordEdit->setEchoMode(QLineEdit::Password);
    recoveryPasswordEdit->setMaxLength(128); // Limit password to 128 characters
    
    // Add show/hide password button for recovery
    QPushButton* recoveryShowPasswordBtn = new QPushButton("👁");
    recoveryShowPasswordBtn->setMaximumWidth(40);
    recoveryShowPasswordBtn->setCheckable(true);
    recoveryShowPasswordBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #f5f5f7; "
        "color: #1d1d1f; "
        "border: 1px solid #d1d1d6; "
        "border-radius: 4px; "
        "padding: 6px; "
        "min-width: 0; "
        "}"
        "QPushButton:hover { background-color: #e8e8ed; }"
        "QPushButton:checked { background-color: #007aff; color: white; }"
    );
    connect(recoveryShowPasswordBtn, &QPushButton::toggled, [this](bool checked) {
        recoveryPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
    
    QHBoxLayout* recoveryPasswordLayout = new QHBoxLayout();
    recoveryPasswordLayout->addWidget(recoveryPasswordEdit);
    recoveryPasswordLayout->addWidget(recoveryShowPasswordBtn);
    
    recoveryResultText = new QTextEdit();
    recoveryResultText->setReadOnly(true);
    recoveryResultText->setMaximumHeight(100);
    
    QPushButton* submitBtn = new QPushButton("Восстановить");
    QPushButton* backBtn = new QPushButton("Назад");
    
    connect(submitBtn, &QPushButton::clicked, this, &MainWindow::onRecoverySubmitClicked);
    connect(backBtn, &QPushButton::clicked, this, &MainWindow::onRecoveryBackClicked);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(backBtn);
    btnLayout->addWidget(submitBtn);
    
    layout->addWidget(titleLabel);
    layout->addSpacing(20);
    layout->addWidget(usernameLabel);
    layout->addWidget(recoveryUsernameEdit);
    layout->addWidget(seedLabel);
    layout->addWidget(recoverySeedEdit);
    layout->addWidget(passwordLabel);
    layout->addLayout(recoveryPasswordLayout);
    layout->addSpacing(10);
    layout->addWidget(recoveryResultText);
    layout->addSpacing(10);
    layout->addLayout(btnLayout);
    
    stackedWidget->addWidget(recoveryPage);
}

void MainWindow::createUserMenuPage()
{
    userMenuPage = new QWidget();
    userMenuPage->setObjectName("userMenuPage");
    
    // Main horizontal layout - Apple style with sidebar
    QHBoxLayout* mainLayout = new QHBoxLayout(userMenuPage);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Left sidebar
    QWidget* sidebar = new QWidget();
    sidebar->setStyleSheet("background-color: #f5f5f7; border-right: 1px solid #d1d1d6;");
    sidebar->setFixedWidth(280);
    
    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 20, 16, 20);
    sidebarLayout->setSpacing(12);
    
    // Sidebar title
    QLabel* sidebarTitle = new QLabel("Passwords");
    QFont titleFont = sidebarTitle->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    sidebarTitle->setFont(titleFont);
    sidebarTitle->setStyleSheet("color: #1d1d1f; margin-bottom: 8px;");
    
    // Search box
    searchBox = new QLineEdit();
    searchBox->setPlaceholderText("Поиск паролей...");
    searchBox->setStyleSheet(
        "QLineEdit { "
        "background-color: white; "
        "border: 1px solid #d1d1d6; "
        "border-radius: 8px; "
        "padding: 8px 12px; "
        "font-size: 13px; "
        "}"
    );
    connect(searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    
    // Entries list
    entriesList = new QListWidget();
    entriesList->setStyleSheet(
        "QListWidget { "
        "background-color: transparent; "
        "border: none; "
        "}"
        "QListWidget::item { "
        "background-color: white; "
        "border-radius: 6px; "
        "padding: 12px; "
        "margin: 2px 0; "
        "color: #1d1d1f; "
        "}"
        "QListWidget::item:selected { "
        "background-color: #007aff; "
        "color: white; "
        "}"
        "QListWidget::item:hover:!selected { "
        "background-color: #e8e8ed; "
        "}"
    );
    
    connect(entriesList, &QListWidget::currentTextChanged, this, &MainWindow::showEntryDetails);
    
    // Add button at bottom of sidebar
    QPushButton* addBtn = new QPushButton("+ Добавить пароль");
    addBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #007aff; "
        "color: white; "
        "border-radius: 8px; "
        "padding: 12px; "
        "font-size: 14px; "
        "font-weight: 600; "
        "min-width: 0; "
        "}"
        "QPushButton:hover { background-color: #0051d5; }"
    );
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddEntryClicked);
    
    sidebarLayout->addWidget(sidebarTitle);
    sidebarLayout->addWidget(searchBox);
    sidebarLayout->addSpacing(8);
    sidebarLayout->addWidget(entriesList);
    
    // Add utility buttons
    QPushButton* generateBtn = new QPushButton("Генератор паролей");
    generateBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #34c759; "
        "color: white; "
        "border-radius: 8px; "
        "padding: 10px; "
        "font-size: 13px; "
        "font-weight: 600; "
        "}"
        "QPushButton:hover { background-color: #248a3d; }"
    );
    connect(generateBtn, &QPushButton::clicked, this, &MainWindow::onGeneratePasswordClicked);
    
    QPushButton* checkStrengthBtn = new QPushButton("Проверка сложности");
    checkStrengthBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #ff9500; "
        "color: white; "
        "border-radius: 8px; "
        "padding: 10px; "
        "font-size: 13px; "
        "font-weight: 600; "
        "}"
        "QPushButton:hover { background-color: #c77700; }"
    );
    connect(checkStrengthBtn, &QPushButton::clicked, this, &MainWindow::onCheckStrengthClicked);
    
    sidebarLayout->addWidget(generateBtn);
    sidebarLayout->addWidget(checkStrengthBtn);
    sidebarLayout->addWidget(addBtn);
    
    // Right detail panel
    QWidget* detailPanel = new QWidget();
    detailPanel->setStyleSheet("background-color: white;");
    
    QVBoxLayout* detailLayout = new QVBoxLayout(detailPanel);
    detailLayout->setContentsMargins(32, 32, 32, 32);
    detailLayout->setSpacing(20);
    
    // Detail view widget
    entryDetailWidget = new QWidget();
    QVBoxLayout* detailContentLayout = new QVBoxLayout(entryDetailWidget);
    detailContentLayout->setSpacing(16);
    
    // Service name (title)
    detailServiceLabel = new QLabel("Выберите пароль");
    QFont serviceFont = detailServiceLabel->font();
    serviceFont.setPointSize(22);
    serviceFont.setBold(true);
    detailServiceLabel->setFont(serviceFont);
    detailServiceLabel->setStyleSheet("color: #1d1d1f;");
    
    // Login
    QLabel* loginTitleLabel = new QLabel("Username");
    loginTitleLabel->setStyleSheet("color: #86868b; font-size: 12px; font-weight: 500;");
    detailLoginLabel = new QLabel("");
    detailLoginLabel->setStyleSheet("color: #1d1d1f; font-size: 15px; padding: 8px 0;");
    
    // Password
    QLabel* passwordTitleLabel = new QLabel("Password");
    passwordTitleLabel->setStyleSheet("color: #86868b; font-size: 12px; font-weight: 500;");
    detailPasswordLabel = new QLabel("••••••••");
    detailPasswordLabel->setStyleSheet("color: #1d1d1f; font-size: 15px; padding: 8px 0;");
    
    // Password buttons
    QHBoxLayout* passwordBtnLayout = new QHBoxLayout();
    passwordBtnLayout->setSpacing(8);
    
    showPasswordBtn = new QPushButton("Показать");
    copyPasswordBtn = new QPushButton("Копировать");
    
    QString smallBtnStyle = 
        "QPushButton { "
        "background-color: #f5f5f7; "
        "color: #007aff; "
        "border: none; "
        "border-radius: 6px; "
        "padding: 8px 16px; "
        "font-size: 13px; "
        "min-width: 0; "
        "}"
        "QPushButton:hover { background-color: #e8e8ed; }";
    
    showPasswordBtn->setStyleSheet(smallBtnStyle);
    copyPasswordBtn->setStyleSheet(smallBtnStyle);
    
    connect(showPasswordBtn, &QPushButton::clicked, this, &MainWindow::onShowPasswordClicked);
    connect(copyPasswordBtn, &QPushButton::clicked, this, &MainWindow::onCopyPasswordClicked);
    
    passwordBtnLayout->addWidget(showPasswordBtn);
    passwordBtnLayout->addWidget(copyPasswordBtn);
    passwordBtnLayout->addStretch();
    
    // URL
    QLabel* urlTitleLabel = new QLabel("Website");
    urlTitleLabel->setStyleSheet("color: #86868b; font-size: 12px; font-weight: 500;");
    detailUrlLabel = new QLabel("");
    detailUrlLabel->setStyleSheet("font-size: 15px; padding: 8px 0;");
    detailUrlLabel->setOpenExternalLinks(true);
    detailUrlLabel->setWordWrap(true);  // Enable word wrapping for long URLs
    detailUrlLabel->setTextFormat(Qt::RichText);  // Allow HTML links
    
    // Notes
    QLabel* noteTitleLabel = new QLabel("Notes");
    noteTitleLabel->setStyleSheet("color: #86868b; font-size: 12px; font-weight: 500;");
    detailNoteLabel = new QLabel("");
    detailNoteLabel->setStyleSheet("color: #1d1d1f; font-size: 13px; padding: 8px 0;");
    detailNoteLabel->setWordWrap(true);
    detailNoteLabel->setTextFormat(Qt::PlainText);  // Prevent HTML interpretation
    detailNoteLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    detailContentLayout->addWidget(detailServiceLabel);
    detailContentLayout->addSpacing(8);
    detailContentLayout->addWidget(loginTitleLabel);
    detailContentLayout->addWidget(detailLoginLabel);
    detailContentLayout->addWidget(passwordTitleLabel);
    detailContentLayout->addWidget(detailPasswordLabel);
    detailContentLayout->addLayout(passwordBtnLayout);
    detailContentLayout->addWidget(urlTitleLabel);
    detailContentLayout->addWidget(detailUrlLabel);
    detailContentLayout->addWidget(noteTitleLabel);
    detailContentLayout->addWidget(detailNoteLabel);
    detailContentLayout->addStretch();
    
    // Action buttons at bottom
    QHBoxLayout* actionBtnLayout = new QHBoxLayout();
    actionBtnLayout->setSpacing(12);
    
    editEntryBtn = new QPushButton("Редактировать");
    deleteEntryBtn = new QPushButton("Удалить");
    QPushButton* logoutBtn = new QPushButton("Выход");
    
    editEntryBtn->setStyleSheet(
        "QPushButton { background-color: #007aff; min-width: 100px; }"
        "QPushButton:hover { background-color: #0051d5; }"
    );
    deleteEntryBtn->setStyleSheet(
        "QPushButton { background-color: #ff3b30; min-width: 100px; }"
        "QPushButton:hover { background-color: #d70015; }"
    );
    logoutBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #f5f5f7; "
        "color: #1d1d1f; "
        "min-width: 100px; "
        "}"
        "QPushButton:hover { background-color: #e8e8ed; }"
    );
    
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    connect(editEntryBtn, &QPushButton::clicked, this, &MainWindow::onEditEntryClicked);
    connect(deleteEntryBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEntryClicked);
    
    actionBtnLayout->addWidget(editEntryBtn);
    actionBtnLayout->addWidget(deleteEntryBtn);
    actionBtnLayout->addStretch();
    actionBtnLayout->addWidget(logoutBtn);
    
    detailLayout->addWidget(entryDetailWidget);
    detailLayout->addLayout(actionBtnLayout);
    
    // Add panels to main layout
    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(detailPanel, 1);
    
    stackedWidget->addWidget(userMenuPage);
}

void MainWindow::createAddEntryPage()
{
    addEntryPage = new QWidget();
    addEntryPage->setObjectName("addEntryPage");
    
    QVBoxLayout* layout = new QVBoxLayout(addEntryPage);
    layout->setContentsMargins(60, 40, 60, 40);
    layout->setSpacing(20);
    
    // Title
    QLabel* titleLabel = new QLabel("Добавление нового пароля");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(22);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #1d1d1f;");
    
    // Service name
    QLabel* serviceLabel = new QLabel("Название сервиса *");
    serviceLabel->setStyleSheet("color: #86868b; font-size: 12px; font-weight: 500;");
    addServiceEdit = new QLineEdit();
    addServiceEdit->setPlaceholderText("например, GitHub, Gmail, Netflix");
    addServiceEdit->setMaxLength(100); // Limit service name to 100 characters
    
    // Login/Email
    QLabel* loginLabel = new QLabel("Имя пользователя или Email *");
    loginLabel->setStyleSheet("color: #86868b; font-size: 12px; font-weight: 500;");
    addLoginEdit = new QLineEdit();
    addLoginEdit->setPlaceholderText("например, user@example.com");
    addLoginEdit->setMaxLength(100); // Limit login to 100 characters
    
    // Password
    QLabel* passwordLabel = new QLabel("Пароль *");
    passwordLabel->setStyleSheet("color: #86868b; font-size: 12px; font-weight: 500;");
    addPasswordEdit = new QLineEdit();
    addPasswordEdit->setEchoMode(QLineEdit::Password);
    addPasswordEdit->setPlaceholderText("Введите надёжный пароль");
    addPasswordEdit->setMaxLength(30); // Limit password to 30 characters
    
    // Add show/hide password button for add entry
    QPushButton* addShowPasswordBtn = new QPushButton("👁");
    addShowPasswordBtn->setMaximumWidth(40);
    addShowPasswordBtn->setCheckable(true);
    addShowPasswordBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #f5f5f7; "
        "color: #1d1d1f; "
        "border: 1px solid #d1d1d6; "
        "border-radius: 4px; "
        "padding: 6px; "
        "min-width: 0; "
        "}"
        "QPushButton:hover { background-color: #e8e8ed; }"
        "QPushButton:checked { background-color: #007aff; color: white; }"
    );
    connect(addShowPasswordBtn, &QPushButton::toggled, [this](bool checked) {
        addPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
    
    QHBoxLayout* addPasswordLayout = new QHBoxLayout();
    addPasswordLayout->addWidget(addPasswordEdit);
    addPasswordLayout->addWidget(addShowPasswordBtn);
    
    // URL (optional)
    QLabel* urlLabel = new QLabel("Веб-сайт (необязательно)");
    urlLabel->setStyleSheet("color: #86868b; font-size: 12px; font-weight: 500;");
    addUrlEdit = new QLineEdit();
    addUrlEdit->setPlaceholderText("например, https://github.com");
    addUrlEdit->setMaxLength(500); // Limit URL to 500 characters
    
    // Notes (optional)
    QLabel* noteLabel = new QLabel("Заметки (необязательно)");
    noteLabel->setStyleSheet("color: #86868b; font-size: 12px; font-weight: 500;");
    addNoteEdit = new QTextEdit();
    addNoteEdit->setPlaceholderText("Добавьте дополнительную информацию...");
    addNoteEdit->setMaximumHeight(100);
    
    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    
    QPushButton* cancelBtn = new QPushButton("Отмена");
    cancelBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #f5f5f7; "
        "color: #1d1d1f; "
        "border-radius: 8px; "
        "padding: 12px 24px; "
        "font-size: 14px; "
        "font-weight: 600; "
        "min-width: 120px; "
        "}"
        "QPushButton:hover { background-color: #e8e8ed; }"
    );
    
    QPushButton* submitBtn = new QPushButton("Сохранить пароль");
    submitBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #007aff; "
        "color: white; "
        "border-radius: 8px; "
        "padding: 12px 24px; "
        "font-size: 14px; "
        "font-weight: 600; "
        "min-width: 120px; "
        "}"
        "QPushButton:hover { background-color: #0051d5; }"
    );
    
    connect(cancelBtn, &QPushButton::clicked, this, &MainWindow::onAddEntryPageCancelClicked);
    connect(submitBtn, &QPushButton::clicked, this, &MainWindow::onAddEntryPageSubmitClicked);
    
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(submitBtn);
    
    // Add all widgets to layout
    layout->addWidget(titleLabel);
    layout->addSpacing(10);
    layout->addWidget(serviceLabel);
    layout->addWidget(addServiceEdit);
    layout->addWidget(loginLabel);
    layout->addWidget(addLoginEdit);
    layout->addWidget(passwordLabel);
    layout->addLayout(addPasswordLayout);
    layout->addWidget(urlLabel);
    layout->addWidget(addUrlEdit);
    layout->addWidget(noteLabel);
    layout->addWidget(addNoteEdit);
    layout->addSpacing(10);
    layout->addLayout(btnLayout);
    layout->addStretch();
    
    stackedWidget->addWidget(addEntryPage);
}

void MainWindow::showMainMenu()
{
    stackedWidget->setCurrentWidget(mainMenuPage);
}

void MainWindow::showUserMenu()
{
    stackedWidget->setCurrentWidget(userMenuPage);
    updateEntriesList();
}

void MainWindow::updateEntriesList()
{
    // Clear current selection first to ensure details update correctly
    entriesList->setCurrentRow(-1);
    entriesList->clear();
    
    if (!client->isAuthenticated()) {
        return;
    }
    
    // Get actual entries from vault
    auto vaultJson = client->getVaultEntries();
    
    if (vaultJson.is_array()) {
        for (const auto& entry : vaultJson) {
            QString service = QString::fromStdString(entry["_service"]);
            QString login = QString::fromStdString(entry["_login"]);
            
            // Create list item with service and login
            QString itemText = service;
            if (!login.isEmpty()) {
                itemText += " (" + login + ")";
            }
            
            entriesList->addItem(itemText);
        }
    }
    
    if (entriesList->count() > 0) {
        entriesList->setCurrentRow(0);
    } else {
        // Show empty state
        detailServiceLabel->setText("Пока нет паролей");
        detailLoginLabel->setText("");
        detailPasswordLabel->setText("");
        detailUrlLabel->setText("");
        detailNoteLabel->setText("Нажмите '+ Добавить пароль' для создания первой записи");
    }
}

void MainWindow::showEntryDetails(const QString& itemText)
{
    if (itemText.isEmpty()) {
        detailServiceLabel->setText("Выберите пароль");
        detailLoginLabel->setText("");
        detailPasswordLabel->setText("••••••••");
        detailUrlLabel->setText("");
        detailNoteLabel->setText("");
        return;
    }
    
    // Get vault entries
    auto vaultJson = client->getVaultEntries();
    
    if (!vaultJson.is_array()) {
        return;
    }
    
    // Find the matching entry
    for (const auto& entry : vaultJson) {
        QString service = QString::fromStdString(entry["_service"]);
        QString login = QString::fromStdString(entry["_login"]);
        
        QString checkText = service;
        if (!login.isEmpty()) {
            checkText += " (" + login + ")";
        }
        
        if (checkText == itemText) {
            // Found the entry - display its details
            QString password = QString::fromStdString(entry["_password"]);
            QString url = QString::fromStdString(entry["_url"]);
            QString note = QString::fromStdString(entry["_note"]);
            
            detailServiceLabel->setText(service);
            detailLoginLabel->setText(login);
            detailPasswordLabel->setText("••••••••");
            detailPasswordLabel->setProperty("actualPassword", password);
            
            if (!url.isEmpty()) {
                detailUrlLabel->setText(QString("<a href='%1' style='color: #007aff; text-decoration: underline;'>%1</a>").arg(url));
                detailUrlLabel->setStyleSheet("font-size: 15px; padding: 8px 0;");
                detailUrlLabel->setWordWrap(true);  // Ensure word wrap is enabled
            } else {
                detailUrlLabel->setText("(нет веб-сайта)");
                detailUrlLabel->setStyleSheet("color: #86868b; font-size: 13px; padding: 8px 0; font-style: italic;");
                detailUrlLabel->setWordWrap(true);
            }
            
            if (!note.isEmpty()) {
                detailNoteLabel->setText(note);
            } else {
                detailNoteLabel->setText("(нет заметок)");
            }
            
            // Reset show/hide button
            showPasswordBtn->setText("Показать");
            
            return;
        }
    }
}

void MainWindow::onRegisterClicked()
{
    regUsernameEdit->clear();
    regPasswordEdit->clear();
    regResultText->clear();
    stackedWidget->setCurrentWidget(registerPage);
}

void MainWindow::onLoginClicked()
{
    loginUsernameEdit->clear();
    loginPasswordEdit->clear();
    loginResultText->clear();
    loginAttempts = 0;
    loginAttemptsLabel->setText("Попыток осталось: 3");
    loginRecoverBtn->setVisible(false);
    stackedWidget->setCurrentWidget(loginPage);
}

void MainWindow::onPasswordEvalClicked()
{
    evalPasswordEdit->clear();
    evalResultText->clear();
    stackedWidget->setCurrentWidget(passwordEvalPage);
}

void MainWindow::onRecoverPasswordClicked()
{
    recoveryUsernameEdit->clear();
    recoverySeedEdit->clear();
    recoveryPasswordEdit->clear();
    recoveryResultText->clear();
    stackedWidget->setCurrentWidget(recoveryPage);
}

void MainWindow::onRegisterSubmitClicked()
{
    try {
        QString username = regUsernameEdit->text().trimmed();
        QString password = regPasswordEdit->text();
        
        if (username.isEmpty() || password.isEmpty()) {
            regResultText->setText("Пожалуйста, заполните все поля!");
            regResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
            return;
        }
        
        // Check if user exists
        if (client->checkUserExists(username.toStdString())) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Пользователь существует");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Пользователь с таким логином уже существует!");
            msgBox.setInformativeText("Хотите войти в существующий аккаунт?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            
            QAbstractButton* yesBtn = msgBox.button(QMessageBox::Yes);
            yesBtn->setText("Да, войти");
            QAbstractButton* noBtn = msgBox.button(QMessageBox::No);
            noBtn->setText("Нет");
            
            int reply = msgBox.exec();
            
            if (reply == QMessageBox::Yes) {
                // Switch to login page with this username
                loginUsernameEdit->setText(username);
                onLoginClicked();
            }
            return;
        }
        
        // Validate password
        std::string errorMessage;
        if (!validatePassword(password.toStdString(), errorMessage)) {
            regResultText->setText(QString("ОШИБКА: %1").arg(QString::fromStdString(errorMessage)));
            regResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
            return;
        }
        
        // Check for weak password
        if (isWeakPassword(password.toStdString())) {
            regResultText->setText("ОШИБКА: Пароль слишком распространенный!\n"
                                  "Пожалуйста, выберите более безопасный пароль.");
            regResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
            return;
        }
        
        // First, ask for code word before registering
        bool codeWordValid = false;
        QString codeWord;
        while (!codeWordValid) {
            bool ok;
            
            // Create custom styled input dialog
            QInputDialog inputDialog(this);
            inputDialog.setWindowTitle("Кодовое слово");
            inputDialog.setLabelText(
                "<b>Введите кодовое слово для шифрования ваших данных (3-30 символов)</b><br><br>"
                "<font color='#e74c3c'><b>ВАЖНО!</b></font><br>"
                "• Кодовое слово используется для шифрования вашего хранилища<br>"
                "• Оно будет запрашиваться при каждом входе<br>"
                "• <b>НЕВОЗМОЖНО</b> изменить кодовое слово в будущем<br>"
                "• Можете использовать одно из 12 слов восстановления<br>"
                "• Если забудете кодовое слово, доступ к данным будет утерян навсегда<br><br>"
                "Кодовое слово:");
            inputDialog.setTextValue("");
            inputDialog.setInputMode(QInputDialog::TextInput);
            
            // Apply light background styling
            inputDialog.setStyleSheet(
                "QInputDialog { background-color: white; }"
                "QLabel { color: #1d1d1f; background-color: transparent; }"
                "QLineEdit { background-color: white; border: 1px solid #d1d1d6; border-radius: 4px; padding: 6px; }"
                "QPushButton { background-color: #007aff; color: white; border: none; border-radius: 6px; padding: 8px 16px; }"
                "QPushButton:hover { background-color: #0051d5; }"
            );
            
            ok = (inputDialog.exec() == QDialog::Accepted);
            codeWord = inputDialog.textValue();
            
            if (!ok || codeWord.isEmpty()) {
                QMessageBox::warning(this, "Ошибка", 
                    "Регистрация отменена. Кодовое слово обязательно для защиты данных.");
                return;
            }
            
            std::string errorMessage;
            if (client->validateCodeWord(codeWord.toStdString(), errorMessage)) {
                codeWordValid = true;
            } else {
                QMessageBox::warning(this, "Неверное кодовое слово", 
                    QString::fromStdString(errorMessage));
            }
        }
        
        // Now register user with code word and get seed words
        std::vector<std::string> seedWords;
        if (client->registerUser(username.toStdString(), password.toStdString(), 
                                codeWord.toStdString(), seedWords)) {
            regResultText->setText("Регистрация успешна!");
            regResultText->setStyleSheet("color: #27ae60; background-color: #d5f4e6; padding: 10px; border-radius: 5px;");
            regPasswordEdit->clear();
            
            // Display seed words in a beautiful dialog
            QString seedWordsText = "<b>Ваши слова для восстановления пароля</b><br><br>";
            seedWordsText += "<font color='#e74c3c'><b>ВАЖНО! Сохраните эти слова в безопасном месте!</b></font><br>";
            seedWordsText += "Они понадобятся для восстановления доступа к аккаунту.<br><br>";
            seedWordsText += "<table style='width:100%; border-collapse: collapse;'>";
            
            for (size_t i = 0; i < seedWords.size(); i++) {
                if (i % 3 == 0) seedWordsText += "<tr>";
                seedWordsText += QString("<td style='padding: 5px; background-color: #ecf0f1; border: 1px solid #bdc3c7; font-weight: bold;'>%1. %2</td>")
                    .arg(i + 1).arg(QString::fromStdString(seedWords[i]));
                if (i % 3 == 2 || i == seedWords.size() - 1) seedWordsText += "</tr>";
            }
            seedWordsText += "</table>";
            
            QMessageBox seedDialog(this);
            seedDialog.setWindowTitle("Слова для восстановления");
            seedDialog.setIcon(QMessageBox::Information);
            seedDialog.setText(seedWordsText);
            seedDialog.setStandardButtons(QMessageBox::Ok);
            seedDialog.setDefaultButton(QMessageBox::Ok);
            seedDialog.setTextFormat(Qt::RichText);
            
            // Make the dialog larger to show all words nicely
            seedDialog.setStyleSheet("QLabel{min-width: 500px; min-height: 300px;}");
            
            seedDialog.exec();
            
            QMessageBox::information(this, "Успех", 
                "Регистрация завершена! Теперь вы можете войти в систему.");
            showMainMenu();
        } else {
            regResultText->setText("Ошибка регистрации. Возможно, проблема с подключением к серверу.");
            regResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
        }
    } catch (const std::exception& e) {
        regResultText->setText(QString("Ошибка регистрации: %1").arg(e.what()));
        regResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
    } catch (...) {
        regResultText->setText("Неизвестная ошибка при регистрации. Попробуйте снова.");
        regResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
    }
}

void MainWindow::onRegisterBackClicked()
{
    showMainMenu();
}

void MainWindow::onLoginSubmitClicked()
{
    try {
        QString username = loginUsernameEdit->text().trimmed();
        QString password = loginPasswordEdit->text();
        
        if (username.isEmpty() || password.isEmpty()) {
            loginResultText->setText("Пожалуйста, заполните все поля!");
            loginResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
            return;
        }
        
        currentLoginUsername = username;
        performLogin(username, password);
    } catch (const std::exception& e) {
        loginResultText->setText(QString("Ошибка входа: %1").arg(e.what()));
        loginResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
    } catch (...) {
        loginResultText->setText("Неизвестная ошибка при входе. Попробуйте снова.");
        loginResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
    }
}

void MainWindow::performLogin(const QString& username, const QString& password)
{
    // Create loading dialog
    QProgressDialog* loadingDialog = new QProgressDialog("Подключение к серверу и загрузка данных...", 
                                                         QString(), 0, 0, this);
    loadingDialog->setWindowTitle("Вход в систему");
    loadingDialog->setWindowModality(Qt::WindowModal);
    loadingDialog->setCancelButton(nullptr);  // No cancel button
    loadingDialog->setMinimumDuration(0);  // Show immediately
    
    // Light background styling for better visibility
    loadingDialog->setStyleSheet(
        "QProgressDialog {"
        "    background-color: white;"
        "}"
        "QLabel {"
        "    color: #1d1d1f;"
        "    font-size: 14px;"
        "    padding: 10px;"
        "}"
        "QProgressBar {"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 4px;"
        "    background-color: #f0f0f0;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #007aff;"
        "    border-radius: 2px;"
        "}"
    );
    loadingDialog->setValue(0);
    loadingDialog->show();
    QApplication::processEvents();  // Force display
    
    try {
        if (attemptLogin(username, password)) {
            // Update loading message
            loadingDialog->setLabelText("Загрузка паролей...");
            QApplication::processEvents();
            
            loginResultText->setText("Вход выполнен успешно!");
            loginResultText->setStyleSheet("color: #27ae60; background-color: #d5f4e6; padding: 10px; border-radius: 5px;");
            loginPasswordEdit->clear();
            
            // Load data from server
            try {
                client->syncFromServer();
            } catch (...) {
                // Ignore sync errors, user is still logged in
            }
            
            // Close loading dialog
            loadingDialog->close();
            delete loadingDialog;
            
            // Show user menu after short delay
            QTimer::singleShot(300, this, &MainWindow::showUserMenu);
        } else {
            // Close loading dialog on failure
            loadingDialog->close();
            delete loadingDialog;
            
            loginAttempts++;
            int remaining = 3 - loginAttempts;
            
            if (remaining > 0) {
                loginAttemptsLabel->setText(QString("Попыток осталось: %1").arg(remaining));
                loginResultText->setText(QString("Неверный пароль. Осталось попыток: %1").arg(remaining));
                loginResultText->setStyleSheet("color: #e67e22; background-color: #fde3cf; padding: 10px; border-radius: 5px;");
                loginPasswordEdit->clear();
                loginPasswordEdit->setFocus();
            } else {
                loginAttemptsLabel->setText("Попыток осталось: 0");
                loginAttemptsLabel->setStyleSheet("color: red; font-weight: bold;");
                loginResultText->setText("Исчерпаны все попытки входа!\n"
                                        "Нажмите 'Восстановить пароль' чтобы сбросить пароль.");
                loginRecoverBtn->setVisible(true);
                loginPasswordEdit->setEnabled(false);
            }
        }
    } catch (const std::exception& e) {
        loadingDialog->close();
        delete loadingDialog;
        
        loginResultText->setText(QString("Ошибка при попытке входа: %1").arg(e.what()));
        loginResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
    } catch (...) {
        loadingDialog->close();
        delete loadingDialog;
        
        loginResultText->setText("Неизвестная ошибка. Проверьте подключение к серверу.");
        loginResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
    }
}

bool MainWindow::attemptLogin(const QString& username, const QString& password)
{
    try {
        // Ask for code word before attempting login
        QInputDialog inputDialog(this);
        inputDialog.setWindowTitle("Кодовое слово");
        inputDialog.setLabelText("Введите кодовое слово для расшифровки ваших данных:");
        inputDialog.setTextValue("");
        inputDialog.setTextEchoMode(QLineEdit::Password);
        inputDialog.setInputMode(QInputDialog::TextInput);
        
        // Apply light background styling
        inputDialog.setStyleSheet(
            "QInputDialog { background-color: white; }"
            "QLabel { color: #1d1d1f; background-color: transparent; }"
            "QLineEdit { background-color: white; border: 1px solid #d1d1d6; border-radius: 4px; padding: 6px; }"
            "QPushButton { background-color: #007aff; color: white; border: none; border-radius: 6px; padding: 8px 16px; }"
            "QPushButton:hover { background-color: #0051d5; }"
        );
        
        bool ok = (inputDialog.exec() == QDialog::Accepted);
        QString codeWord = inputDialog.textValue();
        
        if (!ok || codeWord.isEmpty()) {
            return false;
        }
        
        return client->login(username.toStdString(), password.toStdString(), codeWord.toStdString());
    } catch (...) {
        return false;
    }
}

void MainWindow::onLoginBackClicked()
{
    loginPasswordEdit->setEnabled(true);
    showMainMenu();
}

void MainWindow::onLoginRecoverClicked()
{
    recoveryUsernameEdit->setText(currentLoginUsername);
    stackedWidget->setCurrentWidget(recoveryPage);
}

void MainWindow::onEvaluateSubmitClicked()
{
    QString password = evalPasswordEdit->text();
    
    if (password.isEmpty()) {
        evalResultText->setText("Введите пароль для оценки!");
        evalResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 15px; border-radius: 8px;");
        return;
    }
    
    int strength = evaluatePasswordStrength(password.toStdString());
    std::string description = getPasswordStrengthDescription(strength);
    std::string timeToCrack = estimateTimeToCrack(password.toStdString());
    
    // Determine color based on strength
    QString strengthColor;
    QString bgColor;
    if (strength < 30) {
        strengthColor = "#e74c3c"; // Red
        bgColor = "#fadbd8";
    } else if (strength < 50) {
        strengthColor = "#e67e22"; // Orange
        bgColor = "#fde3cf";
    } else if (strength < 70) {
        strengthColor = "#f39c12"; // Yellow
        bgColor = "#fef5e7";
    } else if (strength < 90) {
        strengthColor = "#3498db"; // Blue
        bgColor = "#d6eaf8";
    } else {
        strengthColor = "#27ae60"; // Green
        bgColor = "#d5f4e6";
    }
    
    QString result = QString("<div style='padding: 15px; background-color: %1; border-radius: 8px;'>").arg(bgColor);
    result += "<h3 style='margin: 0 0 10px 0; color: #2c3e50;'>Оценка сложности пароля</h3>";
    result += "<table style='width: 100%; border-collapse: collapse;'>";
    result += QString("<tr><td style='padding: 8px; font-weight: bold;'>Пароль:</td><td style='padding: 8px;'>%1</td></tr>").arg(password);
    result += QString("<tr><td style='padding: 8px; font-weight: bold;'>Оценка:</td><td style='padding: 8px;'><span style='color: %1; font-size: 18px; font-weight: bold;'>%2/100</span></td></tr>")
        .arg(strengthColor).arg(strength);
    result += QString("<tr><td style='padding: 8px; font-weight: bold;'>Уровень:</td><td style='padding: 8px; color: %1; font-weight: bold;'>%2</td></tr>")
        .arg(strengthColor).arg(QString::fromStdString(description));
    result += QString("<tr><td style='padding: 8px; font-weight: bold;'>Время взлома:</td><td style='padding: 8px;'>~%1</td></tr>")
        .arg(QString::fromStdString(timeToCrack));
    result += "</table>";
    
    if (isWeakPassword(password.toStdString())) {
        result += "<div style='margin-top: 15px; padding: 10px; background-color: #fadbd8; border-left: 4px solid #e74c3c; border-radius: 4px;'>";
        result += "<p style='margin: 0; color: #c0392b; font-weight: bold;'>ВНИМАНИЕ: Этот пароль найден в списке распространенных паролей!</p>";
        result += "<p style='margin: 5px 0 0 0; color: #c0392b;'>Настоятельно рекомендуется использовать другой пароль.</p>";
        result += "</div>";
    }
    
    result += "</div>";
    
    evalResultText->setHtml(result);
}

void MainWindow::onEvaluateBackClicked()
{
    showMainMenu();
}

void MainWindow::onRecoverySubmitClicked()
{
    QString username = recoveryUsernameEdit->text().trimmed();
    QString seedPhrase = recoverySeedEdit->toPlainText().trimmed();
    QString newPassword = recoveryPasswordEdit->text();
    
    if (username.isEmpty() || seedPhrase.isEmpty() || newPassword.isEmpty()) {
        recoveryResultText->setText("Пожалуйста, заполните все поля!");
        recoveryResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
        return;
    }
    
    // Validate password
    std::string errorMessage;
    if (!validatePassword(newPassword.toStdString(), errorMessage)) {
        recoveryResultText->setText(QString("ОШИБКА: %1").arg(QString::fromStdString(errorMessage)));
        recoveryResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
        return;
    }
    
    // Check for weak password
    if (isWeakPassword(newPassword.toStdString())) {
        recoveryResultText->setText("ОШИБКА: Пароль слишком распространенный!\n"
                                   "Пожалуйста, выберите более безопасный пароль.");
        recoveryResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
        return;
    }
    
    // NOTE: We cannot directly check if the new password equals the old password
    // because we don't have access to the old password during recovery.
    // The recovery process uses seed words, not the old password.
    // However, we add a warning message to encourage users to choose a different password.
    QMessageBox::StandardButton userChoice = QMessageBox::question(this, "Восстановление пароля",
        "Убедитесь, что новый пароль отличается от старого пароля.\n\n"
        "Использование того же пароля снижает безопасность.\n\n"
        "Продолжить восстановление с этим паролем?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (userChoice == QMessageBox::No) {
        return;
    }
    
    // Recover password and get new seed words
    std::vector<std::string> newSeedWords;
    
    // Ask for code word
    QInputDialog inputDialog(this);
    inputDialog.setWindowTitle("Кодовое слово");
    inputDialog.setLabelText(
        "<b>Введите ваше кодовое слово</b><br><br>"
        "Кодовое слово необходимо для расшифровки ваших данных.<br>"
        "Оно остается неизменным при смене пароля.<br><br>"
        "Кодовое слово:");
    inputDialog.setTextValue("");
    inputDialog.setTextEchoMode(QLineEdit::Password);
    inputDialog.setInputMode(QInputDialog::TextInput);
    
    // Apply light background styling
    inputDialog.setStyleSheet(
        "QInputDialog { background-color: white; }"
        "QLabel { color: #1d1d1f; background-color: transparent; }"
        "QLineEdit { background-color: white; border: 1px solid #d1d1d6; border-radius: 4px; padding: 6px; }"
        "QPushButton { background-color: #007aff; color: white; border: none; border-radius: 6px; padding: 8px 16px; }"
        "QPushButton:hover { background-color: #0051d5; }"
    );
    
    bool ok = (inputDialog.exec() == QDialog::Accepted);
    QString codeWord = inputDialog.textValue();
    
    if (!ok || codeWord.isEmpty()) {
        recoveryResultText->setText("Восстановление отменено. Кодовое слово обязательно.");
        recoveryResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
        return;
    }
    
    if (client->recoverPassword(username.toStdString(), seedPhrase.toStdString(), 
                                newPassword.toStdString(), codeWord.toStdString(), newSeedWords)) {
        
        recoveryResultText->setText("Пароль успешно восстановлен!\n"
                                   "Теперь вы можете войти с новым паролем.");
        recoveryResultText->setStyleSheet("color: #27ae60; background-color: #d5f4e6; padding: 10px; border-radius: 5px;");
        
        // Show new seed words in a dialog
        QString seedWordsText = "<b>Ваши НОВЫЕ слова для восстановления пароля</b><br><br>";
        seedWordsText += "<font color='#e74c3c'><b>ВАЖНО! Сохраните эти слова в безопасном месте!</b></font><br>";
        seedWordsText += "Старые слова больше не работают. Используйте эти новые слова для будущего восстановления.<br><br>";
        seedWordsText += "<table style='width:100%; border-collapse: collapse;'>";
        
        for (size_t i = 0; i < newSeedWords.size(); i++) {
            if (i % 3 == 0) seedWordsText += "<tr>";
            seedWordsText += QString("<td style='padding: 5px; background-color: #ecf0f1; border: 1px solid #bdc3c7; font-weight: bold;'>%1. %2</td>")
                .arg(i + 1).arg(QString::fromStdString(newSeedWords[i]));
            if (i % 3 == 2 || i == newSeedWords.size() - 1) seedWordsText += "</tr>";
        }
        seedWordsText += "</table>";
        
        QMessageBox seedDialog(this);
        seedDialog.setWindowTitle("Новые слова для восстановления");
        seedDialog.setIcon(QMessageBox::Information);
        seedDialog.setText(seedWordsText);
        seedDialog.setStandardButtons(QMessageBox::Ok);
        seedDialog.setDefaultButton(QMessageBox::Ok);
        seedDialog.setTextFormat(Qt::RichText);
        
        // Make the dialog larger and with better styling
        seedDialog.setStyleSheet(
            "QLabel{min-width: 500px; min-height: 300px;} "
            "QMessageBox { background-color: white; } "
            "QPushButton { background-color: #007aff; color: white; padding: 8px 16px; border-radius: 5px; min-width: 80px; } "
            "QPushButton:hover { background-color: #0051d5; }"
        );
        
        seedDialog.exec();
        
        // Clear fields and return to login page
        recoveryPasswordEdit->clear();
        recoverySeedEdit->clear();
        recoveryUsernameEdit->clear();
        
        // Show a message that they should now login
        QMessageBox::information(this, "Пароль восстановлен", 
            "Ваш пароль был успешно изменен!\n\n"
            "Пожалуйста, войдите в систему, используя ваше имя пользователя и НОВЫЙ пароль.");
        
        // Go to login page
        stackedWidget->setCurrentWidget(loginPage);
    } else {
        recoveryResultText->setText("Не удалось восстановить пароль.\n"
                                   "Проверьте правильность кодовых слов.");
        recoveryResultText->setStyleSheet("color: #e74c3c; background-color: #fadbd8; padding: 10px; border-radius: 5px;");
    }
}

void MainWindow::onRecoveryBackClicked()
{
    showMainMenu();
}

void MainWindow::onViewEntriesClicked()
{
    updateEntriesList();
}

void MainWindow::onAddEntryClicked()
{
    isEditingEntry = false;
    
    // Clear all fields
    addServiceEdit->clear();
    addLoginEdit->clear();
    addPasswordEdit->clear();
    addUrlEdit->clear();
    addNoteEdit->clear();
    
    // Enable service and login fields for adding
    addServiceEdit->setEnabled(true);
    addLoginEdit->setEnabled(true);
    
    // Update title and button text
    QLabel* titleLabel = addEntryPage->findChild<QLabel*>();
    if (titleLabel) {
        titleLabel->setText("Добавление нового пароля");
    }
    
    QPushButton* submitBtn = addEntryPage->findChildren<QPushButton*>().last();
    if (submitBtn) {
        submitBtn->setText("Сохранить пароль");
    }
    
    // Switch to add entry page
    stackedWidget->setCurrentWidget(addEntryPage);
    addServiceEdit->setFocus();
}

void MainWindow::onAddEntryPageSubmitClicked()
{
    QString service = addServiceEdit->text().trimmed();
    QString login = addLoginEdit->text().trimmed();
    QString password = addPasswordEdit->text();
    QString url = addUrlEdit->text().trimmed();
    QString note = addNoteEdit->toPlainText().trimmed();
    
    // Validate required fields
    if (service.isEmpty()) {
        QMessageBox::warning(this, "Отсутствует информация", "Пожалуйста, введите название сервиса.");
        addServiceEdit->setFocus();
        return;
    }
    
    if (login.isEmpty()) {
        QMessageBox::warning(this, "Отсутствует информация", "Пожалуйста, введите имя пользователя или email.");
        addLoginEdit->setFocus();
        return;
    }
    
    // Validate login - minimum 3 characters
    if (login.length() < 3) {
        QMessageBox::warning(this, "Ошибка валидации", 
                           "Имя пользователя должно содержать минимум 3 символа.");
        addLoginEdit->setFocus();
        return;
    }
    
    if (password.isEmpty()) {
        QMessageBox::warning(this, "Отсутствует информация", "Пожалуйста, введите пароль.");
        addPasswordEdit->setFocus();
        return;
    }
    
    // Validate password - minimum 6 characters
    if (password.length() < 6) {
        QMessageBox::warning(this, "Слабый пароль", 
                           "Пароль должен содержать минимум 6 символов для безопасности.");
        addPasswordEdit->setFocus();
        return;
    }
    
    // Check for very weak passwords (only numbers or only letters)
    bool hasLetters = false;
    bool hasDigits = false;
    for (QChar c : password) {
        if (c.isLetter()) hasLetters = true;
        if (c.isDigit()) hasDigits = true;
    }
    
    if (!hasLetters || !hasDigits) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Предупреждение о безопасности",
                                     "Пароль содержит только буквы или только цифры. "
                                     "Рекомендуется использовать комбинацию букв и цифр.\n\n"
                                     "Всё равно сохранить?",
                                     QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            addPasswordEdit->setFocus();
            return;
        }
    }
    
    bool success = false;
    
    if (isEditingEntry) {
        // Update existing entry
        success = client->updateEntryFull(
            currentEditService.toStdString(),
            currentEditLogin.toStdString(),
            password.toStdString(),
            url.toStdString(),
            note.toStdString()
        );
        
        if (success) {
            QMessageBox::information(this, "Успешно", "Пароль успешно обновлён!");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось обновить пароль. Попробуйте снова.");
        }
    } else {
        // Add new entry
        success = client->addEntry(
            service.toStdString(),
            login.toStdString(),
            password.toStdString(),
            url.toStdString(),
            note.toStdString()
        );
        
        if (success) {
            // Sync to server immediately after add
            if (!client->syncToServer()) {
                QMessageBox::warning(this, "Предупреждение", 
                    "Пароль сохранён локально, но не удалось синхронизировать с сервером.");
            }
            QMessageBox::information(this, "Успешно", "Пароль успешно сохранён!");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось сохранить пароль. Попробуйте снова.");
        }
    }
    
    if (success) {
        // Go back to user menu and refresh
        showUserMenu();
    }
}

void MainWindow::onAddEntryPageCancelClicked()
{
    showUserMenu();
}

void MainWindow::onLogoutClicked()
{
    client->logout();
    QMessageBox::information(this, "Выход", "Вы вышли из системы.");
    showMainMenu();
}

void MainWindow::onShowPasswordClicked()
{
    // Get actual password from property
    QString password = detailPasswordLabel->property("actualPassword").toString();
    
    // Only allow toggling if there's actually a password set
    if (password.isEmpty()) {
        return;
    }
    
    if (showPasswordBtn->text() == "Показать") {
        detailPasswordLabel->setText(password);
        showPasswordBtn->setText("Скрыть");
    } else {
        detailPasswordLabel->setText("••••••••");
        showPasswordBtn->setText("Показать");
    }
}

void MainWindow::onCopyPasswordClicked()
{
    // Get actual password from property
    QString password = detailPasswordLabel->property("actualPassword").toString();
    if (!password.isEmpty()) {
        QApplication::clipboard()->setText(password);
        
        // Show temporary feedback
        copyPasswordBtn->setText("Скопировано!");
        QTimer::singleShot(2000, [this]() {
            copyPasswordBtn->setText("Копировать");
        });
    }
}

void MainWindow::onEditEntryClicked()
{
    QString currentItem = entriesList->currentItem() ? entriesList->currentItem()->text() : "";
    if (currentItem.isEmpty()) {
        QMessageBox::warning(this, "Редактировать", "Пожалуйста, выберите пароль для редактирования.");
        return;
    }
    
    // Find the entry in vault
    auto vaultJson = client->getVaultEntries();
    if (!vaultJson.is_array()) {
        return;
    }
    
    for (const auto& entry : vaultJson) {
        QString service = QString::fromStdString(entry["_service"]);
        QString login = QString::fromStdString(entry["_login"]);
        
        QString checkText = service;
        if (!login.isEmpty()) {
            checkText += " (" + login + ")";
        }
        
        if (checkText == currentItem) {
            // Found the entry - populate form
            isEditingEntry = true;
            currentEditService = service;
            currentEditLogin = login;
            
            addServiceEdit->setText(service);
            addLoginEdit->setText(login);
            addPasswordEdit->setText(QString::fromStdString(entry["_password"]));
            addUrlEdit->setText(QString::fromStdString(entry["_url"]));
            addNoteEdit->setText(QString::fromStdString(entry["_note"]));
            
            // Disable service and login fields during editing (they are the key identifiers)
            addServiceEdit->setEnabled(false);
            addLoginEdit->setEnabled(false);
            
            // Update title and button text
            QLabel* titleLabel = addEntryPage->findChild<QLabel*>();
            if (titleLabel) {
                titleLabel->setText("Редактирование пароля");
            }
            
            QPushButton* submitBtn = addEntryPage->findChildren<QPushButton*>().last();
            if (submitBtn) {
                submitBtn->setText("Обновить пароль");
            }
            
            // Switch to add/edit page
            stackedWidget->setCurrentWidget(addEntryPage);
            addPasswordEdit->setFocus();
            
            return;
        }
    }
}

void MainWindow::onDeleteEntryClicked()
{
    QString currentItem = entriesList->currentItem() ? entriesList->currentItem()->text() : "";
    if (currentItem.isEmpty()) {
        QMessageBox::warning(this, "Удалить", "Пожалуйста, выберите пароль для удаления.");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Удалить пароль",
        QString("Вы уверены, что хотите удалить '%1'?").arg(currentItem),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Parse service and login from the item text
        // Format is "ServiceName (username)"
        int openParen = currentItem.indexOf('(');
        int closeParen = currentItem.indexOf(')');
        
        if (openParen > 0 && closeParen > openParen) {
            QString service = currentItem.left(openParen).trimmed();
            QString login = currentItem.mid(openParen + 1, closeParen - openParen - 1).trimmed();
            
            // Call the deleteEntry function
            if (client->deleteEntry(service.toStdString(), login.toStdString())) {
                // Sync to server immediately after delete
                if (!client->syncToServer()) {
                    QMessageBox::warning(this, "Предупреждение", 
                        "Запись удалена локально, но не удалось синхронизировать с сервером.");
                }
                
                QMessageBox::information(this, "Успешно", 
                    QString("Пароль '%1' успешно удален.").arg(service));
                
                // Refresh the entries list
                updateEntriesList();
                
                // Clear the detail view
                detailServiceLabel->setText("Не выбрано");
                detailLoginLabel->setText("");
                detailPasswordLabel->setText("");
                detailPasswordLabel->setProperty("isVisible", false);
                showPasswordBtn->setText("Показать");
                detailUrlLabel->setText("");
                detailNoteLabel->setText("");
            } else {
                QMessageBox::warning(this, "Ошибка", 
                    "Не удалось удалить пароль. Попробуйте еще раз.");
            }
        } else {
            QMessageBox::warning(this, "Ошибка", 
                "Не удалось определить данные записи для удаления.");
        }
    }
}

void MainWindow::onSearchTextChanged(const QString& text)
{
    // Simple filter for entries list
    for (int i = 0; i < entriesList->count(); ++i) {
        QListWidgetItem* item = entriesList->item(i);
        bool matches = item->text().contains(text, Qt::CaseInsensitive);
        item->setHidden(!matches);
    }
}

std::string MainWindow::generateCustomPassword(int length, bool useUpper, bool useLower, bool useDigits, bool useSpecial)
{
    std::string charset;
    if (useLower) charset += "abcdefghijklmnopqrstuvwxyz";
    if (useUpper) charset += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (useDigits) charset += "0123456789";
    if (useSpecial) charset += "!@#$%^&*()-_=+[]{}|;:,.<>?";
    
    if (charset.empty()) {
        charset = "abcdefghijklmnopqrstuvwxyz"; // Fallback
    }
    
    std::string password;
    try {
        for (int i = 0; i < length; i++) {
            uint32_t randomIndex = randombytes_uniform(charset.length());
            password += charset[randomIndex];
        }
    } catch (...) {
        // Fallback to simpler method if libsodium fails
        // Note: This has modulo bias but is only used as emergency fallback
        srand(time(NULL));
        for (int i = 0; i < length; i++) {
            password += charset[rand() % charset.length()];
        }
    }
    
    return password;
}

void MainWindow::onGeneratePasswordClicked()
{
    try {
        // Create settings dialog
        QDialog* settingsDialog = new QDialog(this);
        settingsDialog->setWindowTitle("Настройки генератора паролей");
        settingsDialog->setMinimumWidth(450);
        settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        settingsDialog->setStyleSheet("QDialog { background-color: white; }");
        
        QVBoxLayout* settingsLayout = new QVBoxLayout(settingsDialog);
        
        // Length slider
        QLabel* lengthLabel = new QLabel("Длина пароля: 16", settingsDialog);
        lengthLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #000000;");
        
        QSlider* lengthSlider = new QSlider(Qt::Horizontal, settingsDialog);
        lengthSlider->setMinimum(8);
        lengthSlider->setMaximum(64);
        lengthSlider->setValue(16);
        lengthSlider->setStyleSheet(
            "QSlider::groove:horizontal { "
            "background: #d1d1d6; "
            "height: 6px; "
            "border-radius: 3px; "
            "} "
            "QSlider::handle:horizontal { "
            "background: #007aff; "
            "width: 18px; "
            "height: 18px; "
            "border-radius: 9px; "
            "margin: -6px 0; "
            "}"
        );
        
        connect(lengthSlider, &QSlider::valueChanged, lengthLabel, [lengthLabel](int value) {
            lengthLabel->setText(QString("Длина пароля: %1").arg(value));
        });
        
        // Checkboxes with visible styling
        QString checkboxStyle = 
            "QCheckBox { font-size: 13px; padding: 5px; color: #000000; } "
            "QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid #666; background-color: white; border-radius: 3px; } "
            "QCheckBox::indicator:checked { background-color: #007aff; border: 2px solid #007aff; }";
        
        QCheckBox* upperCheck = new QCheckBox("Заглавные буквы (A-Z)", settingsDialog);
        upperCheck->setChecked(true);
        upperCheck->setStyleSheet(checkboxStyle);
        
        QCheckBox* lowerCheck = new QCheckBox("Строчные буквы (a-z)", settingsDialog);
        lowerCheck->setChecked(true);
        lowerCheck->setStyleSheet(checkboxStyle);
        
        QCheckBox* digitsCheck = new QCheckBox("Цифры (0-9)", settingsDialog);
        digitsCheck->setChecked(true);
        digitsCheck->setStyleSheet(checkboxStyle);
        
        QCheckBox* specialCheck = new QCheckBox("Специальные символы (!@#$%^&*...)", settingsDialog);
        specialCheck->setChecked(true);
        specialCheck->setStyleSheet(checkboxStyle);
        
        // Buttons
        QPushButton* generateBtn = new QPushButton("Сгенерировать пароль", settingsDialog);
        generateBtn->setStyleSheet(
            "QPushButton { "
            "background-color: #007aff; "
            "color: white; "
            "border-radius: 6px; "
            "padding: 12px 20px; "
            "font-size: 14px; "
            "font-weight: 600; "
            "}"
            "QPushButton:hover { background-color: #0051d5; }"
        );
        
        QPushButton* cancelBtn = new QPushButton("Отмена", settingsDialog);
        cancelBtn->setStyleSheet(
            "QPushButton { "
            "background-color: #f5f5f7; "
            "color: #1d1d1f; "
            "border-radius: 6px; "
            "padding: 12px 20px; "
            "font-size: 14px; "
            "}"
            "QPushButton:hover { background-color: #e8e8ed; }"
        );
        
        connect(cancelBtn, &QPushButton::clicked, settingsDialog, &QDialog::reject);
        
        // Store values to be used after dialog closes
        int* resultLength = new int(16);
        bool* resultUpper = new bool(true);
        bool* resultLower = new bool(true);
        bool* resultDigits = new bool(true);
        bool* resultSpecial = new bool(true);
        bool* wasAccepted = new bool(false);
        
        connect(generateBtn, &QPushButton::clicked, settingsDialog, [=]() {
            int length = lengthSlider->value();
            bool useUpper = upperCheck->isChecked();
            bool useLower = lowerCheck->isChecked();
            bool useDigits = digitsCheck->isChecked();
            bool useSpecial = specialCheck->isChecked();
            
            if (!useUpper && !useLower && !useDigits && !useSpecial) {
                QMessageBox::warning(settingsDialog, "Ошибка", 
                    "Выберите хотя бы один тип символов!");
                return;
            }
            
            *resultLength = length;
            *resultUpper = useUpper;
            *resultLower = useLower;
            *resultDigits = useDigits;
            *resultSpecial = useSpecial;
            *wasAccepted = true;
            settingsDialog->accept();
        });
        
        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->addWidget(cancelBtn);
        btnLayout->addWidget(generateBtn);
        
        settingsLayout->addWidget(new QLabel("<b style='color: #000000;'>Настройки генератора паролей</b>", settingsDialog));
        settingsLayout->addSpacing(10);
        settingsLayout->addWidget(lengthLabel);
        settingsLayout->addWidget(lengthSlider);
        settingsLayout->addSpacing(15);
        settingsLayout->addWidget(new QLabel("<b style='color: #000000;'>Использовать символы:</b>", settingsDialog));
        settingsLayout->addWidget(upperCheck);
        settingsLayout->addWidget(lowerCheck);
        settingsLayout->addWidget(digitsCheck);
        settingsLayout->addWidget(specialCheck);
        settingsLayout->addSpacing(20);
        settingsLayout->addLayout(btnLayout);
        
        // Show dialog and wait for result
        settingsDialog->exec();
        
        // If user clicked generate, show result
        if (*wasAccepted) {
            try {
                std::string password = generateCustomPassword(*resultLength, *resultUpper, *resultLower, *resultDigits, *resultSpecial);
                
                // Show result dialog
                QDialog* resultDialog = new QDialog(this);
                resultDialog->setWindowTitle("Сгенерированный пароль");
                resultDialog->setMinimumWidth(450);
                resultDialog->setAttribute(Qt::WA_DeleteOnClose);
                resultDialog->setStyleSheet("QDialog { background-color: white; }");
                
                QVBoxLayout* resultLayout = new QVBoxLayout(resultDialog);
                
                QLabel* titleLabel = new QLabel("Ваш новый надёжный пароль:", resultDialog);
                titleLabel->setStyleSheet("font-size: 14px; color: #86868b; margin-bottom: 10px;");
                
                QLineEdit* passwordEdit = new QLineEdit(QString::fromStdString(password), resultDialog);
                passwordEdit->setReadOnly(true);
                passwordEdit->setStyleSheet(
                    "QLineEdit { "
                    "background-color: #f5f5f7; "
                    "border: 2px solid #007aff; "
                    "border-radius: 6px; "
                    "padding: 12px; "
                    "font-size: 16px; "
                    "font-family: monospace; "
                    "font-weight: bold; "
                    "color: #1d1d1f; "
                    "}"
                );
                
                // Show strength
                int strength = evaluatePasswordStrength(password);
                QString strengthText = QString("Оценка надёжности: %1/100").arg(strength);
                QLabel* strengthLabel = new QLabel(strengthText, resultDialog);
                QString strengthColor = strength >= 80 ? "#27ae60" : (strength >= 60 ? "#3498db" : "#f39c12");
                strengthLabel->setStyleSheet(QString("font-size: 13px; color: %1; font-weight: bold;").arg(strengthColor));
                
                QPushButton* copyBtn = new QPushButton("Копировать в буфер обмена", resultDialog);
                copyBtn->setStyleSheet(
                    "QPushButton { "
                    "background-color: #007aff; "
                    "color: white; "
                    "border-radius: 6px; "
                    "padding: 10px 20px; "
                    "font-size: 14px; "
                    "font-weight: 600; "
                    "}"
                    "QPushButton:hover { background-color: #0051d5; }"
                );
                
                connect(copyBtn, &QPushButton::clicked, copyBtn, [passwordEdit, copyBtn]() {
                    QApplication::clipboard()->setText(passwordEdit->text());
                    copyBtn->setText("✓ Скопировано!");
                    QTimer::singleShot(2000, copyBtn, [copyBtn]() {
                        copyBtn->setText("Копировать в буфер обмена");
                    });
                });
                
                QPushButton* closeBtn = new QPushButton("Закрыть", resultDialog);
                closeBtn->setStyleSheet(
                    "QPushButton { "
                    "background-color: #f5f5f7; "
                    "color: #1d1d1f; "
                    "border-radius: 6px; "
                    "padding: 10px 20px; "
                    "font-size: 14px; "
                    "}"
                    "QPushButton:hover { background-color: #e8e8ed; }"
                );
                connect(closeBtn, &QPushButton::clicked, resultDialog, &QDialog::accept);
                
                QHBoxLayout* btnLayout2 = new QHBoxLayout();
                btnLayout2->addWidget(copyBtn);
                btnLayout2->addWidget(closeBtn);
                
                resultLayout->addWidget(titleLabel);
                resultLayout->addWidget(passwordEdit);
                resultLayout->addWidget(strengthLabel);
                resultLayout->addSpacing(15);
                resultLayout->addLayout(btnLayout2);
                
                resultDialog->exec();
            } catch (const std::exception& e) {
                QMessageBox::critical(this, "Ошибка генерации", 
                    QString("Не удалось сгенерировать пароль: %1").arg(e.what()));
            } catch (...) {
                QMessageBox::critical(this, "Ошибка генерации", 
                    "Не удалось сгенерировать пароль. Неизвестная ошибка.");
            }
        }
        
        // Clean up
        delete resultLength;
        delete resultUpper;
        delete resultLower;
        delete resultDigits;
        delete resultSpecial;
        delete wasAccepted;
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Ошибка", 
            QString("Не удалось открыть генератор паролей: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Ошибка", 
            "Не удалось открыть генератор паролей. Неизвестная ошибка.");
    }
}

void MainWindow::onCheckStrengthClicked()
{
    try {
        // Prompt for password to check
        QDialog dialog(this);
        dialog.setWindowTitle("Проверка сложности пароля");
        dialog.setMinimumWidth(500);
        dialog.setMinimumHeight(400);
        
        QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QLabel* titleLabel = new QLabel("Введите пароль для проверки:");
    titleLabel->setStyleSheet("font-size: 14px; color: #86868b; margin-bottom: 5px;");
    
    QLineEdit* passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Введите пароль...");
    passwordEdit->setStyleSheet(
        "QLineEdit { "
        "background-color: white; "
        "border: 2px solid #d1d1d6; "
        "border-radius: 6px; "
        "padding: 10px; "
        "font-size: 14px; "
        "}"
        "QLineEdit:focus { border-color: #007aff; }"
    );
    
    QTextEdit* resultText = new QTextEdit();
    resultText->setReadOnly(true);
    resultText->setMinimumHeight(200);
    
    QPushButton* checkBtn = new QPushButton("Проверить");
    checkBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #007aff; "
        "color: white; "
        "border-radius: 6px; "
        "padding: 10px 20px; "
        "font-size: 14px; "
        "font-weight: 600; "
        "}"
        "QPushButton:hover { background-color: #0051d5; }"
    );
    
    connect(checkBtn, &QPushButton::clicked, [passwordEdit, resultText]() {
        try {
            QString password = passwordEdit->text();
            if (password.isEmpty()) {
                resultText->setHtml("<p style='color: #e74c3c; font-weight: bold;'>Введите пароль для оценки!</p>");
                return;
            }
            
            // Evaluate password strength
            int strength = evaluatePasswordStrength(password.toStdString());
            std::string description = getPasswordStrengthDescription(strength);
            std::string timeToCrack = estimateTimeToCrack(password.toStdString());
            
            // Determine color based on strength
            QString strengthColor;
            QString bgColor;
            if (strength < 30) {
                strengthColor = "#e74c3c"; // Red
                bgColor = "#fadbd8";
            } else if (strength < 50) {
                strengthColor = "#e67e22"; // Orange
                bgColor = "#fde3cf";
            } else if (strength < 70) {
                strengthColor = "#f39c12"; // Yellow
                bgColor = "#fef5e7";
            } else if (strength < 90) {
                strengthColor = "#3498db"; // Blue
                bgColor = "#d6eaf8";
            } else {
                strengthColor = "#27ae60"; // Green
                bgColor = "#d5f4e6";
            }
            
            QString result = QString("<div style='padding: 15px; background-color: %1; border-radius: 8px;'>").arg(bgColor);
            result += "<h3 style='margin: 0 0 10px 0; color: #2c3e50;'>Оценка сложности пароля</h3>";
            result += "<table style='width: 100%; border-collapse: collapse;'>";
            result += QString("<tr><td style='padding: 8px; font-weight: bold;'>Пароль:</td><td style='padding: 8px;'>%1</td></tr>").arg(password);
            result += QString("<tr><td style='padding: 8px; font-weight: bold;'>Оценка:</td><td style='padding: 8px;'><span style='color: %1; font-size: 18px; font-weight: bold;'>%2/100</span></td></tr>")
                .arg(strengthColor).arg(strength);
            result += QString("<tr><td style='padding: 8px; font-weight: bold;'>Уровень:</td><td style='padding: 8px; color: %1; font-weight: bold;'>%2</td></tr>")
                .arg(strengthColor).arg(QString::fromStdString(description));
            result += QString("<tr><td style='padding: 8px; font-weight: bold;'>Время взлома:</td><td style='padding: 8px;'>~%1</td></tr>")
                .arg(QString::fromStdString(timeToCrack));
            result += "</table>";
            
            if (isWeakPassword(password.toStdString())) {
                result += "<div style='margin-top: 15px; padding: 10px; background-color: #fadbd8; border-left: 4px solid #e74c3c; border-radius: 4px;'>";
                result += "<p style='margin: 0; color: #c0392b; font-weight: bold;'>ВНИМАНИЕ: Этот пароль найден в списке распространенных паролей!</p>";
                result += "<p style='margin: 5px 0 0 0; color: #666;'>Использовать такой пароль крайне не рекомендуется!</p>";
                result += "</div>";
            }
            
            result += "</div>";
            resultText->setHtml(result);
            
        } catch (const std::exception& e) {
            resultText->setHtml(QString("<p style='color: #e74c3c; font-weight: bold;'>Ошибка при проверке: %1</p>").arg(e.what()));
        } catch (...) {
            resultText->setHtml("<p style='color: #e74c3c; font-weight: bold;'>Неизвестная ошибка при проверке пароля</p>");
        }
    });
    
    QPushButton* closeBtn = new QPushButton("Закрыть");
    closeBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #f5f5f7; "
        "color: #1d1d1f; "
        "border-radius: 6px; "
        "padding: 10px 20px; "
        "font-size: 14px; "
        "}"
        "QPushButton:hover { background-color: #e8e8ed; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(checkBtn);
    btnLayout->addWidget(closeBtn);
    
    layout->addWidget(titleLabel);
    layout->addWidget(passwordEdit);
    layout->addSpacing(10);
    layout->addWidget(resultText);
    layout->addSpacing(10);
    layout->addLayout(btnLayout);
    
    dialog.exec();
    
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Ошибка", 
            QString("Не удалось открыть проверку сложности: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Ошибка", 
            "Не удалось открыть проверку сложности. Неизвестная ошибка.");
    }
}
