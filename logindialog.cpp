#include "logindialog.h"
#include "database.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QMessageBox>
#include <QApplication>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent), isFirstLaunch(false)
{
    Database::instance().initialize();

    // Check if we need to migrate the legacy settings password to a user account
    QSettings settings("Haqna", "DiwanApp");
    QString legacyPass = settings.value("app_password").toString();

    if (!legacyPass.isEmpty() && !Database::instance().hasUsers()) {
        // Create an admin user with the legacy password hash
        QSqlQuery query;
        query.prepare("INSERT INTO users (username, password_hash, role) VALUES ('admin', :hash, 'admin')");
        query.bindValue(":hash", legacyPass);
        query.exec();
        settings.remove("app_password"); // Clear it so we don't do this again
    }

    isFirstLaunch = !Database::instance().hasUsers();

    setupUi();
}

LoginDialog::~LoginDialog()
{
}

void LoginDialog::setupUi()
{
    this->setWindowTitle(isFirstLaunch ? "إعداد حساب المسؤول" : "تسجيل الدخول");
    this->setFixedSize(400, 300);
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
        "مرحباً بك.\nيرجى إنشاء اسم مستخدم وكلمة مرور لمدير النظام:" :
        "يرجى إدخال بيانات الدخول للوصول إلى نظام الأرشفة:", this);
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setStyleSheet("font-size: 14px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(instructionLabel);

    usernameLineEdit = new QLineEdit(this);
    usernameLineEdit->setPlaceholderText(isFirstLaunch ? "اسم المستخدم (المدير)..." : "اسم المستخدم...");
    usernameLineEdit->setStyleSheet("padding: 10px; font-size: 16px; border: 1px solid #bdc3c7; border-radius: 5px;");
    mainLayout->addWidget(usernameLineEdit);

    passwordLineEdit = new QLineEdit(this);
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    passwordLineEdit->setPlaceholderText("كلمة المرور...");
    passwordLineEdit->setStyleSheet("padding: 10px; font-size: 16px; border: 1px solid #bdc3c7; border-radius: 5px;");
    mainLayout->addWidget(passwordLineEdit);

    loginButton = new QPushButton(isFirstLaunch ? "حفظ وإنشاء حساب" : "دخول", this);
    loginButton->setMinimumHeight(40);
    loginButton->setStyleSheet("background-color: #2980b9; color: white; font-weight: bold; font-size: 16px; border-radius: 5px;");
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginButtonClicked);
    connect(passwordLineEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginButtonClicked);
    connect(usernameLineEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginButtonClicked);

    mainLayout->addSpacing(10);
    mainLayout->addWidget(loginButton);
    mainLayout->addStretch();
}

void LoginDialog::onLoginButtonClicked()
{
    QString username = usernameLineEdit->text().trimmed();
    QString password = passwordLineEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "تنبيه", "يرجى إدخال اسم المستخدم وكلمة المرور.");
        return;
    }

    if (isFirstLaunch) {
        if (Database::instance().addUser(username, password, "admin")) {
            QMessageBox::information(this, "نجاح", "تم إنشاء الحساب بنجاح. يرجى تسجيل الدخول الآن.");
            isFirstLaunch = false;
            instructionLabelUpdate();
        } else {
            QMessageBox::critical(this, "خطأ", "فشل في إنشاء الحساب!");
        }
    } else {
        if (Database::instance().authenticate(username, password)) {
            accept();
        } else {
            QMessageBox::critical(this, "خطأ", "بيانات الدخول غير صحيحة!");
            passwordLineEdit->clear();
        }
    }
}

void LoginDialog::instructionLabelUpdate() {
    this->setWindowTitle("تسجيل الدخول");
    loginButton->setText("دخول");
    passwordLineEdit->clear();
    // Rebuild UI or rely on current state for next login loop
}
