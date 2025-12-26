#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QStatusBar>

#include "cellwidget.h"
#include "notebookmanager.h"
#include "backupmanager.h"
#include "pythonexecutor.h"
#include "commandpalette.h"
#include "thememanager.h"
#include "settingsdialog.h"
#include "variableinspector.h"
#include "codecompleter.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Notebook file operations
    void newNotebook();
    void openNotebook(const QString &filePath = QString());
    void saveNotebook();
    void saveNotebookAs(const QString &filePath = QString());

    // Export operations (Phase 4)
    void exportToHtml();
    void exportToPython();
    void exportToIpynb();
    void exportNotebook(NotebookManager::ExportFormat format);

    // Cell management
    void addCodeCell();
    void addMarkdownCell();
    void executeAllCells();
    void executeCurrentCell();
    void deleteCurrentCell();

    // Backup management
    void enableAutoBackup(bool enabled);
    void createManualBackup();

    void updateStatusBar();
    void updateKernelIndicator();
    void setCurrentCell(CellWidget *cell);
    void moveCell(CellWidget *cell, int direction);  // direction: -1 for up, 1 for down

signals:
    void cellExecuted(int cellIndex, bool success);
    void notebookModified(bool modified);

private slots:
    void onActionNew();
    void onActionOpen();
    void onActionSave();
    void onActionSaveAs();
    void onActionExport();
    void onActionExportHtml();
    void onActionExportPython();
    void onActionExportIpynb();
    void onActionExit();

    void onActionAddCodeCell();
    void onActionAddMarkdownCell();
    void onActionRunCell();
    void onActionRunAll();
    void onActionInterruptKernel();
    void onActionRestartKernel();

    void onActionMoveCellUp();
    void onActionMoveCellDown();

    void onActionToggleBackup();
    void onActionCreateBackup();

    void onActionAbout();
    void onActionSettings();

    // Phase 5: Settings slots
    void onThemeChanged(const QString &theme);
    void onFontChanged(const QFont &font);

    // Phase 5: Variable inspector slots
    void onRefreshVariables();
    void onCellChanged();
    void onAutoSaveTimeout();
    void onCellMoveUp(CellWidget *cell);
    void onCellMoveDown(CellWidget *cell);
    void onCellIndexChanged(CellWidget *cell, int oldIndex, int newIndex);

    // Command palette (Phase 3)
    void showCommandPalette();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();
    void setupCommandPalette();

    void updateWindowTitle();
    void setModified(bool modified);
    bool maybeSave();
    bool saveToFile(const QString &filePath);
    void loadFromFile(const QString &filePath);

    void loadCellsFromManager();
    void clearAllCells();
    void setAllCellsReadOnly(bool readOnly);
    void executeCell(CellWidget *cell);
    void syncCellsToManager();  // Phase 4: Sync cells to notebook manager

    // Cell reordering (Phase 3)
    void reorderCells(int fromIndex, int toIndex);
    void updateCellIndices();
    void updateDropIndicator(const QPoint &pos);

    PythonExecutor *pythonExecutor;
    NotebookManager *notebookManager;
    BackupManager *backupManager;
    CommandPalette *commandPalette;  // Phase 3: Command palette
    ThemeManager *themeManager;  // Phase 5: Theme manager
    SettingsDialog *settingsDialog;  // Phase 5: Settings dialog
    VariableInspector *variableInspector;  // Phase 5: Variable inspector
    CodeCompleter *codeCompleter;  // Phase 5: Code completer

    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QScrollArea *scrollArea;
    QWidget *cellsContainer;
    QVBoxLayout *cellsLayout;

    QMenuBar *menuBar;
    QToolBar *toolBar;
    QStatusBar *statusBar;

    QList<CellWidget *> cells;
    CellWidget *currentCell;

    QString currentFilePath;
    bool isModified;
    bool autoBackupEnabled;
    int autoSaveInterval;
    int executionCounter;
    bool kernelBusy;

    QTimer *autoSaveTimer;
    QTimer *statusResetTimer;

    // Drop indicator for drag and drop (Phase 3)
    QFrame *dropIndicator;
    int dropIndicatorIndex;
    bool showDropIndicator;

    static const int DEFAULT_AUTO_SAVE_INTERVAL = 300000; // 5 minutes
};

#endif // MAINWINDOW_H
