#include "settingsdialog.h"
#include "thememanager.h"
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDir>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(new QTabWidget(this))
    , m_themeComboBox(nullptr)
    , m_themePreviewLabel(nullptr)
    , m_darkModeCheckBox(nullptr)
    , m_fontLabel(nullptr)
    , m_fontValueLabel(nullptr)
    , m_fontButton(nullptr)
    , m_fontSizeLabel(nullptr)
    , m_fontSizeSpinBox(nullptr)
    , m_lineNumbersCheckBox(nullptr)
    , m_autoIndentCheckBox(nullptr)
    , m_wordWrapCheckBox(nullptr)
    , m_autoSaveCheckBox(nullptr)
    , m_autoSaveIntervalSpinBox(nullptr)
    , m_workDirLabel(nullptr)
    , m_workDirLineEdit(nullptr)
    , m_browseWorkDirButton(nullptr)
    , m_buttonBox(nullptr)
    , m_settings(new QSettings("Lotus", "Notebook", this))
    , m_currentTheme("Light")
    , m_currentFont(QFont("Consolas", 11))
    , m_currentFontSize(11)
    , m_autoSaveEnabled(false)
    , m_autoSaveInterval(5)
{
    setupUi();
    setupConnections();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUi()
{
    setWindowTitle(tr("Settings - Lotus Notebook"));
    setMinimumWidth(450);
    setMinimumHeight(350);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tabWidget);

    // Create Appearance tab
    m_appearanceTab = new QWidget;
    QVBoxLayout *appearanceLayout = new QVBoxLayout(m_appearanceTab);

    QGroupBox *themeGroupBox = new QGroupBox(tr("Theme"));
    QVBoxLayout *themeLayout = new QVBoxLayout(themeGroupBox);

    QLabel *themeLabel = new QLabel(tr("Select Application Theme:"));
    m_themeComboBox = new QComboBox;
    m_themeComboBox->addItem(tr("Light"), "Light");
    m_themeComboBox->addItem(tr("Dark"), "Dark");
    m_themeComboBox->addItem(tr("System"), "System");

    themeLayout->addWidget(themeLabel);
    themeLayout->addWidget(m_themeComboBox);

    m_themePreviewLabel = new QLabel(tr("Theme Preview"));
    m_themePreviewLabel->setAlignment(Qt::AlignCenter);
    m_themePreviewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_themePreviewLabel->setMinimumHeight(60);
    themeLayout->addWidget(m_themePreviewLabel);

    appearanceLayout->addWidget(themeGroupBox);
    appearanceLayout->addStretch();

    m_tabWidget->addTab(m_appearanceTab, tr("Appearance"));

    // Create Editor tab
    m_editorTab = new QWidget;
    QVBoxLayout *editorLayout = new QVBoxLayout(m_editorTab);

    QGroupBox *fontGroupBox = new QGroupBox(tr("Font"));
    QGridLayout *fontLayout = new QGridLayout(fontGroupBox);

    m_fontLabel = new QLabel(tr("Font Family:"));
    m_fontValueLabel = new QLabel(m_currentFont.family());
    m_fontValueLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_fontButton = new QPushButton(tr("Change Font..."));

    fontLayout->addWidget(m_fontLabel, 0, 0);
    fontLayout->addWidget(m_fontValueLabel, 0, 1);
    fontLayout->addWidget(m_fontButton, 0, 2);

    m_fontSizeLabel = new QLabel(tr("Font Size:"));
    m_fontSizeSpinBox = new QSpinBox;
    m_fontSizeSpinBox->setRange(8, 72);
    m_fontSizeSpinBox->setValue(m_currentFontSize);

    fontLayout->addWidget(m_fontSizeLabel, 1, 0);
    fontLayout->addWidget(m_fontSizeSpinBox, 1, 1, 1, 2);

    editorLayout->addWidget(fontGroupBox);

    QGroupBox *editorOptionsGroupBox = new QGroupBox(tr("Editor Options"));
    QVBoxLayout *optionsLayout = new QVBoxLayout(editorOptionsGroupBox);

    m_lineNumbersCheckBox = new QCheckBox(tr("Show Line Numbers"));
    m_autoIndentCheckBox = new QCheckBox(tr("Auto Indent"));
    m_wordWrapCheckBox = new QCheckBox(tr("Word Wrap"));

    optionsLayout->addWidget(m_lineNumbersCheckBox);
    optionsLayout->addWidget(m_autoIndentCheckBox);
    optionsLayout->addWidget(m_wordWrapCheckBox);

    editorLayout->addWidget(editorOptionsGroupBox);
    editorLayout->addStretch();

    m_tabWidget->addTab(m_editorTab, tr("Editor"));

    // Create General tab
    m_generalTab = new QWidget;
    QVBoxLayout *generalLayout = new QVBoxLayout(m_generalTab);

    QGroupBox *autoSaveGroupBox = new QGroupBox(tr("Auto-Save"));
    QVBoxLayout *autoSaveLayout = new QVBoxLayout(autoSaveGroupBox);

    m_autoSaveCheckBox = new QCheckBox(tr("Enable Auto-Save"));
    connect(m_autoSaveCheckBox, &QCheckBox::toggled, this, &SettingsDialog::onAutoSaveToggled);

    QHBoxLayout *intervalLayout = new QHBoxLayout;
    QLabel *intervalLabel = new QLabel(tr("Auto-save interval (minutes):"));
    m_autoSaveIntervalSpinBox = new QSpinBox;
    m_autoSaveIntervalSpinBox->setRange(1, 60);
    m_autoSaveIntervalSpinBox->setEnabled(m_autoSaveEnabled);
    connect(m_autoSaveIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsDialog::onAutoSaveIntervalChanged);

    intervalLayout->addWidget(intervalLabel);
    intervalLayout->addWidget(m_autoSaveIntervalSpinBox);
    intervalLayout->addStretch();

    autoSaveLayout->addWidget(m_autoSaveCheckBox);
    autoSaveLayout->addLayout(intervalLayout);

    generalLayout->addWidget(autoSaveGroupBox);

    QGroupBox *workDirGroupBox = new QGroupBox(tr("Working Directory"));
    QVBoxLayout *workDirLayout = new QVBoxLayout(workDirGroupBox);

    QHBoxLayout *workDirInputLayout = new QHBoxLayout;
    m_workDirLabel = new QLabel(tr("Default notebooks directory:"));
    m_workDirLineEdit = new QLineEdit;
    m_workDirLineEdit->setPlaceholderText(tr("Leave empty for default location"));

    workDirInputLayout->addWidget(m_workDirLabel);
    workDirInputLayout->addWidget(m_workDirLineEdit);

    m_browseWorkDirButton = new QPushButton(tr("Browse..."));
    connect(m_browseWorkDirButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseWorkDir);

    workDirLayout->addWidget(m_workDirLabel);
    workDirLayout->addWidget(m_workDirLineEdit);
    workDirLayout->addWidget(m_browseWorkDirButton);

    generalLayout->addWidget(workDirGroupBox);
    generalLayout->addStretch();

    m_tabWidget->addTab(m_generalTab, tr("General"));

    // Create button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onButtonBoxAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::saveSettings);

    mainLayout->addWidget(m_buttonBox);
}

