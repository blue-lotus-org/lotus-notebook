#include "mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QIcon>
#include <QMimeData>
#include <QJsonParseError>
#include <QPainter>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , pythonExecutor(nullptr)
    , notebookManager(nullptr)
    , backupManager(nullptr)
    , commandPalette(nullptr)
    , themeManager(nullptr)
    , settingsDialog(nullptr)
    , variableInspector(nullptr)
    , codeCompleter(nullptr)
    , centralWidget(nullptr)
    , mainLayout(nullptr)
    , scrollArea(nullptr)
    , cellsContainer(nullptr)
    , cellsLayout(nullptr)
    , menuBar(nullptr)
    , toolBar(nullptr)
    , statusBar(nullptr)
    , currentCell(nullptr)
    , currentFilePath("")
    , isModified(false)
    , autoBackupEnabled(true)
    , autoSaveInterval(DEFAULT_AUTO_SAVE_INTERVAL)
    , executionCounter(0)
    , kernelBusy(false)
    , autoSaveTimer(nullptr)
    , statusResetTimer(nullptr)
    , dropIndicator(nullptr)
    , dropIndicatorIndex(-1)
    , showDropIndicator(false)
{
    setupUi();

    // Initialize managers
    notebookManager = new NotebookManager(this);
    backupManager = new BackupManager(this);
    pythonExecutor = new PythonExecutor(this);
    commandPalette = new CommandPalette(this);

    // Phase 5: Initialize theme manager
    themeManager = ThemeManager::instance();

    // Phase 5: Apply saved theme on startup
    switch (themeManager->currentTheme()) {
        case ThemeManager::Dark:
            themeManager->applyTheme(ThemeManager::Dark);
            break;
        case ThemeManager::Light:
        default:
            themeManager->applyTheme(ThemeManager::Light);
            break;
    }

    // Phase 5: Initialize settings dialog
    settingsDialog = new SettingsDialog(this);

    // Phase 5: Initialize variable inspector
    variableInspector = new VariableInspector("Variables", this);
    variableInspector->setObjectName("VariableInspector");
    addDockWidget(Qt::RightDockWidgetArea, variableInspector);
    variableInspector->setConnected(true);

    // Phase 5: Initialize code completer
    codeCompleter = new CodeCompleter(this);

    setupConnections();
    setupCommandPalette();

    // Setup auto-save timer
    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setInterval(autoSaveInterval);
    connect(autoSaveTimer, &QTimer::timeout, this, &MainWindow::onAutoSaveTimeout);

    statusResetTimer = new QTimer(this);
    statusResetTimer->setSingleShot(true);

    // Auto-start if configured
    autoSaveTimer->start();

    updateWindowTitle();
    statusBar->showMessage("Ready", 3000);
}

MainWindow::~MainWindow()
{
    // Clean up cells
    clearAllCells();

    if (pythonExecutor) {
        delete pythonExecutor;
    }
    if (notebookManager) {
        delete notebookManager;
    }
    if (backupManager) {
        delete backupManager;
    }
    if (commandPalette) {
        delete commandPalette;
    }
    if (settingsDialog) {
        delete settingsDialog;
    }
    if (themeManager) {
        // ThemeManager is a singleton, don't delete it
    }
    if (variableInspector) {
        delete variableInspector;
    }
    if (codeCompleter) {
        delete codeCompleter;
    }
}

