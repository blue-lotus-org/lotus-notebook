#include "backupmanager.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>

const QString BackupManager::BACKUP_EXTENSION = ".backup";
const QString BackupManager::METADATA_EXTENSION = ".meta";

BackupManager::BackupManager(QObject *parent)
    : QObject(parent)
    , autoBackupEnabled(true)
    , autoBackupInterval(DEFAULT_AUTO_BACKUP_INTERVAL)
    , maxBackups(DEFAULT_MAX_BACKUPS)
    , isInitialized(false)
    , autoBackupTimer(nullptr)
{
    initialize();
}

BackupManager::~BackupManager()
{
    if (autoBackupTimer) {
        autoBackupTimer->stop();
        delete autoBackupTimer;
    }
}

bool BackupManager::initialize()
{
    // Set default backup directory
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (defaultDir.isEmpty()) {
        defaultDir = QDir::homePath() + "/.lotus-notebook/backups";
    }

    backupDirectory = defaultDir;

    // Create backup directory if it doesn't exist
    QDir dir(backupDirectory);
    if (!dir.exists()) {
        isInitialized = dir.mkpath(backupDirectory);
    } else {
        isInitialized = true;
    }

    if (!isInitialized) {
        lastErrorMessage = QString("Failed to create backup directory: %1").arg(backupDirectory);
        return false;
    }

    // Setup auto-backup timer
    autoBackupTimer = new QTimer(this);
    autoBackupTimer->setInterval(autoBackupInterval);
    connect(autoBackupTimer, &QTimer::timeout, this, &BackupManager::onAutoBackupTimeout);

    if (autoBackupEnabled) {
        autoBackupTimer->start();
    }

    return true;
}

QString BackupManager::generateBackupPath(const QString &originalPath) const
{
    QFileInfo fileInfo(originalPath);
    QString fileName = fileInfo.fileName();
    QString baseName = fileInfo.completeBaseName();
    QString extension = fileInfo.suffix();

    // Generate unique filename with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    QString safeFileName = baseName.replace(QRegExp("[^a-zA-Z0-9_-]"), "_");

    QString backupFileName = QString("%1_%2%3").arg(safeFileName, timestamp, BACKUP_EXTENSION);

    return backupDirectory + "/" + backupFileName;
}

QString BackupManager::getBackupKey(const QString &originalPath) const
{
    QFileInfo fileInfo(originalPath);
    return fileInfo.completeBaseName().replace(QRegExp("[^a-zA-Z0-9_-]"), "_");
}

