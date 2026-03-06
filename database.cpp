#include "database.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDateTime>

Database::Database() : currentUser(""), currentRole("")
{
}

Database::~Database()
{
    if (db.isOpen()) {
        db.close();
    }
}

Database& Database::instance()
{
    static Database instance;
    return instance;
}

bool Database::initialize()
{
    // Make sure we have a directory for our data
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = dataDir + "/diwan.db";
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "Error opening database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    // 1. Documents Table
    bool success = query.exec("CREATE TABLE IF NOT EXISTS documents ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "type TEXT NOT NULL, "
                              "docNumber TEXT NOT NULL, "
                              "date TEXT NOT NULL, "
                              "subject TEXT NOT NULL, "
                              "correspondent TEXT NOT NULL, "
                              "filePath TEXT, " // Kept for legacy compatibility
                              "needsFollowUp INTEGER DEFAULT 0)");
    if (!success) qDebug() << "Error creating documents table:" << query.lastError().text();

    // 2. Attachments Table
    success &= query.exec("CREATE TABLE IF NOT EXISTS attachments ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "document_id INTEGER, "
                          "file_path TEXT NOT NULL, "
                          "FOREIGN KEY(document_id) REFERENCES documents(id))");
    if (!success) qDebug() << "Error creating attachments table:" << query.lastError().text();

    // 3. Users Table
    success &= query.exec("CREATE TABLE IF NOT EXISTS users ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "username TEXT UNIQUE NOT NULL, "
                          "password_hash TEXT NOT NULL, "
                          "role TEXT NOT NULL)");
    if (!success) qDebug() << "Error creating users table:" << query.lastError().text();

    // 4. Audit Log Table
    success &= query.exec("CREATE TABLE IF NOT EXISTS audit_log ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "username TEXT NOT NULL, "
                          "action TEXT NOT NULL, "
                          "timestamp TEXT NOT NULL, "
                          "details TEXT NOT NULL)");
    if (!success) qDebug() << "Error creating audit_log table:" << query.lastError().text();

    if (success) {
        // Migrations
        QSqlQuery migrateQuery;

        // Migrate A: Check and add needsFollowUp if missing
        if (migrateQuery.exec("PRAGMA table_info(documents)")) {
            bool hasNeedsFollowUp = false;
            while (migrateQuery.next()) {
                if (migrateQuery.value(1).toString() == "needsFollowUp") {
                    hasNeedsFollowUp = true;
                    break;
                }
            }
            if (!hasNeedsFollowUp) {
                QSqlQuery addColQuery;
                addColQuery.exec("ALTER TABLE documents ADD COLUMN needsFollowUp INTEGER DEFAULT 0");
            }
        }

        // Migrate B: Move existing single filePaths to attachments table
        QSqlQuery fetchLegacy("SELECT id, filePath FROM documents WHERE filePath IS NOT NULL AND filePath != ''");
        while (fetchLegacy.next()) {
            int docId = fetchLegacy.value(0).toInt();
            QString path = fetchLegacy.value(1).toString();

            // Check if it already exists in attachments to avoid duplicates
            QSqlQuery checkAttach;
            checkAttach.prepare("SELECT COUNT(*) FROM attachments WHERE document_id = :id AND file_path = :path");
            checkAttach.bindValue(":id", docId);
            checkAttach.bindValue(":path", path);
            if (checkAttach.exec() && checkAttach.next() && checkAttach.value(0).toInt() == 0) {
                QSqlQuery insertAttach;
                insertAttach.prepare("INSERT INTO attachments (document_id, file_path) VALUES (:id, :path)");
                insertAttach.bindValue(":id", docId);
                insertAttach.bindValue(":path", path);
                insertAttach.exec();
            }

            // Clear legacy filePath to signify migration
            QSqlQuery clearLegacy;
            clearLegacy.prepare("UPDATE documents SET filePath = '' WHERE id = :id");
            clearLegacy.bindValue(":id", docId);
            clearLegacy.exec();
        }
    }

    return success;
}