void MainWindow::setupUi()
{
    resize(1200, 800);
    setMinimumSize(800, 600);

    // Set window icon
    setWindowTitle("Lotus Notebook");

    // Central widget
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Scroll area for cells
    scrollArea = new QScrollArea(centralWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setAcceptDrops(true);

    // Cells container
    cellsContainer = new QWidget(scrollArea);
    cellsContainer->setAcceptDrops(true);
    cellsLayout = new QVBoxLayout(cellsContainer);
    cellsLayout->setContentsMargins(20, 20, 20, 20);
    cellsLayout->setSpacing(10);
    cellsLayout->addStretch();

    scrollArea->setWidget(cellsContainer);
    mainLayout->addWidget(scrollArea);

    // Drop indicator (Phase 3)
    dropIndicator = new QFrame(cellsContainer);
    dropIndicator->setStyleSheet(R"(
        QFrame {
            background-color: #2E7D32;
            border: none;
        }
    )");
    dropIndicator->setFixedHeight(4);
    dropIndicator->setVisible(false);

    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    // Apply stylesheet
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        QMenuBar {
            background-color: #ffffff;
            border-bottom: 1px solid #e0e0e0;
            padding: 4px;
        }
        QToolBar {
            background-color: #ffffff;
            border-bottom: 1px solid #e0e0e0;
            spacing: 4px;
            padding: 4px;
        }
        QStatusBar {
            background-color: #ffffff;
            border-top: 1px solid #e0e0e0;
        }
        QScrollArea {
            background-color: #f5f5f5;
        }
        QScrollBar:vertical {
            background-color: #f0f0f0;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background-color: #c0c0c0;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");
}

void MainWindow::setupMenuBar()
{
    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // File menu
    QMenu *fileMenu = menuBar->addMenu("File");

    QAction *newAction = new QAction("New", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onActionNew);
    fileMenu->addAction(newAction);

    QAction *openAction = new QAction("Open...", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onActionOpen);
    fileMenu->addAction(openAction);

    QAction *saveAction = new QAction("Save", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onActionSave);
    fileMenu->addAction(saveAction);

    QAction *saveAsAction = new QAction("Save As...", this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onActionSaveAs);
    fileMenu->addAction(saveAsAction);

    fileMenu->addSeparator();

    // Export submenu
    QMenu *exportMenu = fileMenu->addMenu("Export");

    QAction *exportHtmlAction = new QAction("As HTML...", this);
    exportHtmlAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportHtmlAction, &QAction::triggered, this, &MainWindow::onActionExportHtml);
    exportMenu->addAction(exportHtmlAction);

    QAction *exportPythonAction = new QAction("As Python Script...", this);
    connect(exportPythonAction, &QAction::triggered, this, &MainWindow::onActionExportPython);
    exportMenu->addAction(exportPythonAction);

    QAction *exportIpynbAction = new QAction("As Jupyter Notebook...", this);
    connect(exportIpynbAction, &QAction::triggered, this, &MainWindow::onActionExportIpynb);
    exportMenu->addAction(exportIpynbAction);

    fileMenu->addSeparator();

    QAction *exitAction = new QAction("Exit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onActionExit);
    fileMenu->addAction(exitAction);

    // Edit menu
    QMenu *editMenu = menuBar->addMenu("Edit");

    QAction *undoAction = new QAction("Undo", this);
    undoAction->setShortcut(QKeySequence::Undo);
    editMenu->addAction(undoAction);

    QAction *redoAction = new QAction("Redo", this);
    redoAction->setShortcut(QKeySequence::Redo);
    editMenu->addAction(redoAction);

    editMenu->addSeparator();

    QAction *cutAction = new QAction("Cut", this);
    cutAction->setShortcut(QKeySequence::Cut);
    editMenu->addAction(cutAction);

    QAction *copyAction = new QAction("Copy", this);
    copyAction->setShortcut(QKeySequence::Copy);
    editMenu->addAction(copyAction);

    QAction *pasteAction = new QAction("Paste", this);
    pasteAction->setShortcut(QKeySequence::Paste);
    editMenu->addAction(pasteAction);

    // View menu
    QMenu *viewMenu = menuBar->addMenu("View");

    QAction *toggleBackupAction = new QAction("Auto-Backup", this);
    toggleBackupAction->setCheckable(true);
    toggleBackupAction->setChecked(autoBackupEnabled);
    connect(toggleBackupAction, &QAction::triggered, this, &MainWindow::onActionToggleBackup);
    viewMenu->addAction(toggleBackupAction);

    QAction *createBackupAction = new QAction("Create Backup Now", this);
    connect(createBackupAction, &QAction::triggered, this, &MainWindow::onActionCreateBackup);
    viewMenu->addAction(createBackupAction);

    // Command palette action (Phase 3)
    QAction *commandPaletteAction = new QAction("Command Palette...", this);
    commandPaletteAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
    connect(commandPaletteAction, &QAction::triggered, this, &MainWindow::showCommandPalette);
    viewMenu->addAction(commandPaletteAction);

    viewMenu->addSeparator();

    // Phase 5: Variable inspector toggle
    QAction *toggleVariableInspectorAction = new QAction("Variable Inspector", this);
    toggleVariableInspectorAction->setCheckable(true);
    toggleVariableInspectorAction->setChecked(true);
    connect(toggleVariableInspectorAction, &QAction::toggled, variableInspector, &QDockWidget::setVisible);
    viewMenu->addAction(toggleVariableInspectorAction);

    // Insert menu
    QMenu *insertMenu = menuBar->addMenu("Insert");

    QAction *codeCellAction = new QAction("Code Cell", this);
    codeCellAction->setShortcut(QKeySequence("Ctrl+="));
    connect(codeCellAction, &QAction::triggered, this, &MainWindow::onActionAddCodeCell);
    insertMenu->addAction(codeCellAction);

    QAction *markdownCellAction = new QAction("Markdown Cell", this);
    markdownCellAction->setShortcut(QKeySequence("Ctrl+Shift+M"));
    connect(markdownCellAction, &QAction::triggered, this, &MainWindow::onActionAddMarkdownCell);
    insertMenu->addAction(markdownCellAction);

    // Cell menu
    QMenu *cellMenu = menuBar->addMenu("Cell");

    QAction *runCellAction = new QAction("Run Cell", this);
    runCellAction->setShortcut(QKeySequence("Ctrl+Enter"));
    connect(runCellAction, &QAction::triggered, this, &MainWindow::onActionRunCell);
    cellMenu->addAction(runCellAction);

    QAction *runAllAction = new QAction("Run All", this);
    runAllAction->setShortcut(QKeySequence("Ctrl+Shift+Enter"));
    connect(runAllAction, &QAction::triggered, this, &MainWindow::onActionRunAll);
    cellMenu->addAction(runAllAction);

    cellMenu->addSeparator();

    QAction *moveCellUpAction = new QAction("Move Cell Up", this);
    moveCellUpAction->setShortcut(QKeySequence("Alt+Up"));
    connect(moveCellUpAction, &QAction::triggered, this, &MainWindow::onActionMoveCellUp);
    cellMenu->addAction(moveCellUpAction);

    QAction *moveCellDownAction = new QAction("Move Cell Down", this);
    moveCellDownAction->setShortcut(QKeySequence("Alt+Down"));
    connect(moveCellDownAction, &QAction::triggered, this, &MainWindow::onActionMoveCellDown);
    cellMenu->addAction(moveCellDownAction);

    cellMenu->addSeparator();

    QAction *restartKernelAction = new QAction("Restart Kernel", this);
    connect(restartKernelAction, &QAction::triggered, this, &MainWindow::onActionRestartKernel);
    cellMenu->addAction(restartKernelAction);

    // Phase 5: Settings menu
    QMenu *settingsMenu = menuBar->addMenu("Settings");

    QAction *settingsAction = new QAction("Preferences...", this);
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onActionSettings);
    settingsMenu->addAction(settingsAction);

    // Help menu
    QMenu *helpMenu = menuBar->addMenu("Help");

    QAction *aboutAction = new QAction("About Lotus Notebook", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onActionAbout);
    helpMenu->addAction(aboutAction);
}

void MainWindow::setupToolBar()
{
    toolBar = new QToolBar(this);
    toolBar->setMovable(false);
    addToolBar(toolBar);

    // File actions
    QAction *newAction = toolBar->addAction("New");
    connect(newAction, &QAction::triggered, this, &MainWindow::onActionNew);

    QAction *openAction = toolBar->addAction("Open");
    connect(openAction, &QAction::triggered, this, &MainWindow::onActionOpen);

    QAction *saveAction = toolBar->addAction("Save");
    connect(saveAction, &QAction::triggered, this, &MainWindow::onActionSave);

    toolBar->addSeparator();

    // Cell actions
    QAction *addCodeAction = toolBar->addAction("+ Code");
    connect(addCodeAction, &QAction::triggered, this, &MainWindow::onActionAddCodeCell);

    QAction *addMarkdownAction = toolBar->addAction("+ Markdown");
    connect(addMarkdownAction, &QAction::triggered, this, &MainWindow::onActionAddMarkdownCell);

    toolBar->addSeparator();

    // Run actions
    QAction *runAction = toolBar->addAction("Run");
    runAction->setShortcut(QKeySequence("Ctrl+Enter"));
    connect(runAction, &QAction::triggered, this, &MainWindow::onActionRunCell);

    QAction *runAllAction = toolBar->addAction("Run All");
    connect(runAllAction, &QAction::triggered, this, &MainWindow::onActionRunAll);

    QAction *restartAction = toolBar->addAction("Restart");
    restartAction->setToolTip("Restart Kernel (Ctrl+R)");
    connect(restartAction, &QAction::triggered, this, &MainWindow::onActionRestartKernel);

    QAction *interruptAction = toolBar->addAction("Stop");
    interruptAction->setIcon(QIcon::fromTheme("process-stop"));
    interruptAction->setToolTip("Interrupt Kernel (Ctrl+I)");
    connect(interruptAction, &QAction::triggered, this, &MainWindow::onActionInterruptKernel);
}

void MainWindow::setupStatusBar()
{
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);

    // Status labels
    QLabel *kernelStatus = new QLabel("Kernel: Ready", this);
    statusBar->addPermanentWidget(kernelStatus);

    QLabel *cellCount = new QLabel("Cells: 0", this);
    statusBar->addPermanentWidget(cellCount);

    QLabel *backupStatus = new QLabel("Backup: Enabled", this);
    statusBar->addPermanentWidget(backupStatus);
}