void SettingsDialog::setupConnections()
{
    connect(m_themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onThemeChanged);
    connect(m_fontButton, &QPushButton::clicked, this, &SettingsDialog::onSelectFont);
    connect(m_fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsDialog::onFontSizeChanged);
}

void SettingsDialog::loadSettings()
{
    m_settings->beginGroup("Settings");

    // Load theme
    m_currentTheme = m_settings->value("theme", "Light").toString();
    int themeIndex = m_themeComboBox->findData(m_currentTheme);
    if (themeIndex >= 0) {
        m_themeComboBox->setCurrentIndex(themeIndex);
    }

    // Load font
    QString fontFamily = m_settings->value("fontFamily", "Consolas").toString();
    m_currentFont.setFamily(fontFamily);
    m_currentFontSize = m_settings->value("fontSize", 11).toInt();
    m_currentFont.setPointSize(m_currentFontSize);
    m_fontValueLabel->setText(m_currentFont.family());
    m_fontSizeSpinBox->setValue(m_currentFontSize);

    // Load editor options
    m_lineNumbersCheckBox->setChecked(m_settings->value("lineNumbers", true).toBool());
    m_autoIndentCheckBox->setChecked(m_settings->value("autoIndent", true).toBool());
    m_wordWrapCheckBox->setChecked(m_settings->value("wordWrap", true).toBool());

    // Load auto-save settings
    m_autoSaveEnabled = m_settings->value("autoSave", false).toBool();
    m_autoSaveCheckBox->setChecked(m_autoSaveEnabled);
    m_autoSaveInterval = m_settings->value("autoSaveInterval", 5).toInt();
    m_autoSaveIntervalSpinBox->setValue(m_autoSaveInterval);

    // Load working directory
    m_workDirLineEdit->setText(m_settings->value("workDir", "").toString());

    m_settings->endGroup();
}

