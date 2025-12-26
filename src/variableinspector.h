#ifndef VARIABLEINSPECTOR_H
#define VARIABLEINSPECTOR_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCheckBox>
#include <QSpinBox>

class VariableInspector : public QDockWidget
{
    Q_OBJECT

public:
    explicit VariableInspector(const QString &title, QWidget *parent = nullptr);
    ~VariableInspector();

    // Refresh the variable inspector
    void refresh();

    // Clear all variables
    void clear();

    // Add or update a variable
    void setVariable(const QString &name, const QString &type, const QString &value);
    void setVariable(const QString &name, const QJsonObject &varInfo);

    // Remove a variable
    void removeVariable(const QString &name);

    // Set connection status
    void setConnected(bool connected);
    void setKernelBusy(bool busy);

signals:
    void refreshRequested();

private slots:
    void onRefreshClicked();
    void onAutoRefreshToggled(bool checked);
    void onIntervalChanged(int value);
    void onTimeout();

private:
    void setupUi();
    void setupConnections();
    void updateStatus();

    // Update tree from internal data
    void updateTree();

    // Fetch variables from Python kernel
    void fetchVariables();

    // Parse Python representation
    QString parsePythonValue(const QString &repr) const;

    // Tree widget for variables
    QTreeWidget *m_treeWidget;

    // Labels
    QLabel *m_statusLabel;
    QLabel *m_countLabel;

    // Buttons
    QPushButton *m_refreshButton;
    QPushButton *m_clearButton;

    // Auto-refresh controls
    QCheckBox *m_autoRefreshCheckBox;
    QSpinBox *m_intervalSpinBox;

    // Timer for auto-refresh
    QTimer *m_refreshTimer;

    // Variable data storage
    QJsonObject m_variables;

    // Status
    bool m_isConnected;
    bool m_kernelBusy;
    int m_autoRefreshInterval;

    // Icons for different types
    QIcon m_intIcon;
    QIcon m_floatIcon;
    QIcon m_strIcon;
    QIcon m_listIcon;
    QIcon m_dictIcon;
    QIcon m_tupleIcon;
    QIcon m_boolIcon;
    QIcon m_noneIcon;
    QIcon m_arrayIcon;
    QIcon m_dataFrameIcon;
};

#endif // VARIABLEINSPECTOR_H
