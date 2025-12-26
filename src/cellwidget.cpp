#include "cellwidget.h"
#include "syntaxhighlighter.h"

#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QScrollBar>
#include <QStyle>
#include <QMouseEvent>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

int CellWidget::cellCounter = 0;

CellWidget::CellWidget(CellType type, QWidget *parent)
    : QFrame(parent)
    , cellType(type)
    , executing(false)
    , readOnly(false)
    , editMode(true)
    , executionCount(-1)
    , cellLabel(nullptr)
    , runButton(nullptr)
    , menuButton(nullptr)
    , contentStack(nullptr)
    , editor(nullptr)
    , markdownViewer(nullptr)
    , outputContainer(nullptr)
    , outputLayout(nullptr)
    , outputScrollArea(nullptr)
    , outputContentWidget(nullptr)
    , contextMenu(nullptr)
    , dropIndicator(nullptr)
{
    cellIndex = ++cellCounter;
    setupUi();
    setupConnections();
    updateType();
}

CellWidget::~CellWidget()
{
}

void CellWidget::setupUi()
{
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Raised);
    setLineWidth(1);
    setAcceptDrops(true);

    // Main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Header layout
    headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);

    // Cell label
    cellLabel = new QLabel(this);
    cellLabel->setStyleSheet(R"(
        QLabel {
            color: #666;
            font-size: 12px;
            min-width: 60px;
        }
    )");
    headerLayout->addWidget(cellLabel);

    // Run button
    runButton = new QPushButton(this);
    runButton->setIcon(QIcon::fromTheme("media-playback-start"));
    runButton->setToolTip("Run cell (Ctrl+Enter)");
    runButton->setMaximumSize(30, 30);
    runButton->setStyleSheet(R"(
        QPushButton {
            border: none;
            background: transparent;
            icon-size: 20px;
        }
        QPushButton:hover {
            background: #e0e0e0;
            border-radius: 4px;
        }
    )");
    headerLayout->addWidget(runButton);

    // Move up button
    QPushButton *moveUpButton = new QPushButton(this);
    moveUpButton->setIcon(QIcon::fromTheme("go-up"));
    moveUpButton->setToolTip("Move cell up (Alt+Up)");
    moveUpButton->setMaximumSize(24, 24);
    moveUpButton->setStyleSheet(R"(
        QPushButton {
            border: none;
            background: transparent;
            icon-size: 16px;
        }
        QPushButton:hover {
            background: #e0e0e0;
            border-radius: 4px;
        }
    )");
    headerLayout->addWidget(moveUpButton);

    // Move down button
    QPushButton *moveDownButton = new QPushButton(this);
    moveDownButton->setIcon(QIcon::fromTheme("go-down"));
    moveDownButton->setToolTip("Move cell down (Alt+Down)");
    moveDownButton->setMaximumSize(24, 24);
    moveDownButton->setStyleSheet(R"(
        QPushButton {
            border: none;
            background: transparent;
            icon-size: 16px;
        }
        QPushButton:hover {
            background: #e0e0e0;
            border-radius: 4px;
        }
    )");
    headerLayout->addWidget(moveDownButton);

    headerLayout->addStretch();

    // Menu button
    menuButton = new QToolButton(this);
    menuButton->setIcon(QIcon::fromTheme("format-list-unordered"));
    menuButton->setPopupMode(QToolButton::InstantPopup);
    menuButton->setToolTip("Cell operations");
    menuButton->setMaximumSize(30, 30);
    menuButton->setStyleSheet(R"(
        QToolButton {
            border: none;
            background: transparent;
            icon-size: 20px;
        }
        QToolButton:hover {
            background: #e0e0e0;
            border-radius: 4px;
        }
    )");
    headerLayout->addWidget(menuButton);

    mainLayout->addLayout(headerLayout);

    // Content stack for edit/render mode (Phase 3)
    contentStack = new QStackedWidget(this);

    // Editor
    editor = new QTextEdit(this);
    editor->setPlaceholderText("Enter code here...");
    editor->setFont(QFont("Fira Code", 11));
    editor->setTabStopDistance(40);
    editor->setLineWrapMode(QTextEdit::WidgetWidth);
    editor->setStyleSheet(R"(
        QTextEdit {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 8px;
            font-family: 'Fira Code', 'Consolas', monospace;
            font-size: 11px;
        }
        QTextEdit:focus {
            border-color: #2E7D32;
        }
    )");

    // Add syntax highlighter for code cells
    if (cellType == CodeCell) {
        new SyntaxHighlighter(editor->document());
    }

    contentStack->addWidget(editor);

    // Markdown viewer (Phase 3)
    markdownViewer = new QTextBrowser(this);
    markdownViewer->setStyleSheet(R"(
        QTextBrowser {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 8px;
            font-family: 'Arial', sans-serif;
            font-size: 12px;
        }
        QTextBrowser:focus {
            border-color: #2E7D32;
        }
    )");
    markdownViewer->setOpenExternalLinks(true);
    markdownViewer->setFocusPolicy(Qt::ClickFocus);
    markdownViewer->installEventFilter(this);
    contentStack->addWidget(markdownViewer);

    mainLayout->addWidget(contentStack);

    // Phase 2: Enhanced scrollable output area
    outputContainer = new QWidget(this);
    outputContainer->setStyleSheet(R"(
        QWidget {
            background-color: #fafafa;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
        }
    )");

    outputLayout = new QVBoxLayout(outputContainer);
    outputLayout->setContentsMargins(8, 8, 8, 8);
    outputLayout->setSpacing(8);

    // Scroll area for outputs
    outputScrollArea = new QScrollArea(outputContainer);
    outputScrollArea->setWidgetResizable(true);
    outputScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    outputScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    outputScrollArea->setFrameStyle(QFrame::NoFrame);
    outputScrollArea->setStyleSheet(R"(
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollArea > QWidget > QWidget {
            background: transparent;
        }
        QScrollBar:vertical {
            width: 10px;
            background: #f0f0f0;
            border-radius: 5px;
        }
        QScrollBar:vertical::handle {
            background: #c0c0c0;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar:vertical::add-line, QScrollBar:vertical::sub-line {
            height: 0px;
        }
    )");

    outputContentWidget = new QWidget();
    outputContentWidget->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(outputContentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);

    outputScrollArea->setWidget(outputContentWidget);
    outputLayout->addWidget(outputScrollArea);

    outputContainer->setVisible(false);

    mainLayout->addWidget(outputContainer);

    // Drop indicator (Phase 3)
    dropIndicator = new QFrame(this);
    dropIndicator->setStyleSheet(R"(
        QFrame {
            background-color: #2E7D32;
            border: none;
        }
    )");
    dropIndicator->setFixedHeight(4);
    dropIndicator->setVisible(false);

    // Context menu setup
    setupContextMenu();
}

