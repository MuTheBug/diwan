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
#include <QPrinter>
#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QSettings>
#include <QBuffer>
#include <QCryptographicHash>
#include <QInputDialog>
#include "editdialog.h"
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Setup archive directory
    QSettings settings("Haqna", "DiwanApp");
    archiveDir = settings.value("archive_path", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Haqna_Archive").toString();
    QDir dir(archiveDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    setupUi();
    setupMenu();

    // Ensure database is initialized
    if (!Database::instance().initialize()) {
        QMessageBox::critical(this, "خطأ", "فشل في الاتصال بقاعدة البيانات!");
    }

    refreshTable();
    onTypeChanged(typeComboBox->currentText()); // Initialize fields correctly

    // Check for follow-up notifications upon login
    QTimer::singleShot(500, this, &MainWindow::checkFollowUpNotifications);
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

    // Header Layout for Logo and Title
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: white; border-bottom: 2px solid #bdc3c7; border-radius: 5px;");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(10, 10, 10, 10);

    // Logo label
    QLabel *logoLabel = new QLabel(this);
    QPixmap logoPixmap(":/logo.jpg");
    logoLabel->setPixmap(logoPixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);

    // Title label
    QLabel *titleLabel = new QLabel("نظام الأرشفة الإلكتروني - ديوان جمعية حقنا", this);
    titleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    titleLabel->setStyleSheet("font-size: 26px; font-weight: bold; color: #2c3e50; border: none;");

    headerLayout->addWidget(logoLabel);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    mainLayout->addWidget(headerWidget);

    tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet("QTabBar::tab { padding: 8px 15px; font-weight: bold; font-size: 12px; }");

    QWidget *dashboardTab = new QWidget();
    setupDashboardTab(dashboardTab);

    QWidget *archiveTab = new QWidget();
    setupArchiveTab(archiveTab);

    QWidget *settingsTab = new QWidget();
    setupSettingsTab(settingsTab);

    tabWidget->addTab(dashboardTab, "الرئيسية (الإحصائيات)");
    tabWidget->addTab(archiveTab, "الأرشيف و المعاملات");
    tabWidget->addTab(settingsTab, "إعدادات النظام");

    mainLayout->addWidget(tabWidget);
}

void MainWindow::setupDashboardTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);

    // Stats layout
    QHBoxLayout *statsLayout = new QHBoxLayout();

    QGroupBox *totalGroup = new QGroupBox("إجمالي المعاملات");
    QVBoxLayout *l1 = new QVBoxLayout(totalGroup);
    statTotalLabel = new QLabel("0");
    statTotalLabel->setAlignment(Qt::AlignCenter);
    statTotalLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #34495e;");
    l1->addWidget(statTotalLabel);

    QGroupBox *incomingGroup = new QGroupBox("البريد الوارد");
    QVBoxLayout *l2 = new QVBoxLayout(incomingGroup);
    statIncomingLabel = new QLabel("0");
    statIncomingLabel->setAlignment(Qt::AlignCenter);
    statIncomingLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #27ae60;");
    l2->addWidget(statIncomingLabel);

    QGroupBox *outgoingGroup = new QGroupBox("البريد الصادر");
    QVBoxLayout *l3 = new QVBoxLayout(outgoingGroup);
    statOutgoingLabel = new QLabel("0");
    statOutgoingLabel->setAlignment(Qt::AlignCenter);
    statOutgoingLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #2980b9;");
    l3->addWidget(statOutgoingLabel);

    statsLayout->addWidget(totalGroup);
    statsLayout->addWidget(incomingGroup);
    statsLayout->addWidget(outgoingGroup);
    layout->addLayout(statsLayout);

    // Pending Follow-ups table
    QLabel *pendingLabel = new QLabel("المعاملات التي تحتاج لمتابعة:");
    pendingLabel->setStyleSheet("font-weight: bold; font-size: 16px; margin-top: 15px;");
    layout->addWidget(pendingLabel);

    pendingTableWidget = new QTableWidget(this);
    pendingTableWidget->setColumnCount(7);
    pendingTableWidget->setHorizontalHeaderLabels({"المعرف", "النوع", "الرقم", "التاريخ", "الموضوع", "الجهة", "المرفقات"});
    pendingTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    pendingTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    pendingTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    pendingTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pendingTableWidget->hideColumn(0);
    pendingTableWidget->hideColumn(6);
    layout->addWidget(pendingTableWidget);

    viewPendingPdfButton = new QPushButton("عرض المرفقات", this);
    viewPendingPdfButton->setStyleSheet("background-color: #2980b9; color: white; font-weight: bold; font-size: 14px; border-radius: 5px; padding: 10px;");
    connect(viewPendingPdfButton, &QPushButton::clicked, [this]() {
        int r = pendingTableWidget->currentRow();
        if(r >= 0) {
            QString pathsStr = pendingTableWidget->item(r, 6)->text();
            QStringList paths = pathsStr.split(";", Qt::SkipEmptyParts);
            for (const QString& p : paths) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(p));
            }
            if (paths.isEmpty()) QMessageBox::information(this, "معلومة", "لا توجد مرفقات.");
        } else {
            QMessageBox::warning(this, "تنبيه", "حدد معاملة أولاً.");
        }
    });
    layout->addWidget(viewPendingPdfButton);
}

