#include "notebookmanager.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QJsonDocument>
#include <QDebug>
#include <QTextDocument>

NotebookManager::NotebookManager(QObject *parent)
    : QObject(parent)
    , notebookName("Untitled Notebook")
    , description("")
    , kernelSpec("python3")
    , createdDate(QDateTime::currentDateTime().toString(Qt::ISODate))
    , modifiedDate(QDateTime::currentDateTime().toString(Qt::ISODate))
    , formatVersion(1)
{
}

bool NotebookManager::loadNotebook(const QJsonObject &root)
{
    cells.clear();

    // Check version
    int version = root["version"].toInt(1);
    if (version > formatVersion) {
        qWarning() << "Unsupported notebook version:" << version;
        return false;
    }

    // Load metadata
    notebookName = root["notebook_name"].toString("Untitled Notebook");
    description = root["description"].toString("");
    kernelSpec = root["kernel"].toString("python3");
    createdDate = root["created_at"].toString(QDateTime::currentDateTime().toString(Qt::ISODate));
    modifiedDate = root["modified_at"].toString(QDateTime::currentDateTime().toString(Qt::ISODate));

    // Load cells
    QJsonArray cellsArray = root["cells"].toArray();

    for (const QJsonValue &cellValue : cellsArray) {
        if (!cellValue.isObject()) {
            continue;
        }

        QJsonObject cellJson = cellValue.toObject();
        CellData cellData;

        if (parseCell(cellJson, cellData)) {
            cells.append(cellData);
        }
    }

    emit cellsChanged();
    emit notebookLoaded(notebookName);

    return true;
}

QJsonObject NotebookManager::saveNotebook() const
{
    QJsonObject root;

    root["version"] = formatVersion;
    root["notebook_name"] = notebookName;
    root["description"] = description;
    root["kernel"] = kernelSpec;
    root["created_at"] = createdDate;
    root["modified_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Save cells
    QJsonArray cellsArray;

    for (const CellData &cell : cells) {
        cellsArray.append(cellToJson(cell));
    }

    root["cells"] = cellsArray;

    return root;
}

bool NotebookManager::importNotebook(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    if (extension == "ipynb") {
        return importFromIpynb(filePath);
    } else if (extension == "lotus") {
        return importFromLotus(filePath);
    } else {
        // Try to detect format from content
        return importFromLotus(filePath);
    }
}

bool NotebookManager::importFromLotus(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse notebook:" << error.errorString();
        return false;
    }

    bool success = loadNotebook(doc.object());
    if (success) {
        emit importCompleted(filePath, true);
    } else {
        emit importCompleted(filePath, false);
    }

    return success;
}

bool NotebookManager::importFromIpynb(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse ipynb:" << error.errorString();
        emit importCompleted(filePath, false);
        return false;
    }

    QJsonObject root = doc.object();
    bool success = convertIpynbToLotus(root);

    if (success) {
        // Update metadata from ipynb
        QJsonObject metadata = root["metadata"].toObject();
        QJsonObject kernelspec = metadata["kernelspec"].toObject();
        kernelSpec = kernelspec["name"].toString("python3");
        notebookName = metadata["title"].toString(QFileInfo(filePath).baseName());

        emit importCompleted(filePath, true);
    } else {
        emit importCompleted(filePath, false);
    }

    return success;
}

bool NotebookManager::exportNotebook(const QString &filePath, ExportFormat format)
{
    switch (format) {
        case FormatLotus:
            return exportToLotus(filePath);
        case FormatIpynb:
            return exportToIpynb(filePath);
        case FormatHtml:
            return exportToHtml(filePath);
        case FormatPython:
            return exportToPython(filePath);
        default:
            return exportToLotus(filePath);
    }
}

bool NotebookManager::exportToLotus(const QString &filePath)
{
    QJsonDocument doc(saveNotebook());

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot write to file:" << filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    emit exportCompleted(filePath);
    return true;
}

