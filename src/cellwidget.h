#ifndef CELLWIDGET_H
#define CELLWIDGET_H

#include <QWidget>
#include <QString>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTextBrowser>
#include <QToolButton>
#include <QFrame>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QScrollArea>
#include <QStackedWidget>
#include <QMimeData>
#include <QDrag>

class CellWidget : public QFrame
{
    Q_OBJECT

public:
    enum CellType {
        CodeCell,
        MarkdownCell
    };

    enum OutputType {
        TextOutput,
        ErrorOutput,
        HtmlOutput,
        ImageOutput,
        TableOutput,
        RichOutput
    };

    explicit CellWidget(CellType type, QWidget *parent = nullptr);
    ~CellWidget();

    // Cell properties
    CellType getType() const { return cellType; }
    void setType(CellType type);

    // Content management
    QString getCode() const;
    QString getMarkdown() const;
    void setCode(const QString &code);
    void setMarkdown(const QString &markdown);
    void setContent(const QString &content);
    QString getContent() const;

    // Output management - Phase 2 enhanced
    void setOutput(const QString &text, OutputType type = TextOutput);
    void setHtmlOutput(const QString &html);
    void setTableOutput(const QString &htmlTable);
    void setRichOutput(const QString &html);
    void setPlot(const QByteArray &imageData);
    void clearOutput();

    // Multiple outputs support
    void addOutput(const QString &text, OutputType type);
    void addHtmlOutput(const QString &html);
    void addPlot(const QByteArray &imageData);

    // Execution state
    bool isExecuting() const { return executing; }
    void setExecuting(bool exec) { executing = exec; updateExecutionState(); }

    // Execution count (Jupyter-style numbering)
    int getExecutionCount() const { return executionCount; }
    void setExecutionCount(int count) { executionCount = count; updateExecutionState(); }
    void clearExecutionCount() { executionCount = -1; updateExecutionState(); }

    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

    // Cell operations
    void insertCellAbove();
    void insertCellBelow();
    void deleteCell();

    // Edit state
    void setReadOnly(bool readOnly);
    bool isReadOnly() const { return readOnly; }

    // Focus
    void setFocus();

    // Drag and drop support
    int getCellIndex() const { return cellIndex; }
    void setCellIndex(int index) { cellIndex = index; }

    // Markdown rendering (Phase 3)
    bool isInEditMode() const { return editMode; }
    void setEditMode(bool edit);
    void renderMarkdown();

    // Phase 5: Theme support
    void updateTheme();
    void setEditorFont(const QFont &font);

signals:
    void contentChanged();
    void executed(bool success);
    void deleteRequested(CellWidget *cell);
    void runRequested(CellWidget *cell);
    void insertAboveRequested(CellWidget *cell);
    void insertBelowRequested(CellWidget *cell);
    void moveUpRequested(CellWidget *cell);
    void moveDownRequested(CellWidget *cell);
    void cellIndexChanged(CellWidget *cell, int oldIndex, int newIndex);
    void toggleEditModeRequested(CellWidget *cell);

private slots:
    void onRunButtonClicked();
    void onMenuButtonClicked();
    void onDeleteAction();
    void onInsertAboveAction();
    void onInsertBelowAction();
    void onMoveUpAction();
    void onMoveDownAction();
    void onCopyAction();
    void onCutAction();
    void onPasteAction();
    void onTextChanged();
    void updateExecutionState();
    void onMarkdownDoubleClick(QMouseEvent *event);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    void setupUi();
    void setupConnections();
    void setupContextMenu();
    void updateType();

    CellType cellType;
    bool executing;
    bool readOnly;
    bool editMode;
    int executionCount;

    // Layout
    QVBoxLayout *mainLayout;
    QHBoxLayout *headerLayout;

    // Header components
    QLabel *cellLabel;
    QPushButton *runButton;
    QToolButton *menuButton;

    // Editor stack for markdown toggle (Phase 3)
    QStackedWidget *contentStack;
    QTextEdit *editor;
    QTextBrowser *markdownViewer;

    // Output area - Phase 2 enhanced with scrollable outputs
    QWidget *outputContainer;
    QVBoxLayout *outputLayout;
    QScrollArea *outputScrollArea;
    QWidget *outputContentWidget;

    // Multiple output widgets
    QList<QWidget*> outputWidgets;

    // Context menu
    QMenu *contextMenu;

    // Drag and drop
    QPoint dragStartPosition;

    // Drop indicator
    QFrame *dropIndicator;

    static int cellCounter;
    int cellIndex;
};

#endif // CELLWIDGET_H
