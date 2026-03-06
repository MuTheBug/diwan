#include "mainwindow.h"
#include "database.h"

#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QGroupBox>
#include <QFormLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Setup archive directory
    archiveDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Haqna_Archive";
    QDir dir(archiveDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    setupUi();

    // Ensure database is initialized
    if (!Database::instance().initialize()) {
        QMessageBox::critical(this, "خطأ", "فشل في الاتصال بقاعدة البيانات!");
    }

    refreshTable();
    onTypeChanged(typeComboBox->currentText()); // Initialize fields correctly
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    this->setWindowTitle("برنامج ديوان - جمعية حقنا");
    this->setMinimumSize(900, 600);
    this->setLayoutDirection(Qt::RightToLeft);

    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Title label
    QLabel *titleLabel = new QLabel("نظام الأرشفة الإلكتروني - ديوان جمعية حقنا", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2c3e50; margin: 10px;");
    mainLayout->addWidget(titleLabel);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    mainLayout->addLayout(contentLayout);

    // --- Right Side: Form ---
    QGroupBox *formGroupBox = new QGroupBox("إضافة معاملة جديدة", this);
    formGroupBox->setMaximumWidth(350);
    QVBoxLayout *formLayout = new QVBoxLayout(formGroupBox);

    typeComboBox = new QComboBox(this);
    typeComboBox->addItems({"وارد", "صادر"});
    connect(typeComboBox, &QComboBox::currentTextChanged, this, &MainWindow::onTypeChanged);

    numberLineEdit = new QLineEdit(this);
    numberLineEdit->setReadOnly(true);
    numberLineEdit->setPlaceholderText("تلقائي...");

    dateEdit = new QDateEdit(QDate::currentDate(), this);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");

    subjectLineEdit = new QLineEdit(this);

    correspondentLabel = new QLabel("الجهة المرسلة:", this);
    correspondentLineEdit = new QLineEdit(this);

    QHBoxLayout *fileLayout = new QHBoxLayout();
    fileLineEdit = new QLineEdit(this);
    fileLineEdit->setReadOnly(true);
    selectFileButton = new QPushButton("اختيار PDF...", this);
    connect(selectFileButton, &QPushButton::clicked, this, &MainWindow::onSelectFileButtonClicked);
    fileLayout->addWidget(fileLineEdit);
    fileLayout->addWidget(selectFileButton);

    QFormLayout *innerFormLayout = new QFormLayout();
    innerFormLayout->addRow("النوع:", typeComboBox);
    innerFormLayout->addRow("الرقم:", numberLineEdit);
    innerFormLayout->addRow("التاريخ:", dateEdit);
    innerFormLayout->addRow("الموضوع:", subjectLineEdit);
    innerFormLayout->addRow(correspondentLabel, correspondentLineEdit);
    innerFormLayout->addRow("ملف PDF:", fileLayout);

    formLayout->addLayout(innerFormLayout);

    addButton = new QPushButton("إضافة المعاملة وحفظ الأرشيف", this);
    addButton->setMinimumHeight(40);
    addButton->setStyleSheet("background-color: #27ae60; color: white; font-weight: bold; font-size: 14px; border-radius: 5px;");
    connect(addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    formLayout->addWidget(addButton);

    formLayout->addStretch();
    contentLayout->addWidget(formGroupBox);

    // --- Left Side: Table & View ---
    QVBoxLayout *tableLayout = new QVBoxLayout();

    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(7);
    tableWidget->setHorizontalHeaderLabels({"المعرف", "النوع", "الرقم", "التاريخ", "الموضوع", "الجهة", "مسار الملف"});
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->hideColumn(0); // Hide ID
    tableWidget->hideColumn(6); // Hide file path

    viewPdfButton = new QPushButton("عرض ملف PDF المحدد", this);
    viewPdfButton->setMinimumHeight(40);
    viewPdfButton->setStyleSheet("background-color: #2980b9; color: white; font-weight: bold; font-size: 14px; border-radius: 5px;");
    connect(viewPdfButton, &QPushButton::clicked, this, &MainWindow::onViewPdfButtonClicked);

    tableLayout->addWidget(tableWidget);
    tableLayout->addWidget(viewPdfButton);

    contentLayout->addLayout(tableLayout);
}

void MainWindow::onTypeChanged(const QString &type)
{
    if (type == "وارد") {
        correspondentLabel->setText("الجهة المرسلة:");
    } else {
        correspondentLabel->setText("الجهة الموجه إليها:");
    }
    numberLineEdit->setText(Database::instance().generateNextNumber(type));
}

void MainWindow::onSelectFileButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "اختر ملف PDF", "", "PDF Files (*.pdf)");
    if (!fileName.isEmpty()) {
        currentSelectedFilePath = fileName;
        fileLineEdit->setText(QFileInfo(fileName).fileName());
    }
}

