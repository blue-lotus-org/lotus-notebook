#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

class BackupManager : public QObject
{
    Q_OBJECT

public:
    struct BackupInfo {
        QString filePath;
        QString originalPath;
        QDateTime timestamp;
        int size;
        bool success;
        QString errorMessage;
    };

    explicit BackupManager(QObject *parent = nullptr);
    ~BackupManager();

    // Configuration
    void setBackupDirectory(const QString &directory);
    QString getBackupDirectory() const { return backupDirectory; }

    void setMaxBackups(int max);
    int getMaxBackups() const { return maxBackups; }

    void setAutoBackupInterval(int milliseconds);
    int getAutoBackupInterval() const { return autoBackupInterval; }

    void enableAutoBackup(bool enable);
    bool isAutoBackupEnabled() const { return autoBackupEnabled; }

    // Backup operations
    bool createBackup(const QString &originalPath, const QByteArray &data);
    bool createBackup(const QString &originalPath);

    // Restore operations
    bool restoreFromBackup(const QString &backupPath, QByteArray &data);
    QString findLatestBackup(const QString &originalPath) const;
    QList<BackupInfo> listBackups(const QString &originalPath) const;

    // Cleanup
    void cleanupOldBackups(const QString &originalPath);
    void cleanupAllBackups();
    bool deleteBackup(const QString &backupPath);

    // Status
    bool isReady() const { return isInitialized; }
    QString lastError() const { return lastErrorMessage; }

    // Backup metadata
    void setBackupMetadata(const QJsonObject &metadata);
    QJsonObject getBackupMetadata(const QString &backupPath) const;

signals:
    void backupCreated(const QString &backupPath, bool success);
    void backupRestored(const QString &backupPath, bool success);
    void backupFailed(const QString &originalPath, const QString &error);
    void cleanupCompleted(int filesDeleted);

private slots:
    void onAutoBackupTimeout();

private:
    bool initialize();
    QString generateBackupPath(const QString &originalPath) const;
    QString getBackupKey(const QString &originalPath) const;

    QString backupDirectory;
    bool autoBackupEnabled;
    int autoBackupInterval;
    int maxBackups;
    bool isInitialized;
    QString lastErrorMessage;
    QTimer *autoBackupTimer;

    static const QString BACKUP_EXTENSION;
    static const QString METADATA_EXTENSION;
    static const int DEFAULT_MAX_BACKUPS = 10;
    static const int DEFAULT_AUTO_BACKUP_INTERVAL = 300000; // 5 minutes
};

#endif // BACKUPMANAGER_H
