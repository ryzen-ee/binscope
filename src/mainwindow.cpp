#include "mainwindow.h"
#include "parser.h"
#include "icon_data.h"
#include <string>
#include <vector>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QHeaderView>
#include <QElapsedTimer>
#include <QLabel>
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QSizePolicy>
#include <QScrollBar>
#include <QPainter>
#include <QPaintEvent>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QDir>
#include <QStandardPaths>
#include <QPixmap>
#include <QCoreApplication>
#include <fstream>
#include <map>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), worker(nullptr)
{
    setWindowTitle("Binary Analyzer");
    resize(1000, 700);
    
    stackedWidget = new QStackedWidget;
    setCentralWidget(stackedWidget);
    
    // Create file screen
    fileScreen = new QWidget;
    QVBoxLayout* fileLayout = new QVBoxLayout(fileScreen);
    fileLayout->addStretch();
    
    QLabel* iconLabel = new QLabel;
    QPixmap pixmap = IconData::getPixmap();
    if (!pixmap.isNull()) {
        iconLabel->setPixmap(pixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    iconLabel->setAlignment(Qt::AlignCenter);
    fileLayout->addWidget(iconLabel);
    
    QLabel* titleLabel = new QLabel("binscope");
    titleLabel->setObjectName("titleLabel");
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    fileLayout->addWidget(titleLabel);
    
    QLabel* subtitleLabel = new QLabel("Huge file sizes may cause the window to not respond. Please wait (that's all you can do :/)");
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setStyleSheet("font-size: 12px;");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setWordWrap(true);
    fileLayout->addWidget(subtitleLabel);
    
    QHBoxLayout* fileInputLayout = new QHBoxLayout;
    QLabel* fileLabel = new QLabel("File:");
    fileLabel->setObjectName("fileLabel");
    fileLabel->setStyleSheet("font-size: 16px;");
    filePathInput = new QLineEdit;
    filePathInput->setObjectName("filePathInput");
    filePathInput->setMinimumWidth(350);
    filePathInput->setStyleSheet("padding: 8px; font-size: 14px;");
    browseBtn = new QPushButton("Browse");
    browseBtn->setObjectName("browseBtn");
    browseBtn->setStyleSheet("padding: 8px 16px; font-size: 14px;");
    connect(browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowse);
    fileInputLayout->addWidget(fileLabel);
    fileInputLayout->addWidget(filePathInput);
    fileInputLayout->addWidget(browseBtn);
    fileLayout->addLayout(fileInputLayout);
    fileLayout->addSpacing(10);
    
    fileLayout->addSpacing(20);
    
    analyzeBtn = new QPushButton("Analyze");
    analyzeBtn->setMinimumWidth(150);
    analyzeBtn->setMinimumHeight(45);
    analyzeBtn->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    connect(analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyze);
    fileLayout->addWidget(analyzeBtn, 0, Qt::AlignCenter);
    
    QLabel* versionLabel = new QLabel("binscope v1.1.0");
    versionLabel->setObjectName("versionLabel");
    versionLabel->setStyleSheet("font-family: monospace; font-size: 12px; color: #888;");
    versionLabel->setAlignment(Qt::AlignCenter);
    fileLayout->addWidget(versionLabel, 0, Qt::AlignCenter);
    
    errorLabel = new QLabel;
    errorLabel->setStyleSheet("color: red; font-size: 14px;");
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->hide();
    fileLayout->addWidget(errorLabel);
    
    progressBar = new QProgressBar;
    progressBar->setMinimumWidth(400);
    progressBar->setStyleSheet("QProgressBar { border: 2px solid grey; border-radius: 5px; text-align: center; }");
    progressBar->hide();
    fileLayout->addWidget(progressBar, 0, Qt::AlignCenter);
    
    statusLabel = new QLabel;
    statusLabel->setStyleSheet("color: #666; font-size: 12px;");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->hide();
    fileLayout->addWidget(statusLabel, 0, Qt::AlignCenter);
    
    etaLabel = new QLabel;
    etaLabel->setStyleSheet("color: #888; font-size: 11px;");
    etaLabel->setAlignment(Qt::AlignCenter);
    etaLabel->hide();
    fileLayout->addWidget(etaLabel, 0, Qt::AlignCenter);
    
    QWidget* bottomRow = new QWidget;
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomRow);
    
    QPushButton* settingsBtn = new QPushButton("Settings");
    settingsBtn->setObjectName("settingsBtn");
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setToolTip("Open settings");
    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        stackedWidget->setCurrentIndex(2);
    });
    bottomLayout->addWidget(settingsBtn);
    bottomLayout->addStretch();
    
    debugBtn = new QPushButton("Debug");
    debugBtn->setObjectName("debugBtn");
    debugBtn->setMinimumWidth(100);
    debugBtn->setCursor(Qt::PointingHandCursor);
    debugBtn->setToolTip("Enter debug mode (enable in Settings first)");
    connect(debugBtn, &QPushButton::clicked, this, [this]() {
        onDebugMode();
    });
    bottomLayout->addWidget(debugBtn);
    fileLayout->addWidget(bottomRow);
    
    stackedWidget->addWidget(fileScreen);
    
    // Create main screen
    mainScreen = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(mainScreen);
    
    tabWidget = new QTabWidget;
    tabWidget->setStyleSheet("QTabWidget::pane { border: 1px solid #ccc; } QTabBar::tab { padding: 8px 16px; }");
    
    QWidget* overviewContainer = new QWidget;
    QVBoxLayout* overviewLayout = new QVBoxLayout(overviewContainer);
    overviewLayout->setContentsMargins(0, 0, 0, 0);
    
    overviewEdit = new QTextEdit;
    overviewEdit->setObjectName("overviewEdit");
    overviewEdit->setReadOnly(true);
    overviewEdit->setStyleSheet("QTextEdit { background-color: #2a2a2a; color: #ffffff; font-family: monospace; font-size: 14px; }");
    overviewLayout->addWidget(overviewEdit);
    
    QPushButton* openAnotherBtn = new QPushButton("Open another file");
    openAnotherBtn->setObjectName("openAnotherBtn");
    openAnotherBtn->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; background-color: #3a3a3a; color: #ffffff; border: none; } QPushButton:hover { background-color: #4a4a4a; }");
    connect(openAnotherBtn, &QPushButton::clicked, this, &MainWindow::showFileScreen);
    overviewLayout->addWidget(openAnotherBtn);
    
    tabWidget->addTab(overviewContainer, "Overview");
    
    sectionsTable = new QTableWidget;
    sectionsTable->setObjectName("sectionsTable");
    sectionsTable->setColumnCount(7);
    sectionsTable->setHorizontalHeaderLabels({"Name", "VA", "Virtual Size", "Raw Size", "Entropy", "Permissions", "Flags"});
    sectionsTable->horizontalHeader()->setStretchLastSection(true);
    sectionsTable->setStyleSheet("QTableWidget { background-color: #2a2a2a; color: #ffffff; gridline-color: #555; } QHeaderView::section { background-color: #3a3a3a; color: #ffffff; padding: 4px; }");
    sectionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sectionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    sectionsTable->setContextMenuPolicy(Qt::ActionsContextMenu);
    
    QWidget* sectionsContainer = new QWidget;
    QVBoxLayout* sectionsLayout = new QVBoxLayout(sectionsContainer);
    sectionsLayout->setContentsMargins(0, 0, 0, 0);
    sectionsLayout->addWidget(sectionsTable);
    QPushButton* openAnotherSectionsBtn = new QPushButton("Open another file");
    openAnotherSectionsBtn->setObjectName("openAnotherSectionsBtn");
    openAnotherSectionsBtn->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; background-color: #3a3a3a; color: #ffffff; border: none; } QPushButton:hover { background-color: #4a4a4a; }");
    connect(openAnotherSectionsBtn, &QPushButton::clicked, this, &MainWindow::showFileScreen);
    sectionsLayout->addWidget(openAnotherSectionsBtn);
    tabWidget->addTab(sectionsContainer, "Sections");
    
    importsTable = new QTableWidget;
    importsTable->setObjectName("importsTable");
    importsTable->setColumnCount(3);
    importsTable->setHorizontalHeaderLabels({"DLL", "Function", "Address"});
    importsTable->setStyleSheet("QTableWidget { background-color: #2a2a2a; color: #ffffff; gridline-color: #555; } QHeaderView::section { background-color: #3a3a3a; color: #ffffff; padding: 4px; }");
    importsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    importsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    importsTable->setContextMenuPolicy(Qt::ActionsContextMenu);
    
    QWidget* importsContainer = new QWidget;
    QVBoxLayout* importsLayout = new QVBoxLayout(importsContainer);
    importsLayout->setContentsMargins(0, 0, 0, 0);
    importsLayout->addWidget(importsTable);
    QPushButton* openAnotherImportsBtn = new QPushButton("Open another file");
    openAnotherImportsBtn->setObjectName("openAnotherImportsBtn");
    openAnotherImportsBtn->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; background-color: #3a3a3a; color: #ffffff; border: none; } QPushButton:hover { background-color: #4a4a4a; }");
    connect(openAnotherImportsBtn, &QPushButton::clicked, this, &MainWindow::showFileScreen);
    importsLayout->addWidget(openAnotherImportsBtn);
    tabWidget->addTab(importsContainer, "Imports");
    
    stringsEdit = new QTextEdit;
    stringsEdit->setObjectName("stringsEdit");
    stringsEdit->setReadOnly(true);
    stringsEdit->setStyleSheet("QTextEdit { background-color: #2a2a2a; color: #ffffff; font-family: monospace; font-size: 14px; }");
    stringsEdit->setLineWrapMode(QTextEdit::NoWrap);
    stringsEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QWidget* stringsContainer = new QWidget;
    QVBoxLayout* stringsLayout = new QVBoxLayout(stringsContainer);
    stringsLayout->setContentsMargins(0, 0, 0, 0);
    stringsLayout->addWidget(stringsEdit);
    QPushButton* openAnotherStringsBtn = new QPushButton("Open another file");
    openAnotherStringsBtn->setObjectName("openAnotherStringsBtn");
    openAnotherStringsBtn->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; background-color: #3a3a3a; color: #ffffff; border: none; } QPushButton:hover { background-color: #4a4a4a; }");
    connect(openAnotherStringsBtn, &QPushButton::clicked, this, &MainWindow::showFileScreen);
    stringsLayout->addWidget(openAnotherStringsBtn);
    tabWidget->addTab(stringsContainer, "Strings");
    
    hexView = new HexView;
    tabWidget->addTab(hexView, "Data");
    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 4) {
            hexView->setFocus();
        }
    });
    
    settingsTab = new QWidget;
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsTab);
    settingsLayout->setContentsMargins(40, 40, 40, 40);
    
    QWidget* themeRow = new QWidget;
    QHBoxLayout* themeRowLayout = new QHBoxLayout(themeRow);
    themeRowLayout->setSpacing(15);
    
    QLabel* themeLabel = new QLabel("Theme:");
    themeLabel->setObjectName("themeLabel");
    themeLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    themeRowLayout->addWidget(themeLabel);
    
    QLabel* themeDesc = new QLabel("Click to switch between dark and light mode");
    themeDesc->setObjectName("themeDesc");
    themeDesc->setStyleSheet("font-size: 12px;");
    themeRowLayout->addWidget(themeDesc, 1);
    
    QPushButton* themeBtn = new QPushButton("Switch to Light");
    themeBtn->setObjectName("themeBtn");
    themeBtn->setFixedSize(140, 35);
    themeBtn->setCursor(Qt::PointingHandCursor);
    connect(themeBtn, &QPushButton::clicked, this, [this, themeBtn]() {
        if (themeBtn->text() == "Switch to Dark") {
            themeBtn->setText("Switch to Light");
            applyTheme(true);
        } else {
            themeBtn->setText("Switch to Dark");
            applyTheme(false);
        }
    });
    themeRowLayout->addWidget(themeBtn);
    
    settingsLayout->addWidget(themeRow);
    
    QWidget* debugRow = new QWidget;
    QHBoxLayout* debugRowLayout = new QHBoxLayout(debugRow);
    debugRowLayout->setSpacing(15);
    
    QLabel* debugLabel = new QLabel("Debug Mode:");
    debugLabel->setObjectName("debugLabel");
    debugLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    debugRowLayout->addWidget(debugLabel);
    
    QLabel* debugDesc = new QLabel("Use placeholder info for testing without loading files");
    debugDesc->setObjectName("debugDesc");
    debugDesc->setStyleSheet("font-size: 12px;");
    debugRowLayout->addWidget(debugDesc, 1);
    
    QCheckBox* debugCheck = new QCheckBox("Enable");
    debugCheck->setObjectName("debugCheck");
    debugCheck->setCursor(Qt::PointingHandCursor);
    debugCheck->setChecked(isDebugMode);
    debugRowLayout->addWidget(debugCheck);
    
    settingsLayout->addWidget(debugRow);
    
    debugWarningLabel = new QLabel;
    debugWarningLabel->setObjectName("debugWarningLabel");
    debugWarningLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    debugWarningLabel->hide();
    settingsLayout->addWidget(debugWarningLabel);
    
    settingsLayout->addStretch();
    
    QWidget* buttonRow = new QWidget;
    QHBoxLayout* buttonRowLayout = new QHBoxLayout(buttonRow);
    
    QPushButton* cancelBtn = new QPushButton("Revert");
    cancelBtn->setObjectName("cancelBtn");
    cancelBtn->setFixedSize(100, 35);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, [this, themeBtn, debugCheck]() {
        if (loadSettings()) {
            applyTheme(isDarkMode);
            themeBtn->setText(isDarkMode ? "Switch to Light" : "Switch to Dark");
            debugCheck->setChecked(isDebugMode);
            updateDebugWarning();
        }
    });
    buttonRowLayout->addWidget(cancelBtn);
    
    QPushButton* saveBtn = new QPushButton("Save");
    saveBtn->setObjectName("saveBtn");
    saveBtn->setFixedSize(100, 35);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveStatusLabel = new QLabel;
    saveStatusLabel->setObjectName("saveStatusLabel");
    saveStatusLabel->setStyleSheet("font-size: 12px; margin-left: 10px;");
    buttonRowLayout->addWidget(saveStatusLabel);
    buttonRowLayout->addStretch();
    buttonRowLayout->addWidget(saveBtn);
    
    connect(saveBtn, &QPushButton::clicked, this, [this, themeBtn, debugCheck]() {
        bool isDark = (themeBtn->text() == "Switch to Light");
        isDebugMode = debugCheck->isChecked();
        saveSettings(isDark, isDebugMode);
        QString configPath = getConfigPath();
        saveStatusLabel->setText("Saved to " + configPath);
        updateDebugWarning();
    });
    
    settingsLayout->addWidget(buttonRow);
    
    tabWidget->addTab(settingsTab, "Settings");
    
    updateDebugWarning();
    
    tabWidget->setTabToolTip(0, "File information including name, size, format, architecture, hashes");
    tabWidget->setTabToolTip(1, "Sections of the binary with virtual addresses, sizes, entropy and permissions");
    tabWidget->setTabToolTip(2, "Imported DLLs and their functions");
    tabWidget->setTabToolTip(3, "All readable text strings found in the file");
    tabWidget->setTabToolTip(4, "Hexadecimal view of file contents - use arrow keys to navigate");
    tabWidget->setTabToolTip(5, "Application settings (coming soon)");
    
    QAction* copyAction = new QAction("Copy", this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::copySelectedCells);
    sectionsTable->addAction(copyAction);
    importsTable->addAction(copyAction);
    
    QAction* copyStringsAction = new QAction("Copy", this);
    copyStringsAction->setShortcut(QKeySequence::Copy);
    connect(copyStringsAction, &QAction::triggered, this, [this]() {
        stringsEdit->copy();
    });
    stringsEdit->addAction(copyStringsAction);
    
    mainLayout->addWidget(tabWidget);
    stackedWidget->addWidget(mainScreen);
    
    settingsScreen = new QWidget;
    QVBoxLayout* settingsScreenLayout = new QVBoxLayout(settingsScreen);
    settingsScreenLayout->setContentsMargins(20, 20, 20, 20);
    
    QWidget* settingsHeaderRow = new QWidget;
    QHBoxLayout* settingsHeaderLayout = new QHBoxLayout(settingsHeaderRow);
    
    QPushButton* backBtn = new QPushButton("Back");
    backBtn->setObjectName("backBtn");
    backBtn->setCursor(Qt::PointingHandCursor);
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        stackedWidget->setCurrentIndex(0);
    });
    settingsHeaderLayout->addWidget(backBtn);
    settingsHeaderLayout->addStretch();
    
    QLabel* settingsTitleLabel = new QLabel("Settings");
    settingsTitleLabel->setStyleSheet("font-size: 24px; font-weight: bold;");
    settingsHeaderLayout->addWidget(settingsTitleLabel);
    settingsHeaderLayout->addStretch();
    settingsScreenLayout->addWidget(settingsHeaderRow);
    
    QScrollArea* settingsScroll = new QScrollArea;
    settingsScroll->setWidgetResizable(true);
    
    QWidget* settingsContent = new QWidget;
    QVBoxLayout* settingsContentLayout = new QVBoxLayout(settingsContent);
    settingsContentLayout->setContentsMargins(40, 20, 40, 20);
    
    QWidget* sThemeRow = new QWidget;
    QHBoxLayout* sThemeRowLayout = new QHBoxLayout(sThemeRow);
    sThemeRowLayout->setSpacing(15);
    
    QLabel* sThemeLabel = new QLabel("Theme:");
    sThemeLabel->setObjectName("sThemeLabel");
    sThemeLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    sThemeRowLayout->addWidget(sThemeLabel);
    
    QLabel* sThemeDesc = new QLabel("Click to switch between dark and light mode");
    sThemeDesc->setObjectName("sThemeDesc");
    sThemeDesc->setStyleSheet("font-size: 12px;");
    sThemeRowLayout->addWidget(sThemeDesc, 1);
    
    QPushButton* sThemeBtn = new QPushButton("Switch to Light");
    sThemeBtn->setObjectName("sThemeBtn");
    sThemeBtn->setFixedSize(140, 35);
    sThemeBtn->setCursor(Qt::PointingHandCursor);
    connect(sThemeBtn, &QPushButton::clicked, this, [this, sThemeBtn]() {
        if (sThemeBtn->text() == "Switch to Dark") {
            sThemeBtn->setText("Switch to Light");
            applyTheme(true);
        } else {
            sThemeBtn->setText("Switch to Dark");
            applyTheme(false);
        }
    });
    sThemeRowLayout->addWidget(sThemeBtn);
    
    settingsContentLayout->addWidget(sThemeRow);
    
    QWidget* sDebugRow = new QWidget;
    QHBoxLayout* sDebugRowLayout = new QHBoxLayout(sDebugRow);
    sDebugRowLayout->setSpacing(15);
    
    QLabel* sDebugLabel = new QLabel("Debug Mode:");
    sDebugLabel->setObjectName("sDebugLabel");
    sDebugLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    sDebugRowLayout->addWidget(sDebugLabel);
    
    QLabel* sDebugDesc = new QLabel("Use placeholder info for testing without loading files");
    sDebugDesc->setObjectName("sDebugDesc");
    sDebugDesc->setStyleSheet("font-size: 12px;");
    sDebugRowLayout->addWidget(sDebugDesc, 1);
    
    QCheckBox* sDebugCheck = new QCheckBox("Enable");
    sDebugCheck->setObjectName("sDebugCheck");
    sDebugCheck->setCursor(Qt::PointingHandCursor);
    sDebugCheck->setChecked(isDebugMode);
    sDebugRowLayout->addWidget(sDebugCheck);
    
    settingsContentLayout->addWidget(sDebugRow);
    
    sDebugWarningLabel = new QLabel;
    sDebugWarningLabel->setObjectName("sDebugWarningLabel");
    sDebugWarningLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    sDebugWarningLabel->hide();
    settingsContentLayout->addWidget(sDebugWarningLabel);
    
    settingsContentLayout->addStretch();
    
    QWidget* sButtonRow = new QWidget;
    QHBoxLayout* sButtonRowLayout = new QHBoxLayout(sButtonRow);
    
    QPushButton* sCancelBtn = new QPushButton("Revert");
    sCancelBtn->setObjectName("sCancelBtn");
    sCancelBtn->setFixedSize(100, 35);
    sCancelBtn->setCursor(Qt::PointingHandCursor);
    connect(sCancelBtn, &QPushButton::clicked, this, [this, sThemeBtn, sDebugCheck]() {
        if (loadSettings()) {
            applyTheme(isDarkMode);
            sThemeBtn->setText(isDarkMode ? "Switch to Light" : "Switch to Dark");
            sDebugCheck->setChecked(isDebugMode);
            updateDebugWarning();
        }
    });
    sButtonRowLayout->addWidget(sCancelBtn);
    
    QPushButton* sSaveBtn = new QPushButton("Save");
    sSaveBtn->setObjectName("sSaveBtn");
    sSaveBtn->setFixedSize(100, 35);
    sSaveBtn->setCursor(Qt::PointingHandCursor);
    saveStatusLabel = new QLabel;
    saveStatusLabel->setObjectName("saveStatusLabel");
    saveStatusLabel->setStyleSheet("font-size: 12px; margin-left: 10px;");
    sButtonRowLayout->addWidget(saveStatusLabel);
    sButtonRowLayout->addStretch();
    sButtonRowLayout->addWidget(sSaveBtn);
    
    connect(sSaveBtn, &QPushButton::clicked, this, [this, sThemeBtn, sDebugCheck]() {
        bool isDark = (sThemeBtn->text() == "Switch to Light");
        isDebugMode = sDebugCheck->isChecked();
        saveSettings(isDark, isDebugMode);
        QString configPath = getConfigPath();
        saveStatusLabel->setText("Saved to " + configPath);
        updateDebugWarning();
    });
    
    settingsContentLayout->addWidget(sButtonRow);
    
    settingsScroll->setWidget(settingsContent);
    settingsScreenLayout->addWidget(settingsScroll);
    
    stackedWidget->addWidget(settingsScreen);
    
    if (loadSettings()) {
        applyTheme(isDarkMode);
        QPushButton* themeBtn = settingsTab->findChild<QPushButton*>("themeBtn");
        if (themeBtn) {
            themeBtn->setText(isDarkMode ? "Switch to Light" : "Switch to Dark");
        }
        QPushButton* sThemeBtn = settingsScreen ? settingsScreen->findChild<QPushButton*>("sThemeBtn") : nullptr;
        if (sThemeBtn) {
            sThemeBtn->setText(isDarkMode ? "Switch to Light" : "Switch to Dark");
        }
    } else {
        applyTheme(true);
    }
    
    updateDebugWarning();
}