void MainWindow::setupConnections()
{
    // Phase 5: Connect settings dialog signals
    if (settingsDialog) {
        connect(settingsDialog, &SettingsDialog::themeChanged,
                this, &MainWindow::onThemeChanged);
        connect(settingsDialog, &SettingsDialog::fontChanged,
                this, &MainWindow::onFontChanged);
    }

    // Phase 5: Connect variable inspector signals
    if (variableInspector) {
        connect(variableInspector, &VariableInspector::refreshRequested,
                this, &MainWindow::onRefreshVariables);
    }
}

void MainWindow::setupCommandPalette()
{
    // Add commands to the palette (Phase 3)

    // File commands
    commandPalette->addCommand(
        "New Notebook",
        "Create a new notebook",
        "Ctrl+N",
        [this]() { onActionNew(); },
        "File"
    );

    commandPalette->addCommand(
        "Open Notebook",
        "Open an existing notebook",
        "Ctrl+O",
        [this]() { onActionOpen(); },
        "File"
    );

    commandPalette->addCommand(
        "Save Notebook",
        "Save the current notebook",
        "Ctrl+S",
        [this]() { onActionSave(); },
        "File"
    );

    commandPalette->addCommand(
        "Save Notebook As...",
        "Save the notebook with a new name or format",
        "Ctrl+Shift+S",
        [this]() { onActionSaveAs(); },
        "File"
    );

    commandPalette->addCommand(
        "Export as HTML",
        "Export notebook as HTML document",
        "",
        [this]() { exportToHtml(); },
        "File"
    );

    commandPalette->addCommand(
        "Export as Python",
        "Export notebook as Python script",
        "",
        [this]() { exportToPython(); },
        "File"
    );

    commandPalette->addCommand(
        "Export as Jupyter Notebook",
        "Export notebook as .ipynb file",
        "",
        [this]() { exportToIpynb(); },
        "File"
    );

    // Cell commands
    commandPalette->addCommand(
        "Add Code Cell",
        "Insert a new code cell below the current cell",
        "Ctrl+B",
        [this]() { addCodeCell(); },
        "Cell"
    );

    commandPalette->addCommand(
        "Add Markdown Cell",
        "Insert a new markdown cell below the current cell",
        "Ctrl+M",
        [this]() { addMarkdownCell(); },
        "Cell"
    );

    commandPalette->addCommand(
        "Run Cell",
        "Execute the current cell",
        "Ctrl+Enter",
        [this]() { onActionRunCell(); },
        "Cell"
    );

    commandPalette->addCommand(
        "Run All Cells",
        "Execute all cells in the notebook",
        "Ctrl+Shift+Enter",
        [this]() { onActionRunAll(); },
        "Cell"
    );

    commandPalette->addCommand(
        "Move Cell Up",
        "Move the current cell up",
        "Alt+Up",
        [this]() { onActionMoveCellUp(); },
        "Cell"
    );

    commandPalette->addCommand(
        "Move Cell Down",
        "Move the current cell down",
        "Alt+Down",
        [this]() { onActionMoveCellDown(); },
        "Cell"
    );

    commandPalette->addCommand(
        "Delete Cell",
        "Delete the current cell",
        "",
        [this]() { deleteCurrentCell(); },
        "Cell"
    );

    // Kernel commands
    commandPalette->addCommand(
        "Interrupt Kernel",
        "Interrupt the running code",
        "Ctrl+I",
        [this]() { onActionInterruptKernel(); },
        "Kernel"
    );

    commandPalette->addCommand(
        "Restart Kernel",
        "Restart the Python kernel",
        "",
        [this]() { onActionRestartKernel(); },
        "Kernel"
    );

    // View commands
    commandPalette->addCommand(
        "Toggle Command Palette",
        "Open the command palette",
        "Ctrl+Shift+P",
        [this]() { showCommandPalette(); },
        "View"
    );
}