void SettingsDialog::saveSettings()
{
    m_settings->beginGroup("Settings");

    // Save theme
    m_settings->setValue("theme", m_themeComboBox->currentData().toString());
    emit themeChanged(m_themeComboBox->currentData().toString());

    // Save font
    m_settings->setValue("fontFamily", m_currentFont.family());
    m_settings->setValue("fontSize", m_currentFontSize);
    emit fontChanged(m_currentFont);

    // Save editor options
    m_settings->setValue("lineNumbers", m_lineNumbersCheckBox->isChecked());
    m_settings->setValue("autoIndent", m_autoIndentCheckBox->isChecked());
    m_settings->setValue("wordWrap", m_wordWrapCheckBox->isChecked());

    // Save auto-save settings
    m_settings->setValue("autoSave", m_autoSaveCheckBox->isChecked());
    m_settings->setValue("autoSaveInterval", m_autoSaveIntervalSpinBox->value());

    // Save working directory
    m_settings->setValue("workDir", m_workDirLineEdit->text());

    m_settings->endGroup();
    m_settings->sync();

    QMessageBox::information(this, tr("Settings Saved"), tr("Your settings have been saved successfully."));
}

void SettingsDialog::onThemeChanged(int index)
{
    QString selectedTheme = m_themeComboBox->itemData(index).toString();

    // Update preview
    QPalette palette;
    if (selectedTheme == "Dark") {
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);
    } else if (selectedTheme == "Light") {
        palette.setColor(QPalette::Window, Qt::white);
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, Qt::lightGray);
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::BrightText, Qt::white);
        palette.setColor(QPalette::Link, Qt::blue);
        palette.setColor(QPalette::Highlight, Qt::darkBlue);
        palette.setColor(QPalette::HighlightedText, Qt::white);
    } else {
        // System theme - use default
        palette = QPalette();
    }

    m_themePreviewLabel->setPalette(palette);
    m_currentTheme = selectedTheme;
}

void SettingsDialog::onSelectFont()
{
    bool ok;
    QFont selectedFont = QFontDialog::getFont(&ok, m_currentFont, this, tr("Select Editor Font"));

    if (ok) {
        m_currentFont = selectedFont;
        m_currentFontSize = selectedFont.pointSize();
        m_fontValueLabel->setText(m_currentFont.family());
        m_fontSizeSpinBox->setValue(m_currentFontSize);
    }
}

void SettingsDialog::onAutoSaveToggled(bool checked)
{
    m_autoSaveIntervalSpinBox->setEnabled(checked);
    emit autoSaveChanged(checked);
}

void SettingsDialog::onAutoSaveIntervalChanged(int value)
{
    emit autoSaveIntervalChanged(value);
}

void SettingsDialog::onBrowseWorkDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Working Directory"),
                                                    m_workDirLineEdit->text(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        m_workDirLineEdit->setText(dir);
    }
}

void SettingsDialog::onButtonBoxAccepted()
{
    saveSettings();
    accept();
}

void SettingsDialog::onFontSizeChanged(int size)
{
    m_currentFontSize = size;
    m_currentFont.setPointSize(size);
}