void CellWidget::setupContextMenu()
{
    contextMenu = new QMenu(this);

    QAction *runAction = new QAction("Run Cell", this);
    runAction->setIcon(QIcon::fromTheme("media-playback-start"));
    runAction->setShortcut(QKeySequence("Ctrl+Enter"));
    connect(runAction, &QAction::triggered, this, &CellWidget::onRunButtonClicked);
    contextMenu->addAction(runAction);

    contextMenu->addSeparator();

    QAction *insertAboveAction = new QAction("Insert Cell Above", this);
    insertAboveAction->setShortcut(QKeySequence("Alt+Enter"));
    connect(insertAboveAction, &QAction::triggered, this, &CellWidget::onInsertAboveAction);
    contextMenu->addAction(insertAboveAction);

    QAction *insertBelowAction = new QAction("Insert Cell Below", this);
    connect(insertBelowAction, &QAction::triggered, this, &CellWidget::onInsertBelowAction);
    contextMenu->addAction(insertBelowAction);

    contextMenu->addSeparator();

    QAction *moveUpAction = new QAction("Move Cell Up", this);
    moveUpAction->setShortcut(QKeySequence("Alt+Up"));
    connect(moveUpAction, &QAction::triggered, this, &CellWidget::onMoveUpAction);
    contextMenu->addAction(moveUpAction);

    QAction *moveDownAction = new QAction("Move Cell Down", this);
    moveDownAction->setShortcut(QKeySequence("Alt+Down"));
    connect(moveDownAction, &QAction::triggered, this, &CellWidget::onMoveDownAction);
    contextMenu->addAction(moveDownAction);

    contextMenu->addSeparator();

    QAction *copyAction = new QAction("Copy", this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &CellWidget::onCopyAction);
    contextMenu->addAction(copyAction);

    QAction *cutAction = new QAction("Cut", this);
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, this, &CellWidget::onCutAction);
    contextMenu->addAction(cutAction);

    QAction *pasteAction = new QAction("Paste", this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &CellWidget::onPasteAction);
    contextMenu->addAction(pasteAction);

    // Markdown toggle action (Phase 3)
    if (cellType == MarkdownCell) {
        contextMenu->addSeparator();

        QAction *toggleEditAction = new QAction("Toggle Edit/Render", this);
        toggleEditAction->setShortcut(QKeySequence("Ctrl+Shift+M"));
        connect(toggleEditAction, &QAction::triggered, this, [this]() {
            setEditMode(!editMode);
        });
        contextMenu->addAction(toggleEditAction);
    }

    contextMenu->addSeparator();

    QAction *deleteAction = new QAction("Delete Cell", this);
    deleteAction->setIcon(QIcon::fromTheme("edit-delete"));
    connect(deleteAction, &QAction::triggered, this, &CellWidget::onDeleteAction);
    contextMenu->addAction(deleteAction);

    menuButton->setMenu(contextMenu);
}