void MainWindow::updateWindowTitle()
{
    QString title = "Lotus Notebook";

    if (!currentFilePath.isEmpty()) {
        QFileInfo fileInfo(currentFilePath);
        title = fileInfo.fileName() + " - Lotus Notebook";
    }

    if (isModified) {
        title += " *";
    }

    setWindowTitle(title);
}

void MainWindow::setModified(bool modified)
{
    if (isModified != modified) {
        isModified = modified;
        updateWindowTitle();
        emit notebookModified(modified);
    }
}

void MainWindow::newNotebook()
{
    if (!maybeSave()) {
        return;
    }

    clearAllCells();
    currentFilePath.clear();
    setModified(false);

    // Add initial code cell
    addCodeCell();

    statusBar->showMessage("New notebook created", 2000);
}

void MainWindow::openNotebook(const QString &filePath)
{
    if (!maybeSave()) {
        return;
    }

    if (filePath.isEmpty()) {
        QString selectedPath = QFileDialog::getOpenFileName(
            this,
            "Open Notebook",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "Lotus Notebook (*.lotus);;Jupyter Notebook (*.ipynb);;All Files (*)"
        );

        if (selectedPath.isEmpty()) {
            return;
        }

        loadFromFile(selectedPath);
    } else {
        loadFromFile(filePath);
    }
}

void MainWindow::loadFromFile(const QString &filePath)
{
    // Use notebook manager to import the file
    if (!notebookManager->importNotebook(filePath)) {
        QMessageBox::warning(
            this,
            "Import Failed",
            QString("Failed to import file: %1").arg(filePath)
        );
        return;
    }

    // Clear existing cells and load from manager
    clearAllCells();
    loadCellsFromManager();

    currentFilePath = filePath;
    setModified(false);
    statusBar->showMessage(QString("Opened: %1").arg(QFileInfo(filePath).fileName()), 3000);
}

void MainWindow::syncCellsToManager()
{
    notebookManager->clearCells();

    for (CellWidget *cell : cells) {
        NotebookManager::CellData cellData;

        if (cell->getType() == CellWidget::CodeCell) {
            cellData.type = NotebookManager::CellData::CodeCell;
        } else {
            cellData.type = NotebookManager::CellData::MarkdownCell;
        }

        cellData.content = cell->getContent();
        cellData.executionCount = cell->getExecutionCount();
        cellData.wasExecuted = cell->getExecutionCount() > 0;

        notebookManager->addCell(cellData);
    }

    notebookManager->setNotebookName(QFileInfo(currentFilePath).baseName());
}

bool MainWindow::saveToFile(const QString &filePath)
{
    // Sync current cells to notebook manager
    syncCellsToManager();

    // Use notebook manager to export
    QJsonDocument doc(notebookManager->saveNotebook());

    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(
            this,
            "Save Failed",
            QString("Cannot save file: %1").arg(file.errorString())
        );
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    currentFilePath = filePath;
    setModified(false);

    // Create backup
    if (autoBackupEnabled && backupManager) {
        backupManager->createBackup(filePath, doc.toJson());
    }

    statusBar->showMessage(QString("Saved: %1").arg(QFileInfo(filePath).fileName()), 2000);

    return true;
}

void MainWindow::saveNotebook()
{
    if (currentFilePath.isEmpty()) {
        saveNotebookAs();
    } else {
        saveToFile(currentFilePath);
    }
}

