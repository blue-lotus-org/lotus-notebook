#include <QApplication>
#include <QMainWindow>
#include <QDebug>
#include <QPalette>
#include <QFont>
#include <QColor>
#include <csignal>

#include "mainwindow.h"

// Global pointer to main window for signal handling
static MainWindow *globalMainWindow = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signum)
{
    qDebug() << "Received signal" << signum << ", shutting down gracefully...";

    if (globalMainWindow) {
        globalMainWindow->close();
    }

    QApplication::quit();
}

int main(int argc, char *argv[])
{
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Application setup
    QApplication app(argc, argv);

    // Application metadata
    QApplication::setApplicationName("Lotus Notebook");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("Lotus");
    QApplication::setOrganizationDomain("lotus-notebook");
    QApplication::setApplicationDisplayName("Lotus Notebook");

    // Style and palette
    QApplication::setStyle("Fusion");

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(245, 245, 245));
    palette.setColor(QPalette::WindowText, QColor(51, 51, 51));
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    palette.setColor(QPalette::ToolTipText, QColor(51, 51, 51));
    palette.setColor(QPalette::Text, QColor(51, 51, 51));
    palette.setColor(QPalette::Button, QColor(245, 245, 245));
    palette.setColor(QPalette::ButtonText, QColor(51, 51, 51));
    palette.setColor(QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::Link, QColor(46, 125, 50));
    palette.setColor(QPalette::Highlight, QColor(46, 125, 50));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(150, 150, 150));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(150, 150, 150));

    QApplication::setPalette(palette);

    // Font settings
    QFont defaultFont("Arial", 10);
    QApplication::setFont(defaultFont);

    // Create and show main window
    MainWindow mainWindow;
    globalMainWindow = &mainWindow;

    // Check for command line arguments (file to open)
    if (argc > 1) {
        QString filePath = QString::fromUtf8(argv[1]);
        mainWindow.openNotebook(filePath);
    }

    // Show window
    mainWindow.show();

    // Handle application events
    QObject::connect(&app, &QApplication::aboutToQuit, []() {
        qDebug() << "Application shutting down...";
        globalMainWindow = nullptr;
    });

    // Run application
    int result = QApplication::exec();

    return result;
}