MainWindow::~MainWindow() {
    if (worker && worker->isRunning()) {
        worker->terminate();
        worker->wait();
    }
}

void MainWindow::onBrowse() {
    QString file = QFileDialog::getOpenFileName(this, "Select Binary File", QString(), "*.*");
    if (!file.isEmpty()) {
        filePathInput->setText(file);
        errorLabel->hide();
    }
}

void MainWindow::onAnalyze() {
    QString path = filePathInput->text();
    if (path.isEmpty()) {
        errorLabel->setText("No file selected");
        errorLabel->show();
        return;
    }
    
    errorLabel->hide();
    progressBar->show();
    progressBar->setValue(0);
    statusLabel->show();
    statusLabel->setText("Starting analysis...");
    etaLabel->show();
    etaLabel->setText("Calculating...");
    analyzeBtn->setEnabled(false);
    browseBtn->setEnabled(false);
    filePathInput->setEnabled(false);
    QApplication::processEvents();
    QCoreApplication::processEvents();
    
    QElapsedTimer* timer = new QElapsedTimer;
    timer->start();
    
    worker = new AnalysisWorker(path, this);
    connect(worker, &AnalysisWorker::progress, this, [this, timer](int value, QString status) {
        progressBar->setValue(value);
        statusLabel->setText(status);
        QApplication::processEvents();
        
        if (value > 5 && value < 95) {
            qint64 elapsed = timer->elapsed();
            if (elapsed > 500) {
                qint64 estimatedTotal = elapsed * 100 / value;
                qint64 remaining = estimatedTotal - elapsed;
                if (remaining < 0 || remaining > 7200000) {
                    etaLabel->setText("Processing...");
                } else if (remaining < 60000) {
                    etaLabel->setText(QString("ETA: %1 seconds").arg(remaining / 1000));
                } else {
                    etaLabel->setText(QString("ETA: %1 minutes").arg(remaining / 60000));
                }
            }
        }
    });
    connect(worker, &AnalysisWorker::finished, this, [this, timer]() {
        delete timer;
        onAnalysisFinished();
    });
    worker->start();
    QApplication::processEvents();
}