void MainWindow::setupArchiveTab(QWidget *tab)
{
    QHBoxLayout *contentLayout = new QHBoxLayout(tab);

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

    followUpCheckBox = new QCheckBox("يحتاج لمتابعة؟", this);

    attachmentsList = new QListWidget(this);
    attachmentsList->setMaximumHeight(80);

    QHBoxLayout *attachBtns = new QHBoxLayout();
    addFileButton = new QPushButton("+ إرفاق", this);
    removeFileButton = new QPushButton("- إزالة", this);
    attachBtns->addWidget(addFileButton);
    attachBtns->addWidget(removeFileButton);

    connect(addFileButton, &QPushButton::clicked, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, "اختر المرفقات", "", "Files (*.*)");
        for (const QString& file : files) {
            currentAttachmentsPaths.append(file);
            attachmentsList->addItem(QFileInfo(file).fileName());
        }
    });

    connect(removeFileButton, &QPushButton::clicked, [this]() {
        int r = attachmentsList->currentRow();
        if (r >= 0) {
            currentAttachmentsPaths.removeAt(r);
            delete attachmentsList->takeItem(r);
        }
    });

    QFormLayout *innerFormLayout = new QFormLayout();
    innerFormLayout->addRow("النوع:", typeComboBox);
    innerFormLayout->addRow("الرقم:", numberLineEdit);
    innerFormLayout->addRow("التاريخ:", dateEdit);
    innerFormLayout->addRow("الموضوع:", subjectLineEdit);
    innerFormLayout->addRow(correspondentLabel, correspondentLineEdit);
    innerFormLayout->addRow("متابعة:", followUpCheckBox);
    innerFormLayout->addRow("المرفقات:", attachmentsList);
    innerFormLayout->addRow("", attachBtns);

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

    QGroupBox *filterGroup = new QGroupBox("تصفية متقدمة", this);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterGroup);

    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("ابحث في الموضوع/الجهة/الرقم...");
    connect(searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchFilterChanged);

    filterTypeComboBox = new QComboBox(this);
    filterTypeComboBox->addItems({"الكل", "وارد", "صادر"});
    connect(filterTypeComboBox, &QComboBox::currentTextChanged, this, &MainWindow::onSearchFilterChanged);

    filterDateEnabled = new QCheckBox("تاريخ من:", this);
    connect(filterDateEnabled, &QCheckBox::toggled, this, &MainWindow::onSearchFilterChanged);
    filterStartDate = new QDateEdit(QDate::currentDate().addDays(-30), this);
    filterStartDate->setDisplayFormat("yyyy-MM-dd");
    filterStartDate->setCalendarPopup(true);
    connect(filterStartDate, &QDateEdit::dateChanged, this, &MainWindow::onSearchFilterChanged);

    QLabel *toLabel = new QLabel("إلى:");
    filterEndDate = new QDateEdit(QDate::currentDate(), this);
    filterEndDate->setDisplayFormat("yyyy-MM-dd");
    filterEndDate->setCalendarPopup(true);
    connect(filterEndDate, &QDateEdit::dateChanged, this, &MainWindow::onSearchFilterChanged);

    filterLayout->addWidget(new QLabel("البحث:"));
    filterLayout->addWidget(searchLineEdit);
    filterLayout->addWidget(new QLabel("النوع:"));
    filterLayout->addWidget(filterTypeComboBox);
    filterLayout->addWidget(filterDateEnabled);
    filterLayout->addWidget(filterStartDate);
    filterLayout->addWidget(toLabel);
    filterLayout->addWidget(filterEndDate);

    tableLayout->addWidget(filterGroup);

    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(7);
    tableWidget->setHorizontalHeaderLabels({"المعرف", "النوع", "الرقم", "التاريخ", "الموضوع", "الجهة", "المرفقات"});
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->hideColumn(0); // Hide ID
    tableWidget->hideColumn(6); // Hide file path

    QHBoxLayout *actionButtonsLayout = new QHBoxLayout();

    viewPdfButton = new QPushButton("عرض المرفقات", this);
    viewPdfButton->setStyleSheet("background-color: #2980b9; color: white; padding: 10px; border-radius: 5px;");
    connect(viewPdfButton, &QPushButton::clicked, this, &MainWindow::onViewPdfButtonClicked);

    editButton = new QPushButton("تعديل القيد", this);
    editButton->setStyleSheet("background-color: #f39c12; color: white; padding: 10px; border-radius: 5px;");
    connect(editButton, &QPushButton::clicked, this, &MainWindow::onEditButtonClicked);

    deleteButton = new QPushButton("حذف القيد", this);
    deleteButton->setStyleSheet("background-color: #c0392b; color: white; padding: 10px; border-radius: 5px;");
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteButtonClicked);

    reportButton = new QPushButton("توليد تقرير (PDF)", this);
    reportButton->setStyleSheet("background-color: #8e44ad; color: white; padding: 10px; border-radius: 5px;");
    connect(reportButton, &QPushButton::clicked, this, &MainWindow::onGenerateReportClicked);

    actionButtonsLayout->addWidget(viewPdfButton);
    actionButtonsLayout->addWidget(editButton);
    actionButtonsLayout->addWidget(reportButton);
    actionButtonsLayout->addWidget(deleteButton);

    tableLayout->addWidget(tableWidget);
    tableLayout->addLayout(actionButtonsLayout);

    contentLayout->addLayout(tableLayout);
}