void MainWindow::saveNotebookAs(const QString &suggestedPath)
{
    QString filePath;

    if (suggestedPath.isEmpty()) {
        // Show format selection dialog
        QStringList formats;
        formats << "Lotus Notebook (*.lotus)"
                << "Jupyter Notebook (*.ipynb)"
                << "HTML Document (*.html)"
                << "Python Script (*.py)";

        bool ok;
        QString selected = QInputDialog::getItem(
            this,
            "Save Notebook As",
            "Select file format:",
            formats,
            0,
            false,
            &ok
        );

        if (!ok) {
            return;
        }

        QString defaultExt;
        if (selected.contains("Lotus")) {
            defaultExt = "lotus";
        } else if (selected.contains("Jupyter")) {
            defaultExt = "ipynb";
        } else if (selected.contains("HTML")) {
            defaultExt = "html";
        } else {
            defaultExt = "py";
        }

        QString suggestedName = currentFilePath.isEmpty() ?
            notebookManager->getNotebookName() : QFileInfo(currentFilePath).baseName();

        filePath = QFileDialog::getSaveFileName(
            this,
            "Save Notebook As",
            suggestedName,
            selected
        );

        if (!filePath.isEmpty() && !filePath.endsWith("." + defaultExt)) {
            filePath += "." + defaultExt;
        }
    } else {
        filePath = suggestedPath;
    }

    if (!filePath.isEmpty()) {
        saveToFile(filePath);
    }
}

void MainWindow::onActionNew()
{
    newNotebook();
}

void MainWindow::onActionOpen()
{
    openNotebook();
}

void MainWindow::onActionSave()
{
    saveNotebook();
}

void MainWindow::onActionSaveAs()
{
    saveNotebookAs();
}

void MainWindow::onActionExport()
{
    // Show export dialog with options
    QStringList formats;
    formats << "HTML Document (*.html)"
            << "Python Script (*.py)"
            << "Jupyter Notebook (*.ipynb)";

    bool ok;
    QString selected = QInputDialog::getItem(
        this,
        "Export Notebook",
        "Select export format:",
        formats,
        0,
        false,
        &ok
    );

    if (!ok) {
        return;
    }

    if (selected.contains("HTML")) {
        exportToHtml();
    } else if (selected.contains("Python")) {
        exportToPython();
    } else if (selected.contains("Jupyter")) {
        exportToIpynb();
    }
}

void MainWindow::onActionExportHtml()
{
    exportToHtml();
}

void MainWindow::onActionExportPython()
{
    exportToPython();
}

void MainWindow::onActionExportIpynb()
{
    exportToIpynb();
}

void MainWindow::exportNotebook(NotebookManager::ExportFormat format)
{
    QString filter;
    QString defaultExt;

    switch (format) {
        case NotebookManager::FormatHtml:
            filter = "HTML Document (*.html)";
            defaultExt = "html";
            break;
        case NotebookManager::FormatPython:
            filter = "Python Script (*.py)";
            defaultExt = "py";
            break;
        case NotebookManager::FormatIpynb:
            filter = "Jupyter Notebook (*.ipynb)";
            defaultExt = "ipynb";
            break;
        default:
            filter = "Lotus Notebook (*.lotus)";
            defaultExt = "lotus";
            break;
    }

    QString suggestedName = currentFilePath.isEmpty() ?
        notebookManager->getNotebookName() : QFileInfo(currentFilePath).baseName();

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Notebook",
        suggestedName,
        filter
    );

    if (filePath.isEmpty()) {
        return;
    }

    // Add extension if not present
    if (!filePath.endsWith("." + defaultExt)) {
        filePath += "." + defaultExt;
    }

    // Sync cells to manager before export
    syncCellsToManager();

    bool success = notebookManager->exportNotebook(filePath, format);

    if (success) {
        statusBar->showMessage(QString("Exported: %1").arg(QFileInfo(filePath).fileName()), 3000);
    } else {
        QMessageBox::warning(
            this,
            "Export Failed",
            "Failed to export notebook. Please check the file path and try again."
        );
    }
}

void MainWindow::exportToHtml()
{
    exportNotebook(NotebookManager::FormatHtml);
}

void MainWindow::exportToPython()
{
    exportNotebook(NotebookManager::FormatPython);
}

void MainWindow::exportToIpynb()
{
    exportNotebook(NotebookManager::FormatIpynb);
}

void MainWindow::onActionExit()
{
    close();
}

void MainWindow::onActionAddCodeCell()
{
    addCodeCell();
}

void MainWindow::onActionAddMarkdownCell()
{
    addMarkdownCell();
}

void MainWindow::onActionRunCell()
{
    // Run current cell or first cell if none selected
    CellWidget *cellToRun = currentCell;

    if (!cellToRun && !cells.isEmpty()) {
        cellToRun = cells.first();
    }

    if (cellToRun) {
        executeCell(cellToRun);
    }
}

void MainWindow::onActionRunAll()
{
    for (CellWidget *cell : cells) {
        if (cell->getType() == CellWidget::CodeCell) {
            executeCell(cell);
        }
    }
}

void MainWindow::onActionInterruptKernel()
{
    if (kernelBusy && pythonExecutor) {
        pythonExecutor->interrupt();
        kernelBusy = false;
        updateKernelIndicator();

        // Stop all executing cells
        for (CellWidget *cell : cells) {
            if (cell->isExecuting()) {
                cell->setExecuting(false);
            }
        }

        statusBar->showMessage("Kernel interrupted", 2000);
    }
}