void MainWindow::onAnalysisFinished() {
    analyzeBtn->setEnabled(true);
    browseBtn->setEnabled(true);
    filePathInput->setEnabled(true);
    progressBar->hide();
    etaLabel->hide();
    statusLabel->setText("Loading UI...");
    statusLabel->show();
    QApplication::processEvents();
    QCoreApplication::processEvents();
    QApplication::processEvents();
    
    if (worker->exception) {
        try {
            std::rethrow_exception(worker->exception);
        } catch (const std::exception& e) {
            errorLabel->setText(e.what());
            errorLabel->show();
        }
    } else if (worker->result) {
        currentInfo = std::move(worker->result);
        showMainScreen();
    }
    
    statusLabel->hide();
    worker->deleteLater();
    worker = nullptr;
}

void MainWindow::onDebugMode() {
    isDebugMode = true;
    currentInfo = std::make_unique<BinaryInfo>();
    currentInfo->fileName = "debug_sample.exe";
    currentInfo->filePath = "";
    currentInfo->fileSize = 123456;
    currentInfo->format = "PE32";
    currentInfo->architecture = "x86";
    currentInfo->entryPoint = "0x00401000";
    currentInfo->subsystem = "GUI";
    currentInfo->hashes.md5 = "d41d8cd98f00b204e9800998ecf8427e";
    currentInfo->hashes.sha1 = "da39a3ee5e6b4b0d3255bfef95601890afd80709";
    currentInfo->hashes.sha256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    
    SectionInfo sec1;
    sec1.name = ".text";
    sec1.virtualAddress = "0x1000";
    sec1.virtualSize = "0x5000";
    sec1.rawSize = "0x4000";
    sec1.entropy = 6.5;
    sec1.permissions = "R-X";
    currentInfo->sections.push_back(sec1);
    
    SectionInfo sec2;
    sec2.name = ".data";
    sec2.virtualAddress = "0x6000";
    sec2.virtualSize = "0x1000";
    sec2.rawSize = "0x800";
    sec2.entropy = 3.2;
    sec2.permissions = "RW-";
    currentInfo->sections.push_back(sec2);
    
    ImportInfo imp1;
    imp1.dllName = "kernel32.dll";
    FunctionInfo func1;
    func1.name = "CreateFileA";
    func1.address = "0x00401000";
    func1.suspicious = false;
    imp1.functions.push_back(func1);
    FunctionInfo func2;
    func2.name = "VirtualAlloc";
    func2.address = "0x00401020";
    func2.suspicious = true;
    imp1.functions.push_back(func2);
    currentInfo->imports.push_back(imp1);
    
    ImportInfo imp2;
    imp2.dllName = "ntdll.dll";
    FunctionInfo func3;
    func3.name = "NtCreateFile";
    func3.address = "0x00401100";
    func3.suspicious = true;
    imp2.functions.push_back(func3);
    currentInfo->imports.push_back(imp2);
    
    StringInfo str1;
    str1.value = "C:\\Windows\\System32\\config\\system";
    str1.category = "Paths";
    currentInfo->strings.push_back(str1);
    
    StringInfo str2;
    str2.value = "http://malicious-domain.com/payload.exe";
    str2.category = "URLs";
    currentInfo->strings.push_back(str2);
    
    StringInfo str3;
    str3.value = "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    str3.category = "Registry Keys";
    currentInfo->strings.push_back(str3);
    
    StringInfo str4;
    str4.value = "cmd /c del /q /f C:\\*.*";
    str4.category = "Commands";
    currentInfo->strings.push_back(str4);
    
    StringInfo str5;
    str5.value = "This is a debug test string for testing purposes.";
    str5.category = "General";
    currentInfo->strings.push_back(str5);
    
    showMainScreen();
}