void MainWindow::setupSettingsTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);
    QFormLayout *formLayout = new QFormLayout();

    QHBoxLayout *pathLayout = new QHBoxLayout();
    archivePathLineEdit = new QLineEdit(archiveDir, this);
    archivePathLineEdit->setReadOnly(true);
    changePathButton = new QPushButton("تغيير المسار", this);
    pathLayout->addWidget(archivePathLineEdit);
    pathLayout->addWidget(changePathButton);
    formLayout->addRow("مسار مجلد الأرشيف (PDFs):", pathLayout);

    connect(changePathButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "اختر مجلد الأرشيف الجديد", archiveDir);
        if(!dir.isEmpty()) {
            archiveDir = dir;
            archivePathLineEdit->setText(dir);
            QSettings settings("Haqna", "DiwanApp");
            settings.setValue("archive_path", dir);
            QMessageBox::information(this, "نجاح", "تم تحديث مسار الحفظ بنجاح.");
        }
    });

    layout->addLayout(formLayout);
    layout->addStretch();
}

void MainWindow::setupMenu()
{
    QMenuBar *menuBar = this->menuBar();

    QMenu *fileMenu = menuBar->addMenu("ملف");
    QAction *exportAction = fileMenu->addAction("تصدير قاعدة البيانات");
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportDatabase);

    QAction *importAction = fileMenu->addAction("استيراد قاعدة البيانات");
    connect(importAction, &QAction::triggered, this, &MainWindow::onImportDatabase);

    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("خروج");
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);

    QMenu *helpMenu = menuBar->addMenu("مساعدة");
    QAction *aboutAction = helpMenu->addAction("حول البرنامج");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAboutApp);
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

    QStringList savedPaths;
    for (const QString& path : currentAttachmentsPaths) {
        QString saved = savePdfToArchive(path);
        if (!saved.isEmpty()) {
            savedPaths.append(saved);
        }
    }

    DocumentRecord record;
    record.type = typeComboBox->currentText();
    record.docNumber = numberLineEdit->text();
    record.date = dateEdit->date().toString("yyyy-MM-dd");
    record.subject = subjectLineEdit->text();
    record.correspondent = correspondentLineEdit->text();
    record.attachments = savedPaths;
    record.needsFollowUp = followUpCheckBox->isChecked();

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
    currentAttachmentsPaths.clear();
    attachmentsList->clear();
    dateEdit->setDate(QDate::currentDate());
    followUpCheckBox->setChecked(false);
    onTypeChanged(typeComboBox->currentText()); // Update the auto-number
}

