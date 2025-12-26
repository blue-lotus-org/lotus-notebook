#include "codecompleter.h"
#include <QCoreApplication>

CodeCompleter::CodeCompleter(QObject *parent)
    : QCompleter(parent)
    , m_model(new QStringListModel(this))
    , m_updateTimer(new QTimer(this))
{
    setModel(m_model);
    setCompletionMode(QCompleter::PopupCompletion);
    setCaseSensitivity(Qt::CaseInsensitive);
    setWrapAround(true);
    
    // Setup Python-specific completions
    setupPythonCompletions();
    
    // Connect update timer
    connect(m_updateTimer, &QTimer::timeout, this, &CodeCompleter::onUpdateTimer);
    m_updateTimer->start(5000);  // Periodic update every 5 seconds
}

CodeCompleter::~CodeCompleter()
{
}

void CodeCompleter::setupPythonCompletions()
{
    // Python keywords
    m_pythonKeywords << "and" << "as" << "assert" << "async" << "await"
                     << "break" << "class" << "continue" << "def" << "del"
                     << "elif" << "else" << "except" << "finally" << "for"
                     << "from" << "global" << "if" << "import" << "in"
                     << "is" << "lambda" << "nonlocal" << "not" << "or"
                     << "pass" << "raise" << "return" << "try" << "while"
                     << "with" << "yield" << "True" << "False" << "None";

    // Python built-in functions and types
    m_pythonBuiltins << "abs" << "all" << "any" << "ascii" << "bin"
                     << "bool" << "bytearray" << "bytes" << "callable" << "chr"
                     << "classmethod" << "compile" << "complex" << "delattr"
                     << "dict" << "dir" << "divmod" << "enumerate" << "eval"
                     << "exec" << "filter" << "float" << "format" << "frozenset"
                     << "getattr" << "globals" << "hasattr" << "hash" << "help"
                     << "hex" << "id" << "input" << "int" << "isinstance"
                     << "issubclass" << "iter" << "len" << "list" << "locals"
                     << "map" << "max" << "memoryview" << "min" << "next"
                     << "object" << "oct" << "open" << "ord" << "pow"
                     << "print" << "property" << "range" << "repr" << "reversed"
                     << "round" << "set" << "setattr" << "slice" << "sorted"
                     << "staticmethod" << "str" << "sum" << "super" << "tuple"
                     << "type" << "vars" << "zip" << "__import__";

    // Common functions from standard library
    m_commonFunctions << "os" << "sys" << "math" << "random" << "datetime"
                      << "json" << "re" << "collections" << "itertools" << "functools"
                      << "pathlib" << "argparse" << "csv" << "io" << "logging"
                      << "threading" << "multiprocessing" << "subprocess" << "socket"
                      << "urllib" << "http" << "email" << "html" << "xml"
                      << "webbrowser" << "turtle" << "PIL" << "numpy"
                      << "pandas" << "matplotlib" << "scipy" << "sklearn" << "requests";

    // Combine all completions - use modern Qt API to avoid deprecation warnings
    m_allCompletions = QSet<QString>(m_pythonKeywords.begin(), m_pythonKeywords.end());
    m_allCompletions.unite(QSet<QString>(m_pythonBuiltins.begin(), m_pythonBuiltins.end()));
    m_allCompletions.unite(QSet<QString>(m_commonFunctions.begin(), m_commonFunctions.end()));
    
    updateCompletions();
}

void CodeCompleter::updateCompletions()
{
    QStringList completions = m_allCompletions.values();
    completions.sort();
    m_model->setStringList(completions);
}

void CodeCompleter::addContextCompletions(const QStringList &names)
{
    for (const QString &name : names) {
        m_allCompletions.insert(name);
    }
    updateCompletions();
}

void CodeCompleter::addCompletion(const QString &completion)
{
    if (!completion.isEmpty()) {
        m_allCompletions.insert(completion);
        updateCompletions();
        emit completionAdded(completion);
    }
}

void CodeCompleter::setPopupMode(QCompleter::CompletionMode mode)
{
    setCompletionMode(mode);
}

void CodeCompleter::onUpdateTimer()
{
    // Periodic update - could fetch new completions from Python context
    // For now, just ensure completions are current
    updateCompletions();
}