void MainWindow::showMainScreen() {
    stackedWidget->setCurrentIndex(1);
    QApplication::processEvents();
    updateOverviewTab();
    QApplication::processEvents();
    updateSectionsTab();
    QApplication::processEvents();
    updateImportsTab();
    QApplication::processEvents();
    updateStringsTab();
    QApplication::processEvents();
    updateHexView();
    QApplication::processEvents();
    
    QCheckBox* settingsDebugCheck = settingsTab->findChild<QCheckBox*>("debugCheck");
    if (settingsDebugCheck) {
        settingsDebugCheck->setChecked(isDebugMode);
    }
    QPushButton* settingsThemeBtn = settingsTab->findChild<QPushButton*>("themeBtn");
    if (settingsThemeBtn) {
        settingsThemeBtn->setText(isDarkMode ? "Switch to Light" : "Switch to Dark");
    }
}

void MainWindow::showFileScreen() {
    stackedWidget->setCurrentIndex(0);
    currentInfo.reset();
}

QString formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;
    while (size >= 1024 && unitIndex < 4) {
        size /= 1024;
        unitIndex++;
    }
    QString formatted = QString::number(size, 'f', 2) + " " + units[unitIndex];
    QString bytesStr = QString::number(bytes);
    int n = bytesStr.length() - 3;
    while (n > 0) {
        bytesStr.insert(n, ",");
        n -= 3;
    }
    return formatted + " (" + bytesStr + " bytes)";
}

void MainWindow::updateOverviewTab() {
    QString text = "=== File Overview ===\n\n";
    text += "File Name: " + QString::fromStdString(currentInfo->fileName) + "\n";
    text += "File Size: " + formatSize(currentInfo->fileSize) + "\n";
    text += "Format: " + QString::fromStdString(currentInfo->format) + "\n";
    text += "Architecture: " + QString::fromStdString(currentInfo->architecture) + "\n";
    text += "Entry Point: " + QString::fromStdString(currentInfo->entryPoint) + "\n";
    text += "Subsystem: " + QString::fromStdString(currentInfo->subsystem) + "\n";
    if (!currentInfo->fileCreated.empty()) {
        text += "File Created: " + QString::fromStdString(currentInfo->fileCreated) + "\n";
    }
    if (!currentInfo->compileTime.empty()) {
        text += "Compile Time: " + QString::fromStdString(currentInfo->compileTime) + "\n";
    }
    text += "\n=== Hashes ===\n";
    text += "MD5: " + QString::fromStdString(currentInfo->hashes.md5) + "\n";
    text += "SHA1: " + QString::fromStdString(currentInfo->hashes.sha1) + "\n";
    text += "SHA256: " + QString::fromStdString(currentInfo->hashes.sha256) + "\n";
    overviewEdit->setPlainText(text);
}

