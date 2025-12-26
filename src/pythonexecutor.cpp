// Include Python headers before Qt headers to avoid conflicts
#ifdef slots
#undef slots
#endif
#include <Python.h>
#ifdef slots
#define slots Q_SLOTS
#endif

#include "pythonexecutor.h"

#include <QFile>
#include <QIODevice>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
#include <csignal>
#include <csetjmp>

// Global variables for output capture
QString g_capturedStdout;
QString g_capturedStderr;

// Global jump buffer for timeout handling
static jmp_buf jumpBuffer;
static volatile sig_atomic_t jumpSet = 0;

static void timeoutHandler(int sig)
{
    (void)sig;
    if (jumpSet) {
        longjmp(jumpBuffer, 1);
    }
}

PythonExecutor::PythonExecutor(QObject *parent)
    : QObject(parent)
    , initialized(false)
    , lastErrorMessage("")
    , executionTimeout(10000)  // 10 seconds default
    , memoryLimit(0)           // No limit by default
    , plotCaptureEnabled(true)
    , plotFormat("PNG")
    , executionInterrupted(false)
    , pMainModule(nullptr)
    , pGlobals(nullptr)
    , pStdout(nullptr)
    , pStderr(nullptr)
    , pPlotCaptureModule(nullptr)
{
}

PythonExecutor::~PythonExecutor()
{
    cleanup();
}

bool PythonExecutor::initialize()
{
    if (initialized) {
        return true;
    }

    // Initialize Python interpreter
    Py_Initialize();

    if (!Py_IsInitialized()) {
        lastErrorMessage = "Failed to initialize Python interpreter";
        return false;
    }

    // Setup sys.path
    PyObject *sysPath = PySys_GetObject("path");
    if (sysPath) {
        // Add current directory
        PyList_Append(sysPath, PyUnicode_FromString("."));
    }

    // Create main module
    pMainModule = PyImport_AddModule("__main__");
    if (!pMainModule) {
        lastErrorMessage = "Failed to create main module";
        cleanup();
        return false;
    }

    // Get global dictionary
    pGlobals = PyModule_GetDict(pMainModule);
    if (!pGlobals) {
        lastErrorMessage = "Failed to get global dictionary";
        cleanup();
        return false;
    }

    // Setup stdout/stderr redirection
    if (!setupStdoutRedirection()) {
        cleanup();
        return false;
    }

    // Setup plot capture
    if (plotCaptureEnabled) {
        setupPlotCapture();
    }

    // Import common modules
    PyRun_SimpleString(
        "import sys\n"
        "import io\n"
        "import base64\n"
        "import signal\n"
    );

    // Import numpy with error handling
    PyObject *pModule = PyImport_ImportModule("numpy");
    if (pModule) {
        Py_DECREF(pModule);
    } else {
        PyErr_Clear();  // Continue without numpy
    }

    // Import matplotlib with error handling
    pModule = PyImport_ImportModule("matplotlib");
    if (pModule) {
        Py_DECREF(pModule);

        // Setup matplotlib for non-interactive mode
        PyRun_SimpleString(
            "import matplotlib\n"
            "matplotlib.use('Agg')\n"
            "import matplotlib.pyplot as plt\n"
            "import io\n"
            "import base64\n"
        );
    } else {
        PyErr_Clear();  // Continue without matplotlib
    }

    initialized = true;
    return true;
}

void PythonExecutor::cleanup()
{
    if (!initialized) {
        return;
    }

    // Cleanup Python objects
    if (pPlotCaptureModule) {
        Py_XDECREF(pPlotCaptureModule);
        pPlotCaptureModule = nullptr;
    }

    // Reset stdout/stderr
    PySys_SetObject("stdout", Py_None);
    PySys_SetObject("stderr", Py_None);

    // Finalize Python
    Py_Finalize();

    initialized = false;
}

void PythonExecutor::restart()
{
    cleanup();
    initialize();
}

