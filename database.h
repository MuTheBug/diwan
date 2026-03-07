#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>
#include <QStringList>

struct DocumentRecord {
    int id;
    QString type; // "صادر" or "وارد"
    QString docNumber;
    QString date;
    QString subject;
    QString correspondent;
    QStringList attachments;
    bool needsFollowUp; // 1 for true, 0 for false
};

class Database
{
public:
    static Database& instance();
    bool initialize();

    // Document Management
    bool addRecord(const DocumentRecord& record);
    QList<DocumentRecord> getAllRecords();
    QString generateNextNumber(const QString& type);
    bool updateRecord(const DocumentRecord& record);
    bool deleteRecord(int id);

    // Session (Mocked for legacy compatibility)
    bool isAdmin() const;

    // DB Admin
    QString getDatabasePath() const;
    void closeDatabase();

private:
    Database();
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase db;
};

#endif // DATABASE_H