void MainWindow::onActionMoveCellUp()
{
    if (currentCell) {
        moveCell(currentCell, -1);
    }
}

void MainWindow::onActionMoveCellDown()
{
    if (currentCell) {
        moveCell(currentCell, 1);
    }
}

void MainWindow::deleteCurrentCell()
{
    if (!currentCell) {
        return;
    }

    int index = cells.indexOf(currentCell);
    if (index < 0) return;

    // Remove from layout and list
    cellsLayout->removeWidget(currentCell);
    cells.removeAt(index);
    delete currentCell;
    currentCell = nullptr;

    // Update indices
    updateCellIndices();

    // Select next cell or previous
    if (index < cells.count()) {
        currentCell = cells[index];
    } else if (!cells.isEmpty()) {
        currentCell = cells.last();
    }

    if (currentCell) {
        currentCell->setFocus();
    }

    setModified(true);
    updateStatusBar();
}

void MainWindow::onActionRestartKernel()
{
    if (QMessageBox::question(
        this,
        "Restart Kernel",
        "Are you sure you want to restart the Python kernel? All variables will be lost.",
        QMessageBox::Yes | QMessageBox::No
    ) == QMessageBox::Yes) {

        if (pythonExecutor) {
            pythonExecutor->restart();
        }

        // Clear all outputs
        for (CellWidget *cell : cells) {
            cell->clearOutput();
        }

        statusBar->showMessage("Kernel restarted", 2000);
    }
}

void MainWindow::onActionToggleBackup()
{
    autoBackupEnabled = !autoBackupEnabled;

    if (autoBackupEnabled) {
        autoSaveTimer->start();
        statusBar->showMessage("Auto-backup enabled", 2000);
    } else {
        autoSaveTimer->stop();
        statusBar->showMessage("Auto-backup disabled", 2000);
    }
}

void MainWindow::onActionCreateBackup()
{
    if (!currentFilePath.isEmpty()) {
        QFile file(currentFilePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            backupManager->createBackup(currentFilePath, data);
            statusBar->showMessage("Backup created", 2000);
        }
    } else {
        statusBar->showMessage("Save notebook first to create backup", 2000);
    }
}

void MainWindow::onActionAbout()
{
    QMessageBox::about(
        this,
        "About Lotus Notebook",
        QString(
            "<h1>Lotus Notebook</h1>"
            "<p>Version 1.0.0</p>"
            "<p>A lightweight Jupyter-like notebook application for Python development.</p>"
            "<p><b>Features:</b></p>"
            "<ul>"
            "<li>Interactive Python execution</li>"
            "<li>Markdown support with rendering</li>"
            "<li>Drag and drop cell reordering</li>"
            "<li>Command palette for quick access</li>"
            "<li>Auto-backup for data safety</li>"
            "<li>Clean and simple interface</li>"
            "</ul>"
            "<p>Built with Qt and embedded Python.</p>"
        )
    );
}

void MainWindow::onActionSettings()
{
    // Phase 5: Show settings dialog
    if (settingsDialog) {
        settingsDialog->exec();
    }
}

void MainWindow::onThemeChanged(const QString &theme)
{
    if (themeManager) {
        if (theme == "Dark") {
            themeManager->applyTheme(ThemeManager::Dark);
        } else if (theme == "Light") {
            themeManager->applyTheme(ThemeManager::Light);
        } else {
            // System theme - could be handled differently
            themeManager->applyTheme(ThemeManager::Light);
        }

        // Update all cells to reflect theme change
        for (CellWidget *cell : cells) {
            cell->updateTheme();
        }
    }
}

void MainWindow::onFontChanged(const QFont &font)
{
    if (themeManager) {
        themeManager->setEditorFont(font);

        // Update all cells to use new font
        for (CellWidget *cell : cells) {
            cell->setEditorFont(font);
        }
    }
}

void MainWindow::onCellChanged()
{
    setModified(true);
}

void MainWindow::onAutoSaveTimeout()
{
    if (isModified && !currentFilePath.isEmpty()) {
        saveToFile(currentFilePath);
    }
}

void MainWindow::onCellMoveUp(CellWidget *cell)
{
    moveCell(cell, -1);
}

void MainWindow::onCellMoveDown(CellWidget *cell)
{
    moveCell(cell, 1);
}

void MainWindow::onCellIndexChanged(CellWidget *cell, int oldIndex, int newIndex)
{
    Q_UNUSED(cell);
    Q_UNUSED(oldIndex);
    Q_UNUSED(newIndex);
    // Cell index change is handled by dropEvent
}

void MainWindow::showCommandPalette()
{
    commandPalette->showPalette();
}

void MainWindow::moveCell(CellWidget *cell, int direction)
{
    int index = cells.indexOf(cell);
    if (index < 0) return;

    int newIndex = index + direction;
    if (newIndex < 0 || newIndex >= cells.count()) return;

    reorderCells(index, newIndex);
    setModified(true);
}

void MainWindow::reorderCells(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= cells.count()) return;
    if (toIndex < 0 || toIndex >= cells.count()) return;

    // Remove from old position
    CellWidget *cell = cells.takeAt(fromIndex);
    QLayoutItem *item = cellsLayout->takeAt(fromIndex);

    // Insert at new position
    cells.insert(toIndex, cell);
    cellsLayout->insertItem(toIndex, item);

    // Update cell indices
    updateCellIndices();

    setModified(true);
}