bool NotebookManager::exportToIpynb(const QString &filePath)
{
    QJsonObject root;

    // Jupyter notebook metadata
    root["nbformat"] = 4;
    root["nbformat_minor"] = 5;

    // Metadata
    QJsonObject metadata;
    QJsonObject kernelspec;
    kernelspec["name"] = kernelSpec.isEmpty() ? "python3" : kernelSpec;
    kernelspec["display_name"] = "Python 3";
    metadata["kernelspec"] = kernelspec;

    // Language info
    QJsonObject languageInfo;
    languageInfo["name"] = "python";
    languageInfo["mimetetype"] = "text/x-python";
    metadata["language_info"] = languageInfo;

    root["metadata"] = metadata;

    // Cells
    QJsonArray cellsArray;

    for (const CellData &cell : cells) {
        QJsonObject cellJson;

        // Cell type
        if (cell.type == CellData::CodeCell) {
            cellJson["cell_type"] = "code";
            if (cell.executionCount > 0) {
                cellJson["execution_count"] = cell.executionCount;
            }
        } else {
            cellJson["cell_type"] = "markdown";
        }

        // Source - Jupyter uses array of lines
        QJsonArray sourceArray;
        QStringList lines = cell.content.split('\n');
        for (const QString &line : lines) {
            sourceArray.append(line);
        }
        cellJson["source"] = sourceArray;

        // Metadata
        QJsonObject cellMetadata;
        cellJson["metadata"] = cellMetadata;

        cellsArray.append(cellJson);
    }

    root["cells"] = cellsArray;

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot write to file:" << filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    emit exportCompleted(filePath);
    return true;
}

bool NotebookManager::exportToHtml(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot write to file:" << filePath;
        return false;
    }

    QTextStream out(&file);

    // Write HTML header
    out << generateHtmlHeader();

    // Write cells
    for (const CellData &cell : cells) {
        out << cellToHtml(cell);
    }

    // Write HTML footer
    out << generateHtmlFooter();

    file.close();

    emit exportCompleted(filePath);
    return true;
}

bool NotebookManager::exportToPython(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot write to file:" << filePath;
        return false;
    }

    QTextStream out(&file);

    // Header
    out << "#!/usr/bin/env python3\n";
    out << "# -*- coding: utf-8 -*-\n";
    out << "# Generated by Lotus Notebook\n";
    out << "# Notebook: " << notebookName << "\n";
    out << "# Exported: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    out << "\n";

    // Write cells
    for (const CellData &cell : cells) {
        if (cell.type == CellData::CodeCell) {
            // Write code cell directly
            if (!cell.content.isEmpty()) {
                out << cell.content << "\n\n";
            }
        } else {
            // Write markdown as comments
            QStringList lines = cell.content.split('\n');
            for (const QString &line : lines) {
                out << "# " << line << "\n";
            }
            out << "\n";
        }
    }

    file.close();

    emit exportCompleted(filePath);
    return true;
}

QString NotebookManager::generateHtmlHeader() const
{
    QString header = QString(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <meta charset=\"utf-8\">\n"
        "    <title>%1</title>\n"
        "    <style>\n"
        "        body {\n"
        "            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;\n"
        "            max-width: 900px;\n"
        "            margin: 0 auto;\n"
        "            padding: 20px;\n"
        "            background-color: #fafafa;\n"
        "            color: #333;\n"
        "        }\n"
        "        h1 { color: #24292e; border-bottom: 1px solid #e1e4e8; padding-bottom: 10px; }\n"
        "        .cell { margin-bottom: 15px; border: 1px solid #e1e4e8; border-radius: 6px; overflow: hidden; }\n"
        "        .code-cell { background-color: #ffffff; }\n"
        "        .markdown-cell { background-color: #ffffff; padding: 15px; }\n"
        "        .input-prompt { color: #6a737d; font-size: 12px; padding: 8px 12px; background-color: #f6f8fa; border-bottom: 1px solid #e1e4e8; }\n"
        "        pre { margin: 0; padding: 15px; overflow-x: auto; font-family: monospace; font-size: 12px; }\n"
        "        code { font-family: monospace; font-size: 85%; padding: 0.2em 0.4em; background-color: rgba(27,31,35,0.05); border-radius: 3px; }\n"
        "        pre code { padding: 0; background-color: transparent; }\n"
        "        .footer { margin-top: 40px; padding-top: 20px; border-top: 1px solid #e1e4e8; color: #6a737d; font-size: 12px; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>%1</h1>\n"
        "    <p><em>Exported from Lotus Notebook on %2</em></p>\n"
    ).arg(notebookName, QDateTime::currentDateTime().toString(Qt::ISODate));

    return header;
}

QString NotebookManager::generateHtmlFooter() const
{
    return QString(
        "\n"
        "    <div class=\"footer\">\n"
        "        <p>Generated by Lotus Notebook</p>\n"
        "    </div>\n"
        "</body>\n"
        "</html>\n"
    );
}

QString NotebookManager::cellToHtml(const CellData &cell) const
{
    if (cell.type == CellData::CodeCell) {
        QString code = escapeHtml(cell.content);
        return QString(R"(
<div class="cell code-cell">
    <div class="input-prompt">In [%1]:</div>
    <pre><code>%2</code></pre>
</div>
)").arg(cell.executionCount > 0 ? QString::number(cell.executionCount) : " ", code);
    } else {
        // Convert markdown to HTML using QTextDocument
        QTextDocument doc;
        doc.setMarkdown(cell.content);
        QString html = doc.toHtml();

        // Clean up the HTML for standalone use
        html.remove(QRegExp("<style.*?</style>", Qt::CaseInsensitive));
        html.remove(QRegExp("<head.*?</head>", Qt::CaseInsensitive));

        return QString(R"(
<div class="cell markdown-cell">
%1
</div>
)").arg(html);
    }
}