void MainWindow::updateDashboardStats()
{
    QList<DocumentRecord> records = Database::instance().getAllRecords();

    int total = records.size();
    int incoming = 0;
    int outgoing = 0;

    pendingTableWidget->setRowCount(0);
    int pendingRow = 0;

    for (const auto& r : records) {
        if (r.type == "وارد") incoming++;
        else if (r.type == "صادر") outgoing++;

        if (r.needsFollowUp) {
            pendingTableWidget->insertRow(pendingRow);
            pendingTableWidget->setItem(pendingRow, 0, new QTableWidgetItem(QString::number(r.id)));
            pendingTableWidget->setItem(pendingRow, 1, new QTableWidgetItem(r.type));
            pendingTableWidget->setItem(pendingRow, 2, new QTableWidgetItem(r.docNumber));
            pendingTableWidget->setItem(pendingRow, 3, new QTableWidgetItem(r.date));
            pendingTableWidget->setItem(pendingRow, 4, new QTableWidgetItem(r.subject));
            pendingTableWidget->setItem(pendingRow, 5, new QTableWidgetItem(r.correspondent));
            pendingTableWidget->setItem(pendingRow, 6, new QTableWidgetItem(r.attachments.join(";")));
            pendingRow++;
        }
    }

    statTotalLabel->setText(QString::number(total));
    statIncomingLabel->setText(QString::number(incoming));
    statOutgoingLabel->setText(QString::number(outgoing));
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
        tableWidget->setItem(i, 6, new QTableWidgetItem(records[i].attachments.join(";")));

        if (records[i].needsFollowUp) {
            for (int col = 1; col <= 5; ++col) {
                tableWidget->item(i, col)->setBackground(QBrush(QColor("#fcf3cf"))); // Light yellow for follow-ups
            }
        }
    }

    onSearchFilterChanged(); // Re-apply filters
    updateDashboardStats();
}

void MainWindow::checkFollowUpNotifications()
{
    QList<DocumentRecord> records = Database::instance().getAllRecords();
    int pendingCount = 0;
    QDate today = QDate::currentDate();

    for (const auto& r : records) {
        if (r.needsFollowUp) {
            QDate docDate = QDate::fromString(r.date, "yyyy-MM-dd");
            if (docDate.daysTo(today) > 3) {
                pendingCount++;
            }
        }
    }

    if (pendingCount > 0) {
        QMessageBox::information(this, "تنبيه المعاملات المعلقة",
                                 QString("يوجد لديك %1 معاملة تحتاج لمتابعة وتأخرت لأكثر من 3 أيام!\nيرجى مراجعة لوحة الإحصائيات (الرئيسية).").arg(pendingCount));
    }
}

void MainWindow::onViewPdfButtonClicked()
{
    int currentRow = tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "تنبيه", "يرجى تحديد معاملة من الجدول لعرضها.");
        return;
    }

    QString pathsStr = tableWidget->item(currentRow, 6)->text();
    QStringList paths = pathsStr.split(";", Qt::SkipEmptyParts);

    if (paths.isEmpty()) {
        QMessageBox::information(this, "معلومة", "لا توجد مرفقات لهذه المعاملة.");
        return;
    }

    bool anyOpened = false;
    for (const QString& path : paths) {
        if (QFile::exists(path)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            anyOpened = true;
        }
    }

    if (!anyOpened) {
        QMessageBox::warning(this, "خطأ", "لم يتم العثور على أي من الملفات المطلوبة. ربما تم حذفها.");
    }
}

void MainWindow::onDeleteButtonClicked()
{
    int currentRow = tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "تنبيه", "يرجى تحديد معاملة من الجدول لحذفها.");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "تأكيد الحذف", "هل أنت متأكد من حذف هذه المعاملة؟\nلن يتم حذف ملف الـ PDF من الأرشيف.",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        int id = tableWidget->item(currentRow, 0)->text().toInt();
        if (Database::instance().deleteRecord(id)) {
            refreshTable();
            QMessageBox::information(this, "نجاح", "تم حذف المعاملة بنجاح.");
        } else {
            QMessageBox::critical(this, "خطأ", "فشل في حذف المعاملة.");
        }
    }
}