void MainWindow::updateSectionsTab() {
    sectionsTable->setRowCount(currentInfo->sections.size());
    for (size_t i = 0; i < currentInfo->sections.size(); ++i) {
        const auto& sec = currentInfo->sections[i];
        sectionsTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(sec.name)));
        sectionsTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(sec.virtualAddress)));
        sectionsTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(sec.virtualSize)));
        sectionsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(sec.rawSize)));
        sectionsTable->setItem(i, 4, new QTableWidgetItem(QString::number(sec.entropy, 'f', 2)));
        sectionsTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(sec.permissions)));
        
        if (!sec.suspiciousFlags.empty()) {
            QTableWidgetItem* flagItem = new QTableWidgetItem(QString::fromStdString(sec.suspiciousFlags));
            flagItem->setForeground(Qt::red);
            sectionsTable->setItem(i, 6, flagItem);
        } else {
            sectionsTable->setItem(i, 6, new QTableWidgetItem("-"));
        }
    }
    sectionsTable->resizeColumnsToContents();
    sectionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void MainWindow::updateImportsTab() {
    int rowCount = 0;
    for (const auto& imp : currentInfo->imports) {
        rowCount += imp.functions.size();
        if (imp.functions.empty()) rowCount++;
    }
    importsTable->setRowCount(rowCount > 0 ? rowCount : 1);
    
    if (currentInfo->imports.empty()) {
        importsTable->setItem(0, 0, new QTableWidgetItem("No imports found"));
        importsTable->setItem(0, 1, new QTableWidgetItem("-"));
        importsTable->setItem(0, 2, new QTableWidgetItem("-"));
    } else {
        int row = 0;
        for (const auto& imp : currentInfo->imports) {
            if (imp.functions.empty()) {
                importsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(imp.dllName)));
                importsTable->setItem(row, 1, new QTableWidgetItem("(none)"));
                importsTable->setItem(row, 2, new QTableWidgetItem("-"));
                row++;
            } else {
                for (const auto& func : imp.functions) {
                    importsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(imp.dllName)));
                    importsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(func.name)));
                    importsTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(func.address)));
                    if (func.suspicious) {
                        for (int c = 0; c < 3; ++c) {
                            importsTable->item(row, c)->setForeground(Qt::red);
                        }
                    }
                    row++;
                }
            }
        }
    }
    importsTable->resizeColumnsToContents();
    importsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    importsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}

void MainWindow::updateStringsTab() {
    QString text;
    
    std::map<std::string, std::vector<std::string>> categorized;
    for (const auto& str : currentInfo->strings) {
        categorized[str.category].push_back(str.value);
    }
    
    for (const auto& cat : categorized) {
        text += "=== " + QString::fromStdString(cat.first) + " ===\n";
        for (const auto& s : cat.second) {
            text += QString::fromStdString(s) + "\n";
        }
        text += "\n";
    }
    
    if (text.isEmpty()) {
        text = "No strings found";
    }
    stringsEdit->setPlainText(text);
}

void MainWindow::copySelectedCells() {
    QTableWidget* table = qobject_cast<QTableWidget*>(focusWidget());
    if (!table) {
        table = sectionsTable;
    }
    
    QString clipboardText;
    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    for (const auto& range : ranges) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            QString rowText;
            for (int col = range.leftColumn(); col <= range.rightColumn(); ++col) {
                if (table->item(row, col)) {
                    rowText += table->item(row, col)->text();
                }
                if (col < range.rightColumn()) {
                    rowText += "\t";
                }
            }
            clipboardText += rowText + "\n";
        }
    }
    
    if (!clipboardText.isEmpty()) {
        QApplication::clipboard()->setText(clipboardText);
    }
}

HexView::HexView(QWidget* parent)
    : QWidget(parent), m_fileSize(0), m_startOffset(0), m_visibleRows(20), m_scrollBar(nullptr) {
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: #1e1e1e;");
    setMinimumHeight(400);
    setMouseTracking(true);
    
    m_scrollBar = new QScrollBar(Qt::Vertical, this);
    m_scrollBar->setStyleSheet("QScrollBar { width: 15px; background: #252525; } QScrollBar::handle { background: #555; }");
    connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int value) {
        size_t newOffset = value * BYTES_PER_ROW;
        if (newOffset != m_startOffset) {
            m_startOffset = newOffset;
            loadData();
            update();
        }
    });
}

void HexView::setFilePath(const QString& path, size_t fileSize) {
    m_filePath = path;
    m_fileSize = fileSize;
    m_startOffset = 0;
    
    size_t totalRows = (m_fileSize + BYTES_PER_ROW - 1) / BYTES_PER_ROW;
    m_scrollBar->setRange(0, totalRows > (size_t)m_visibleRows ? totalRows - m_visibleRows : 0);
    m_scrollBar->setPageStep(m_visibleRows);
    m_scrollBar->setValue(0);
    
    loadData();
    update();
}

void HexView::setDarkMode(bool dark) {
    m_darkMode = dark;
    if (dark) {
        setStyleSheet("background-color: #1e1e1e;");
        m_scrollBar->setStyleSheet("QScrollBar { width: 15px; background: #252525; } QScrollBar::handle { background: #555; }");
    } else {
        setStyleSheet("background-color: #ffffff;");
        m_scrollBar->setStyleSheet("QScrollBar { width: 15px; background: #f0f0f0; } QScrollBar::handle { background: #c0c0c0; }");
    }
    update();
}

void HexView::loadData() {
    if (m_filePath.isEmpty() || m_fileSize == 0) {
        m_data.clear();
        return;
    }
    
    std::ifstream file(m_filePath.toStdString(), std::ios::binary);
    if (!file.is_open()) {
        m_data.clear();
        return;
    }
    
    size_t loadSize = m_visibleRows * BYTES_PER_ROW * 2;
    if (m_startOffset + loadSize > m_fileSize) {
        loadSize = m_fileSize - m_startOffset;
    }
    
    file.seekg(m_startOffset);
    m_data.resize(loadSize);
    file.read(m_data.data(), loadSize);
    file.close();
    m_dataOffset = m_startOffset;
}

void HexView::resizeEvent(QResizeEvent* event) {
    int scrollBarWidth = 15;
    m_scrollBar->setGeometry(width() - scrollBarWidth, 0, scrollBarWidth, height() - 30);
    
    m_visibleRows = (height() - 30) / ROW_HEIGHT;
    if (m_visibleRows < 1) m_visibleRows = 1;
    
    size_t totalRows = (m_fileSize + BYTES_PER_ROW - 1) / BYTES_PER_ROW;
    m_scrollBar->setRange(0, totalRows > (size_t)m_visibleRows ? totalRows - m_visibleRows : 0);
    m_scrollBar->setPageStep(m_visibleRows);
    
    loadData();
    QWidget::resizeEvent(event);
}

bool HexView::event(QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        keyPressEvent(static_cast<QKeyEvent*>(event));
        return true;
    } else if (event->type() == QEvent::FocusIn) {
        update();
        return true;
    } else if (event->type() == QEvent::Wheel) {
        wheelEvent(static_cast<QWheelEvent*>(event));
        return true;
    }
    return QWidget::event(event);
}

void HexView::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) {
        if (m_startOffset >= BYTES_PER_ROW * 3) {
            m_startOffset -= BYTES_PER_ROW * 3;
        } else {
            m_startOffset = 0;
        }
    } else {
        if (m_startOffset + BYTES_PER_ROW * m_visibleRows < m_fileSize) {
            m_startOffset += BYTES_PER_ROW * 3;
            if (m_startOffset + BYTES_PER_ROW * m_visibleRows > m_fileSize) {
                m_startOffset = (m_fileSize > BYTES_PER_ROW * m_visibleRows) ? 
                    m_fileSize - BYTES_PER_ROW * m_visibleRows : 0;
                m_startOffset = (m_startOffset / BYTES_PER_ROW) * BYTES_PER_ROW;
            }
        }
    }
    loadData();
    update();
    event->accept();
}

void HexView::mousePressEvent(QMouseEvent* event) {
    setFocus();
    QWidget::mousePressEvent(event);
}

void HexView::keyPressEvent(QKeyEvent* event) {
    size_t totalRows = (m_fileSize + BYTES_PER_ROW - 1) / BYTES_PER_ROW;
    
    if (event->key() == Qt::Key_Down) {
        if (m_startOffset + BYTES_PER_ROW * m_visibleRows < m_fileSize) {
            m_startOffset += BYTES_PER_ROW;
            loadData();
            update();
        }
    } else if (event->key() == Qt::Key_Up) {
        if (m_startOffset >= BYTES_PER_ROW) {
            m_startOffset -= BYTES_PER_ROW;
            loadData();
            update();
        }
    } else if (event->key() == Qt::Key_PageDown) {
        if (m_startOffset + BYTES_PER_ROW * m_visibleRows < m_fileSize) {
            m_startOffset += BYTES_PER_ROW * (m_visibleRows - 1);
            if (m_startOffset + BYTES_PER_ROW * m_visibleRows > m_fileSize) {
                m_startOffset = (totalRows > (size_t)m_visibleRows) ? 
                    (totalRows - m_visibleRows) * BYTES_PER_ROW : 0;
            }
            loadData();
            update();
        }
    } else if (event->key() == Qt::Key_PageUp) {
        if (m_startOffset >= BYTES_PER_ROW * (m_visibleRows - 1)) {
            m_startOffset -= BYTES_PER_ROW * (m_visibleRows - 1);
        } else {
            m_startOffset = 0;
        }
        loadData();
        update();
    } else if (event->key() == Qt::Key_Home && (event->modifiers() & Qt::ControlModifier)) {
        m_startOffset = 0;
        loadData();
        update();
    } else if (event->key() == Qt::Key_End && (event->modifiers() & Qt::ControlModifier)) {
        m_startOffset = (totalRows > (size_t)m_visibleRows) ? 
            (totalRows - m_visibleRows) * BYTES_PER_ROW : 0;
        loadData();
        update();
    }
    event->accept();
}