void MainWindow::updateCellIndices()
{
    for (int i = 0; i < cells.count(); ++i) {
        cells[i]->setCellIndex(i);
    }
}

void MainWindow::updateDropIndicator(const QPoint &pos)
{
    // Calculate which cell is below the drop position
    int insertIndex = cells.count(); // Default to end

    for (int i = 0; i < cells.count(); ++i) {
        QRect cellRect = cells[i]->geometry();
        int middle = cellRect.top() + cellRect.height() / 2;

        if (pos.y() < middle) {
            insertIndex = i;
            break;
        }
    }

    dropIndicatorIndex = insertIndex;

    // Position the drop indicator
    if (insertIndex < cells.count()) {
        QRect cellRect = cells[insertIndex]->geometry();
        dropIndicator->setGeometry(cellRect.left(), cellRect.top() - 2,
                                    cellRect.width(), 4);
    } else if (cells.count() > 0) {
        QWidget *lastCell = cells.last();
        dropIndicator->setGeometry(lastCell->geometry().left(),
                                    lastCell->geometry().bottom() + 2,
                                    lastCell->geometry().width(), 4);
    }

    dropIndicator->setVisible(true);
}

void MainWindow::addCodeCell()
{
    CellWidget *cell = new CellWidget(CellWidget::CodeCell, cellsContainer);
    cell->setMaximumWidth(1200);

    connect(cell, &CellWidget::contentChanged, this, &MainWindow::onCellChanged);
    connect(cell, &CellWidget::executed, this, &MainWindow::onCellChanged);
    connect(cell, &CellWidget::runRequested, this, &MainWindow::executeCell);
    connect(cell, &CellWidget::moveUpRequested, this, [this, cell]() { onCellMoveUp(cell); });
    connect(cell, &CellWidget::moveDownRequested, this, [this, cell]() { onCellMoveDown(cell); });
    connect(cell, &CellWidget::cellIndexChanged, this, &MainWindow::onCellIndexChanged);

    // Insert before the stretch
    cellsLayout->insertWidget(cellsLayout->count() - 1, cell);
    cells.append(cell);

    // Update indices
    updateCellIndices();

    // Set as current cell
    currentCell = cell;

    // Scroll to make it visible
    cell->setFocus();

    setModified(true);
    updateStatusBar();
}

void MainWindow::addMarkdownCell()
{
    CellWidget *cell = new CellWidget(CellWidget::MarkdownCell, cellsContainer);
    cell->setMaximumWidth(1200);

    connect(cell, &CellWidget::contentChanged, this, &MainWindow::onCellChanged);
    connect(cell, &CellWidget::runRequested, this, &MainWindow::executeCell);
    connect(cell, &CellWidget::moveUpRequested, this, [this, cell]() { onCellMoveUp(cell); });
    connect(cell, &CellWidget::moveDownRequested, this, [this, cell]() { onCellMoveDown(cell); });
    connect(cell, &CellWidget::cellIndexChanged, this, &MainWindow::onCellIndexChanged);

    cellsLayout->insertWidget(cellsLayout->count() - 1, cell);
    cells.append(cell);

    // Update indices
    updateCellIndices();

    currentCell = cell;
    cell->setFocus();

    setModified(true);
    updateStatusBar();
}

void MainWindow::executeCell(CellWidget *cell)
{
    if (!cell || cell->getType() != CellWidget::CodeCell) {
        return;
    }

    if (!pythonExecutor) {
        cell->setOutput("Error: Python executor not initialized", CellWidget::ErrorOutput);
        return;
    }

    QString code = cell->getCode();

    if (code.trimmed().isEmpty()) {
        return;
    }

    // Set kernel busy state
    kernelBusy = true;
    cell->setExecuting(true);
    updateKernelIndicator();

    // Execute code
    PythonExecutor::ExecutionResult result = pythonExecutor->execute(code);

    // Clear busy state
    kernelBusy = false;
    cell->setExecuting(false);

    // Update execution count (Jupyter-style)
    executionCounter++;
    cell->setExecutionCount(executionCounter);

    // Display output - Phase 2: Support for rich outputs
    cell->clearOutput();

    if (result.success) {
        // Display plain text output first
        if (!result.textOutput.isEmpty()) {
            cell->addOutput(result.textOutput, CellWidget::TextOutput);
        }

        // Display all rich outputs
        for (const PythonExecutor::Output &output : result.outputs) {
            switch (output.type) {
                case PythonExecutor::Output::Text:
                    cell->addOutput(output.content, CellWidget::TextOutput);
                    break;
                case PythonExecutor::Output::Html:
                    cell->addHtmlOutput(output.content);
                    break;
                case PythonExecutor::Output::Table:
                    cell->setTableOutput(output.content);
                    break;
                case PythonExecutor::Output::Image:
                    cell->addPlot(output.imageData);
                    break;
                case PythonExecutor::Output::Error:
                    cell->addHtmlOutput(output.content);
                    break;
                case PythonExecutor::Output::Rich:
                    cell->addHtmlOutput(output.content);
                    break;
                case PythonExecutor::Output::Markdown:
                    cell->addHtmlOutput(output.content);
                    break;
            }
        }

        // Display matplotlib plot if available
        if (!result.plotData.isEmpty()) {
            cell->addPlot(result.plotData);
        }
    } else {
        // Display error with rich formatting
        cell->setOutput(result.error, CellWidget::ErrorOutput);
    }

    updateKernelIndicator();
    emit cellExecuted(cells.indexOf(cell), result.success);

    // Phase 5: Refresh variable inspector after execution
    if (result.success && variableInspector) {
        onRefreshVariables();
    }
}

