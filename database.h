#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>

struct DocumentRecord {
    int id;
    QString type; // "صادر" or "وارد"
    QString docNumber;
    QString date;
    QString subject;
    QString correspondent;
    QString filePath;
    bool needsFollowUp; // 1 for true, 0 for false
};

class Database
{
public:
    static Database& instance();
    bool initialize();
    bool addRecord(const DocumentRecord& record);
    QList<DocumentRecord> getAllRecords();
    QString generateNextNumber(const QString& type);
    bool updateRecord(const DocumentRecord& record);
    bool deleteRecord(int id);
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
