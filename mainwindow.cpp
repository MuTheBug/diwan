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
    setupMenu();

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

    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(logoLabel);

    mainLayout->addWidget(headerWidget);

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

    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLabel *searchLabel = new QLabel("البحث:", this);
    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("ابحث في الموضوع أو الجهة أو الرقم...");
    connect(searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(searchLineEdit);
    tableLayout->addLayout(searchLayout);

    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(7);
    tableWidget->setHorizontalHeaderLabels({"المعرف", "النوع", "الرقم", "التاريخ", "الموضوع", "الجهة", "مسار الملف"});
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->hideColumn(0); // Hide ID
    tableWidget->hideColumn(6); // Hide file path

    QHBoxLayout *actionButtonsLayout = new QHBoxLayout();

    viewPdfButton = new QPushButton("عرض ملف PDF المحدد", this);
    viewPdfButton->setMinimumHeight(40);
    viewPdfButton->setStyleSheet("background-color: #2980b9; color: white; font-weight: bold; font-size: 14px; border-radius: 5px;");
    connect(viewPdfButton, &QPushButton::clicked, this, &MainWindow::onViewPdfButtonClicked);

    deleteButton = new QPushButton("حذف القيد", this);
    deleteButton->setMinimumHeight(40);
    deleteButton->setStyleSheet("background-color: #c0392b; color: white; font-weight: bold; font-size: 14px; border-radius: 5px;");
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteButtonClicked);

    reportButton = new QPushButton("توليد تقرير (PDF)", this);
    reportButton->setMinimumHeight(40);
    reportButton->setStyleSheet("background-color: #8e44ad; color: white; font-weight: bold; font-size: 14px; border-radius: 5px;");
    connect(reportButton, &QPushButton::clicked, this, &MainWindow::onGenerateReportClicked);

    actionButtonsLayout->addWidget(viewPdfButton);
    actionButtonsLayout->addWidget(reportButton);
    actionButtonsLayout->addWidget(deleteButton);

    tableLayout->addWidget(tableWidget);
    tableLayout->addLayout(actionButtonsLayout);

    contentLayout->addLayout(tableLayout);
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

void MainWindow::onSearchTextChanged(const QString &text)
{
    for (int i = 0; i < tableWidget->rowCount(); ++i) {
        bool match = false;
        // Search in Number (2), Subject (4), and Correspondent (5)
        if (tableWidget->item(i, 2)->text().contains(text, Qt::CaseInsensitive) ||
            tableWidget->item(i, 4)->text().contains(text, Qt::CaseInsensitive) ||
            tableWidget->item(i, 5)->text().contains(text, Qt::CaseInsensitive)) {
            match = true;
        }
        tableWidget->setRowHidden(i, !match);
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

    QString html = "<html dir='rtl'><head><style>"
                   "body { font-family: Arial, sans-serif; }"
                   "h1 { text-align: right; color: #2c3e50; }"
                   "table { width: 100%; border-collapse: collapse; margin-top: 20px; }"
                   "th, td { border: 1px solid #bdc3c7; padding: 8px; text-align: right; }"
                   "th { background-color: #34495e; color: white; }"
                   "</style></head><body>"
                   "<h1>تقرير أرشفة ديوان - جمعية حقنا</h1>"
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