void MainWindow::loadCellsFromManager()
{
    const QList<NotebookManager::CellData> &cellDataList = notebookManager->getCells();

    for (const NotebookManager::CellData &data : cellDataList) {
        CellWidget *cell;

        if (data.type == NotebookManager::CellData::CodeCell) {
            cell = new CellWidget(CellWidget::CodeCell, cellsContainer);
        } else {
            cell = new CellWidget(CellWidget::MarkdownCell, cellsContainer);
        }

        cell->setContent(data.content);
        cell->setMaximumWidth(1200);

        // Connect signals
        connect(cell, &CellWidget::contentChanged, this, &MainWindow::onCellChanged);
        connect(cell, &CellWidget::executed, this, &MainWindow::onCellChanged);
        connect(cell, &CellWidget::runRequested, this, &MainWindow::executeCell);
        connect(cell, &CellWidget::moveUpRequested, this, [this, cell]() { onCellMoveUp(cell); });
        connect(cell, &CellWidget::moveDownRequested, this, [this, cell]() { onCellMoveDown(cell); });
        connect(cell, &CellWidget::cellIndexChanged, this, &MainWindow::onCellIndexChanged);

        cellsLayout->insertWidget(cellsLayout->count() - 1, cell);
        cells.append(cell);
    }

    updateCellIndices();
}

void MainWindow::clearAllCells()
{
    for (CellWidget *cell : cells) {
        cellsLayout->removeWidget(cell);
        delete cell;
    }
    cells.clear();
    currentCell = nullptr;
}

void MainWindow::setAllCellsReadOnly(bool readOnly)
{
    for (CellWidget *cell : cells) {
        cell->setReadOnly(readOnly);
    }
}

void MainWindow::updateStatusBar()
{
    // Update cell count in status bar
    // This is a placeholder - actual implementation would update the label
}

void MainWindow::updateKernelIndicator()
{
    // Update kernel state in status bar
    // The status bar already shows messages, but we could add a permanent indicator
    if (kernelBusy) {
        statusBar->setStyleSheet("QStatusBar { background-color: #ffeb3b; }");
    } else {
        statusBar->setStyleSheet("");
    }
}

bool MainWindow::maybeSave()
{
    if (!isModified) {
        return true;
    }

    QMessageBox::StandardButton ret = QMessageBox::question(
        this,
        "Unsaved Changes",
        "You have unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    switch (ret) {
        case QMessageBox::Save:
            saveNotebook();
            return true;
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Handle keyboard shortcuts
    switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (event->modifiers() & Qt::ControlModifier) {
                // Ctrl+Enter: Run current cell
                onActionRunCell();
                return;
            }
            break;

        case Qt::Key_B:
            if (event->modifiers() & Qt::ControlModifier) {
                // Ctrl+B: Add code cell below
                addCodeCell();
                return;
            }
            break;

        case Qt::Key_M:
            if (event->modifiers() & Qt::ControlModifier) {
                // Ctrl+M: Add markdown cell below
                addMarkdownCell();
                return;
            }
            break;

        case Qt::Key_I:
            if (event->modifiers() & Qt::ControlModifier) {
                // Ctrl+I: Interrupt kernel
                onActionInterruptKernel();
                return;
            }
            break;

        case Qt::Key_P:
            if ((event->modifiers() & Qt::ControlModifier) &&
                (event->modifiers() & Qt::ShiftModifier)) {
                // Ctrl+Shift+P: Show command palette
                showCommandPalette();
                return;
            }
            break;

        case Qt::Key_Up:
            if (event->modifiers() & Qt::AltModifier) {
                // Alt+Up: Move cell up
                if (currentCell) {
                    onCellMoveUp(currentCell);
                }
                return;
            }
            break;

        case Qt::Key_Down:
            if (event->modifiers() & Qt::AltModifier) {
                // Alt+Down: Move cell down
                if (currentCell) {
                    onCellMoveDown(currentCell);
                }
                return;
            }
            break;
    }

    // Call base class implementation
    QMainWindow::keyPressEvent(event);
}

// Phase 3: Drag and drop event handlers
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-lotus-cell")) {
        event->acceptProposedAction();
        dropIndicator->setVisible(true);
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-lotus-cell")) {
        event->acceptProposedAction();
        updateDropIndicator(event->pos());
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    dropIndicator->setVisible(false);
}

void MainWindow::dropEvent(QDropEvent *event)
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

    // Calculate target index
    int targetIndex = dropIndicatorIndex;

    if (sourceIndex != targetIndex) {
        reorderCells(sourceIndex, targetIndex);
    }

    event->acceptProposedAction();
}

// Phase 5: Variable inspector slot
void MainWindow::onRefreshVariables()
{
    if (!pythonExecutor || !pythonExecutor->isInitialized() || !variableInspector) {
        return;
    }

    // Fetch variables from Python
    QJsonObject variables = pythonExecutor->getVariables();
    
    // Update the variable inspector
    variableInspector->clear();
    for (auto it = variables.begin(); it != variables.end(); ++it) {
        variableInspector->setVariable(it.key(), it.value().toObject());
    }
}