void HexView::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    
    QColor bgColor = m_darkMode ? QColor("#1e1e1e") : QColor("#ffffff");
    QColor addrColor = m_darkMode ? QColor("#888888") : QColor("#666666");
    QColor hexColor = m_darkMode ? QColor("#00ff00") : QColor("#006600");
    QColor statusBgColor = m_darkMode ? QColor("#252525") : QColor("#e0e0e0");
    QColor statusTextColor = m_darkMode ? QColor("#888888") : QColor("#000000");
    
    painter.fillRect(rect(), bgColor);
    
    QFont mono("monospace", 12);
    painter.setFont(mono);
    QFontMetrics fm(mono);
    int charWidth = fm.horizontalAdvance("0");
    
    size_t totalRows = (m_fileSize + BYTES_PER_ROW - 1) / BYTES_PER_ROW;
    
    int statusBarHeight = 30;
    int availableHeight = height() - statusBarHeight;
    int maxRows = availableHeight / ROW_HEIGHT;
    if (maxRows < 1) maxRows = 1;
    
    int y = 10;
    for (int i = 0; i < maxRows; i++) {
        size_t rowOffset = m_startOffset + i * BYTES_PER_ROW;
        if (rowOffset >= m_fileSize) break;
        
        QString addr = QString("%1").arg(rowOffset, 8, 16, QChar('0')).toUpper();
        painter.setPen(addrColor);
        painter.drawText(10, y + fm.ascent(), addr);
        
        int x = ADDRESS_WIDTH + 20;
        
        for (int j = 0; j < BYTES_PER_ROW; j++) {
            if (rowOffset + j < m_fileSize && i * BYTES_PER_ROW + j < (size_t)m_data.size()) {
                uint8_t b = static_cast<uint8_t>(m_data[i * BYTES_PER_ROW + j]);
                QString hex = QString("%1").arg(b, 2, 16, QChar('0')).toUpper();
                painter.setPen(hexColor);
                painter.drawText(x, y + fm.ascent(), hex);
                x += charWidth * 3;
                if (j == 7) x += charWidth;
            }
        }
        
        x = ADDRESS_WIDTH + 20 + HEX_WIDTH + 30;
        for (int j = 0; j < BYTES_PER_ROW; j++) {
            if (rowOffset + j < m_fileSize && i * BYTES_PER_ROW + j < (size_t)m_data.size()) {
                uint8_t b = static_cast<uint8_t>(m_data[i * BYTES_PER_ROW + j]);
                QChar c = (b >= 32 && b < 127) ? QChar(b) : QChar('.');
                painter.setPen(hexColor);
                painter.drawText(x, y + fm.ascent(), QString(c));
                x += charWidth;
            }
        }
        
        y += ROW_HEIGHT;
    }
    
    painter.fillRect(0, availableHeight, width(), statusBarHeight, statusBgColor);
    
    QString status = QString("Offset: 0x%1 | Rows: %2-%3 of %4 | Size: %5 bytes")
        .arg(m_startOffset, 8, 16, QChar('0')).toUpper()
        .arg(m_startOffset / BYTES_PER_ROW + 1)
        .arg(qMin(m_startOffset / BYTES_PER_ROW + maxRows, (size_t)totalRows))
        .arg(totalRows)
        .arg(m_fileSize);
    
    painter.setPen(statusTextColor);
    painter.drawText(10, availableHeight + 20, status);
}

void MainWindow::updateHexView() {
    if (!currentInfo) return;
    hexView->setFilePath(QString::fromStdString(currentInfo->filePath), currentInfo->fileSize);
}

void MainWindow::applySettings() {
    QList<QPushButton*> themeButtons = settingsTab->findChildren<QPushButton*>();
    
    if (themeButtons.count() > 0) {
        applyTheme(themeButtons[0]->isChecked());
    }
}

