#ifndef CODECOMPLETER_H
#define CODECOMPLETER_H

#include <QCompleter>
#include <QStringList>
#include <QStringListModel>
#include <QSet>
#include <QTimer>

class CodeCompleter : public QCompleter
{
    Q_OBJECT

public:
    explicit CodeCompleter(QObject *parent = nullptr);
    ~CodeCompleter();

    // Update completions from Python keywords and built-ins
    void updateCompletions();

    // Add completions from current context (imported modules, defined variables)
    void addContextCompletions(const QStringList &names);

    // Add a single completion
    void addCompletion(const QString &completion);

    // Set completion mode
    void setPopupMode(QCompleter::CompletionMode mode);

    // Get the completion model
    QStringListModel *completionModel() const { return m_model; }

signals:
    void completionAdded(const QString &completion);

private slots:
    void onUpdateTimer();

private:
    void setupPythonCompletions();
    
    QStringListModel *m_model;
    QTimer *m_updateTimer;
    QSet<QString> m_allCompletions;
    QStringList m_pythonKeywords;
    QStringList m_pythonBuiltins;
    QStringList m_commonFunctions;
};

#endif // CODECOMPLETER_H
