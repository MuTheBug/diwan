#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QAction>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddButtonClicked();
    void onSelectFileButtonClicked();
    void onViewPdfButtonClicked();
    void onTypeChanged(const QString &type);
    void refreshTable();

    // New Features
    void onDeleteButtonClicked();
    void onSearchTextChanged(const QString &text);
    void onExportDatabase();
    void onImportDatabase();
    void onAboutApp();

private:
    void setupUi();
    void setupMenu();
    void clearForm();
    QString savePdfToArchive(const QString& sourcePath);

    // UI Elements
    QTableWidget *tableWidget;
    QLineEdit *searchLineEdit;
    QPushButton *deleteButton;

    QComboBox *typeComboBox;
    QLineEdit *numberLineEdit;
    QDateEdit *dateEdit;
    QLineEdit *subjectLineEdit;
    QLineEdit *correspondentLineEdit;
    QLabel *correspondentLabel;

    QLineEdit *fileLineEdit;
    QPushButton *selectFileButton;

    QPushButton *addButton;
    QPushButton *viewPdfButton;

    QString currentSelectedFilePath;
    QString archiveDir;
};

#endif // MAINWINDOW_H
