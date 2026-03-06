#include "database.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

Database::Database()
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
    bool success = query.exec("CREATE TABLE IF NOT EXISTS documents ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "type TEXT NOT NULL, "
                              "docNumber TEXT NOT NULL, "
                              "date TEXT NOT NULL, "
                              "subject TEXT NOT NULL, "
                              "correspondent TEXT NOT NULL, "
                              "filePath TEXT NOT NULL)");

    if (!success) {
        qDebug() << "Error creating table:" << query.lastError().text();
    }

    return success;
}

bool Database::addRecord(const DocumentRecord& record)
{
    if (!db.isOpen()) return false;

    QSqlQuery query;
    query.prepare("INSERT INTO documents (type, docNumber, date, subject, correspondent, filePath) "
                  "VALUES (:type, :docNumber, :date, :subject, :correspondent, :filePath)");
    query.bindValue(":type", record.type);
    query.bindValue(":docNumber", record.docNumber);
    query.bindValue(":date", record.date);
    query.bindValue(":subject", record.subject);
    query.bindValue(":correspondent", record.correspondent);
    query.bindValue(":filePath", record.filePath);

    if (!query.exec()) {
        qDebug() << "Error inserting record:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<DocumentRecord> Database::getAllRecords()
{
    QList<DocumentRecord> records;
    if (!db.isOpen()) return records;

    QSqlQuery query("SELECT id, type, docNumber, date, subject, correspondent, filePath FROM documents ORDER BY id DESC");
    while (query.next()) {
        DocumentRecord record;
        record.id = query.value(0).toInt();
        record.type = query.value(1).toString();
        record.docNumber = query.value(2).toString();
        record.date = query.value(3).toString();
        record.subject = query.value(4).toString();
        record.correspondent = query.value(5).toString();
        record.filePath = query.value(6).toString();
        records.append(record);
    }
    return records;
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
