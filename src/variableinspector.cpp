#include "variableinspector.h"
#include "pythonexecutor.h"
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QContextMenuEvent>
#include <QAction>
#include <QJsonArray>
#include <QJsonDocument>
#include <QInputDialog>
#include <QMessageBox>
#include <QHBoxLayout>

VariableInspector::VariableInspector(const QString &title, QWidget *parent)
    : QDockWidget(title, parent)
    , m_treeWidget(nullptr)
    , m_statusLabel(nullptr)
    , m_countLabel(nullptr)
    , m_refreshButton(nullptr)
    , m_clearButton(nullptr)
    , m_autoRefreshCheckBox(nullptr)
    , m_intervalSpinBox(nullptr)
    , m_refreshTimer(nullptr)
    , m_isConnected(false)
    , m_kernelBusy(false)
    , m_autoRefreshInterval(5000)
{
    setupUi();
    setupConnections();
    
    // Load icons
    m_intIcon = QIcon::fromTheme("dialog-information");
    m_floatIcon = QIcon::fromTheme("dialog-information");
    m_strIcon = QIcon::fromTheme("text-plain");
    m_listIcon = QIcon::fromTheme("view-list");
    m_dictIcon = QIcon::fromTheme("view-list-tree");
    m_tupleIcon = QIcon::fromTheme("view-list");
    m_boolIcon = QIcon::fromTheme("dialog-information");
    m_noneIcon = QIcon::fromTheme("dialog-information");
    m_arrayIcon = QIcon::fromTheme("table");
    m_dataFrameIcon = QIcon::fromTheme("table");
}

VariableInspector::~VariableInspector()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
        delete m_refreshTimer;
    }
}

void VariableInspector::setupUi()
{
    setObjectName("VariableInspector");
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);

    QWidget *widget = new QWidget(this);
    setWidget(widget);

    QVBoxLayout *mainLayout = new QVBoxLayout(widget);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Title bar with count
    QHBoxLayout *titleLayout = new QHBoxLayout;
    m_statusLabel = new QLabel(this);
    m_statusLabel->setText("Not connected");
    m_statusLabel->setStyleSheet("font-weight: bold;");
    
    m_countLabel = new QLabel(this);
    m_countLabel->setAlignment(Qt::AlignRight);
    m_countLabel->setText("0 variables");
    
    titleLayout->addWidget(m_statusLabel);
    titleLayout->addWidget(m_countLabel);
    mainLayout->addLayout(titleLayout);

    // Control bar
    QHBoxLayout *controlsLayout = new QHBoxLayout;
    controlsLayout->setSpacing(4);
    
    m_refreshButton = new QPushButton(this);
    m_refreshButton->setText("Refresh");
    m_refreshButton->setIcon(QIcon::fromTheme("view-refresh"));
    m_refreshButton->setToolTip("Refresh variables (F5)");
    
    m_clearButton = new QPushButton(this);
    m_clearButton->setText("Clear");
    m_clearButton->setIcon(QIcon::fromTheme("edit-clear"));
    m_clearButton->setToolTip("Clear all variables");
    
    m_autoRefreshCheckBox = new QCheckBox("Auto", this);
    m_autoRefreshCheckBox->setChecked(false);
    
    m_intervalSpinBox = new QSpinBox(this);
    m_intervalSpinBox->setRange(1, 60);
    m_intervalSpinBox->setValue(5);
    m_intervalSpinBox->setSuffix("s");
    m_intervalSpinBox->setEnabled(false);
    
    controlsLayout->addWidget(m_refreshButton);
    controlsLayout->addWidget(m_clearButton);
    controlsLayout->addWidget(m_autoRefreshCheckBox);
    controlsLayout->addWidget(m_intervalSpinBox);
    controlsLayout->addStretch();
    
    mainLayout->addLayout(controlsLayout);

    // Tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setHeaderLabels(QStringList() << "Name" << "Type" << "Value");
    m_treeWidget->header()->setStretchLastSection(false);
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setSortingEnabled(true);
    m_treeWidget->sortByColumn(0, Qt::AscendingOrder);
    
    mainLayout->addWidget(m_treeWidget);

    // Create refresh timer
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(m_autoRefreshInterval);
}

void VariableInspector::setupConnections()
{
    connect(m_refreshButton, &QPushButton::clicked, this, &VariableInspector::onRefreshClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &VariableInspector::clear);
    connect(m_autoRefreshCheckBox, &QCheckBox::toggled, this, &VariableInspector::onAutoRefreshToggled);
    connect(m_intervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &VariableInspector::onIntervalChanged);
    connect(m_refreshTimer, &QTimer::timeout, this, &VariableInspector::onTimeout);
    
    // Emit signal when refresh is requested
    connect(this, &VariableInspector::refreshRequested, this, &VariableInspector::fetchVariables);
}

