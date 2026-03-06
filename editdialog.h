#ifndef EDITDIALOG_H
#define EDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include "database.h"

class EditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditDialog(const DocumentRecord& record, QWidget *parent = nullptr);
    ~EditDialog();
    DocumentRecord getUpdatedRecord() const;

private slots:
    void onSaveButtonClicked();

private:
    void setupUi();

    DocumentRecord m_record;

    QComboBox *typeComboBox;
    QLineEdit *numberLineEdit;
    QDateEdit *dateEdit;
    QLineEdit *subjectLineEdit;
    QLineEdit *correspondentLineEdit;
    QCheckBox *followUpCheckBox;

    QPushButton *saveButton;
    QPushButton *cancelButton;
};

#endif // EDITDIALOG_H
