#include "editdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDate>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>

EditDialog::EditDialog(const DocumentRecord& record, QWidget *parent) :
    QDialog(parent), m_record(record)
{
    setupUi();
}

EditDialog::~EditDialog()
{
}

void EditDialog::setupUi()
{
    this->setWindowTitle("تعديل المعاملة");
    this->setFixedSize(400, 350);
    this->setLayoutDirection(Qt::RightToLeft);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *formLayout = new QFormLayout();

    typeComboBox = new QComboBox(this);
    typeComboBox->addItems({"وارد", "صادر"});
    typeComboBox->setCurrentText(m_record.type);

    numberLineEdit = new QLineEdit(m_record.docNumber, this);

    dateEdit = new QDateEdit(QDate::fromString(m_record.date, "yyyy-MM-dd"), this);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");

    subjectLineEdit = new QLineEdit(m_record.subject, this);

    correspondentLineEdit = new QLineEdit(m_record.correspondent, this);

    followUpCheckBox = new QCheckBox("يحتاج لمتابعة", this);
    followUpCheckBox->setChecked(m_record.needsFollowUp);

    attachmentsList = new QListWidget(this);
    for (const QString& path : m_record.attachments) {
        QListWidgetItem *item = new QListWidgetItem(QFileInfo(path).fileName(), attachmentsList);
        item->setData(Qt::UserRole, path);
    }

    QHBoxLayout *attachBtns = new QHBoxLayout();
    addFileButton = new QPushButton("+ إضافة", this);
    removeFileButton = new QPushButton("- إزالة", this);
    QPushButton *openFileButton = new QPushButton("فتح المحدد", this);
    attachBtns->addWidget(addFileButton);
    attachBtns->addWidget(removeFileButton);
    attachBtns->addWidget(openFileButton);

    connect(addFileButton, &QPushButton::clicked, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, "اختر المرفقات", "", "Files (*.*)");
        for (const QString& file : files) {
            QListWidgetItem *item = new QListWidgetItem(QFileInfo(file).fileName(), attachmentsList);
            item->setData(Qt::UserRole, file); // We store original path here, main app copies it
            m_record.attachments.append(file); // Track new files to be saved later
        }
    });

    connect(removeFileButton, &QPushButton::clicked, [this]() {
        int r = attachmentsList->currentRow();
        if (r >= 0) {
            QString path = attachmentsList->item(r)->data(Qt::UserRole).toString();
            m_record.attachments.removeAll(path);
            delete attachmentsList->takeItem(r);
        }
    });

    connect(openFileButton, &QPushButton::clicked, [this]() {
        int r = attachmentsList->currentRow();
        if (r >= 0) {
            QString path = attachmentsList->item(r)->data(Qt::UserRole).toString();
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });

    formLayout->addRow("النوع:", typeComboBox);
    formLayout->addRow("الرقم:", numberLineEdit);
    formLayout->addRow("التاريخ:", dateEdit);
    formLayout->addRow("الموضوع:", subjectLineEdit);
    formLayout->addRow("الجهة:", correspondentLineEdit);
    formLayout->addRow("", followUpCheckBox);
    formLayout->addRow("المرفقات:", attachmentsList);
    formLayout->addRow("", attachBtns);

    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    saveButton = new QPushButton("حفظ التعديلات", this);
    saveButton->setStyleSheet("background-color: #27ae60; color: white; font-weight: bold; border-radius: 3px; padding: 5px;");

    cancelButton = new QPushButton("إلغاء", this);
    cancelButton->setStyleSheet("background-color: #7f8c8d; color: white; font-weight: bold; border-radius: 3px; padding: 5px;");

    connect(saveButton, &QPushButton::clicked, this, &EditDialog::onSaveButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &EditDialog::reject);

    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonsLayout);
}

void EditDialog::onSaveButtonClicked()
{
    if (subjectLineEdit->text().isEmpty() || correspondentLineEdit->text().isEmpty() || numberLineEdit->text().isEmpty()) {
        QMessageBox::warning(this, "تنبيه", "يرجى تعبئة جميع الحقول المطلوبة.");
        return;
    }

    m_record.type = typeComboBox->currentText();
    m_record.docNumber = numberLineEdit->text();
    m_record.date = dateEdit->date().toString("yyyy-MM-dd");
    m_record.subject = subjectLineEdit->text();
    m_record.correspondent = correspondentLineEdit->text();
    m_record.needsFollowUp = followUpCheckBox->isChecked();

    accept();
}

DocumentRecord EditDialog::getUpdatedRecord() const
{
    return m_record;
}