bool BackupManager::createBackup(const QString &originalPath, const QByteArray &data)
{
    if (!isInitialized) {
        lastErrorMessage = "Backup manager not initialized";
        emit backupFailed(originalPath, lastErrorMessage);
        return false;
    }

    if (originalPath.isEmpty() || data.isEmpty()) {
        lastErrorMessage = "Invalid parameters";
        emit backupFailed(originalPath, lastErrorMessage);
        return false;
    }

    // Generate backup path
    QString backupPath = generateBackupPath(originalPath);

    // Create backup file
    QFile backupFile(backupPath);

    if (!backupFile.open(QIODevice::WriteOnly)) {
        lastErrorMessage = QString("Failed to create backup file: %1").arg(backupFile.errorString());
        emit backupFailed(originalPath, lastErrorMessage);
        return false;
    }

    // Write data
    qint64 bytesWritten = backupFile.write(data);
    backupFile.close();

    if (bytesWritten != data.size()) {
        lastErrorMessage = "Failed to write all data to backup file";
        backupFile.remove();
        emit backupFailed(originalPath, lastErrorMessage);
        return false;
    }

    // Create metadata file
    QJsonObject metadata;
    metadata["original_path"] = originalPath;
    metadata["backup_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    metadata["file_size"] = data.size();
    metadata["file_name"] = QFileInfo(originalPath).fileName();

    QJsonDocument metaDoc(metadata);
    QString metaPath = backupPath + METADATA_EXTENSION;

    QFile metaFile(metaPath);
    if (!metaFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create metadata file:" << metaFile.errorString();
    } else {
        metaFile.write(metaDoc.toJson());
        metaFile.close();
    }

    // Cleanup old backups
    cleanupOldBackups(originalPath);

    emit backupCreated(backupPath, true);

    qDebug() << "Backup created:" << backupPath;

    return true;
}

bool BackupManager::createBackup(const QString &originalPath)
{
    QFile file(originalPath);

    if (!file.open(QIODevice::ReadOnly)) {
        lastErrorMessage = QString("Cannot open file: %1").arg(file.errorString());
        emit backupFailed(originalPath, lastErrorMessage);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    return createBackup(originalPath, data);
}

bool BackupManager::restoreFromBackup(const QString &backupPath, QByteArray &data)
{
    if (!QFile::exists(backupPath)) {
        lastErrorMessage = "Backup file does not exist";
        emit backupRestored(backupPath, false);
        return false;
    }

    QFile file(backupPath);

    if (!file.open(QIODevice::ReadOnly)) {
        lastErrorMessage = QString("Cannot open backup file: %1").arg(file.errorString());
        emit backupRestored(backupPath, false);
        return false;
    }

    data = file.readAll();
    file.close();

    emit backupRestored(backupPath, true);

    return true;
}

QString BackupManager::findLatestBackup(const QString &originalPath) const
{
    QDir dir(backupDirectory);
    QString key = getBackupKey(originalPath);

    // Find all matching backup files
    QStringList filters;
    filters << QString("%1_*%2").arg(key, BACKUP_EXTENSION);

    dir.setNameFilters(filters);
    dir.setSorting(QDir::Time | QDir::Reversed);

    QFileInfoList files = dir.entryInfoList();

    if (files.isEmpty()) {
        return QString();
    }

    return files.first().absoluteFilePath();
}

QList<BackupManager::BackupInfo> BackupManager::listBackups(const QString &originalPath) const
{
    QList<BackupInfo> backups;

    QDir dir(backupDirectory);
    QString key = getBackupKey(originalPath);

    // Find all matching backup files
    QStringList filters;
    filters << QString("%1_*%2").arg(key, BACKUP_EXTENSION);

    dir.setNameFilters(filters);
    dir.setSorting(QDir::Time | QDir::Reversed);

    QFileInfoList files = dir.entryInfoList();

    for (const QFileInfo &fileInfo : files) {
        BackupInfo info;
        info.filePath = fileInfo.absoluteFilePath();
        info.originalPath = originalPath;
        info.timestamp = fileInfo.lastModified();
        info.size = fileInfo.size();
        info.success = true;

        // Load metadata
        QString metaPath = fileInfo.absoluteFilePath() + METADATA_EXTENSION;
        if (QFile::exists(metaPath)) {
            QFile metaFile(metaPath);
            if (metaFile.open(QIODevice::ReadOnly)) {
                QJsonParseError error;
                QJsonDocument doc = QJsonDocument::fromJson(metaFile.readAll(), &error);
                if (error.error == QJsonParseError::NoError) {
                    QJsonObject meta = doc.object();
                    info.originalPath = meta["original_path"].toString(originalPath);
                }
                metaFile.close();
            }
        }

        backups.append(info);
    }

    return backups;
}

void BackupManager::cleanupOldBackups(const QString &originalPath)
{
    QList<BackupInfo> backups = listBackups(originalPath);

    while (backups.size() > maxBackups) {
        BackupInfo oldest = backups.takeLast();
        QFile backupFile(oldest.filePath);
        QString metaPath = oldest.filePath + METADATA_EXTENSION;

        backupFile.remove();
        QFile::remove(metaPath);
    }

    if (backups.size() > maxBackups) {
        emit cleanupCompleted(backups.size() - maxBackups);
    }
}

void BackupManager::cleanupAllBackups()
{
    QDir dir(backupDirectory);
    QStringList filters;
    filters << "*" + BACKUP_EXTENSION;
    filters << "*" + METADATA_EXTENSION;

    dir.setNameFilters(filters);
    dir.setSorting(QDir::Time);

    int count = 0;

    for (const QFileInfo &fileInfo : dir.entryInfoList()) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.remove()) {
            count++;
        }
    }

    emit cleanupCompleted(count);
}

bool BackupManager::deleteBackup(const QString &backupPath)
{
    QString metaPath = backupPath + METADATA_EXTENSION;

    bool success = true;

    if (QFile::exists(backupPath)) {
        QFile file(backupPath);
        if (!file.remove()) {
            success = false;
        }
    }

    if (QFile::exists(metaPath)) {
        QFile file(metaPath);
        if (!file.remove()) {
            success = false;
        }
    }

    return success;
}

void BackupManager::setBackupMetadata(const QJsonObject &metadata)
{
    // This could be used to store additional metadata
    Q_UNUSED(metadata)
}

QJsonObject BackupManager::getBackupMetadata(const QString &backupPath) const
{
    QJsonObject empty;

    QString metaPath = backupPath + METADATA_EXTENSION;

    if (!QFile::exists(metaPath)) {
        return empty;
    }

    QFile file(metaPath);

    if (!file.open(QIODevice::ReadOnly)) {
        return empty;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        return empty;
    }

    return doc.object();
}

void BackupManager::setBackupDirectory(const QString &directory)
{
    backupDirectory = directory;
    initialize();
}

void BackupManager::setMaxBackups(int max)
{
    maxBackups = qMax(1, max);
}

void BackupManager::setAutoBackupInterval(int milliseconds)
{
    autoBackupInterval = milliseconds;
    if (autoBackupTimer) {
        autoBackupTimer->setInterval(autoBackupInterval);
    }
}

void BackupManager::enableAutoBackup(bool enable)
{
    autoBackupEnabled = enable;

    if (autoBackupTimer) {
        if (autoBackupEnabled) {
            autoBackupTimer->start();
        } else {
            autoBackupTimer->stop();
        }
    }
}

void BackupManager::onAutoBackupTimeout()
{
    // This would be called to trigger auto-backup of current notebook
    // The actual implementation would be in MainWindow
    qDebug() << "Auto-backup timeout triggered";
}
