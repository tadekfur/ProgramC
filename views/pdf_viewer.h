#pragma once

#include <QWidget>
#include <QFileSystemModel>
#include <QListView>
#include <QSplitter>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QComboBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QStackedWidget>

// Opcjonalnie QWebEngineView jeśli dostępne
#ifdef QT_WEBENGINEWIDGETS_LIB
#include <QWebEngineView>
#endif

// Na Windows dodaj QAxWidget dla Adobe Reader
#ifdef Q_OS_WIN
// Sprawdź czy ActiveX jest dostępny
#ifdef QT_AXCONTAINER_LIB
#include <QAxWidget>
#define ACTIVEX_AVAILABLE
#endif
#endif

/**
 * Widok do przeglądania plików PDF z zamówieniami lub materiałami produkcyjnymi.
 * 
 * Umożliwia:
 * - Podgląd listy plików PDF z dwóch konfigurowalnych katalogów
 * - Podgląd zawartości plików PDF
 * - Otwieranie plików PDF do podglądu
 * - Drukowanie plików PDF
 */
class PdfViewer : public QWidget {
    Q_OBJECT

public:
    enum ViewMode {
        OrdersMode,      // Tryb przeglądania zamówień
        MaterialsMode    // Tryb przeglądania materiałów produkcyjnych
    };

    explicit PdfViewer(ViewMode mode, QWidget *parent = nullptr);
    ~PdfViewer();
    
    void refreshFileList();
    
public slots:
    void openSelectedFile();
    void printSelectedFile();
    
private slots:
    void onOrdersFileSelected(const QModelIndex &index);
    void onConfirmationsFileSelected(const QModelIndex &index);
    void onSearchTextChanged(const QString &text);
    void onSortOrderChanged(int index);
    void updatePdfPreview(const QString &filePath);
    
private:
    void setupUI();
    void setupConnections();
    void setupDirectories();
    void updateFileFilter();
    void renderPdfPreview(const QString &filePath);
    
    // Funkcje dla różnych metod podglądu PDF
    bool tryEmbeddedPdfViewer(const QString &filePath);
    bool tryPdfToImageConversion(const QString &filePath);
    
    // Nowe funkcje dla alternatywnych metod podglądu PDF
    bool tryAlternativePdfViewer(const QString &filePath);
    bool trySumatraPdfEmbedded(const QString &filePath);
    bool trySimplePdfInfo(const QString &filePath);
    
#ifdef Q_OS_WIN
    bool tryAdobeReaderViewer(const QString &filePath);
    bool tryWindowsShellPreview(const QString &filePath);
#endif
    
    // UI komponenty
    QSplitter *m_mainSplitter;
    QSplitter *m_leftSplitter; // Splitter dla podziału lewego panelu
    QListView *m_ordersFileList; // Lista plików zamówień produkcyjnych
    QListView *m_confirmationsFileList; // Lista plików potwierdzeń
    QLineEdit *m_searchBox;
    QComboBox *m_sortOrderCombo;
    QPushButton *m_openButton;
    QPushButton *m_printButton;
    QPushButton *m_refreshButton;
    QLabel *m_previewLabel;
    QGroupBox *m_ordersGroup;
    QGroupBox *m_confirmationsGroup;
    QStackedWidget *m_pdfPreviewWidget; // Widget do podglądu PDF
    QScrollArea *m_previewScrollArea; // Obszar przewijania dla podglądu
    
    // WebEngine dla podglądu PDF (opcjonalnie)
    #ifdef QT_WEBENGINEWIDGETS_LIB
    QWebEngineView *m_webEngineView = nullptr;
    #endif
    
    // Adobe Reader dla podglądu PDF (Windows)
    #ifdef ACTIVEX_AVAILABLE
    QAxWidget *m_adobeReaderWidget = nullptr;
    #endif
    
    // Model danych
    QFileSystemModel *m_ordersFileModel; // Model dla listy plików zamówień
    QFileSystemModel *m_confirmationsFileModel; // Model dla listy plików potwierdzeń
    
    // Stan
    QString m_selectedFilePath;
    QString m_selectedConfirmationFilePath; // Ścieżka wybranego pliku potwierdzenia
    ViewMode m_viewMode;
    QString m_ordersDir; // Katalog dla zamówień produkcyjnych
    QString m_confirmationsDir; // Katalog dla potwierdzeń
};