bool Database::addRecord(const DocumentRecord& record)
{
    if (!db.isOpen()) return false;

    QSqlQuery query;
    query.prepare("INSERT INTO documents (type, docNumber, date, subject, correspondent, filePath, needsFollowUp) "
                  "VALUES (:type, :docNumber, :date, :subject, :correspondent, '', :needsFollowUp)");
    query.bindValue(":type", record.type);
    query.bindValue(":docNumber", record.docNumber);
    query.bindValue(":date", record.date);
    query.bindValue(":subject", record.subject);
    query.bindValue(":correspondent", record.correspondent);
    query.bindValue(":needsFollowUp", record.needsFollowUp ? 1 : 0);

    if (!query.exec()) {
        qDebug() << "Error inserting record:" << query.lastError().text();
        return false;
    }

    int docId = query.lastInsertId().toInt();

    for (const QString& path : record.attachments) {
        QSqlQuery insertAttach;
        insertAttach.prepare("INSERT INTO attachments (document_id, file_path) VALUES (:id, :path)");
        insertAttach.bindValue(":id", docId);
        insertAttach.bindValue(":path", path);
        insertAttach.exec();
    }

    logAction("إضافة", "تمت إضافة معاملة " + record.type + " رقم " + record.docNumber);
    return true;
}

QList<DocumentRecord> Database::getAllRecords()
{
    QList<DocumentRecord> records;
    if (!db.isOpen()) return records;

    QSqlQuery query("SELECT id, type, docNumber, date, subject, correspondent, needsFollowUp FROM documents ORDER BY id DESC");
    while (query.next()) {
        DocumentRecord record;
        record.id = query.value(0).toInt();
        record.type = query.value(1).toString();
        record.docNumber = query.value(2).toString();
        record.date = query.value(3).toString();
        record.subject = query.value(4).toString();
        record.correspondent = query.value(5).toString();
        record.needsFollowUp = query.value(6).toInt() == 1;

        // Fetch attachments
        QSqlQuery fetchAttach;
        fetchAttach.prepare("SELECT file_path FROM attachments WHERE document_id = :id");
        fetchAttach.bindValue(":id", record.id);
        if (fetchAttach.exec()) {
            while (fetchAttach.next()) {
                record.attachments.append(fetchAttach.value(0).toString());
            }
        }

        records.append(record);
    }
    return records;
}

bool Database::updateRecord(const DocumentRecord& record)
{
    if (!db.isOpen()) return false;

    QSqlQuery query;
    query.prepare("UPDATE documents SET "
                  "type = :type, "
                  "docNumber = :docNumber, "
                  "date = :date, "
                  "subject = :subject, "
                  "correspondent = :correspondent, "
                  "needsFollowUp = :needsFollowUp "
                  "WHERE id = :id");
    query.bindValue(":type", record.type);
    query.bindValue(":docNumber", record.docNumber);
    query.bindValue(":date", record.date);
    query.bindValue(":subject", record.subject);
    query.bindValue(":correspondent", record.correspondent);
    query.bindValue(":needsFollowUp", record.needsFollowUp ? 1 : 0);
    query.bindValue(":id", record.id);

    if (!query.exec()) {
        qDebug() << "Error updating record:" << query.lastError().text();
        return false;
    }

    // Replace attachments
    QSqlQuery delAttach;
    delAttach.prepare("DELETE FROM attachments WHERE document_id = :id");
    delAttach.bindValue(":id", record.id);
    delAttach.exec();

    for (const QString& path : record.attachments) {
        QSqlQuery insertAttach;
        insertAttach.prepare("INSERT INTO attachments (document_id, file_path) VALUES (:id, :path)");
        insertAttach.bindValue(":id", record.id);
        insertAttach.bindValue(":path", path);
        insertAttach.exec();
    }

    logAction("تعديل", "تم تعديل المعاملة رقم " + record.docNumber);
    return true;
}

QString Database::generateNextNumber(const QString& type)
{
    if (!db.isOpen()) return "1";

    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM documents WHERE type = :type");
    query.bindValue(":type", type);

    if (query.exec() && query.next()) {
        int count = query.value(0).toInt();
        return QString::number(count + 1);
    }
    return "1";
}

bool Database::deleteRecord(int id)
{
    if (!db.isOpen()) return false;

    // Get docNumber for logging
    QString docNumber;
    QSqlQuery fetch;
    fetch.prepare("SELECT docNumber FROM documents WHERE id = :id");
    fetch.bindValue(":id", id);
    if (fetch.exec() && fetch.next()) {
        docNumber = fetch.value(0).toString();
    }

    // Delete attachments
    QSqlQuery delAttach;
    delAttach.prepare("DELETE FROM attachments WHERE document_id = :id");
    delAttach.bindValue(":id", id);
    delAttach.exec();

    QSqlQuery query;
    query.prepare("DELETE FROM documents WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Error deleting record:" << query.lastError().text();
        return false;
    }

    logAction("حذف", "تم حذف المعاملة رقم " + docNumber);
    return true;
}