void CellWidget::setupConnections()
{
    // Find the buttons we created in setupUi
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    for (QPushButton *btn : buttons) {
        if (btn && btn->toolTip().contains("Move cell up")) {
            connect(btn, &QPushButton::clicked, this, &CellWidget::onMoveUpAction);
        } else if (btn && btn->toolTip().contains("Move cell down")) {
            connect(btn, &QPushButton::clicked, this, &CellWidget::onMoveDownAction);
        }
    }

    if (runButton) {
        connect(runButton, &QPushButton::clicked, this, &CellWidget::onRunButtonClicked);
    }
    if (editor) {
        connect(editor, &QTextEdit::textChanged, this, &CellWidget::onTextChanged);
    }
}

void CellWidget::updateType()
{
    switch (cellType) {
        case CodeCell:
            cellLabel->setText("In [ ]:");
            runButton->setVisible(true);
            editor->setPlaceholderText("Enter Python code...");
            editor->setFont(QFont("Fira Code", 11));
            contentStack->setCurrentWidget(editor);
            editMode = true;
            break;

        case MarkdownCell:
            cellLabel->setText("Markdown:");
            runButton->setVisible(false);
            editor->setPlaceholderText("Enter markdown text...");
            editor->setFont(QFont("Arial", 11));
            contentStack->setCurrentWidget(editor);
            editMode = true;
            break;
    }
}

QString CellWidget::getCode() const
{
    if (cellType != CodeCell) {
        return QString();
    }
    return editor->toPlainText();
}

QString CellWidget::getMarkdown() const
{
    if (cellType != MarkdownCell) {
        return QString();
    }
    return editor->toPlainText();
}

QString CellWidget::getContent() const
{
    return editor->toPlainText();
}

void CellWidget::setCode(const QString &code)
{
    if (cellType == CodeCell) {
        editor->setPlainText(code);
    }
}

void CellWidget::setMarkdown(const QString &markdown)
{
    if (cellType == MarkdownCell) {
        editor->setPlainText(markdown);
    }
}

void CellWidget::setContent(const QString &content)
{
    editor->setPlainText(content);
}

// Phase 3: Markdown rendering
void CellWidget::setEditMode(bool edit)
{
    editMode = edit;

    if (cellType == MarkdownCell) {
        if (editMode) {
            // Switch to edit mode
            contentStack->setCurrentWidget(editor);
            editor->setFocus();
        } else {
            // Switch to render mode
            renderMarkdown();
            contentStack->setCurrentWidget(markdownViewer);
        }
    }
}

