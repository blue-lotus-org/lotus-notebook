#include "thememanager.h"
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QDebug>

ThemeManager* ThemeManager::m_instance = nullptr;

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
    , m_currentTheme(Light)
{
    // Default fonts
    m_editorFont = QFont("Fira Code", 11);
    m_uiFont = QFont("Arial", 10);

    loadSettings();
}

ThemeManager::~ThemeManager()
{
    saveSettings();
}

ThemeManager* ThemeManager::instance()
{
    if (!m_instance) {
        m_instance = new ThemeManager();
    }
    return m_instance;
}

void ThemeManager::applyTheme(Theme theme)
{
    if (m_currentTheme == theme) {
        return;
    }

    m_currentTheme = theme;

    // Apply palette
    QApplication::setPalette(getPalette(theme));

    // Apply stylesheet
    QString stylesheet = getStylesheet(theme);
    if (!stylesheet.isEmpty()) {
        qApp->setStyleSheet(stylesheet);
    }

    // Use Fusion style for better theming support
    QApplication::setStyle("Fusion");

    emit themeChanged(theme);
}

QPalette ThemeManager::getPalette(Theme theme) const
{
    QPalette palette;

    if (theme == Dark) {
        // Dark theme palette (Dracula-inspired)
        palette.setColor(QPalette::Window, QColor(40, 42, 54));
        palette.setColor(QPalette::WindowText, QColor(248, 248, 242));
        palette.setColor(QPalette::Base, QColor(40, 42, 54));
        palette.setColor(QPalette::AlternateBase, QColor(68, 71, 90));
        palette.setColor(QPalette::ToolTipBase, QColor(68, 71, 90));
        palette.setColor(QPalette::ToolTipText, QColor(248, 248, 242));
        palette.setColor(QPalette::Text, QColor(248, 248, 242));
        palette.setColor(QPalette::Button, QColor(68, 71, 90));
        palette.setColor(QPalette::ButtonText, QColor(248, 248, 242));
        palette.setColor(QPalette::BrightText, Qt::white);
        palette.setColor(QPalette::Link, QColor(189, 147, 249));
        palette.setColor(QPalette::Highlight, QColor(189, 147, 249));
        palette.setColor(QPalette::HighlightedText, QColor(40, 42, 54));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(100, 100, 100));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(100, 100, 100));
    } else {
        // Light theme palette
        palette = QPalette(Qt::white, Qt::lightGray);
        palette.setColor(QPalette::Window, QColor(255, 255, 255));
        palette.setColor(QPalette::WindowText, QColor(45, 55, 72));
        palette.setColor(QPalette::Base, QColor(255, 255, 255));
        palette.setColor(QPalette::AlternateBase, QColor(243, 244, 246));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, QColor(45, 55, 72));
        palette.setColor(QPalette::Text, QColor(45, 55, 72));
        palette.setColor(QPalette::Button, QColor(243, 244, 246));
        palette.setColor(QPalette::ButtonText, QColor(45, 55, 72));
        palette.setColor(QPalette::BrightText, Qt::blue);
        palette.setColor(QPalette::Link, QColor(49, 130, 206));
        palette.setColor(QPalette::Highlight, QColor(49, 130, 206));
        palette.setColor(QPalette::HighlightedText, Qt::white);
    }

    return palette;
}

QString ThemeManager::getStylesheet(Theme theme) const
{
    if (theme == Dark) {
        return generateDarkStylesheet();
    } else {
        return generateLightStylesheet();
    }
}