// User Management

bool Database::authenticate(const QString& username, const QString& password)
{
    if (!db.isOpen()) return false;

    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);

    QSqlQuery query;
    query.prepare("SELECT role FROM users WHERE username = :username AND password_hash = :hash");
    query.bindValue(":username", username);
    query.bindValue(":hash", hash.toHex());

    if (query.exec() && query.next()) {
        currentUser = username;
        currentRole = query.value(0).toString();
        logAction("دخول", "تسجيل دخول للنظام");
        return true;
    }
    return false;
}

bool Database::addUser(const QString& username, const QString& password, const QString& role)
{
    if (!db.isOpen()) return false;

    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);

    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password_hash, role) VALUES (:username, :hash, :role)");
    query.bindValue(":username", username);
    query.bindValue(":hash", hash.toHex());
    query.bindValue(":role", role);

    bool ok = query.exec();
    if (ok && !currentUser.isEmpty()) logAction("إضافة مستخدم", "تم إضافة المستخدم " + username);
    return ok;
}

bool Database::deleteUser(const QString& username)
{
    if (!db.isOpen() || username == currentUser || username == "admin") return false; // Prevent suicide or removing root

    QSqlQuery query;
    query.prepare("DELETE FROM users WHERE username = :username");
    query.bindValue(":username", username);

    bool ok = query.exec();
    if (ok) logAction("حذف مستخدم", "تم حذف المستخدم " + username);
    return ok;
}

bool Database::changePassword(const QString& username, const QString& newPassword)
{
    if (!db.isOpen()) return false;

    QByteArray hash = QCryptographicHash::hash(newPassword.toUtf8(), QCryptographicHash::Sha256);
    QSqlQuery query;
    query.prepare("UPDATE users SET password_hash = :hash WHERE username = :username");
    query.bindValue(":hash", hash.toHex());
    query.bindValue(":username", username);

    return query.exec();
}

QList<User> Database::getAllUsers()
{
    QList<User> users;
    if (!db.isOpen()) return users;

    QSqlQuery query("SELECT id, username, password_hash, role FROM users");
    while (query.next()) {
        User u;
        u.id = query.value(0).toInt();
        u.username = query.value(1).toString();
        u.passwordHash = query.value(2).toString();
        u.role = query.value(3).toString();
        users.append(u);
    }
    return users;
}

bool Database::hasUsers()
{
    if (!db.isOpen()) return false;
    QSqlQuery query("SELECT COUNT(*) FROM users");
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

// Audit Log

void Database::logAction(const QString& action, const QString& details)
{
    if (!db.isOpen() || currentUser.isEmpty()) return;

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QSqlQuery query;
    query.prepare("INSERT INTO audit_log (username, action, timestamp, details) VALUES (:u, :a, :t, :d)");
    query.bindValue(":u", currentUser);
    query.bindValue(":a", action);
    query.bindValue(":t", timestamp);
    query.bindValue(":d", details);
    query.exec();
}

QList<AuditRecord> Database::getAuditLogs()
{
    QList<AuditRecord> logs;
    if (!db.isOpen()) return logs;

    QSqlQuery query("SELECT id, username, action, timestamp, details FROM audit_log ORDER BY id DESC LIMIT 1000"); // Limit to last 1000
    while (query.next()) {
        AuditRecord rec;
        rec.id = query.value(0).toInt();
        rec.username = query.value(1).toString();
        rec.action = query.value(2).toString();
        rec.timestamp = query.value(3).toString();
        rec.details = query.value(4).toString();
        logs.append(rec);
    }
    return logs;
}

// Session

QString Database::getCurrentUser() const { return currentUser; }
QString Database::getCurrentRole() const { return currentRole; }
bool Database::isAdmin() const { return currentRole == "admin"; }

QString Database::getDatabasePath() const
{
    return db.databaseName();
}

void Database::closeDatabase()
{
    if (db.isOpen()) {
        QString connectionName = db.connectionName();
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
    }
}