void CellWidget::renderMarkdown()
{
    if (cellType != MarkdownCell) {
        return;
    }

    QString markdown = editor->toPlainText();

    // Basic markdown to HTML conversion
    // Note: For full markdown support, Python's markdown library should be used
    // This is a simple client-side implementation for quick rendering
    QString html = markdown;

    // Escape HTML first
    html.replace("&", "&amp;");
    html.replace("<", "&lt;");
    html.replace(">", "&gt;");

    // Headers
    html.replace(QRegExp("^###### (.+)$"), "<h6>\\1</h6>");
    html.replace(QRegExp("^##### (.+)$"), "<h5>\\1</h5>");
    html.replace(QRegExp("^#### (.+)$"), "<h4>\\1</h4>");
    html.replace(QRegExp("^### (.+)$"), "<h3>\\1</h3>");
    html.replace(QRegExp("^## (.+)$"), "<h2>\\1</h2>");
    html.replace(QRegExp("^# (.+)$"), "<h1>\\1</h1>");

    // Bold
    html.replace(QRegExp("\\*\\*(.+?)\\*\\*"), "<strong>\\1</strong>");
    html.replace(QRegExp("__(.+?)__"), "<strong>\\1</strong>");

    // Italic
    html.replace(QRegExp("\\*(.+?)\\*"), "<em>\\1</em>");
    html.replace(QRegExp("_(.+?)_"), "<em>\\1</em>");

    // Code blocks
    html.replace(QRegExp("```(\\w*)\\n([\\s\\S]+?)```"), "<pre><code>\\2</code></pre>");

    // Inline code
    html.replace(QRegExp("`(.+?)`"), "<code>\\1</code>");

    // Lists
    html.replace(QRegExp("^\\* (.+)$"), "<li>\\1</li>");
    html.replace(QRegExp("^- (.+)$"), "<li>\\1</li>");
    html.replace(QRegExp("^\\d+\\. (.+)$"), "<li>\\1</li>");

    // Links
    html.replace(QRegExp("\\[([^\\]]+)\\]\\(([^)]+)\\)"), "<a href=\"\\2\">\\1</a>");

    // Horizontal rules
    html.replace(QRegExp("^---+$"), "<hr>");

    // Blockquotes
    html.replace(QRegExp("^> (.+)$"), "<blockquote>\\1</blockquote>");

    // Paragraphs (double line breaks)
    html.replace("\n\n", "<br><br>");

    // Line breaks
    html.replace("\n", "<br>");

    // Wrap in container
    QString styledHtml = QString(R"(
        <style>
            .lotus-markdown {
                font-family: 'Arial', sans-serif;
                font-size: 12px;
                line-height: 1.6;
                color: #333;
            }
            .lotus-markdown h1 {
                font-size: 24px;
                border-bottom: 1px solid #e0e0e0;
                padding-bottom: 8px;
                margin-bottom: 16px;
            }
            .lotus-markdown h2 {
                font-size: 20px;
                border-bottom: 1px solid #e0e0e0;
                padding-bottom: 6px;
                margin-bottom: 14px;
            }
            .lotus-markdown h3 {
                font-size: 16px;
                margin-bottom: 12px;
            }
            .lotus-markdown pre {
                background-color: #f5f5f5;
                padding: 12px;
                border-radius: 4px;
                overflow-x: auto;
                font-family: 'Fira Code', monospace;
                font-size: 11px;
            }
            .lotus-markdown code {
                background-color: #f5f5f5;
                padding: 2px 6px;
                border-radius: 3px;
                font-family: 'Fira Code', monospace;
                font-size: 11px;
            }
            .lotus-markdown pre code {
                background-color: transparent;
                padding: 0;
            }
            .lotus-markdown blockquote {
                border-left: 4px solid #2E7D32;
                margin: 0;
                padding-left: 16px;
                color: #666;
            }
            .lotus-markdown a {
                color: #1976d2;
                text-decoration: none;
            }
            .lotus-markdown a:hover {
                text-decoration: underline;
            }
            .lotus-markdown hr {
                border: none;
                border-top: 1px solid #e0e0e0;
                margin: 16px 0;
            }
            .lotus-markdown li {
                margin: 4px 0;
            }
        </style>
        <div class="lotus-markdown">%1</div>
    )").arg(html);

    markdownViewer->setText(styledHtml);
}