QString ThemeManager::generateDarkStylesheet() const
{
    return QString(R"(
        /* Global styles */
        QMainWindow {
            background-color: #282a36;
            color: #f8f8f2;
        }
        QWidget {
            background-color: #282a36;
            color: #f8f8f2;
        }
        QMenuBar {
            background-color: #282a36;
            color: #f8f8f2;
            border-bottom: 1px solid #6272a4;
        }
        QMenuBar::item:selected {
            background-color: #44475a;
        }
        QMenu {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #6272a4;
        }
        QMenu::item:selected {
            background-color: #44475a;
        }
        QToolBar {
            background-color: #282a36;
            border-bottom: 1px solid #6272a4;
        }
        QStatusBar {
            background-color: #282a36;
            border-top: 1px solid #6272a4;
        }
        QToolTip {
            background-color: #44475a;
            color: #f8f8f2;
            border: 1px solid #6272a4;
        }

        /* Inputs and editors */
        QTextEdit, QPlainTextEdit {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #6272a4;
            border-radius: 4px;
            selection-background-color: #44475a;
            selection-color: #f8f8f2;
        }
        QTextEdit:focus, QPlainTextEdit:focus {
            border-color: #bd93f9;
        }
        QLineEdit {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #6272a4;
            border-radius: 4px;
            padding: 4px;
        }

        /* Buttons */
        QPushButton {
            background-color: #44475a;
            color: #f8f8f2;
            border: 1px solid #6272a4;
            border-radius: 4px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #6272a4;
        }
        QPushButton:pressed {
            background-color: #bd93f9;
        }
        QPushButton:disabled {
            background-color: #282a36;
            color: #6272a4;
        }

        /* Frames and panels */
        QFrame {
            color: #f8f8f2;
        }
        QFrame[frameShape="1"] {
            border: 1px solid #6272a4;
        }

        /* Lists and trees */
        QListWidget, QTreeWidget {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #6272a4;
        }
        QListWidget::item:selected, QTreeWidget::item:selected {
            background-color: #44475a;
            color: #f8f8f2;
        }
        QListWidget::item:hover, QTreeWidget::item:hover {
            background-color: #44475a;
        }

        /* Headers */
        QHeaderView::section {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #6272a4;
            padding: 4px;
        }

        /* Scrollbars */
        QScrollBar:vertical {
            background: #282a36;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background: #44475a;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background: #282a36;
            height: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:horizontal {
            background: #44475a;
            border-radius: 4px;
            min-width: 20px;
        }

        /* Dialogs */
        QDialog {
            background-color: #282a36;
        }
        QDialogButtonBox {
            background-color: #282a36;
        }

        /* Dock widgets */
        QDockWidget {
            background-color: #282a36;
            color: #f8f8f2;
            titlebar-close-icon: url();
            titlebar-maximize-icon: url();
        }
        QDockWidget::title {
            background-color: #44475a;
            color: #f8f8f2;
            padding: 6px;
        }

        /* Combo boxes */
        QComboBox {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #6272a4;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox::down-arrow {
            image: none;
            border: 2px solid #f8f8f2;
            width: 6px;
            height: 6px;
            border-top: none;
            border-right: none;
            border-left: none;
            border-bottom: none;
            margin-right: 8px;
        }

        /* Group boxes */
        QGroupBox {
            border: 1px solid #6272a4;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 8px;
            color: #f8f8f2;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            color: #f8f8f2;
        }

        /* Labels */
        QLabel {
            color: #f8f8f2;
        }

        /* Tab widgets */
        QTabWidget::pane {
            border: 1px solid #6272a4;
            background-color: #282a36;
        }
        QTabBar::tab {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #6272a4;
            border-bottom: none;
            padding: 6px 12px;
        }
        QTabBar::tab:selected {
            background-color: #44475a;
        }

        /* Spin boxes */
        QSpinBox {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #6272a4;
            border-radius: 4px;
            padding: 4px;
        }

        /* Checkboxes and radio buttons */
        QCheckBox, QRadioButton {
            color: #f8f8f2;
        }
        QCheckBox::indicator, QRadioButton::indicator {
            border: 1px solid #6272a4;
        }
        QCheckBox::indicator:checked {
            background-color: #bd93f9;
            border-color: #bd93f9;
        }
    )");
}

QString ThemeManager::generateLightStylesheet() const
{
    return QString(R"(
        /* Global styles */
        QMainWindow {
            background-color: #f5f5f5;
            color: #333;
        }
        QWidget {
            background-color: #ffffff;
            color: #333;
        }
        QMenuBar {
            background-color: #ffffff;
            color: #333;
            border-bottom: 1px solid #e0e0e0;
        }
        QMenuBar::item:selected {
            background-color: #e0e0e0;
        }
        QMenu {
            background-color: #ffffff;
            color: #333;
            border: 1px solid #e0e0e0;
        }
        QMenu::item:selected {
            background-color: #e0e0e0;
        }
        QToolBar {
            background-color: #ffffff;
            border-bottom: 1px solid #e0e0e0;
        }
        QStatusBar {
            background-color: #ffffff;
            border-top: 1px solid #e0e0e0;
        }
        QToolTip {
            background-color: #ffffff;
            color: #333;
            border: 1px solid #e0e0e0;
        }

        /* Inputs and editors */
        QTextEdit, QPlainTextEdit {
            background-color: #ffffff;
            color: #333;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            selection-background-color: #2E7D32;
            selection-color: #ffffff;
        }
        QTextEdit:focus, QPlainTextEdit:focus {
            border-color: #2E7D32;
        }
        QLineEdit {
            background-color: #ffffff;
            color: #333;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 4px;
        }

        /* Buttons */
        QPushButton {
            background-color: #f5f5f5;
            color: #333;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #e0e0e0;
        }
        QPushButton:pressed {
            background-color: #2E7D32;
            color: #ffffff;
        }
        QPushButton:disabled {
            background-color: #f5f5f5;
            color: #aaa;
        }

        /* Frames and panels */
        QFrame {
            color: #333;
        }
        QFrame[frameShape="1"] {
            border: 1px solid #e0e0e0;
        }

        /* Lists and trees */
        QListWidget, QTreeWidget {
            background-color: #ffffff;
            color: #333;
            border: 1px solid #e0e0e0;
        }
        QListWidget::item:selected, QTreeWidget::item:selected {
            background-color: #e8f5e9;
            color: #2E7D32;
        }
        QListWidget::item:hover, QTreeWidget::item:hover {
            background-color: #f5f5f5;
        }

        /* Headers */
        QHeaderView::section {
            background-color: #f5f5f5;
            color: #333;
            border: 1px solid #e0e0e0;
            padding: 4px;
        }

        /* Scrollbars */
        QScrollBar:vertical {
            background: #f0f0f0;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background: #c0c0c0;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        /* Dialogs */
        QDialog {
            background-color: #ffffff;
        }

        /* Dock widgets */
        QDockWidget {
            background-color: #ffffff;
            color: #333;
        }
        QDockWidget::title {
            background-color: #f5f5f5;
            color: #333;
            padding: 6px;
        }

        /* Combo boxes */
        QComboBox {
            background-color: #ffffff;
            color: #333;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QComboBox::drop-down {
            border: none;
        }

        /* Group boxes */
        QGroupBox {
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 8px;
            color: #333;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
        }

        /* Labels */
        QLabel {
            color: #333;
        }

        /* Tab widgets */
        QTabWidget::pane {
            border: 1px solid #e0e0e0;
            background-color: #ffffff;
        }
        QTabBar::tab {
            background-color: #f5f5f5;
            color: #333;
            border: 1px solid #e0e0e0;
            border-bottom: none;
            padding: 6px 12px;
        }
        QTabBar::tab:selected {
            background-color: #ffffff;
        }

        /* Spin boxes */
        QSpinBox {
            background-color: #ffffff;
            color: #333;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 4px;
        }

        /* Checkboxes and radio buttons */
        QCheckBox, QRadioButton {
            color: #333;
        }
    )");
}

void ThemeManager::setEditorFont(const QFont &font)
{
    if (m_editorFont != font) {
        m_editorFont = font;
        emit fontChanged(m_editorFont, m_uiFont);
    }
}

void ThemeManager::setUiFont(const QFont &font)
{
    if (m_uiFont != font) {
        m_uiFont = font;
        emit fontChanged(m_editorFont, m_uiFont);
    }
}

void ThemeManager::setEditorFontSize(int size)
{
    QFont font = m_editorFont;
    font.setPointSize(size);
    setEditorFont(font);
}

void ThemeManager::saveSettings()
{
    QSettings settings("LotusNotebook", "Settings");

    settings.beginGroup("Appearance");
    settings.setValue("theme", static_cast<int>(m_currentTheme));
    settings.setValue("editorFontFamily", m_editorFont.family());
    settings.setValue("editorFontSize", m_editorFont.pointSize());
    settings.setValue("uiFontFamily", m_uiFont.family());
    settings.setValue("uiFontSize", m_uiFont.pointSize());
    settings.endGroup();
}

void ThemeManager::loadSettings()
{
    QSettings settings("LotusNotebook", "Settings");

    settings.beginGroup("Appearance");
    m_currentTheme = static_cast<Theme>(settings.value("theme", Light).toInt());

    QString editorFontFamily = settings.value("editorFontFamily", "Fira Code").toString();
    int editorFontSize = settings.value("editorFontSize", 11).toInt();
    m_editorFont = QFont(editorFontFamily, editorFontSize);

    QString uiFontFamily = settings.value("uiFontFamily", "Arial").toString();
    int uiFontSize = settings.value("uiFontSize", 10).toInt();
    m_uiFont = QFont(uiFontFamily, uiFontSize);

    settings.endGroup();
}