void VariableInspector::refresh()
{
    fetchVariables();
}

void VariableInspector::clear()
{
    m_variables = QJsonObject();
    m_treeWidget->clear();
    m_countLabel->setText("0 variables");
    updateStatus();
}

void VariableInspector::setVariable(const QString &name, const QString &type, const QString &value)
{
    QJsonObject varInfo;
    varInfo["type"] = type;
    varInfo["value"] = value;
    m_variables[name] = varInfo;
    updateTree();
}

void VariableInspector::setVariable(const QString &name, const QJsonObject &varInfo)
{
    m_variables[name] = varInfo;
    updateTree();
}

void VariableInspector::removeVariable(const QString &name)
{
    m_variables.remove(name);
    updateTree();
}

void VariableInspector::setConnected(bool connected)
{
    m_isConnected = connected;
    updateStatus();
}

void VariableInspector::setKernelBusy(bool busy)
{
    m_kernelBusy = busy;
    updateStatus();
}

void VariableInspector::onRefreshClicked()
{
    fetchVariables();
}

void VariableInspector::onAutoRefreshToggled(bool checked)
{
    m_intervalSpinBox->setEnabled(checked);
    
    if (checked) {
        m_refreshTimer->start();
    } else {
        m_refreshTimer->stop();
    }
}

void VariableInspector::onIntervalChanged(int value)
{
    m_autoRefreshInterval = value * 1000;
    m_refreshTimer->setInterval(m_autoRefreshInterval);
}

void VariableInspector::onTimeout()
{
    if (!m_kernelBusy) {
        fetchVariables();
    }
}

void VariableInspector::fetchVariables()
{
    // Emit signal to request refresh - actual fetching handled by MainWindow
    emit refreshRequested();
}

void VariableInspector::updateTree()
{
    m_treeWidget->clear();
    
    int varCount = 0;
    
    for (auto it = m_variables.begin(); it != m_variables.end(); ++it) {
        QString name = it.key();
        QJsonObject varInfo = it.value().toObject();
        QString type = varInfo["type"].toString();
        QString value = varInfo["value"].toString();
        
        QTreeWidgetItem *item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, name);
        item->setText(1, type);
        item->setText(2, value);
        
        // Set icon based on type
        if (type.contains("int", Qt::CaseInsensitive)) {
            item->setIcon(0, m_intIcon);
        } else if (type.contains("float", Qt::CaseInsensitive) || type.contains("double", Qt::CaseInsensitive)) {
            item->setIcon(0, m_floatIcon);
        } else if (type.contains("str", Qt::CaseInsensitive) || type == "bytes") {
            item->setIcon(0, m_strIcon);
        } else if (type.contains("list", Qt::CaseInsensitive)) {
            item->setIcon(0, m_listIcon);
        } else if (type.contains("dict", Qt::CaseInsensitive)) {
            item->setIcon(0, m_dictIcon);
        } else if (type.contains("tuple", Qt::CaseInsensitive)) {
            item->setIcon(0, m_tupleIcon);
        } else if (type == "bool" || type == "True" || type == "False") {
            item->setIcon(0, m_boolIcon);
        } else if (type.contains("NoneType") || type == "None") {
            item->setIcon(0, m_noneIcon);
        } else if (type.contains("ndarray", Qt::CaseInsensitive) || type.contains("array", Qt::CaseInsensitive)) {
            item->setIcon(0, m_arrayIcon);
        } else if (type.contains("DataFrame", Qt::CaseInsensitive) || type.contains("Series", Qt::CaseInsensitive)) {
            item->setIcon(0, m_dataFrameIcon);
        }
        
        // Make items expandable for complex types
        if (type.contains("dict", Qt::CaseInsensitive) || 
            type.contains("list", Qt::CaseInsensitive) ||
            type.contains("tuple", Qt::CaseInsensitive)) {
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
        
        varCount++;
    }
    
    m_countLabel->setText(QString("%1 variables").arg(varCount));
    m_treeWidget->expandAll();
}

void VariableInspector::updateStatus()
{
    if (!m_isConnected) {
        m_statusLabel->setText("Not connected");
        m_statusLabel->setStyleSheet("font-weight: bold; color: gray;");
    } else if (m_kernelBusy) {
        m_statusLabel->setText("Kernel busy...");
        m_statusLabel->setStyleSheet("font-weight: bold; color: orange;");
    } else {
        m_statusLabel->setText("Connected");
        m_statusLabel->setStyleSheet("font-weight: bold; color: green;");
    }
}

QString VariableInspector::parsePythonValue(const QString &repr) const
{
    // Limit the length of displayed values
    const int maxLength = 100;
    
    if (repr.length() <= maxLength) {
        return repr;
    }
    
    return repr.left(maxLength) + "...";
}