// Phase 2: Enhanced output methods
void CellWidget::setOutput(const QString &text, OutputType type)
{
    clearOutput();
    addOutput(text, type);
}

void CellWidget::setHtmlOutput(const QString &html)
{
    clearOutput();
    addHtmlOutput(html);
}

void CellWidget::setTableOutput(const QString &htmlTable)
{
    clearOutput();

    // Add styled table container
    QLabel *tableLabel = new QLabel(outputContentWidget);
    QString styledTable = QString(R"(
        <style>
            .lotus-dataframe {
                font-family: 'Fira Code', 'Consolas', monospace;
                font-size: 11px;
                border-collapse: collapse;
                width: 100%%;
            }
            .lotus-dataframe th {
                background-color: #2E7D32;
                color: white;
                padding: 8px;
                text-align: left;
            }
            .lotus-dataframe td {
                padding: 6px 8px;
                border-bottom: 1px solid #e0e0e0;
            }
            .lotus-dataframe tr:nth-child(even) {
                background-color: #f5f5f5;
            }
            .lotus-dataframe tr:hover {
                background-color: #e8f5e9;
            }
        </style>
        %1
    )").arg(htmlTable);

    tableLabel->setText(styledTable);
    tableLabel->setTextFormat(Qt::RichText);
    tableLabel->setWordWrap(true);
    tableLabel->setStyleSheet(R"(
        QLabel {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 8px;
        }
    )");

    outputContentWidget->layout()->addWidget(tableLabel);
    outputWidgets.append(tableLabel);
    outputContainer->setVisible(true);
}

void CellWidget::setRichOutput(const QString &html)
{
    clearOutput();

    QLabel *richLabel = new QLabel(outputContentWidget);
    richLabel->setText(html);
    richLabel->setTextFormat(Qt::RichText);
    richLabel->setWordWrap(true);
    richLabel->setStyleSheet(R"(
        QLabel {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 8px;
        }
    )");

    outputContentWidget->layout()->addWidget(richLabel);
    outputWidgets.append(richLabel);
    outputContainer->setVisible(true);
}

void CellWidget::addOutput(const QString &text, OutputType type)
{
    QLabel *outputLabel = new QLabel(outputContentWidget);

    switch (type) {
        case TextOutput:
            outputLabel->setStyleSheet(R"(
                QLabel {
                    background-color: #ffffff;
                    border: 1px solid #e0e0e0;
                    border-radius: 4px;
                    padding: 8px;
                    font-family: 'Fira Code', 'Consolas', monospace;
                    font-size: 11px;
                    color: #333;
                    min-height: 20px;
                }
            )");
            outputLabel->setText(text);
            break;

        case ErrorOutput:
            outputLabel->setStyleSheet(R"(
                QLabel {
                    background-color: #ffebee;
                    border: 1px solid #ffcdd2;
                    border-radius: 4px;
                    padding: 8px;
                    font-family: 'Fira Code', 'Consolas', monospace;
                    font-size: 11px;
                    color: #c62828;
                    min-height: 20px;
                }
            )");
            outputLabel->setText(text);
            break;

        case HtmlOutput:
        case TableOutput:
            outputLabel->setText(text);
            outputLabel->setTextFormat(Qt::RichText);
            break;

        case ImageOutput:
            // Handle image separately
            break;

        case RichOutput:
            outputLabel->setText(text);
            outputLabel->setTextFormat(Qt::RichText);
            break;
    }

    outputLabel->setWordWrap(true);
    outputContentWidget->layout()->addWidget(outputLabel);
    outputWidgets.append(outputLabel);
    outputContainer->setVisible(true);

    // Auto-scroll to bottom
    outputScrollArea->verticalScrollBar()->setValue(
        outputScrollArea->verticalScrollBar()->maximum());
}