void MainWindow::onEditButtonClicked()
{
    int currentRow = tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "تنبيه", "يرجى تحديد معاملة من الجدول لتعديلها.");
        return;
    }

    int id = tableWidget->item(currentRow, 0)->text().toInt();
    QList<DocumentRecord> records = Database::instance().getAllRecords();
    DocumentRecord recordToEdit;
    for (const auto& r : records) {
        if (r.id == id) {
            recordToEdit = r;
            break;
        }
    }

    EditDialog editDialog(recordToEdit, this);
    if (editDialog.exec() == QDialog::Accepted) {
        DocumentRecord updated = editDialog.getUpdatedRecord();

        // Ensure any newly added files are copied to the archive
        QStringList finalPaths;
        for (const QString& path : updated.attachments) {
            // If the path is already in the archive directory, it's an old file.
            // If it's not, it's a new file that needs to be copied.
            if (path.startsWith(archiveDir)) {
                finalPaths.append(path);
            } else {
                QString saved = savePdfToArchive(path);
                if (!saved.isEmpty()) {
                    finalPaths.append(saved);
                }
            }
        }
        updated.attachments = finalPaths;

        if (Database::instance().updateRecord(updated)) {
            refreshTable();
            QMessageBox::information(this, "نجاح", "تم حفظ التعديلات بنجاح.");
        } else {
            QMessageBox::critical(this, "خطأ", "حدث خطأ أثناء حفظ التعديلات.");
        }
    }
}

void MainWindow::onSearchFilterChanged()
{
    QString searchText = searchLineEdit->text();
    QString typeFilter = filterTypeComboBox->currentText();
    bool enableDate = filterDateEnabled->isChecked();
    QDate startDate = filterStartDate->date();
    QDate endDate = filterEndDate->date();

    for (int i = 0; i < tableWidget->rowCount(); ++i) {
        bool matchSearch = false;
        if (tableWidget->item(i, 2)->text().contains(searchText, Qt::CaseInsensitive) ||
            tableWidget->item(i, 4)->text().contains(searchText, Qt::CaseInsensitive) ||
            tableWidget->item(i, 5)->text().contains(searchText, Qt::CaseInsensitive)) {
            matchSearch = true;
        }

        bool matchType = (typeFilter == "الكل") || (tableWidget->item(i, 1)->text() == typeFilter);

        bool matchDate = true;
        if (enableDate) {
            QDate rowDate = QDate::fromString(tableWidget->item(i, 3)->text(), "yyyy-MM-dd");
            matchDate = (rowDate >= startDate && rowDate <= endDate);
        }

        tableWidget->setRowHidden(i, !(matchSearch && matchType && matchDate));
    }
}

void MainWindow::onExportDatabase()
{
    QString defaultName = "diwan_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".db";
    QString savePath = QFileDialog::getSaveFileName(this, "تصدير قاعدة البيانات", defaultName, "SQLite Database (*.db)");

    if (!savePath.isEmpty()) {
        QString currentDbPath = Database::instance().getDatabasePath();

        if (QFile::exists(savePath)) {
            QFile::remove(savePath);
        }

        if (QFile::copy(currentDbPath, savePath)) {
            QMessageBox::information(this, "نجاح", "تم تصدير قاعدة البيانات بنجاح!");
        } else {
            QMessageBox::critical(this, "خطأ", "فشل في تصدير قاعدة البيانات.");
        }
    }
}

void MainWindow::onImportDatabase()
{
    QString openPath = QFileDialog::getOpenFileName(this, "استيراد قاعدة البيانات", "", "SQLite Database (*.db)");

    if (!openPath.isEmpty()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "تأكيد الاستيراد", "استيراد قاعدة بيانات جديدة سيؤدي إلى استبدال البيانات الحالية.\nهل أنت متأكد؟",
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QString currentDbPath = Database::instance().getDatabasePath();
            Database::instance().closeDatabase();

            if (QFile::exists(currentDbPath)) {
                QFile::remove(currentDbPath);
            }

            if (QFile::copy(openPath, currentDbPath)) {
                Database::instance().initialize(); // Reopen
                refreshTable();
                QMessageBox::information(this, "نجاح", "تم استيراد قاعدة البيانات بنجاح!");
            } else {
                QMessageBox::critical(this, "خطأ", "فشل في استيراد قاعدة البيانات. يرجى إعادة تشغيل البرنامج.");
            }
        }
    }
}