void PythonExecutor::interrupt()
{
    // Set the interrupt flag
    executionInterrupted = true;
}

bool PythonExecutor::setupStdoutRedirection()
{
    // Create a simple Python module that captures stdout/stderr
    const char *captureCode = R"(
import sys
import io

class LotusStdoutCapture:
    def __init__(self, name):
        self.name = name
        self.output = ""

    def write(self, text):
        self.output += text

    def flush(self):
        pass

    def isatty(self):
        return False

# Create capture instances
sys.stdout = LotusStdoutCapture("stdout")
sys.stderr = LotusStdoutCapture("stderr")
)";

    int result = PyRun_SimpleString(captureCode);

    if (result != 0) {
        return false;
    }

    // Get the stdout and stderr objects
    PyObject *sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        return false;
    }

    pStdout = PyObject_GetAttrString(sysModule, "stdout");
    pStderr = PyObject_GetAttrString(sysModule, "stderr");

    Py_DECREF(sysModule);

    if (!pStdout || !pStderr) {
        return false;
    }

    return true;
}

bool PythonExecutor::setupPlotCapture()
{
    // Create plot capture module
    const char* plotCaptureCode = R"(
import sys
import io
import base64

class PlotCapture:
    def __init__(self):
        self.captured_plot = None
        self.plot_count = 0

    def capture_current_plot(self):
        """Capture the current matplotlib figure as base64"""
        try:
            import matplotlib.pyplot as plt
            from io import BytesIO

            buf = BytesIO()
            plt.savefig(buf, format='png', bbox_inches='tight', dpi=100)
            buf.seek(0)
            img_base64 = base64.b64encode(buf.read()).decode('utf-8')
            self.plot_count += 1
            return img_base64
        except Exception as e:
            return None

    def clear_plots(self):
        """Clear all current figures"""
        try:
            import matplotlib.pyplot as plt
            plt.clf()
            plt.cla()
            plt.close('all')
        except:
            pass

lotus_plot_capture = PlotCapture()
)";

    int result = PyRun_SimpleString(plotCaptureCode);

    if (result != 0) {
        return false;
    }

    // Get the plot capture object
    pPlotCaptureModule = PyObject_GetAttrString(pMainModule, "lotus_plot_capture");

    return pPlotCaptureModule != nullptr;
}

void PythonExecutor::capturePlot()
{
    if (!pPlotCaptureModule) {
        return;
    }

    // Call capture_current_plot method
    PyObject *pMethod = PyObject_GetAttrString(pPlotCaptureModule, "capture_current_plot");

    if (!pMethod) {
        return;
    }

    PyObject *pResult = PyObject_CallObject(pMethod, NULL);
    Py_DECREF(pMethod);

    if (pResult && PyUnicode_Check(pResult)) {
        const char *base64Data = PyUnicode_AsUTF8(pResult);
        if (base64Data) {
            QByteArray plotData = QByteArray::fromBase64(base64Data);
            emit plotGenerated(plotData);
        }
    }

    Py_XDECREF(pResult);
}