void CellWidget::addHtmlOutput(const QString &html)
{
    QLabel *htmlLabel = new QLabel(outputContentWidget);

    // Add default styling for HTML content
    QString styledHtml = QString(R"(
        <style>
            .lotus-output {
                font-family: Arial, sans-serif;
                font-size: 12px;
                color: #333;
            }
            .lotus-error {
                background-color: #ffebee;
                border: 1px solid #ffcdd2;
                border-radius: 4px;
                padding: 8px;
            }
            .lotus-error-type {
                color: #c62828;
                font-weight: bold;
                font-family: 'Fira Code', monospace;
            }
            .lotus-traceback {
                font-family: 'Fira Code', monospace;
                font-size: 11px;
                color: #333;
                white-space: pre-wrap;
                background-color: #fafafa;
                padding: 8px;
                border-radius: 4px;
                overflow-x: auto;
            }
        </style>
        <div class="lotus-output">%1</div>
    )").arg(html);

    htmlLabel->setText(styledHtml);
    htmlLabel->setTextFormat(Qt::RichText);
    htmlLabel->setWordWrap(true);
    htmlLabel->setStyleSheet(R"(
        QLabel {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 8px;
        }
    )");

    outputContentWidget->layout()->addWidget(htmlLabel);
    outputWidgets.append(htmlLabel);
    outputContainer->setVisible(true);

    // Auto-scroll to bottom
    outputScrollArea->verticalScrollBar()->setValue(
        outputScrollArea->verticalScrollBar()->maximum());
}

void CellWidget::addPlot(const QByteArray &imageData)
{
    QLabel *plotLabel = new QLabel(outputContentWidget);
    plotLabel->setAlignment(Qt::AlignCenter);

    QPixmap pixmap;
    if (pixmap.loadFromData(imageData)) {
        // Scale image while maintaining aspect ratio
        int maxWidth = 600;
        if (pixmap.width() > maxWidth) {
            pixmap = pixmap.scaled(maxWidth, pixmap.height() * maxWidth / pixmap.width(),
                                   Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        plotLabel->setPixmap(pixmap);
    }

    plotLabel->setStyleSheet(R"(
        QLabel {
            background-color: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 8px;
            min-height: 50px;
        }
    )");

    outputContentWidget->layout()->addWidget(plotLabel);
    outputWidgets.append(plotLabel);
    outputContainer->setVisible(true);

    // Auto-scroll to bottom
    outputScrollArea->verticalScrollBar()->setValue(
        outputScrollArea->verticalScrollBar()->maximum());
}

void CellWidget::setPlot(const QByteArray &imageData)
{
    clearOutput();
    addPlot(imageData);
}

void CellWidget::clearOutput()
{
    // Remove all output widgets
    for (QWidget *widget : outputWidgets) {
        outputContentWidget->layout()->removeWidget(widget);
        delete widget;
    }
    outputWidgets.clear();
    outputContainer->setVisible(false);
}

QJsonObject CellWidget::toJson() const
{
    QJsonObject json;

    json["type"] = (cellType == CodeCell) ? "code" : "markdown";
    json["content"] = editor->toPlainText();
    json["execution_count"] = executionCount;

    return json;
}

void CellWidget::fromJson(const QJsonObject &json)
{
    QString type = json["type"].toString();
    QString content = json["content"].toString();
    executionCount = json["execution_count"].toInt(-1);

    if (type == "code") {
        cellType = CodeCell;
    } else {
        cellType = MarkdownCell;
    }

    editor->setPlainText(content);
    updateType();
}

void CellWidget::insertCellAbove()
{
    emit insertAboveRequested(this);
}

void CellWidget::insertCellBelow()
{
    emit insertBelowRequested(this);
}

void CellWidget::deleteCell()
{
    emit deleteRequested(this);
}

void CellWidget::setReadOnly(bool ro)
{
    readOnly = ro;
    editor->setReadOnly(readOnly);

    if (readOnly) {
        runButton->setEnabled(false);
        menuButton->setEnabled(false);
    } else {
        runButton->setEnabled(true);
        menuButton->setEnabled(true);
    }
}

void CellWidget::setFocus()
{
    if (cellType == MarkdownCell && !editMode) {
        markdownViewer->setFocus();
    } else {
        editor->setFocus();
    }
}

// Phase 5: Theme support
void CellWidget::updateTheme()
{
    // Update styles based on current theme
    // The styles are managed by the application-wide stylesheet
    // This method can be used for any widget-specific theme updates
    update();
}

void CellWidget::setEditorFont(const QFont &font)
{
    if (editor) {
        editor->setFont(font);
    }
    if (markdownViewer) {
        markdownViewer->setFont(font);
    }
}

// Phase 3: Drag and drop support
void CellWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();
    }
    QFrame::mousePressEvent(event);
}

void CellWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    // Only start drag if mouse moved more than a threshold
    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    // Create drag object
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData();

    // Store cell index and type in MIME data
    QJsonObject mimeJson;
    mimeJson["cellIndex"] = cellIndex;
    mimeJson["cellType"] = (cellType == CodeCell) ? "code" : "markdown";

    QJsonDocument doc(mimeJson);
    mimeData->setData("application/x-lotus-cell", doc.toJson());

    drag->setMimeData(mimeData);

    // Create pixmap for drag image
    QPixmap pixmap(size());
    render(&pixmap);
    drag->setPixmap(pixmap);

    // Set hot spot
    drag->setHotSpot(event->pos());

    // Execute drag
    Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

    // After drag, notify parent to potentially reorder cells
    if (dropAction == Qt::MoveAction) {
        emit contentChanged();
    }
}

void CellWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-lotus-cell")) {
        event->acceptProposedAction();
        dropIndicator->setVisible(true);
    }
}

void CellWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    dropIndicator->setVisible(false);
}

void CellWidget::dropEvent(QDropEvent *event)
{
    dropIndicator->setVisible(false);

    if (!event->mimeData()->hasFormat("application/x-lotus-cell")) {
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(event->mimeData()->data("application/x-lotus-cell"), &error);

    if (error.error != QJsonParseError::NoError) {
        return;
    }

    QJsonObject mimeJson = doc.object();
    int sourceIndex = mimeJson["cellIndex"].toInt();

    // Notify parent to handle reordering
    emit cellIndexChanged(this, sourceIndex, cellIndex);

    event->acceptProposedAction();
}

QSize CellWidget::sizeHint() const
{
    return QSize(800, minimumHeight());
}

QSize CellWidget::minimumSizeHint() const
{
    return QSize(600, 150);
}

void CellWidget::onRunButtonClicked()
{
    emit runRequested(this);
}

void CellWidget::onMenuButtonClicked()
{
    contextMenu->exec(QCursor::pos());
}

void CellWidget::onDeleteAction()
{
    emit deleteRequested(this);
}

void CellWidget::onInsertAboveAction()
{
    emit insertAboveRequested(this);
}

void CellWidget::onInsertBelowAction()
{
    emit insertBelowRequested(this);
}

void CellWidget::onMoveUpAction()
{
    emit moveUpRequested(this);
}

void CellWidget::onMoveDownAction()
{
    emit moveDownRequested(this);
}

void CellWidget::onCopyAction()
{
    QApplication::clipboard()->setText(editor->toPlainText());
}

void CellWidget::onCutAction()
{
    QApplication::clipboard()->setText(editor->toPlainText());
    editor->clear();
}

void CellWidget::onPasteAction()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (mimeData->hasText()) {
        editor->insertPlainText(mimeData->text());
    }
}

void CellWidget::onTextChanged()
{
    emit contentChanged();
}

void CellWidget::updateExecutionState()
{
    if (executing) {
        runButton->setIcon(QIcon::fromTheme("process-stop"));
        runButton->setToolTip("Interrupt kernel (Ctrl+C)");
        cellLabel->setText("In [*]:");
    } else if (executionCount >= 0) {
        runButton->setIcon(QIcon::fromTheme("media-playback-start"));
        runButton->setToolTip("Run cell (Ctrl+Enter)");
        cellLabel->setText(QString("In [%1]:").arg(executionCount));
    } else {
        runButton->setIcon(QIcon::fromTheme("media-playback-start"));
        runButton->setToolTip("Run cell (Ctrl+Enter)");
        cellLabel->setText("In [ ]:");
    }
}

void CellWidget::onMarkdownDoubleClick(QMouseEvent *event)
{
    Q_UNUSED(event);
    // Toggle to edit mode when markdown viewer is double-clicked
    if (cellType == MarkdownCell && !editMode) {
        setEditMode(true);
    }
}