void MainWindow::applyTheme(bool dark) {
    isDarkMode = dark;
    if (dark) {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(darkPalette);
        qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
        
        QString darkStyle = R"(
            QTextEdit { background-color: #2a2a2a; color: #ffffff; font-family: monospace; }
            QTableWidget { background-color: #2a2a2a; color: #ffffff; gridline-color: #555; }
            QHeaderView::section { background-color: #3a3a3a; color: #ffffff; padding: 4px; }
            QTabWidget::pane { border: 1px solid #555; }
            QTabBar::tab { background-color: #353535; color: #ffffff; padding: 8px 16px; }
            QTabBar::tab:selected { background-color: #2a2a2a; }
        )";
        tabWidget->setStyleSheet(darkStyle);
        overviewEdit->setStyleSheet("QTextEdit { background-color: #2a2a2a; color: #ffffff; font-family: monospace; font-size: 14px; }");
        stringsEdit->setStyleSheet("QTextEdit { background-color: #2a2a2a; color: #ffffff; font-family: monospace; font-size: 14px; }");
        sectionsTable->setStyleSheet("QTableWidget { background-color: #2a2a2a; color: #ffffff; gridline-color: #555; } QHeaderView::section { background-color: #3a3a3a; color: #ffffff; padding: 4px; }");
        importsTable->setStyleSheet("QTableWidget { background-color: #2a2a2a; color: #ffffff; gridline-color: #555; } QHeaderView::section { background-color: #3a3a3a; color: #ffffff; padding: 4px; }");
        
        fileScreen->setStyleSheet("background-color: #353535;");
        QList<QLabel*> fileLabels = fileScreen->findChildren<QLabel*>();
        for (QLabel* lbl : fileLabels) {
            if (lbl->objectName() == "versionLabel") continue;
            lbl->setStyleSheet("color: #ffffff;");
        }
        QLabel* versionLabel = fileScreen->findChild<QLabel*>("versionLabel");
        if (versionLabel) {
            versionLabel->setStyleSheet("font-family: monospace; font-size: 12px; color: #888888;");
        }
        QList<QLineEdit*> fileInputs = fileScreen->findChildren<QLineEdit*>();
        for (QLineEdit* input : fileInputs) {
            input->setStyleSheet("background-color: #2a2a2a; color: #ffffff; padding: 8px; font-size: 14px; border: 1px solid #555;");
        }
        QList<QPushButton*> fileButtons = fileScreen->findChildren<QPushButton*>();
        for (QPushButton* btn : fileButtons) {
            if (btn->objectName() == "debugBtn") {
                if (btn->isEnabled()) {
                    btn->setStyleSheet("QPushButton { background-color: #3a3a3a; color: #ffffff; padding: 8px 16px; font-size: 14px; border: 1px solid #555; } QPushButton:hover { background-color: #4a4a4a; }");
                } else {
                    btn->setStyleSheet("QPushButton { background-color: #2a2a2a; color: #666666; padding: 8px 16px; font-size: 14px; border: 1px solid #444; }");
                }
            } else if (btn->objectName() == "settingsBtn") {
                btn->setStyleSheet("QPushButton { background-color: #3a3a3a; color: #aaaaaa; padding: 8px 16px; font-size: 14px; border: 1px solid #555; } QPushButton:hover { background-color: #4a4a4a; color: #ffffff; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #4a4a4a; color: #ffffff; padding: 8px 16px; font-size: 14px; border: 1px solid #555; } QPushButton:hover { background-color: #5a5a5a; }");
            }
        }
        
        settingsTab->setStyleSheet("background-color: #353535;");
        QList<QLabel*> settingsLabels = settingsTab->findChildren<QLabel*>();
        for (QLabel* lbl : settingsLabels) {
            if (lbl->objectName() == "debugWarningLabel") continue;
            lbl->setStyleSheet("color: #ffffff;");
        }
        QList<QPushButton*> settingsButtons = settingsTab->findChildren<QPushButton*>();
        for (QPushButton* btn : settingsButtons) {
            if (btn->objectName() == "themeBtn") {
                btn->setStyleSheet("QPushButton { background-color: #555555; color: #ffffff; font-size: 14px; border-radius: 4px; border: 1px solid #666666; } QPushButton:hover { background-color: #666666; }");
            } else if (btn->objectName() == "cancelBtn") {
                btn->setStyleSheet("QPushButton { background-color: #4a4a4a; color: #ffffff; font-size: 14px; border-radius: 4px; border: 1px solid #555555; } QPushButton:hover { background-color: #5a5a5a; }");
            } else if (btn->objectName() == "saveBtn") {
                btn->setStyleSheet("QPushButton { background-color: #2a82da; color: #ffffff; font-size: 14px; font-weight: bold; border-radius: 4px; border: none; } QPushButton:hover { background-color: #3a92ea; }");
            }
        }
        
        hexView->setDarkMode(true);
        
        QList<QPushButton*> mainButtons = mainScreen->findChildren<QPushButton*>();
        for (QPushButton* btn : mainButtons) {
            if (btn->objectName() == "saveBtn") {
                btn->setStyleSheet("QPushButton { background-color: #2a82da; color: #ffffff; font-size: 14px; font-weight: bold; border-radius: 4px; border: none; } QPushButton:hover { background-color: #3a92ea; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #4a4a4a; color: #ffffff; padding: 10px; font-size: 14px; border: none; } QPushButton:hover { background-color: #5a5a5a; }");
            }
        }
        
        if (settingsScreen) {
            settingsScreen->setStyleSheet("background-color: #353535;");
            QList<QLabel*> sScreenLabels = settingsScreen->findChildren<QLabel*>();
            for (QLabel* lbl : sScreenLabels) {
                if (lbl->objectName() == "sDebugWarningLabel") continue;
                lbl->setStyleSheet("color: #ffffff;");
            }
            QList<QPushButton*> sScreenButtons = settingsScreen->findChildren<QPushButton*>();
            for (QPushButton* btn : sScreenButtons) {
                if (btn->objectName() == "sThemeBtn") {
                    btn->setStyleSheet("QPushButton { background-color: #555555; color: #ffffff; font-size: 14px; border-radius: 4px; border: 1px solid #666666; } QPushButton:hover { background-color: #666666; }");
                } else if (btn->objectName() == "sCancelBtn") {
                    btn->setStyleSheet("QPushButton { background-color: #4a4a4a; color: #ffffff; font-size: 14px; border-radius: 4px; border: 1px solid #555555; } QPushButton:hover { background-color: #5a5a5a; }");
                } else if (btn->objectName() == "sSaveBtn") {
                    btn->setStyleSheet("QPushButton { background-color: #2a82da; color: #ffffff; font-size: 14px; font-weight: bold; border-radius: 4px; border: none; } QPushButton:hover { background-color: #3a92ea; }");
                } else if (btn->objectName() == "backBtn") {
                    btn->setStyleSheet("QPushButton { background-color: #4a4a4a; color: #ffffff; padding: 8px 16px; font-size: 14px; border: 1px solid #555; } QPushButton:hover { background-color: #5a5a5a; }");
                }
            }
            QList<QCheckBox*> sScreenChecks = settingsScreen->findChildren<QCheckBox*>();
            for (QCheckBox* chk : sScreenChecks) {
                chk->setStyleSheet("color: #ffffff;");
            }
        }
        
    } else {
        QPalette lightPalette;
        lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
        lightPalette.setColor(QPalette::WindowText, Qt::black);
        lightPalette.setColor(QPalette::Base, Qt::white);
        lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
        lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220));
        lightPalette.setColor(QPalette::ToolTipText, Qt::black);
        lightPalette.setColor(QPalette::Text, Qt::black);
        lightPalette.setColor(QPalette::Button, QColor(220, 220, 220));
        lightPalette.setColor(QPalette::ButtonText, Qt::black);
        lightPalette.setColor(QPalette::BrightText, Qt::red);
        lightPalette.setColor(QPalette::Link, QColor(0, 100, 200));
        lightPalette.setColor(QPalette::Highlight, QColor(0, 100, 200));
        lightPalette.setColor(QPalette::HighlightedText, Qt::white);
        qApp->setPalette(lightPalette);
        qApp->setStyleSheet("QToolTip { color: #000000; background-color: #ffffdc; border: 1px solid #000000; }");
        
        QString lightStyle = R"(
            QTextEdit { background-color: #ffffff; color: #000000; font-family: monospace; }
            QTableWidget { background-color: #ffffff; color: #000000; gridline-color: #ccc; }
            QHeaderView::section { background-color: #e0e0e0; color: #000000; padding: 4px; }
            QTabWidget::pane { border: 1px solid #ccc; }
            QTabBar::tab { background-color: #f0f0f0; color: #000000; padding: 8px 16px; }
            QTabBar::tab:selected { background-color: #ffffff; }
        )";
        tabWidget->setStyleSheet(lightStyle);
        overviewEdit->setStyleSheet("QTextEdit { background-color: #ffffff; color: #000000; font-family: monospace; font-size: 14px; }");
        stringsEdit->setStyleSheet("QTextEdit { background-color: #ffffff; color: #000000; font-family: monospace; font-size: 14px; }");
        sectionsTable->setStyleSheet("QTableWidget { background-color: #ffffff; color: #000000; gridline-color: #ccc; } QHeaderView::section { background-color: #e0e0e0; color: #000000; padding: 4px; }");
        importsTable->setStyleSheet("QTableWidget { background-color: #ffffff; color: #000000; gridline-color: #ccc; } QHeaderView::section { background-color: #e0e0e0; color: #000000; padding: 4px; }");
        
        fileScreen->setStyleSheet("background-color: #f5f5f5;");
        QList<QLabel*> fileLabels = fileScreen->findChildren<QLabel*>();
        for (QLabel* lbl : fileLabels) {
            if (lbl->objectName() == "versionLabel") continue;
            lbl->setStyleSheet("color: #000000;");
        }
        QLabel* versionLabel = fileScreen->findChild<QLabel*>("versionLabel");
        if (versionLabel) {
            versionLabel->setStyleSheet("font-family: monospace; font-size: 12px; color: #888888;");
        }
        QList<QLineEdit*> fileInputs = fileScreen->findChildren<QLineEdit*>();
        for (QLineEdit* input : fileInputs) {
            input->setStyleSheet("background-color: #ffffff; color: #000000; padding: 8px; font-size: 14px; border: 1px solid #ccc;");
        }
        QList<QPushButton*> fileButtons = fileScreen->findChildren<QPushButton*>();
        for (QPushButton* btn : fileButtons) {
            if (btn->objectName() == "debugBtn") {
                if (btn->isEnabled()) {
                    btn->setStyleSheet("QPushButton { background-color: #d0d0d0; color: #000000; padding: 8px 16px; font-size: 14px; border: 1px solid #ccc; } QPushButton:hover { background-color: #c0c0c0; }");
                } else {
                    btn->setStyleSheet("QPushButton { background-color: #f0f0f0; color: #999999; padding: 8px 16px; font-size: 14px; border: 1px solid #ddd; }");
                }
            } else if (btn->objectName() == "settingsBtn") {
                btn->setStyleSheet("QPushButton { background-color: #d0d0d0; color: #666666; padding: 8px 16px; font-size: 14px; border: 1px solid #ccc; } QPushButton:hover { background-color: #c0c0c0; color: #000000; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #e0e0e0; color: #000000; padding: 8px 16px; font-size: 14px; border: 1px solid #ccc; } QPushButton:hover { background-color: #d0d0d0; }");
            }
        }
        
        settingsTab->setStyleSheet("background-color: #f5f5f5;");
        QList<QLabel*> settingsLabels = settingsTab->findChildren<QLabel*>();
        for (QLabel* lbl : settingsLabels) {
            if (lbl->objectName() == "debugWarningLabel") continue;
            lbl->setStyleSheet("color: #000000;");
        }
        QList<QPushButton*> settingsButtons = settingsTab->findChildren<QPushButton*>();
        for (QPushButton* btn : settingsButtons) {
            if (btn->objectName() == "themeBtn") {
                btn->setStyleSheet("QPushButton { background-color: #e0e0e0; color: #000000; font-size: 14px; border-radius: 4px; border: 1px solid #ccc; } QPushButton:hover { background-color: #d0d0d0; }");
            } else if (btn->objectName() == "cancelBtn") {
                btn->setStyleSheet("QPushButton { background-color: #d0d0d0; color: #000000; font-size: 14px; border-radius: 4px; border: 1px solid #bbb; } QPushButton:hover { background-color: #c0c0c0; }");
            } else if (btn->objectName() == "saveBtn") {
                btn->setStyleSheet("QPushButton { background-color: #2a82da; color: #ffffff; font-size: 14px; font-weight: bold; border-radius: 4px; border: none; } QPushButton:hover { background-color: #3a92ea; }");
            }
        }
        
        hexView->setDarkMode(false);
        
        QList<QPushButton*> mainButtons = mainScreen->findChildren<QPushButton*>();
        for (QPushButton* btn : mainButtons) {
            if (btn->objectName() == "saveBtn") {
                btn->setStyleSheet("QPushButton { background-color: #2a82da; color: #ffffff; font-size: 14px; font-weight: bold; border-radius: 4px; border: none; } QPushButton:hover { background-color: #3a92ea; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #e0e0e0; color: #000000; padding: 10px; font-size: 14px; border: 1px solid #ccc; } QPushButton:hover { background-color: #d0d0d0; }");
            }
        }
        
        if (settingsScreen) {
            settingsScreen->setStyleSheet("background-color: #f5f5f5;");
            QList<QLabel*> sScreenLabels = settingsScreen->findChildren<QLabel*>();
            for (QLabel* lbl : sScreenLabels) {
                if (lbl->objectName() == "sDebugWarningLabel") continue;
                lbl->setStyleSheet("color: #000000;");
            }
            QList<QPushButton*> sScreenButtons = settingsScreen->findChildren<QPushButton*>();
            for (QPushButton* btn : sScreenButtons) {
                if (btn->objectName() == "sThemeBtn") {
                    btn->setStyleSheet("QPushButton { background-color: #e0e0e0; color: #000000; font-size: 14px; border-radius: 4px; border: 1px solid #ccc; } QPushButton:hover { background-color: #d0d0d0; }");
                } else if (btn->objectName() == "sCancelBtn") {
                    btn->setStyleSheet("QPushButton { background-color: #d0d0d0; color: #000000; font-size: 14px; border-radius: 4px; border: 1px solid #bbb; } QPushButton:hover { background-color: #c0c0c0; }");
                } else if (btn->objectName() == "sSaveBtn") {
                    btn->setStyleSheet("QPushButton { background-color: #2a82da; color: #ffffff; font-size: 14px; font-weight: bold; border-radius: 4px; border: none; } QPushButton:hover { background-color: #3a92ea; }");
                } else if (btn->objectName() == "backBtn") {
                    btn->setStyleSheet("QPushButton { background-color: #e0e0e0; color: #000000; padding: 8px 16px; font-size: 14px; border: 1px solid #ccc; } QPushButton:hover { background-color: #d0d0d0; }");
                }
            }
            QList<QCheckBox*> sScreenChecks = settingsScreen->findChildren<QCheckBox*>();
            for (QCheckBox* chk : sScreenChecks) {
                chk->setStyleSheet("color: #000000;");
            }
        }
    }
}

void MainWindow::applyFontSize(int size) {
    QString style = QString("QTextEdit { font-size: %1px; } QTableWidget { font-size: %1px; }").arg(size);
    overviewEdit->setStyleSheet(QString("QTextEdit { background-color: #2a2a2a; color: #ffffff; font-family: monospace; font-size: %1px; }").arg(size));
    stringsEdit->setStyleSheet(QString("QTextEdit { background-color: #2a2a2a; color: #ffffff; font-family: monospace; font-size: %1px; }").arg(size));
}

void MainWindow::applyMinStringLength(int len) {
}

void MainWindow::applyHumanReadableSizes(bool enabled) {
    if (currentInfo) {
        updateOverviewTab();
    }
}

void MainWindow::applySectionWarnings(bool enabled) {
    if (currentInfo) {
        updateSectionsTab();
    }
}

void MainWindow::updateDebugWarning() {
    if (debugWarningLabel) {
        if (isDebugMode) {
            debugWarningLabel->setText("Debug mode: ON - Select \"Open another file\" or Restart app");
            debugWarningLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: red;");
            debugWarningLabel->show();
            debugBtn->setEnabled(true);
            if (isDarkMode) {
                debugBtn->setStyleSheet("QPushButton { background-color: #3a3a3a; color: #ffffff; padding: 8px 16px; font-size: 14px; border: 1px solid #555; } QPushButton:hover { background-color: #4a4a4a; }");
            } else {
                debugBtn->setStyleSheet("QPushButton { background-color: #d0d0d0; color: #000000; padding: 8px 16px; font-size: 14px; border: 1px solid #ccc; } QPushButton:hover { background-color: #c0c0c0; }");
            }
        } else {
            debugWarningLabel->hide();
            debugBtn->setEnabled(false);
            if (isDarkMode) {
                debugBtn->setStyleSheet("QPushButton { background-color: #2a2a2a; color: #666666; padding: 8px 16px; font-size: 14px; border: 1px solid #444; }");
            } else {
                debugBtn->setStyleSheet("QPushButton { background-color: #f0f0f0; color: #999999; padding: 8px 16px; font-size: 14px; border: 1px solid #ddd; }");
            }
        }
    }
}

void MainWindow::saveSettings(bool darkMode, bool debugMode) {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString appDir = configPath + "/ryzeneebinscope";
    QString configFile = appDir + "/config.json";
    
    QDir dir;
    if (!dir.exists(appDir)) {
        dir.mkpath(appDir);
    }
    
    std::ofstream file(configFile.toStdString());
    if (file.is_open()) {
        file << "{\n";
        file << "  \"theme\": \"" << (darkMode ? "dark" : "light") << "\",\n";
        file << "  \"debug_mode\": " << (debugMode ? "true" : "false") << "\n";
        file << "}\n";
        file.close();
    }
    
    applyTheme(darkMode);
}

QString MainWindow::getConfigPath() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return configPath + "/ryzeneebinscope/config.json";
}

bool MainWindow::loadSettings() {
    QString configFile = getConfigPath();
    
    std::ifstream file(configFile.toStdString());
    if (!file.is_open()) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    size_t themePos = content.find("\"theme\"");
    if (themePos != std::string::npos) {
        size_t colonPos = content.find(":", themePos);
        if (colonPos != std::string::npos) {
            size_t quotePos = content.find("\"", colonPos);
            if (quotePos != std::string::npos) {
                size_t endQuotePos = content.find("\"", quotePos + 1);
                if (endQuotePos != std::string::npos) {
                    std::string themeValue = content.substr(quotePos + 1, endQuotePos - quotePos - 1);
                    isDarkMode = (themeValue == "dark");
                }
            }
        }
    }
    
    size_t debugPos = content.find("\"debug_mode\"");
    if (debugPos != std::string::npos) {
        size_t colonPos = content.find(":", debugPos);
        if (colonPos != std::string::npos) {
            size_t start = colonPos + 1;
            while (start < content.size() && (content[start] == ' ' || content[start] == '\t' || content[start] == '\n' || content[start] == '\r')) start++;
            size_t end = start;
            while (end < content.size() && (content[end] == 't' || content[end] == 'f' || content[end] == 'r' || content[end] == 'u' || content[end] == 'a' || content[end] == 'l' || content[end] == 'e' || content[end] == 's')) end++;
            std::string boolStr = content.substr(start, end - start);
            isDebugMode = (boolStr == "true");
        }
    }
    
    return true;
}