PythonExecutor::ExecutionResult PythonExecutor::execute(const QString &code)
{
    ExecutionResult result;
    result.success = false;
    result.plotData.clear();

    // Reset interrupt flag
    executionInterrupted = false;

    if (!initialized) {
        if (!initialize()) {
            result.error = lastErrorMessage;
            return result;
        }
    }

    // Check for interrupt before starting
    if (executionInterrupted) {
        result.success = false;
        result.error = "Execution interrupted by user";
        return result;
    }

    // Clear previous plot
    if (pPlotCaptureModule) {
        PyObject *pMethod = PyObject_GetAttrString(pPlotCaptureModule, "clear_plots");
        if (pMethod) {
            PyObject_CallObject(pMethod, NULL);
            Py_DECREF(pMethod);
        }
    }

    emit executionStarted();

    // Get stdout/stderr text buffers
    QString stdoutText;
    QString stderrText;

    // Setup signal handler for timeout
    struct sigaction sa, oldSa;
    sa.sa_handler = timeoutHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGALRM, &sa, &oldSa);

    // Set timeout
    alarm(executionTimeout / 1000);

    // Use setjmp/longjmp for timeout handling
    jumpSet = 1;
    int jumpResult = setjmp(jumpBuffer);

    if (jumpResult == 0) {
        // Normal execution
        QByteArray codeBytes = code.toUtf8();
        const char *codeStr = codeBytes.constData();

        // Clear any previous errors
        PyErr_Clear();

        // Execute the code
        PyObject *pResult = PyRun_StringFlags(
            codeStr,
            Py_file_input,
            pGlobals,
            pGlobals,
            NULL
        );

        if (pResult) {
            // Successfully executed
            Py_DECREF(pResult);
            result.success = true;

            // Capture any plots
            if (plotCaptureEnabled) {
                capturePlot();
            }
        } else {
            // Error occurred
            result.success = false;

            // Get error information
            PyObject *pType, *pValue, *pTraceback;
            PyErr_Fetch(&pType, &pValue, &pTraceback);

            QString errorMsg;

            if (pValue) {
                PyObject *pStr = PyObject_Str(pValue);
                if (pStr) {
                    errorMsg = QString::fromUtf8(PyUnicode_AsUTF8(pStr));
                    Py_DECREF(pStr);
                }
            }

            // Get traceback if available
            if (pTraceback) {
                PyObject *pModule = PyImport_ImportModule("traceback");
                if (pModule) {
                    PyObject *pFormatFunc = PyObject_GetAttrString(pModule, "format_tb");
                    if (pFormatFunc) {
                        PyObject *pArgs = PyTuple_Pack(1, pTraceback);
                        PyObject *pList = PyObject_CallObject(pFormatFunc, pArgs);
                        if (pList) {
                            PyObject *pJoined = PyUnicode_Join(PyUnicode_FromString(""), pList);
                            if (pJoined) {
                                errorMsg.prepend(QString::fromUtf8(PyUnicode_AsUTF8(pJoined)));
                                Py_DECREF(pJoined);
                            }
                            Py_DECREF(pList);
                        }
                        Py_DECREF(pArgs);
                        Py_DECREF(pFormatFunc);
                    }
                    Py_DECREF(pModule);
                }
            }

            result.error = errorMsg;
            PyErr_Clear();
        }

        // Get stdout and stderr
        if (pStdout) {
            PyObject *pOutput = PyObject_GetAttrString(pStdout, "output");
            if (pOutput && PyUnicode_Check(pOutput)) {
                stdoutText = QString::fromUtf8(PyUnicode_AsUTF8(pOutput));
                // Clear the buffer
                PyObject_SetAttrString(pStdout, "output", PyUnicode_FromString(""));
                Py_DECREF(pOutput);
            }
        }

        if (pStderr) {
            PyObject *pOutput = PyObject_GetAttrString(pStderr, "output");
            if (pOutput && PyUnicode_Check(pOutput)) {
                stderrText = QString::fromUtf8(PyUnicode_AsUTF8(pOutput));
                PyObject_SetAttrString(pStderr, "output", PyUnicode_FromString(""));
                Py_DECREF(pOutput);
            }
        }
    } else {
        // Timeout occurred
        result.success = false;
        result.error = "Execution timed out after " + QString::number(executionTimeout / 1000) + " seconds";
    }

    jumpSet = 0;

    // Cancel alarm
    alarm(0);

    // Restore signal handler
    sigaction(SIGALRM, &oldSa, nullptr);

    // Combine output and error
    if (!stdoutText.isEmpty()) {
        // Parse rich outputs from stdout
        result.outputs = parseRichOutputs(stdoutText);

        // Extract plain text output (lines before first LOTUS_OUTPUT)
        QString plainText;
        for (const QString &line : stdoutText.split('\n')) {
            if (line.startsWith("LOTUS_OUTPUT:")) {
                break;
            }
            plainText += line + "\n";
        }
        result.textOutput = plainText.trimmed();

        // For backward compatibility, store plain text in outputs if needed
        if (result.textOutput.isEmpty() && result.outputs.isEmpty()) {
            // No outputs at all, use full stdout
            PythonExecutor::Output textOutput;
            textOutput.type = PythonExecutor::Output::Text;
            textOutput.content = stdoutText;
            result.outputs.append(textOutput);
        }
    }

    if (!stderrText.isEmpty()) {
        if (!result.error.isEmpty()) {
            result.error = stderrText + "\n" + result.error;
        } else {
            result.error = stderrText;
        }
    }

    emit executionFinished(result.success);

    return result;
}

