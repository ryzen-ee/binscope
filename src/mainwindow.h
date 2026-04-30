#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "parser.h"
#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QTableWidget>
#include <QThread>
#include <QAction>
#include <QClipboard>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <memory>
#include <atomic>
#include <vector>

class AnalysisWorker : public QThread {
    Q_OBJECT
public:
    AnalysisWorker(const QString& path, QObject* parent = nullptr) : QThread(parent), m_path(path) {}
    std::unique_ptr<BinaryInfo> result;
    std::exception_ptr exception;
    
signals:
    void progress(int value, QString status);
    
protected:
    void run() override {
        try {
            emit progress(10, "Reading file...");
            result = std::make_unique<BinaryInfo>(analyzeBinary(m_path.toStdString().c_str()));
            emit progress(100, "Done");
        } catch (...) {
            exception = std::current_exception();
            emit progress(0, "Error");
        }
    }
private:
    QString m_path;
};

class HexView : public QWidget {
    Q_OBJECT
public:
    HexView(QWidget* parent = nullptr);
    void setFilePath(const QString& path, size_t fileSize);
    void setDarkMode(bool dark);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
private:
    QString m_filePath;
    size_t m_fileSize;
    size_t m_startOffset;
    int m_visibleRows;
    QScrollBar* m_scrollBar;
    bool m_darkMode = true;
    static const int BYTES_PER_ROW = 16;
    static const int ADDRESS_WIDTH = 80;
    static const int HEX_WIDTH = 480;
    static const int ASCII_WIDTH = 160;
    static const int ROW_HEIGHT = 20;
    
    void loadData();
    QByteArray m_data;
    size_t m_dataOffset;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onBrowse();
    void onAnalyze();
    void onAnalysisFinished();
    void onDebugMode();
    void copySelectedCells();
    void applySettings();

private:
    void showFileScreen();
    void showMainScreen();
    void updateOverviewTab();
    void updateSectionsTab();
    void updateImportsTab();
    void updateStringsTab();
    void updateSettingsTab();
    void applyTheme(bool dark);
    void applyFontSize(int size);
    void applyMinStringLength(int len);
    void applyHumanReadableSizes(bool enabled);
    void applySectionWarnings(bool enabled);
    void saveSettings(bool darkMode, bool debugMode);
    bool loadSettings();
    QString getConfigPath();

    QStackedWidget* stackedWidget;
    
    // File screen widgets
    QWidget* fileScreen;
    QLineEdit* filePathInput;
    QPushButton* browseBtn;
    QPushButton* analyzeBtn;
    QLabel* errorLabel;
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QLabel* etaLabel;
    QPushButton* debugBtn;
    
    // Main screen widgets
    QWidget* mainScreen;
    QTabWidget* tabWidget;
    
    std::unique_ptr<BinaryInfo> currentInfo;
    AnalysisWorker* worker;
    
    QTextEdit* overviewEdit;
    QTableWidget* sectionsTable;
    QTableWidget* importsTable;
    QTextEdit* stringsEdit;
    QWidget* settingsTab;
    QWidget* settingsScreen;
    QLabel* sDebugWarningLabel;
    
    class HexView* hexView;
    void updateHexView();
    bool isDarkMode = true;
    bool isDebugMode = false;
    QLabel* debugWarningLabel;
    QLabel* saveStatusLabel;
    void updateDebugWarning();
};

#endif