QString NotebookManager::escapeHtml(const QString &text) const
{
    QString result = text;
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    result.replace("\"", "&quot;");
    result.replace("'", "&#39;");
    return result;
}

bool NotebookManager::parseCell(const QJsonObject &cellJson, CellData &cellData)
{
    QString typeStr = cellJson["type"].toString("code");

    if (typeStr == "code") {
        cellData.type = CellData::CodeCell;
    } else if (typeStr == "markdown") {
        cellData.type = CellData::MarkdownCell;
    } else {
        return false;
    }

    cellData.content = cellJson["content"].toString("");
    cellData.executionCount = cellJson["execution_count"].toInt(0);
    cellData.wasExecuted = cellJson["was_executed"].toBool(false);

    return true;
}

QJsonObject NotebookManager::cellToJson(const CellData &cell) const
{
    QJsonObject cellJson;

    switch (cell.type) {
        case CellData::CodeCell:
            cellJson["type"] = "code";
            break;
        case CellData::MarkdownCell:
            cellJson["type"] = "markdown";
            break;
    }

    cellJson["content"] = cell.content;
    cellJson["execution_count"] = cell.executionCount;
    cellJson["was_executed"] = cell.wasExecuted;

    return cellJson;
}

bool NotebookManager::convertIpynbToLotus(const QJsonObject &ipynbJson)
{
    cells.clear();

    // Check nbformat version
    int nbformat = ipynbJson["nbformat"].toInt(4);
    if (nbformat > 4) {
        qWarning() << "Warning: Unsupported nbformat version" << nbformat;
    }

    // Extract cells
    QJsonArray cellsArray = ipynbJson["cells"].toArray();

    for (const QJsonValue &cellValue : cellsArray) {
        if (!cellValue.isObject()) {
            continue;
        }

        QJsonObject cellJson = cellValue.toObject();
        CellData cellData;

        // Determine cell type
        QString cellType = cellJson["cell_type"].toString("code");

        if (cellType == "code") {
            cellData.type = CellData::CodeCell;

            // Get execution count
            int execCount = cellJson["execution_count"].toInt(0);
            cellData.executionCount = execCount;
            cellData.wasExecuted = execCount > 0;

            // Get source code
            QJsonArray sourceArray = cellJson["source"].toArray();
            QString source;

            for (const QJsonValue &sourceLine : sourceArray) {
                source += sourceLine.toString();
            }

            cellData.content = source;

        } else if (cellType == "markdown") {
            cellData.type = CellData::MarkdownCell;

            // Get markdown content
            QJsonArray sourceArray = cellJson["source"].toArray();
            QString source;

            for (const QJsonValue &sourceLine : sourceArray) {
                source += sourceLine.toString();
            }

            cellData.content = source;
            cellData.executionCount = 0;
            cellData.wasExecuted = false;
        } else {
            continue;
        }

        cells.append(cellData);
    }

    // Update timestamps
    modifiedDate = QDateTime::currentDateTime().toString(Qt::ISODate);

    emit cellsChanged();

    return true;
}

void NotebookManager::addCell(const CellData &cell)
{
    cells.append(cell);
    emit cellsChanged();
}

void NotebookManager::insertCell(int index, const CellData &cell)
{
    if (index < 0 || index > cells.count()) {
        addCell(cell);
        return;
    }

    cells.insert(index, cell);
    emit cellsChanged();
}

void NotebookManager::updateCell(int index, const CellData &cell)
{
    if (index < 0 || index >= cells.count()) {
        return;
    }

    cells[index] = cell;
    emit cellsChanged();
}

void NotebookManager::deleteCell(int index)
{
    if (index < 0 || index >= cells.count()) {
        return;
    }

    cells.removeAt(index);
    emit cellsChanged();
}

void NotebookManager::moveCell(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= cells.count()) {
        return;
    }

    if (toIndex < 0 || toIndex >= cells.count()) {
        return;
    }

    cells.move(fromIndex, toIndex);
    emit cellsChanged();
}

void NotebookManager::clearCells()
{
    cells.clear();
    emit cellsChanged();
}

NotebookManager::CellData NotebookManager::getCell(int index) const
{
    CellData empty;

    if (index < 0 || index >= cells.count()) {
        return empty;
    }

    return cells[index];
}