QString MainWindow::savePdfToArchive(const QString& sourcePath)
{
    QFileInfo fileInfo(sourcePath);
    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString typePrefix = (typeComboBox->currentText() == "وارد") ? "IN_" : "OUT_";
    QString destFileName = typePrefix + timeStamp + "_" + fileInfo.fileName();
    QString destPath = archiveDir + "/" + destFileName;

    if (QFile::copy(sourcePath, destPath)) {
        return destPath;
    }
    return "";
}

void MainWindow::onAddButtonClicked()
{
    if (subjectLineEdit->text().isEmpty() || correspondentLineEdit->text().isEmpty()) {
        QMessageBox::warning(this, "تنبيه", "يرجى تعبئة جميع الحقول المطلوبة.");
        return;
    }

    if (currentSelectedFilePath.isEmpty()) {
        QMessageBox::warning(this, "تنبيه", "يرجى اختيار ملف PDF للأرشفة.");
        return;
    }

    QString savedFilePath = savePdfToArchive(currentSelectedFilePath);
    if (savedFilePath.isEmpty()) {
        QMessageBox::critical(this, "خطأ", "فشل في نسخ ملف PDF إلى مجلد الأرشيف.");
        return;
    }

    DocumentRecord record;
    record.type = typeComboBox->currentText();
    record.docNumber = numberLineEdit->text();
    record.date = dateEdit->date().toString("yyyy-MM-dd");
    record.subject = subjectLineEdit->text();
    record.correspondent = correspondentLineEdit->text();
    record.filePath = savedFilePath;

    if (Database::instance().addRecord(record)) {
        QMessageBox::information(this, "نجاح", "تم حفظ المعاملة والأرشفة بنجاح.");
        clearForm();
        refreshTable();
    } else {
        QMessageBox::critical(this, "خطأ", "فشل في حفظ المعاملة في قاعدة البيانات.");
    }
}

void MainWindow::clearForm()
{
    subjectLineEdit->clear();
    correspondentLineEdit->clear();
    currentSelectedFilePath.clear();
    fileLineEdit->clear();
    dateEdit->setDate(QDate::currentDate());
    onTypeChanged(typeComboBox->currentText()); // Update the auto-number
}

void MainWindow::refreshTable()
{
    tableWidget->setRowCount(0);
    QList<DocumentRecord> records = Database::instance().getAllRecords();

    for (int i = 0; i < records.size(); ++i) {
        tableWidget->insertRow(i);
        tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(records[i].id)));
        tableWidget->setItem(i, 1, new QTableWidgetItem(records[i].type));
        tableWidget->setItem(i, 2, new QTableWidgetItem(records[i].docNumber));
        tableWidget->setItem(i, 3, new QTableWidgetItem(records[i].date));
        tableWidget->setItem(i, 4, new QTableWidgetItem(records[i].subject));
        tableWidget->setItem(i, 5, new QTableWidgetItem(records[i].correspondent));
        tableWidget->setItem(i, 6, new QTableWidgetItem(records[i].filePath));
    }
}

void MainWindow::onViewPdfButtonClicked()
{
    int currentRow = tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "تنبيه", "يرجى تحديد معاملة من الجدول لعرضها.");
        return;
    }

    QString filePath = tableWidget->item(currentRow, 6)->text();
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    } else {
        QMessageBox::warning(this, "خطأ", "لم يتم العثور على الملف المطلوب. ربما تم حذفه.");
    }
}