void MainWindow::onAboutApp()
{
    QMessageBox::about(this, "حول البرنامج",
                       "<h2>برنامج أرشفة ديوان جمعية حقنا</h2>"
                       "<p>هذا البرنامج مخصص لأرشفة وإدارة البريد الصادر والوارد وتسهيل عمل أمينة سر الجمعية.</p>"
                       "<p><b>تم برمجة وتطوير هذا النظام بواسطة:</b><br/>"
                       "<span style='font-size: 16px; color: #2980b9;'>مهند وليد حسون</span></p>"
                       "<p>الإصدار: 1.1</p>");
}

void MainWindow::onGenerateReportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "حفظ التقرير", "تقرير_الديوان_" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".pdf", "PDF Files (*.pdf)");
    if (fileName.isEmpty()) {
        return;
    }

    // Convert logo to base64 to embed in HTML
    QImage logoImage(":/logo.jpg");
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    logoImage.save(&buffer, "JPG");
    QString logoBase64 = QString::fromLatin1(ba.toBase64().data());

    QString html = "<html dir='rtl'><head><style>"
                   "body { font-family: Arial, sans-serif; }"
                   ".header { display: flex; align-items: center; justify-content: flex-start; text-align: right; }"
                   ".header img { height: 80px; margin-left: 15px; }"
                   "h1 { color: #2c3e50; display: inline-block; margin: 0; vertical-align: middle; }"
                   "table { width: 100%; border-collapse: collapse; margin-top: 20px; }"
                   "th, td { border: 1px solid #bdc3c7; padding: 8px; text-align: right; }"
                   "th { background-color: #34495e; color: white; }"
                   "</style></head><body>"
                   "<div class='header'>"
                   "<img src='data:image/jpeg;base64," + logoBase64 + "' />"
                   "<h1>تقرير أرشفة ديوان - جمعية حقنا</h1>"
                   "</div>"
                   "<p>تاريخ التقرير: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + "</p>"
                   "<table>"
                   "<tr><th>النوع</th><th>الرقم</th><th>التاريخ</th><th>الموضوع</th><th>الجهة</th></tr>";

    for (int i = 0; i < tableWidget->rowCount(); ++i) {
        if (!tableWidget->isRowHidden(i)) {
            html += "<tr>";
            html += "<td>" + tableWidget->item(i, 1)->text() + "</td>";
            html += "<td>" + tableWidget->item(i, 2)->text() + "</td>";
            html += "<td>" + tableWidget->item(i, 3)->text() + "</td>";
            html += "<td>" + tableWidget->item(i, 4)->text() + "</td>";
            html += "<td>" + tableWidget->item(i, 5)->text() + "</td>";
            html += "</tr>";
        }
    }

    html += "</table></body></html>";

    QTextDocument document;
    document.setHtml(html);

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageOrientation(QPageLayout::Landscape);

    // Set page margins (in millimeters)
    printer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);

    // Adjust document size to the printer's page rect
    document.setPageSize(printer.pageLayout().paintRectPixels(printer.resolution()).size());

    QPainter painter(&printer);
    QImage logo(":/logo.jpg");

    // Prepare watermark opacity
    painter.setOpacity(0.15); // Semi-transparent for watermark

    // Get logical page size and layout constraints
    QRectF pageRect(0, 0, document.pageSize().width(), document.pageSize().height());

    // Determine the number of pages
    int pageCount = document.pageCount();

    // Iterate and paint each page
    for (int i = 0; i < pageCount; ++i) {
        if (i > 0) {
            printer.newPage();
        }

        painter.save();

        // Draw the watermark logo centered
        if (!logo.isNull()) {
            QImage scaledLogo = logo.scaled(pageRect.width() * 0.5, pageRect.height() * 0.5, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPointF center((pageRect.width() - scaledLogo.width()) / 2.0,
                           (pageRect.height() - scaledLogo.height()) / 2.0);
            painter.setOpacity(0.1); // Keep it very light
            painter.drawImage(center, scaledLogo);
        }

        // Draw the HTML text over the watermark
        painter.setOpacity(1.0); // Reset opacity for text
        QRectF textRect(0, i * pageRect.height(), pageRect.width(), pageRect.height());
        painter.translate(0, -textRect.top());
        QRectF clipRect(0, textRect.top(), pageRect.width(), pageRect.height());

        QAbstractTextDocumentLayout::PaintContext ctx;
        ctx.clip = clipRect;
        document.documentLayout()->draw(&painter, ctx);

        painter.restore();
    }

    painter.end();

    QMessageBox::information(this, "نجاح", "تم توليد التقرير بنجاح وحفظه كملف PDF.");
}
