#include "logindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QApplication>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent), isFirstLaunch(false)
{
    QSettings settings("Haqna", "DiwanApp");
    isFirstLaunch = settings.value("app_password").toString().isEmpty();

    setupUi();
}

LoginDialog::~LoginDialog()
{
}

void LoginDialog::setupUi()
{
    this->setWindowTitle(isFirstLaunch ? "إعداد كلمة المرور" : "تسجيل الدخول");
    this->setFixedSize(400, 250);
    this->setLayoutDirection(Qt::RightToLeft);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Logo layout
    QHBoxLayout *logoLayout = new QHBoxLayout();
    QLabel *logoLabel = new QLabel(this);
    QPixmap logoPixmap(":/logo.jpg");
    logoLabel->setPixmap(logoPixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLayout->addStretch();
    logoLayout->addWidget(logoLabel);
    logoLayout->addStretch();
    mainLayout->addLayout(logoLayout);

    QLabel *instructionLabel = new QLabel(isFirstLaunch ?
        "مرحباً بك في برنامج الديوان.\nيرجى إعداد كلمة مرور جديدة لحماية النظام:" :
        "يرجى إدخال كلمة المرور للوصول إلى نظام الأرشفة:", this);
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setStyleSheet("font-size: 14px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(instructionLabel);

    passwordLineEdit = new QLineEdit(this);
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    passwordLineEdit->setPlaceholderText("كلمة المرور...");
    passwordLineEdit->setStyleSheet("padding: 10px; font-size: 16px; border: 1px solid #bdc3c7; border-radius: 5px;");
    mainLayout->addWidget(passwordLineEdit);

    loginButton = new QPushButton(isFirstLaunch ? "حفظ ودخول" : "دخول", this);
    loginButton->setMinimumHeight(40);
    loginButton->setStyleSheet("background-color: #2980b9; color: white; font-weight: bold; font-size: 16px; border-radius: 5px;");
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginButtonClicked);
    connect(passwordLineEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginButtonClicked);

    mainLayout->addSpacing(10);
    mainLayout->addWidget(loginButton);
    mainLayout->addStretch();
}

void LoginDialog::onLoginButtonClicked()
{
    QString enteredPassword = passwordLineEdit->text();

    if (enteredPassword.isEmpty()) {
        QMessageBox::warning(this, "تنبيه", "يرجى إدخال كلمة المرور.");
        return;
    }

    if (isFirstLaunch) {
        createInitialPassword(enteredPassword);
        QMessageBox::information(this, "نجاح", "تم حفظ كلمة المرور بنجاح.");
        accept();
    } else {
        if (checkPassword(enteredPassword)) {
            accept();
        } else {
            QMessageBox::critical(this, "خطأ", "كلمة المرور غير صحيحة!");
            passwordLineEdit->clear();
        }
    }
}

void LoginDialog::createInitialPassword(const QString& password)
{
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    QSettings settings("Haqna", "DiwanApp");
    settings.setValue("app_password", hash.toHex());
}

bool LoginDialog::checkPassword(const QString& password)
{
    QByteArray inputHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    QSettings settings("Haqna", "DiwanApp");
    QString storedHash = settings.value("app_password").toString();

    return inputHash.toHex() == storedHash;
}