PythonExecutor::ExecutionResult PythonExecutor::executeFile(const QString &filePath)
{
    ExecutionResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.success = false;
        result.error = QString("Cannot open file: %1").arg(filePath);
        return result;
    }

    QByteArray codeData = file.readAll();
    file.close();

    return execute(QString::fromUtf8(codeData));
}

QList<PythonExecutor::Output> PythonExecutor::parseRichOutputs(const QString &stdoutText)
{
    QList<Output> outputs;
    QStringList lines = stdoutText.split('\n');

    for (const QString &line : lines) {
        if (line.startsWith("LOTUS_OUTPUT:")) {
            // Extract JSON output
            QString jsonStr = line.mid(QString("LOTUS_OUTPUT:").length()).trimmed();

            // Parse JSON
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);

            if (error.error == QJsonParseError::NoError) {
                QJsonObject json = doc.object();
                QString type = json["type"].toString();
                QString content = json["content"].toString();

                Output output;
                if (type == "text") {
                    output.type = Output::Text;
                    output.content = content;
                    outputs.append(output);
                } else if (type == "html") {
                    output.type = Output::Html;
                    output.content = content;
                    outputs.append(output);
                } else if (type == "table") {
                    output.type = Output::Table;
                    output.content = content;
                    outputs.append(output);
                } else if (type == "image") {
                    output.type = Output::Image;
                    // Content should be base64-encoded image
                    output.imageData = QByteArray::fromBase64(content.toUtf8());
                    outputs.append(output);
                } else if (type == "error") {
                    output.type = Output::Error;
                    output.content = content;
                    outputs.append(output);
                } else if (type == "rich") {
                    output.type = Output::Rich;
                    output.content = content;
                    outputs.append(output);
                } else if (type == "markdown") {
                    output.type = Output::Markdown;
                    output.content = content;
                    outputs.append(output);
                }
            }
        }
    }

    return outputs;
}

void PythonExecutor::setExecutionTimeout(int milliseconds)
{
    executionTimeout = milliseconds;
}

void PythonExecutor::setMemoryLimit(long long bytes)
{
    memoryLimit = bytes;
}

void PythonExecutor::enablePlotCapture(bool enable)
{
    plotCaptureEnabled = enable;
}

void PythonExecutor::setPlotFormat(const QString &format)
{
    plotFormat = format;
}

// Static callback functions for Python C API
PyObject* PythonExecutor::py_stdout_write(PyObject *self, PyObject *args)
{
    (void)self;
    (void)args;
    Py_RETURN_NONE;
}

PyObject* PythonExecutor::py_stderr_write(PyObject *self, PyObject *args)
{
    (void)self;
    (void)args;
    Py_RETURN_NONE;
}

PyObject* PythonExecutor::py_flush(PyObject *self, PyObject *args)
{
    (void)self;
    (void)args;
    Py_RETURN_NONE;
}

