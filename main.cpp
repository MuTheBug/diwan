#include "mainwindow.h"
#include "logindialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set global layout direction to Right-To-Left for Arabic
    a.setLayoutDirection(Qt::RightToLeft);

    // Apply a simple modern stylesheet
    a.setStyleSheet(
        "QMainWindow { background-color: #ecf0f1; }"
        "QGroupBox { font-weight: bold; font-size: 16px; border: 1px solid #bdc3c7; border-radius: 5px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }"
        "QLineEdit, QDateEdit, QComboBox { padding: 5px; border: 1px solid #bdc3c7; border-radius: 3px; }"
        "QPushButton { padding: 5px; border-radius: 3px; }"
        "QTableWidget { background-color: white; alternate-background-color: #f9f9f9; }"
        "QHeaderView::section { background-color: #34495e; color: white; padding: 5px; font-weight: bold; }"
    );

    // Show login dialog first
    LoginDialog login;
    if (login.exec() == QDialog::Accepted) {
        MainWindow w;
        w.show();
        return a.exec();
    }

    return 0; // Exit if login was cancelled or failed
}
