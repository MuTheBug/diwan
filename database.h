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

struct User {
    int id;
    QString username;
    QString passwordHash;
    QString role; // "admin", "user"
};

struct AuditRecord {
    int id;
    QString username;
    QString action;
    QString timestamp;
    QString details;
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

    // User Management
    bool authenticate(const QString& username, const QString& password);
    bool addUser(const QString& username, const QString& password, const QString& role);
    bool deleteUser(const QString& username);
    bool changePassword(const QString& username, const QString& newPassword);
    QList<User> getAllUsers();
    bool hasUsers();

    // Audit Log
    void logAction(const QString& action, const QString& details);
    QList<AuditRecord> getAuditLogs();

    // Session
    QString getCurrentUser() const;
    QString getCurrentRole() const;
    bool isAdmin() const;

    // DB Admin
    QString getDatabasePath() const;
    void closeDatabase();

private:
    QString currentUser;
    QString currentRole;

    Database();
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase db;
};

#endif // DATABASE_H