// Phase 5: Variable inspection methods
QJsonObject PythonExecutor::getVariables() const
{
    QJsonObject variables;
    
    if (!initialized || !pGlobals) {
        return variables;
    }
    
    // Acquire the Global Interpreter Lock (GIL) for thread-safe Python access
    PyGILState_STATE gstate = PyGILState_Ensure();
    
    // Get all items from globals dictionary
    PyObject *keys = PyDict_Keys(pGlobals);
    if (!keys) {
        PyGILState_Release(gstate);
        return variables;
    }
    
    Py_ssize_t numKeys = PyList_Size(keys);
    
    for (Py_ssize_t i = 0; i < numKeys; ++i) {
        PyObject *key = PyList_GetItem(keys, i);
        QString name = PyUnicode_AsUTF8(key);
        
        // Skip private and special variables
        if (name.startsWith("_") && !name.startsWith("__")) {
            continue;
        }
        
        // Skip internal Python variables
        if (name == "In" || name == "Out" || name == "exit" || name == "quit" ||
            name == "get_ipython" || name == "open" || name == "quit" ||
            name == "exit" || name == "__name__" || name == "__builtins__" ||
            name == "__doc__" || name == "__loader__" || name == "__spec__" ||
            name == "__package__") {
            continue;
        }
        
        // Get the value
        PyObject *value = PyDict_GetItem(pGlobals, key);
        if (!value) {
            continue;
        }
        
        // Get type name
        PyObject *typeObj = PyObject_Type(value);
        QString typeName;
        
        if (typeObj) {
            PyObject *typeNameObj = PyObject_GetAttrString(typeObj, "__name__");
            if (typeNameObj) {
                typeName = PyUnicode_AsUTF8(typeNameObj);
                Py_DECREF(typeNameObj);
            }
            Py_DECREF(typeObj);
        }
        
        // Get repr
        QString repr;
        PyObject *reprObj = PyObject_Repr(value);
        if (reprObj) {
            repr = PyUnicode_AsUTF8(reprObj);
            Py_DECREF(reprObj);
        }
        
        // Create JSON object for this variable
        QJsonObject varInfo;
        varInfo["type"] = typeName;
        varInfo["value"] = repr;
        
        variables[name] = varInfo;
    }
    
    Py_DECREF(keys);
    
    // Release the GIL
    PyGILState_Release(gstate);
    
    return variables;
}

QString PythonExecutor::getVariableType(const QString &name) const
{
    if (!initialized || !pGlobals) {
        return QString();
    }
    
    // Acquire the Global Interpreter Lock (GIL) for thread-safe Python access
    PyGILState_STATE gstate = PyGILState_Ensure();
    
    PyObject *key = PyUnicode_FromString(name.toUtf8().constData());
    if (!key) {
        PyGILState_Release(gstate);
        return QString();
    }
    
    PyObject *value = PyDict_GetItem(pGlobals, key);
    Py_DECREF(key);
    
    if (!value) {
        PyGILState_Release(gstate);
        return QString();
    }
    
    PyObject *typeObj = PyObject_Type(value);
    QString typeName;
    
    if (typeObj) {
        PyObject *typeNameObj = PyObject_GetAttrString(typeObj, "__name__");
        if (typeNameObj) {
            typeName = PyUnicode_AsUTF8(typeNameObj);
            Py_DECREF(typeNameObj);
        }
        Py_DECREF(typeObj);
    }
    
    PyGILState_Release(gstate);
    
    return typeName;
}

QString PythonExecutor::getVariableRepr(const QString &name) const
{
    if (!initialized || !pGlobals) {
        return QString();
    }
    
    // Acquire the Global Interpreter Lock (GIL) for thread-safe Python access
    PyGILState_STATE gstate = PyGILState_Ensure();
    
    PyObject *key = PyUnicode_FromString(name.toUtf8().constData());
    if (!key) {
        PyGILState_Release(gstate);
        return QString();
    }
    
    PyObject *value = PyDict_GetItem(pGlobals, key);
    Py_DECREF(key);
    
    if (!value) {
        PyGILState_Release(gstate);
        return QString();
    }
    
    QString repr;
    PyObject *reprObj = PyObject_Repr(value);
    if (reprObj) {
        repr = PyUnicode_AsUTF8(reprObj);
        Py_DECREF(reprObj);
    }
    
    PyGILState_Release(gstate);
    
    return repr;
}
