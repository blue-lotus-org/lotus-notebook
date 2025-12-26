#ifndef NOTEBOOKMANAGER_H
#define NOTEBOOKMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QDir>

class NotebookManager : public QObject
{
    Q_OBJECT

public:
    enum ExportFormat {
        FormatLotus,    // Lotus Notebook format (.lotus)
        FormatIpynb,    // Jupyter Notebook format (.ipynb)
        FormatHtml,     // HTML export
        FormatPython    // Python script export (.py)
    };

    struct CellData {
        enum Type { CodeCell, MarkdownCell };
        Type type;
        QString content;
        int executionCount;
        bool wasExecuted;
    };

    explicit NotebookManager(QObject *parent = nullptr);

    // File operations
    bool loadNotebook(const QJsonObject &root);
    QJsonObject saveNotebook() const;

    // Import operations
    bool importNotebook(const QString &filePath);
    bool importFromLotus(const QString &filePath);
    bool importFromIpynb(const QString &filePath);

    // Export operations
    bool exportNotebook(const QString &filePath, ExportFormat format);
    bool exportToLotus(const QString &filePath);
    bool exportToIpynb(const QString &filePath);
    bool exportToHtml(const QString &filePath);
    bool exportToPython(const QString &filePath);

    // Cell operations
    void addCell(const CellData &cell);
    void insertCell(int index, const CellData &cell);
    void updateCell(int index, const CellData &cell);
    void deleteCell(int index);
    void moveCell(int fromIndex, int toIndex);
    void clearCells();

    // Get data
    const QList<CellData> &getCells() const { return cells; }
    QList<CellData> &getCells() { return cells; }
    CellData getCell(int index) const;
    int getCellCount() const { return cells.count(); }

    // Metadata
    QString getNotebookName() const { return notebookName; }
    void setNotebookName(const QString &name) { notebookName = name; }

    QString getNotebookDescription() const { return description; }
    void setNotebookDescription(const QString &desc) { description = desc; }

    QString getKernelSpec() const { return kernelSpec; }
    void setKernelSpec(const QString &spec) { kernelSpec = spec; }

signals:
    void cellsChanged();
    void notebookLoaded(const QString &name);
    void notebookSaved(const QString &path);
    void exportCompleted(const QString &path);
    void importCompleted(const QString &path, bool success);

private:
    QList<CellData> cells;
    QString notebookName;
    QString description;
    QString kernelSpec;
    QString createdDate;
    QString modifiedDate;
    int formatVersion;

    // Parsing helpers
    bool parseCell(const QJsonObject &cellJson, CellData &cellData);
    QJsonObject cellToJson(const CellData &cell) const;
    bool convertIpynbToLotus(const QJsonObject &ipynbJson);

    // Export helpers
    QString generateHtmlHeader() const;
    QString generateHtmlFooter() const;
    QString cellToHtml(const CellData &cell) const;
    QString escapeHtml(const QString &text) const;
};

#endif // NOTEBOOKMANAGER_H
