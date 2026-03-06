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
#include <QTabWidget>
#include <QCheckBox>

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
    void onEditButtonClicked();
    void onSearchFilterChanged();
    void onExportDatabase();
    void onImportDatabase();
    void onAboutApp();
    void onGenerateReportClicked();

private:
    void setupUi();
    void setupDashboardTab(QWidget *tab);
    void setupArchiveTab(QWidget *tab);
    void setupSettingsTab(QWidget *tab);
    void setupMenu();
    void clearForm();
    QString savePdfToArchive(const QString& sourcePath);
    void updateDashboardStats();

    // UI Elements
    QTabWidget *tabWidget;

    // Dashboard Tab
    QLabel *statTotalLabel;
    QLabel *statIncomingLabel;
    QLabel *statOutgoingLabel;
    QTableWidget *pendingTableWidget;
    QPushButton *viewPendingPdfButton;

    // Archive Tab
    QTableWidget *tableWidget;
    QLineEdit *searchLineEdit;
    QComboBox *filterTypeComboBox;
    QDateEdit *filterStartDate;
    QDateEdit *filterEndDate;
    QCheckBox *filterDateEnabled;

    QPushButton *deleteButton;
    QPushButton *editButton;
    QPushButton *reportButton;
    QPushButton *viewPdfButton;

    QComboBox *typeComboBox;
    QLineEdit *numberLineEdit;
    QDateEdit *dateEdit;
    QLineEdit *subjectLineEdit;
    QLineEdit *correspondentLineEdit;
    QLabel *correspondentLabel;
    QCheckBox *followUpCheckBox;

    QLineEdit *fileLineEdit;
    QPushButton *selectFileButton;
    QPushButton *addButton;

    // Settings Tab
    QLineEdit *archivePathLineEdit;
    QPushButton *changePathButton;
    QPushButton *changePasswordButton;

    QString currentSelectedFilePath;
    QString archiveDir;
};

#endif // MAINWINDOW_H
