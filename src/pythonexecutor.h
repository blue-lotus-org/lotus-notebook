#ifndef PYTHONEXECUTOR_H
#define PYTHONEXECUTOR_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <QThread>
#include <QList>

// Forward declare Python types to avoid header conflicts
struct _object;
typedef _object PyObject;

class PythonExecutor : public QObject
{
    Q_OBJECT

public:
    // Output types for rich display
    struct Output {
        enum Type {
            Text,
            Error,
            Html,
            Table,
            Image,
            Markdown,
            Rich
        };
        Type type;
        QString content;       // For text, error, html, table, markdown, rich
        QByteArray imageData;  // For image outputs
    };

    struct ExecutionResult {
        bool success;
        QString textOutput;         // Plain text output
        QString error;              // Error message
        QByteArray plotData;        // Matplotlib plot data
        QList<Output> outputs;      // Rich outputs list
    };

    explicit PythonExecutor(QObject *parent = nullptr);
    ~PythonExecutor();

    // Initialize and cleanup
    bool initialize();
    void cleanup();
    void restart();

    // Execute code
    ExecutionResult execute(const QString &code);
    ExecutionResult executeFile(const QString &filePath);

    // Variable inspection (Phase 5)
    QJsonObject getVariables() const;
    QString getVariableType(const QString &name) const;
    QString getVariableRepr(const QString &name) const;

    // Kernel control
    void interrupt();

    // State management
    bool isInitialized() const { return initialized; }
    QString lastError() const { return lastErrorMessage; }

    // Configuration
    void setExecutionTimeout(int milliseconds);
    void setMemoryLimit(long long bytes);
    void enablePlotCapture(bool enable);
    void setPlotFormat(const QString &format);

signals:
    void executionStarted();
    void executionFinished(bool success);
    void outputGenerated(const QString &text);
    void errorGenerated(const QString &error);
    void plotGenerated(const QByteArray &imageData);
    void richOutputGenerated(const QJsonObject &output);

private:
    bool initialized;
    QString lastErrorMessage;
    int executionTimeout;
    long long memoryLimit;
    bool plotCaptureEnabled;
    QString plotFormat;
    bool executionInterrupted;

    // Python state
    PyObject *pMainModule;
    PyObject *pGlobals;
    PyObject *pStdout;
    PyObject *pStderr;
    PyObject *pPlotCaptureModule;

    // Methods
    bool initPython();
    void cleanupPython();
    bool setupStdoutRedirection();
    bool setupPlotCapture();
    void capturePlot();
    QList<Output> parseRichOutputs(const QString &stdoutText);

    // Static callback functions for Python C API
    static PyObject* py_stdout_write(PyObject *self, PyObject *args);
    static PyObject* py_stderr_write(PyObject *self, PyObject *args);
    static PyObject* py_flush(PyObject *self, PyObject *args);
};

// Global stdout capture
extern QString g_capturedStdout;
extern QString g_capturedStderr;

#endif // PYTHONEXECUTOR_H
