#include "pdf_viewer.h"
#include "models/user.h"
#include "utils/app_constants.h"
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QPrinter>
#include <QPrintDialog>
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QDateTime>
#include <QSettings>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QImageReader>
#include <QtGui/QPixmap>
#include <QtCore/QTimer>
#include <QTextBrowser>

// Dodaj obsługę QWebEngineView
#ifdef QT_WEBENGINEWIDGETS_LIB
#include <QWebEngineView>
#endif

// Dodaj obsługę Adobe Reader przez ActiveX (Windows)
#ifdef Q_OS_WIN
#ifdef QT_AXCONTAINER_LIB
#include <QAxWidget>
#include <QRegularExpression>
#define ACTIVEX_AVAILABLE
#endif
#endif

PdfViewer::PdfViewer(ViewMode mode, QWidget *parent)
    : QWidget(parent), 
      m_viewMode(mode),
      m_mainSplitter(nullptr),
      m_leftSplitter(nullptr),
      m_ordersFileList(nullptr),
      m_confirmationsFileList(nullptr),
      m_searchBox(nullptr),
      m_sortOrderCombo(nullptr),
      m_openButton(nullptr),
      m_printButton(nullptr),
      m_refreshButton(nullptr),
      m_previewLabel(nullptr),
      m_ordersGroup(nullptr),
      m_confirmationsGroup(nullptr),
      m_pdfPreviewWidget(nullptr),
      m_previewScrollArea(nullptr),
      m_ordersFileModel(nullptr),
      m_confirmationsFileModel(nullptr),
      m_selectedFilePath(""),
      m_selectedConfirmationFilePath(""),
      m_ordersDir(""),
      m_confirmationsDir("")
#ifdef QT_WEBENGINEWIDGETS_LIB
      , m_webEngineView(nullptr)
#endif
#ifdef ACTIVEX_AVAILABLE
      , m_adobeReaderWidget(nullptr)
#endif
{
    // Zwiększ limit pamięci dla obrazów Qt (domyślnie 256MB)
    QImageReader::setAllocationLimit(512); // 512 MB
    
    setupUI();
    setupConnections();
    setupDirectories();
    refreshFileList();
}

PdfViewer::~PdfViewer() {
    // Modele danych są zarządzane przez Qt parent-child mechanism
    // Nie ma potrzeby ręcznego usuwania widgetów, bo są usuwane przez rodzica
    // Ale dobrą praktyką jest ustawienie wskaźników na nullptr po usunięciu
    m_ordersFileModel = nullptr;
    m_confirmationsFileModel = nullptr;
}

void PdfViewer::setupUI() {
    // Układ główny
    auto *mainLayout = new QVBoxLayout(this);
    
    // Nagłówek
    auto *headerLayout = new QHBoxLayout;
    QString title = m_viewMode == OrdersMode ? "Zamówienia PDF" : "Materiały produkcyjne PDF";
    QLabel *titleLabel = new QLabel(title);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    // Wyszukiwarka i sortowanie
    auto *toolbarLayout = new QHBoxLayout;
    
    m_searchBox = new QLineEdit;
    m_searchBox->setPlaceholderText("Szukaj...");
    m_searchBox->setClearButtonEnabled(true);
    
    m_sortOrderCombo = new QComboBox;
    m_sortOrderCombo->addItem("Sortuj wg nazwy");
    m_sortOrderCombo->addItem("Sortuj wg daty (od najnowszych)");
    m_sortOrderCombo->addItem("Sortuj wg daty (od najstarszych)");
    
    m_refreshButton = new QPushButton("Odśwież");
    
    toolbarLayout->addWidget(m_searchBox);
    toolbarLayout->addWidget(m_sortOrderCombo);
    toolbarLayout->addWidget(m_refreshButton);
    
    // Główny splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->setHandleWidth(1);
    
    if (m_viewMode == OrdersMode) {
        // Lewy panel - tylko dwie listy plików (bez drzewa katalogów)
        QWidget *leftPanel = new QWidget;
        QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->setContentsMargins(5, 5, 5, 5);
        leftPanel->setMinimumWidth(300);
        leftPanel->setMaximumWidth(350);
        
        // Grupa dla potwierdzeń (górna lista)
        m_confirmationsGroup = new QGroupBox("Potwierdzenia PDF");
        m_confirmationsGroup->setStyleSheet("QGroupBox { font-weight: bold; }");
        auto *confirmationsLayout = new QVBoxLayout(m_confirmationsGroup);
        
        // Lista plików potwierdzeń
        m_confirmationsFileList = new QListView;
        m_confirmationsFileList->setViewMode(QListView::ListMode);
        m_confirmationsFileList->setResizeMode(QListView::Adjust);
        m_confirmationsFileList->setIconSize(QSize(16, 16));
        m_confirmationsFileList->setUniformItemSizes(true);
        m_confirmationsFileList->setWordWrap(false);
        m_confirmationsFileList->setAlternatingRowColors(true);
        
        confirmationsLayout->addWidget(m_confirmationsFileList);
        
        // Grupa dla zleceń produkcyjnych (dolna lista)
        m_ordersGroup = new QGroupBox("Zlecenia produkcyjne PDF");
        m_ordersGroup->setStyleSheet("QGroupBox { font-weight: bold; }");
        auto *ordersLayout = new QVBoxLayout(m_ordersGroup);
        
        // Lista plików zleceń produkcyjnych
        m_ordersFileList = new QListView;
        m_ordersFileList->setViewMode(QListView::ListMode);
        m_ordersFileList->setResizeMode(QListView::Adjust);
        m_ordersFileList->setIconSize(QSize(16, 16));
        m_ordersFileList->setUniformItemSizes(true);
        m_ordersFileList->setWordWrap(false);
        m_ordersFileList->setAlternatingRowColors(true);
        
        ordersLayout->addWidget(m_ordersFileList);
        
        // Dodaj obie grupy do lewego panelu (pionowo)
        leftLayout->addWidget(m_confirmationsGroup, 1); // Proporcja 1:1
        leftLayout->addWidget(m_ordersGroup, 1);
        
        // Przyciski akcji na dole lewego panelu
        auto *actionLayout = new QHBoxLayout;
        
        m_openButton = new QPushButton("Otwórz");
        m_printButton = new QPushButton("Drukuj");
        
        m_openButton->setEnabled(false);
        m_printButton->setEnabled(false);
        
        actionLayout->addStretch();
        actionLayout->addWidget(m_openButton);
        actionLayout->addWidget(m_printButton);
        
        leftLayout->addLayout(actionLayout);
        
        // Dodaj lewy panel do głównego splittera
        m_mainSplitter->addWidget(leftPanel);
        
        // Główny panel podglądu PDF
        m_pdfPreviewWidget = new QStackedWidget;
        m_pdfPreviewWidget->setStyleSheet("QStackedWidget { background-color: white; border: 1px solid #ccc; }");
        
        // Utwórz scroll area dla podglądu PDF z lepszymi ustawieniami
        m_previewScrollArea = new QScrollArea;
        m_previewScrollArea->setWidgetResizable(true);
        m_previewScrollArea->setAlignment(Qt::AlignCenter);
        m_previewScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_previewScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_previewScrollArea->setStyleSheet(
            "QScrollArea { "
            "background-color: #f8f9fa; "
            "border: 1px solid #dee2e6; "
            "border-radius: 8px; "
            "}"
            "QScrollBar:vertical { "
            "border: none; "
            "background: #f1f1f1; "
            "width: 12px; "
            "border-radius: 6px; "
            "}"
            "QScrollBar::handle:vertical { "
            "background: #c1c1c1; "
            "border-radius: 6px; "
            "min-height: 20px; "
            "}"
            "QScrollBar::handle:vertical:hover { "
            "background: #a8a8a8; "
            "}"
            "QScrollBar:horizontal { "
            "border: none; "
            "background: #f1f1f1; "
            "height: 12px; "
            "border-radius: 6px; "
            "}"
            "QScrollBar::handle:horizontal { "
            "background: #c1c1c1; "
            "border-radius: 6px; "
            "min-width: 20px; "
            "}"
            "QScrollBar::handle:horizontal:hover { "
            "background: #a8a8a8; "
            "}"
        );
        
        // Widget z informacją o braku zaznaczenia
        m_previewLabel = new QLabel("Wybierz plik PDF z lewego panelu, aby zobaczyć podgląd");
        m_previewLabel->setAlignment(Qt::AlignCenter);
        m_previewLabel->setWordWrap(true);
        m_previewLabel->setMinimumSize(400, 300);
        m_previewLabel->setStyleSheet(
            "QLabel { "
            "background-color: #f8f9fa; "
            "color: #6c757d; "
            "font-size: 14px; "
            "padding: 20px; "
            "border: 2px dashed #dee2e6; "
            "border-radius: 8px; "
            "}"
        );
        
        // Dodaj label do scroll area
        m_previewScrollArea->setWidget(m_previewLabel);
        
        // Dodaj scroll area do stacked widget
        m_pdfPreviewWidget->addWidget(m_previewScrollArea);
        
        // Dodaj widget podglądu do głównego splittera
        m_mainSplitter->addWidget(m_pdfPreviewWidget);
        
        // Ustaw proporcje głównego splittera (lewy panel 30%, podgląd 70%)
        m_mainSplitter->setSizes(QList<int>() << 300 << 700);
    } else {
        // Dla trybu materiałów (na przyszłość - będzie to formularz zamówień)
        QLabel *placeholderLabel = new QLabel("Tryb materiałów będzie dostępny wkrótce");
        placeholderLabel->setAlignment(Qt::AlignCenter);
        m_mainSplitter->addWidget(placeholderLabel);
    }
    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(toolbarLayout);
    mainLayout->addWidget(m_mainSplitter, 1);
    
    // Inicjalizacja modeli plików - tylko dla trybu Orders
    if (m_viewMode == OrdersMode) {
        // Model dla potwierdzeń
        m_confirmationsFileModel = new QFileSystemModel(this);
        m_confirmationsFileModel->setFilter(QDir::Files | QDir::NoDotAndDotDot);
        m_confirmationsFileModel->setNameFilters(QStringList() << "*.pdf");
        m_confirmationsFileModel->setNameFilterDisables(false);
        
        // Model dla zleceń produkcyjnych  
        m_ordersFileModel = new QFileSystemModel(this);
        m_ordersFileModel->setFilter(QDir::Files | QDir::NoDotAndDotDot);
        m_ordersFileModel->setNameFilters(QStringList() << "*.pdf");
        m_ordersFileModel->setNameFilterDisables(false);
        
        // Przypisz modele do list
        m_confirmationsFileList->setModel(m_confirmationsFileModel);
        m_ordersFileList->setModel(m_ordersFileModel);
    }
}

void PdfViewer::setupConnections() {
    // Połączenia dostępne zawsze
    if (m_refreshButton) {
        connect(m_refreshButton, &QPushButton::clicked, this, &PdfViewer::refreshFileList);
    }
    if (m_searchBox) {
        connect(m_searchBox, &QLineEdit::textChanged, this, &PdfViewer::onSearchTextChanged);
    }
    if (m_sortOrderCombo) {
        connect(m_sortOrderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PdfViewer::onSortOrderChanged);
    }
    
    // Połączenia tylko dla trybu zamówień
    if (m_viewMode == OrdersMode) {
        if (m_openButton) {
            connect(m_openButton, &QPushButton::clicked, this, &PdfViewer::openSelectedFile);
        }
        if (m_printButton) {
            connect(m_printButton, &QPushButton::clicked, this, &PdfViewer::printSelectedFile);
        }
        
        if (m_ordersFileList) {
            connect(m_ordersFileList, &QListView::clicked, this, &PdfViewer::onOrdersFileSelected);
            connect(m_ordersFileList, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
                onOrdersFileSelected(index);
                openSelectedFile();
            });
        }
        
        if (m_confirmationsFileList) {
            connect(m_confirmationsFileList, &QListView::clicked, this, &PdfViewer::onConfirmationsFileSelected);
            connect(m_confirmationsFileList, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
                onConfirmationsFileSelected(index);
                openSelectedFile();
            });
        }
    }
}

void PdfViewer::setupDirectories() {
    if (m_viewMode != OrdersMode) return;
    
    try {
        // Domyślna ścieżka do folderu dokumentów
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        
        // Odczytaj ścieżki z ustawień aplikacji
        QSettings settings;
        
        // Ścieżki dla potwierdzeń i zleceń produkcyjnych
        m_confirmationsDir = settings.value("pdf/confirmationDir", defaultPath + "/Potwierdzenia").toString();
        m_ordersDir = settings.value("pdf/productionDir", defaultPath + "/Zlecenia").toString();
        
        // Sprawdź czy katalogi istnieją, jeśli nie - utwórz
        QDir confirmationsDir(m_confirmationsDir);
        if (!confirmationsDir.exists()) {
            if (!confirmationsDir.mkpath(".")) {
                QMessageBox::warning(this, "Błąd", 
                    QString("Nie można utworzyć katalogu potwierdzeń %1.\nUżywam katalogu domyślnego.").arg(m_confirmationsDir));
                m_confirmationsDir = defaultPath + "/Potwierdzenia";
                confirmationsDir = QDir(m_confirmationsDir);
                confirmationsDir.mkpath(".");
            }
        }
        
        QDir ordersDir(m_ordersDir);
        if (!ordersDir.exists()) {
            if (!ordersDir.mkpath(".")) {
                QMessageBox::warning(this, "Błąd", 
                    QString("Nie można utworzyć katalogu zleceń %1.\nUżywam katalogu domyślnego.").arg(m_ordersDir));
                m_ordersDir = defaultPath + "/Zlecenia";
                ordersDir = QDir(m_ordersDir);
                ordersDir.mkpath(".");
            }
        }
        
        // Ustaw ścieżki root dla modeli plików
        if (m_confirmationsFileModel) {
            QModelIndex confirmationsRootIndex = m_confirmationsFileModel->setRootPath(m_confirmationsDir);
            if (m_confirmationsFileList) {
                m_confirmationsFileList->setRootIndex(confirmationsRootIndex);
            }
        }
        
        if (m_ordersFileModel) {
            QModelIndex ordersRootIndex = m_ordersFileModel->setRootPath(m_ordersDir);
            if (m_ordersFileList) {
                m_ordersFileList->setRootIndex(ordersRootIndex);
            }
        }
        
    } catch (const std::exception& e) {
        qDebug() << "Błąd przy konfiguracji katalogów PDF:" << e.what();
    } catch (...) {
        qDebug() << "Nieznany błąd przy konfiguracji katalogów PDF";
    }
}

void PdfViewer::refreshFileList() {
    if (m_viewMode != OrdersMode) return;
    
    // Wyczyść zaznaczenie PRZED odświeżeniem modeli
    m_selectedFilePath = QString();
    m_selectedConfirmationFilePath = QString();
    
    if (m_openButton) m_openButton->setEnabled(false);
    if (m_printButton) m_printButton->setEnabled(false);
    
    // Bezpiecznie wyczyść podgląd - tylko tekst, nie usuwaj widget'ów
    if (m_previewLabel && m_previewScrollArea) {
        m_previewLabel->setText("Wybierz plik PDF z lewego panelu, aby zobaczyć podgląd");
        m_previewLabel->setStyleSheet(
            "QLabel { "
            "background-color: #f8f9fa; "
            "color: #6c757d; "
            "font-size: 14px; "
            "padding: 20px; "
            "border: 2px dashed #dee2e6; "
            "border-radius: 8px; "
            "}"
        );
        m_previewScrollArea->setWidget(m_previewLabel);
    }
    
    // Odświeżenie modeli plików PO wyczyszczeniu podglądu
    if (m_confirmationsFileModel) {
        m_confirmationsFileModel->setRootPath(m_confirmationsFileModel->rootPath());
    }
    
    if (m_ordersFileModel) {
        m_ordersFileModel->setRootPath(m_ordersFileModel->rootPath());
    }
}

void PdfViewer::onOrdersFileSelected(const QModelIndex &index) {
    if (!index.isValid()) return;
    
    QString path = m_ordersFileModel->filePath(index);
    QFileInfo fileInfo(path);
    
    if (fileInfo.isFile() && fileInfo.suffix().toLower() == "pdf") {
        m_selectedFilePath = path;
        m_selectedConfirmationFilePath = QString(); // Czyścimy zaznaczenie potwierdzeń
        m_openButton->setEnabled(true);
        m_printButton->setEnabled(true);
        
        // Aktualizujemy podgląd PDF
        updatePdfPreview(path);
    } else {
        m_selectedFilePath = QString();
        m_openButton->setEnabled(false);
        m_printButton->setEnabled(false);
    }
}

void PdfViewer::onConfirmationsFileSelected(const QModelIndex &index) {
    if (!index.isValid()) return;
    
    QString path = m_confirmationsFileModel->filePath(index);
    QFileInfo fileInfo(path);
    
    if (fileInfo.isFile() && fileInfo.suffix().toLower() == "pdf") {
        m_selectedConfirmationFilePath = path;
        m_selectedFilePath = QString(); // Czyścimy zaznaczenie zleceń
        m_openButton->setEnabled(true);
        m_printButton->setEnabled(true);
        
        // Aktualizujemy podgląd PDF
        updatePdfPreview(path);
    } else {
        m_selectedConfirmationFilePath = QString();
        m_openButton->setEnabled(false);
        m_printButton->setEnabled(false);
    }
}

// Funkcja do aktualizacji podglądu PDF
void PdfViewer::updatePdfPreview(const QString &filePath) {
    if (m_viewMode != OrdersMode || !m_previewLabel || !m_previewScrollArea) {
        return; // Zabezpieczenie przed błędem, gdy nie jesteśmy w trybie zamówień
    }
    
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        m_previewLabel->setText("Wybierz plik PDF z lewego panelu, aby zobaczyć podgląd");
        m_previewLabel->setStyleSheet(
            "QLabel { "
            "background-color: #f8f9fa; "
            "color: #6c757d; "
            "font-size: 14px; "
            "padding: 20px; "
            "border: 2px dashed #dee2e6; "
            "border-radius: 8px; "
            "}"
        );
        m_previewScrollArea->setWidget(m_previewLabel);
        return;
    }

    QFileInfo fileInfo(filePath);
    
    // Spróbuj użyć wbudowanej przeglądarki
    if (tryEmbeddedPdfViewer(filePath)) {
        return; // Sukces z wbudowaną przeglądarką
    }
    
    // Fallback - pokaż szczegółowe informacje o pliku z opcją otwarcia
    QString fileInfoText = QString(
        "<div style='text-align: center; padding: 20px;'>"
        "<h2 style='color: #2c3e50; margin-bottom: 20px;'>📄 %1</h2>"
        "<div style='background: #ecf0f1; padding: 15px; border-radius: 8px; margin: 10px;'>"
        "<p style='margin: 5px 0;'><strong>Rozmiar:</strong> %2 KB</p>"
        "<p style='margin: 5px 0;'><strong>Data modyfikacji:</strong> %3</p>"
        "<p style='margin: 5px 0;'><strong>Ścieżka:</strong> %4</p>"
        "</div>"
        "<div style='margin-top: 20px;'>"
        "<p style='color: #7f8c8d; font-style: italic;'>Kliknij dwukrotnie na pliku lub użyj przycisku \"Otwórz\"<br>"
        "aby otworzyć plik w domyślnej przeglądarce PDF.</p>"
        "</div>"
        "</div>"
    ).arg(fileInfo.fileName())
     .arg(fileInfo.size() / 1024)
     .arg(fileInfo.lastModified().toString("dd.MM.yyyy hh:mm"))
     .arg(fileInfo.absoluteFilePath());
    
    m_previewLabel->setText(fileInfoText);
    m_previewLabel->setStyleSheet(
        "QLabel { "
        "background-color: white; "
        "color: #2c3e50; "
        "font-size: 12px; "
        "padding: 10px; "
        "border: 1px solid #bdc3c7; "
        "border-radius: 8px; "
        "}"
    );
    m_previewScrollArea->setWidget(m_previewLabel);
}

// Nowa funkcja do próby wbudowanego podglądu PDF
bool PdfViewer::tryEmbeddedPdfViewer(const QString &filePath) {
    qDebug() << "Próbuję wbudowany podgląd PDF dla:" << filePath;
    
    // Próba 1: Adobe Reader przez ActiveX (najlepsze rozwiązanie na Windows)
    #ifdef ACTIVEX_AVAILABLE
    if (tryAdobeReaderViewer(filePath)) {
        return true;
    }
    #endif
    
    // Próba 2: Użyj QWebEngineView jeśli dostępne
    #ifdef QT_WEBENGINEWIDGETS_LIB
    try {
        if (!m_webEngineView) {
            qDebug() << "Tworzę nowy QWebEngineView";
            m_webEngineView = new QWebEngineView(this);
            m_webEngineView->setStyleSheet("QWebEngineView { background-color: white; }");
            
            // Dodaj obsługę błędów ładowania
            connect(m_webEngineView, &QWebEngineView::loadFinished, [this](bool success) {
                if (!success) {
                    qDebug() << "Błąd ładowania PDF w QWebEngineView";
                    // Wróć do fallback
                    m_previewLabel->setText("Nie można załadować podglądu PDF.<br>Kliknij dwukrotnie, aby otworzyć w zewnętrznej aplikacji.");
                    m_pdfPreviewWidget->setCurrentWidget(m_previewScrollArea);
                }
            });
            
            // Dodaj widget tylko jeśli jeszcze go nie ma
            if (m_pdfPreviewWidget->indexOf(m_webEngineView) == -1) {
                m_pdfPreviewWidget->addWidget(m_webEngineView);
            }
        }
        
        // Załaduj PDF używając file:// URL
        QUrl fileUrl = QUrl::fromLocalFile(QFileInfo(filePath).absoluteFilePath());
        qDebug() << "Ładuję URL:" << fileUrl.toString();
        
        m_webEngineView->load(fileUrl);
        m_pdfPreviewWidget->setCurrentWidget(m_webEngineView);
        
        return true;
    } catch (const std::exception& e) {
        qDebug() << "Wyjątek w QWebEngineView:" << e.what();
    } catch (...) {
        qDebug() << "Nieznany błąd w QWebEngineView";
    }
    #else
    qDebug() << "QWebEngineView nie jest dostępne";
    #endif
    
    // Próba 3: Konwersja PDF do obrazu za pomocą zewnętrznego narzędzia
    return tryPdfToImageConversion(filePath);
}

// Funkcja do konwersji PDF na obraz
bool PdfViewer::tryPdfToImageConversion(const QString &filePath) {
    qDebug() << "Próbuję konwersję PDF na obraz dla:" << filePath;
    
    // Sprawdź czy mamy dostęp do narzędzi konwersji PDF
    QStringList possibleCommands = {
        "magick", // ImageMagick
        "convert", // ImageMagick starsze wersje
        "pdftoppm", // Poppler utils
        "gs" // Ghostscript
    };
    
    // Dodaj pełne ścieżki dla ImageMagick na Windows
    QStringList fullPathCommands = {
        "C:/Program Files/ImageMagick-7.1.1-Q16-HDRI/magick.exe",
        "C:/Program Files (x86)/ImageMagick-7.1.1-Q16-HDRI/magick.exe",
        "C:/Program Files/ImageMagick-7.1.0-Q16-HDRI/magick.exe",
        "C:/Program Files (x86)/ImageMagick-7.1.0-Q16-HDRI/magick.exe"
    };
    
    QString workingCommand;
    
    // Najpierw sprawdź pełne ścieżki
    for (const QString &cmd : fullPathCommands) {
        if (QFile::exists(cmd)) {
            QProcess testProcess;
            testProcess.start(cmd, QStringList() << "--version");
            if (testProcess.waitForFinished(3000) && testProcess.exitCode() == 0) {
                workingCommand = cmd;
                qDebug() << "Znaleziono ImageMagick:" << cmd;
                break;
            }
        }
    }
    
    // Jeśli nie znaleziono w pełnych ścieżkach, sprawdź PATH
    if (workingCommand.isEmpty()) {
        for (const QString &cmd : possibleCommands) {
            QProcess testProcess;
            testProcess.start(cmd, QStringList() << "--version");
            if (testProcess.waitForFinished(3000) && testProcess.exitCode() == 0) {
                workingCommand = cmd;
                qDebug() << "Znaleziono narzędzie:" << cmd;
                break;
            }
        }
    }
    
    if (workingCommand.isEmpty()) {
        qDebug() << "Brak dostępnych narzędzi do konwersji PDF";
        
        // Spróbuj alternatywnej metody - użyj SumatraPDF lub Adobe Reader w trybie embedded
        if (tryAlternativePdfViewer(filePath)) {
            return true;
        }
        
        // Pokaż informację o braku narzędzi z lepszymi instrukcjami
        QFileInfo fileInfo(filePath);
        m_previewLabel->setText(QString(
            "<div style='text-align: center; padding: 20px; font-family: Segoe UI, Arial, sans-serif;'>"
            "<div style='background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; border-radius: 8px; margin-bottom: 20px;'>"
            "<h2 style='margin: 0; font-size: 18px;'>📄 %1</h2>"
            "<p style='margin: 5px 0 0 0; opacity: 0.9;'>%2 KB • %3</p>"
            "</div>"
            
            "<div style='background: #f8f9fc; padding: 20px; border-radius: 8px; margin: 15px 0; border-left: 4px solid #4CAF50;'>"
            "<h3 style='color: #2c3e50; margin: 0 0 15px 0; font-size: 16px;'>💡 Podgląd PDF zostanie wkrótce dostępny</h3>"
            "<p style='color: #5a6c7d; margin: 0; line-height: 1.6;'>"
            "Pracujemy nad integracją podglądu PDF bezpośrednio w aplikacji.<br>"
            "Tymczasowo kliknij dwukrotnie na pliku lub użyj przycisku <strong>\"Otwórz\"</strong>."
            "</p>"
            "</div>"
            
            "<div style='background: #fff3cd; padding: 15px; border-radius: 8px; border: 1px solid #ffeaa7;'>"
            "<h4 style='color: #856404; margin: 0 0 10px 0; font-size: 14px;'>🔧 Dla zaawansowanych użytkowników:</h4>"
            "<p style='color: #856404; margin: 0; font-size: 12px; line-height: 1.5;'>"
            "Aby włączyć podgląd, zainstaluj ImageMagick, Poppler utils lub Ghostscript"
            "</p>"
            "</div>"
            
            "<div style='margin-top: 20px;'>"
            "<button style='background: #007bff; color: white; border: none; padding: 10px 20px; border-radius: 6px; cursor: pointer; font-size: 14px;'>"
            "🔗 Kliknij dwukrotnie na pliku, aby otworzyć"
            "</button>"
            "</div>"
            "</div>"
        ).arg(fileInfo.fileName())
         .arg(fileInfo.size() / 1024)
         .arg(fileInfo.lastModified().toString("dd.MM.yyyy hh:mm")));
        
        m_previewLabel->setStyleSheet(
            "QLabel { "
            "background-color: white; "
            "color: #2c3e50; "
            "font-size: 13px; "
            "padding: 10px; "
            "border: 1px solid #e9ecef; "
            "border-radius: 12px; "
            "}"
        );
        m_previewScrollArea->setWidget(m_previewLabel);
        return false;
    }
    
    // Utwórz tymczasowy plik obrazu
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempImagePath = tempDir + "/pdf_preview_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".png";
    
    qDebug() << "Konwertuję do:" << tempImagePath;
    
    // Konwertuj pierwszą stronę PDF na obraz
    QProcess convertProcess;
    QStringList arguments;
    
    QString commandName = QFileInfo(workingCommand).baseName().toLower();
    
    if (commandName == "magick" || commandName == "convert") {
        // ImageMagick z optymalną rozdzielczością - ograniczenie rozmiaru dla Qt
        arguments << "-density" << "300" << filePath + "[0]" 
                  << "-background" << "white" << "-alpha" << "remove" 
                  << "-quality" << "85" << "-colorspace" << "sRGB"
                  << "-resize" << "1800x1800>" << "-limit" << "memory" << "64MB"
                  << tempImagePath;
    } else if (commandName == "pdftoppm") {
        // Poppler z optymalną rozdzielczością
        arguments << "-png" << "-f" << "1" << "-l" << "1" << "-r" << "300" 
                  << "-aa" << "yes" << "-aaVector" << "yes" 
                  << filePath << tempImagePath.left(tempImagePath.lastIndexOf("."));
        tempImagePath = tempImagePath.left(tempImagePath.lastIndexOf(".")) + "-1.png";
    } else if (commandName == "gs") {
        // Ghostscript z optymalną rozdzielczością
        arguments << "-dNOPAUSE" << "-dBATCH" << "-sDEVICE=png16m" << "-r300" 
                  << "-dTextAlphaBits=4" << "-dGraphicsAlphaBits=4" 
                  << "-dFirstPage=1" << "-dLastPage=1" 
                  << QString("-sOutputFile=%1").arg(tempImagePath) << filePath;
    }
    
    qDebug() << "Wywołuję:" << workingCommand << arguments;
    
    convertProcess.start(workingCommand, arguments);
    if (!convertProcess.waitForFinished(30000)) { // Zwiększony timeout do 30 sekund
        qDebug() << "Timeout podczas konwersji PDF";
        return false;
    }
    
    if (convertProcess.exitCode() != 0) {
        QString errorOutput = convertProcess.readAllStandardError();
        qDebug() << "Błąd konwersji PDF:" << errorOutput;
        
        // Pokaż błąd w UI
        m_previewLabel->setText(QString(
            "<div style='text-align: center; padding: 20px;'>"
            "<h3>❌ Błąd konwersji PDF</h3>"
            "<p>Nie można przekonwertować PDF na obraz.</p>"
            "<p><b>Błąd:</b> %1</p>"
            "<p><i>Użyj przycisku \"Otwórz\" aby otworzyć PDF w zewnętrznej aplikacji.</i></p>"
            "</div>"
        ).arg(errorOutput.simplified()));
        
        m_previewLabel->setStyleSheet(
            "QLabel { "
            "background-color: #f8d7da; "
            "color: #721c24; "
            "font-size: 12px; "
            "padding: 20px; "
            "border: 1px solid #f5c6cb; "
            "border-radius: 8px; "
            "}"
        );
        m_previewScrollArea->setWidget(m_previewLabel);
        return false;
    }
    
    // Załaduj obraz i wyświetl
    QPixmap pixmap(tempImagePath);
    if (pixmap.isNull()) {
        qDebug() << "Nie można załadować skonwertowanego obrazu:" << tempImagePath;
        return false;
    }
    
    qDebug() << "Sukces! Załadowano obraz rozmiaru:" << pixmap.size();
    
    // Utwórz scroll area z właściwymi ustawieniami jeśli jeszcze nie istnieje
    if (!m_previewScrollArea) {
        qDebug() << "Błąd: brak scroll area";
        return false;
    }
    
    // Utwórz nowy widget kontener dla obrazu
    QWidget *containerWidget = new QWidget();
    containerWidget->setStyleSheet("QWidget { background-color: #f8f9fa; }");
    
    QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(10, 10, 10, 10);
    containerLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    
    // Utwórz label dla obrazu
    QLabel *imageLabel = new QLabel();
    
    // Oblicz dostępną szerokość scroll area viewport minus marginesy
    QSize viewportSize = m_previewScrollArea->viewport()->size();
    int availableWidth = viewportSize.width() - 40; // Marginesy po 20px z każdej strony
    int availableHeight = viewportSize.height() - 40;
    
    qDebug() << "Rozmiar viewport:" << viewportSize << "Dostępna szerokość:" << availableWidth;
    qDebug() << "Oryginalny rozmiar obrazu:" << pixmap.size();
    
    // Oblicz skalowanie - użyj pełnej dostępnej szerokości, ale zachowaj jakość
    QPixmap displayPixmap;
    
    // Maksymalna szerokość dla podglądu PDF
    const int maxWidth = 1200;
    
    // Ogranicz dostępną szerokość do maksymalnej wartości
    int effectiveWidth = qMin(availableWidth, maxWidth);
    
    // Jeśli obraz jest szerszy niż efektywna szerokość, skaluj do szerokości
    if (pixmap.width() > effectiveWidth && effectiveWidth > 300) {
        displayPixmap = pixmap.scaledToWidth(effectiveWidth, Qt::SmoothTransformation);
        qDebug() << "Skalowano do szerokości:" << effectiveWidth << "Nowy rozmiar:" << displayPixmap.size();
    }
    // Jeśli obraz jest wyższy niż dostępna wysokość, skaluj do wysokości
    else if (pixmap.height() > availableHeight && availableHeight > 200) {
        displayPixmap = pixmap.scaledToHeight(availableHeight, Qt::SmoothTransformation);
        qDebug() << "Skalowano do wysokości:" << availableHeight << "Nowy rozmiar:" << displayPixmap.size();
    }
    // W przeciwnym razie użyj oryginalnego rozmiaru lub powiększ jeśli jest za mały
    else {
        // Jeśli obraz jest bardzo mały, powiększ go do przynajmniej 60% efektywnej szerokości
        int minWidth = static_cast<int>(effectiveWidth * 0.60);
        if (pixmap.width() < minWidth && minWidth > 0) {
            displayPixmap = pixmap.scaledToWidth(minWidth, Qt::SmoothTransformation);
            qDebug() << "Powiększono do min. szerokości:" << minWidth << "Nowy rozmiar:" << displayPixmap.size();
        } else {
            displayPixmap = pixmap;
            qDebug() << "Użyto oryginalnego rozmiaru:" << pixmap.size();
        }
    }
    
    // Dodatkowe sprawdzenie - jeśli mimo wszystko obraz jest szerszy niż 1200px, skaluj go w dół
    if (displayPixmap.width() > maxWidth) {
        displayPixmap = displayPixmap.scaledToWidth(maxWidth, Qt::SmoothTransformation);
        qDebug() << "Przeskalowano do maksymalnej szerokości:" << maxWidth << "Końcowy rozmiar:" << displayPixmap.size();
    }
    
    // Ustaw obraz w label z wysokiej jakości wyświetlaniem
    imageLabel->setPixmap(displayPixmap);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet(
        "QLabel { "
        "background-color: white; "
        "border: 3px solid #e0e0e0; "
        "border-radius: 12px; "
        "padding: 15px; "
        "box-shadow: 0 4px 20px rgba(0,0,0,0.1); "
        "}"
    );
    
    // Ustaw rozmiar label dokładnie na rozmiar obrazu plus padding
    imageLabel->setFixedSize(displayPixmap.size() + QSize(30, 30));
    imageLabel->setScaledContents(false); // Ważne: nie skaluj zawartości automatycznie
    
    // Dodaj label do kontenera
    containerLayout->addWidget(imageLabel, 0, Qt::AlignCenter);
    containerLayout->addStretch(); // Dodaj stretch na dole
    
    // Ustaw odpowiedni rozmiar kontenera
    int containerWidth = qMax(displayPixmap.width() + 60, viewportSize.width());
    int containerHeight = displayPixmap.height() + 100;
    containerWidget->setMinimumSize(containerWidth, containerHeight);
    
    // Skonfiguruj scroll area dla optymalnego wyświetlania
    m_previewScrollArea->setWidgetResizable(false); // Nie zmieniaj rozmiaru automatycznie
    m_previewScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_previewScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_previewScrollArea->setAlignment(Qt::AlignCenter);
    
    // Zastąp widget w scroll area - bezpiecznie usuń poprzedni widget
    QWidget *oldWidget = m_previewScrollArea->takeWidget();
    if (oldWidget && oldWidget != m_previewLabel) {
        oldWidget->deleteLater(); // Bezpieczne usunięcie poprzedniego widget'u
    }
    m_previewScrollArea->setWidget(containerWidget);
    
    qDebug() << "Obraz wyświetlony pomyślnie.";
    qDebug() << "Rozmiar wyświetlanego obrazu:" << displayPixmap.size();
    qDebug() << "Rozmiar kontenera:" << containerWidget->minimumSize();
    
    // Usuń tymczasowy plik po 5 sekundach
    QTimer::singleShot(5000, [tempImagePath]() {
        if (QFile::exists(tempImagePath)) {
            QFile::remove(tempImagePath);
            qDebug() << "Usunięto tymczasowy plik:" << tempImagePath;
        }
    });
    
    return true;
}

void PdfViewer::openSelectedFile() {
    // Sprawdzamy który plik jest wybrany (zlecenie lub potwierdzenie)
    QString fileToOpen;
    
    if (!m_selectedFilePath.isEmpty()) {
        fileToOpen = m_selectedFilePath;
    } else if (!m_selectedConfirmationFilePath.isEmpty()) {
        fileToOpen = m_selectedConfirmationFilePath;
    } else {
        QMessageBox::warning(this, "Ostrzeżenie", "Nie wybrano pliku PDF.");
        return;
    }
    
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileToOpen));
}

void PdfViewer::printSelectedFile() {
    // Sprawdzamy który plik jest wybrany (zlecenie lub potwierdzenie)
    QString fileToPrint;
    
    if (!m_selectedFilePath.isEmpty()) {
        fileToPrint = m_selectedFilePath;
    } else if (!m_selectedConfirmationFilePath.isEmpty()) {
        fileToPrint = m_selectedConfirmationFilePath;
    } else {
        QMessageBox::warning(this, "Ostrzeżenie", "Nie wybrano pliku PDF.");
        return;
    }
    
#ifdef Q_OS_WIN
    // Na Windows użyj wbudowanego drukowania PDF
    QProcess process;
    QString program = "powershell.exe";
    QStringList arguments;
    arguments << "-Command" << QString("Start-Process -FilePath \"%1\" -Verb Print").arg(fileToPrint);
    
    process.start(program, arguments);
    if (!process.waitForStarted()) {
        QMessageBox::warning(this, "Błąd drukowania", "Nie można uruchomić procesu drukowania.");
    }
#else
    // Na innych platformach użyj standardowego dialogu drukowania
    QPrinter printer;
    QPrintDialog printDialog(&printer, this);
    
    if (printDialog.exec() == QDialog::Accepted) {
        // Ponieważ drukowanie PDF bezpośrednio wymaga QtPdf, po prostu otwieramy plik
        // w domyślnej aplikacji do przeglądania PDF, która zajmie się drukowaniem
        QMessageBox::information(this, "Drukowanie", 
            "Plik PDF zostanie otwarty w domyślnej przeglądarce PDF.\n"
            "Użyj opcji drukowania w tej aplikacji, aby wydrukować dokument.");
        
        openSelectedFile();
    }
#endif
}

void PdfViewer::onSearchTextChanged(const QString &text) {
    updateFileFilter();
}

void PdfViewer::onSortOrderChanged(int index) {
    if (m_viewMode != OrdersMode) return;
    
    // Określamy kolumnę i kierunek sortowania
    int column = 0; // Domyślnie kolumna nazwy
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    
    switch (index) {
        case 0: // Nazwa (A-Z)
            column = 0; // Kolumna nazwy
            sortOrder = Qt::AscendingOrder;
            break;
        case 1: // Data (od najnowszych)
            column = 3; // Kolumna daty modyfikacji
            sortOrder = Qt::DescendingOrder;
            break;
        case 2: // Data (od najstarszych)
            column = 3; // Kolumna daty modyfikacji
            sortOrder = Qt::AscendingOrder;
            break;
        default:
            column = 0; 
            sortOrder = Qt::AscendingOrder;
    }
    
    // Sortuj oba modele
    if (m_confirmationsFileModel) {
        m_confirmationsFileModel->sort(column, sortOrder);
    }
    if (m_ordersFileModel) {
        m_ordersFileModel->sort(column, sortOrder);
    }
}

void PdfViewer::updateFileFilter() {
    if (m_viewMode != OrdersMode) return;
    
    QString filterText = m_searchBox->text();
    QStringList filters;
    
    if (filterText.isEmpty()) {
        filters << "*.pdf";
    } else {
        filters << QString("*%1*.pdf").arg(filterText);
    }
    
    // Zastosuj filtry do obu modeli
    if (m_confirmationsFileModel) {
        m_confirmationsFileModel->setNameFilters(filters);
    }
    if (m_ordersFileModel) {
        m_ordersFileModel->setNameFilters(filters);
    }
}

#ifdef ACTIVEX_AVAILABLE
// Funkcja do próby użycia Adobe Reader przez ActiveX
bool PdfViewer::tryAdobeReaderViewer(const QString &filePath) {
    qDebug() << "Próbuję Adobe Reader ActiveX dla:" << filePath;
    
    try {
        // Sprawdź czy Adobe Reader jest zainstalowany
        QAxWidget *testWidget = new QAxWidget(this);
        
        // Sprawdź dostępne ActiveX controls związane z Adobe
        QStringList possibleControls = {
            "AcroPDF.PDF.1",           // Adobe Acrobat/Reader ActiveX Control
            "PDF.PdfCtrl.6",           // Adobe PDF Reader Control
            "AcroPDF.PDF",             // Starsza wersja
            "{CA8A9780-280D-11CF-A24D-444553540000}" // CLSID dla Adobe PDF Reader
        };
        
        QString workingControl;
        for (const QString &control : possibleControls) {
            if (testWidget->setControl(control)) {
                workingControl = control;
                qDebug() << "Znaleziono Adobe Reader control:" << control;
                break;
            }
        }
        
        delete testWidget; // Usuń widget testowy
        
        if (workingControl.isEmpty()) {
            qDebug() << "Adobe Reader ActiveX nie jest dostępny";
            return false;
        }
        
        // Utwórz widget Adobe Reader jeśli jeszcze nie istnieje
        if (!m_adobeReaderWidget) {
            qDebug() << "Tworzę nowy Adobe Reader widget";
            m_adobeReaderWidget = new QAxWidget(this);
            
            if (!m_adobeReaderWidget->setControl(workingControl)) {
                qDebug() << "Nie można utworzyć Adobe Reader widget";
                delete m_adobeReaderWidget;
                m_adobeReaderWidget = nullptr;
                return false;
            }
            
            m_adobeReaderWidget->setStyleSheet("QAxWidget { background-color: white; border: 1px solid #ddd; }");
            
            // Dodaj widget do stacked widget jeśli jeszcze go nie ma
            if (m_pdfPreviewWidget->indexOf(m_adobeReaderWidget) == -1) {
                m_pdfPreviewWidget->addWidget(m_adobeReaderWidget);
            }
        }
        
        // Załaduj PDF do Adobe Reader
        QString absolutePath = QFileInfo(filePath).absoluteFilePath();
        qDebug() << "Ładuję PDF do Adobe Reader:" << absolutePath;
        
        // Różne metody ładowania w zależności od kontrolki
        bool loadSuccess = false;
        
        // Metoda 1: LoadFile dla AcroPDF.PDF.1
        if (workingControl.contains("AcroPDF")) {
            QVariant result = m_adobeReaderWidget->dynamicCall("LoadFile(const QString&)", absolutePath);
            loadSuccess = result.toBool();
            qDebug() << "LoadFile result:" << loadSuccess;
            
            if (loadSuccess) {
                // Ustaw dodatkowe właściwości
                m_adobeReaderWidget->dynamicCall("setShowToolbar(bool)", false);
                m_adobeReaderWidget->dynamicCall("setShowScrollbars(bool)", true);
                m_adobeReaderWidget->dynamicCall("setView(const QString&)", "FitH");
            }
        }
        
        // Metoda 2: src property dla innych kontrolek
        if (!loadSuccess) {
            m_adobeReaderWidget->setProperty("src", absolutePath);
            loadSuccess = true; // Zakładamy sukces, jeśli nie było błędu
        }
        
        if (loadSuccess) {
            // Przełącz na widget Adobe Reader
            m_pdfPreviewWidget->setCurrentWidget(m_adobeReaderWidget);
            qDebug() << "Adobe Reader - sukces!";
            return true;
        } else {
            qDebug() << "Nie można załadować PDF do Adobe Reader";
            return false;
        }
        
    } catch (const std::exception& e) {
        qDebug() << "Wyjątek w Adobe Reader:" << e.what();
        return false;
    } catch (...) {
        qDebug() << "Nieznany błąd w Adobe Reader";
        return false;
    }
}
#endif

// Funkcja do próby alternatywnych metod wyświetlania PDF
bool PdfViewer::tryAlternativePdfViewer(const QString &filePath) {
    qDebug() << "Próbuję alternatywne metody wyświetlania PDF dla:" << filePath;
    
    // Metoda 1: Sprawdź czy SumatraPDF jest dostępny
    if (trySumatraPdfEmbedded(filePath)) {
        return true;
    }
    
    // Metoda 2: Użyj Windows Shell dla podglądu
    #ifdef Q_OS_WIN
    if (tryWindowsShellPreview(filePath)) {
        return true;
    }
    #endif
    
    // Metoda 3: Sprawdź czy można użyć QTextBrowser z prostym parsowaniem
    if (trySimplePdfInfo(filePath)) {
        return true;
    }
    
    return false;
}

// Funkcja do próby użycia SumatraPDF w trybie embedded
bool PdfViewer::trySumatraPdfEmbedded(const QString &filePath) {
    qDebug() << "Sprawdzam dostępność SumatraPDF";
    
    // Sprawdź czy SumatraPDF jest zainstalowany
    QStringList possiblePaths = {
        "C:/Program Files/SumatraPDF/SumatraPDF.exe",
        "C:/Program Files (x86)/SumatraPDF/SumatraPDF.exe",
        "C:/Users/" + qgetenv("USERNAME") + "/AppData/Local/SumatraPDF/SumatraPDF.exe"
    };
    
    QString sumatraPath;
    for (const QString &path : possiblePaths) {
        if (QFile::exists(path)) {
            sumatraPath = path;
            break;
        }
    }
    
    if (sumatraPath.isEmpty()) {
        // Sprawdź w PATH
        QProcess testProcess;
        testProcess.start("SumatraPDF.exe", QStringList() << "-version");
        if (testProcess.waitForFinished(3000) && testProcess.exitCode() == 0) {
            sumatraPath = "SumatraPDF.exe";
        } else {
            qDebug() << "SumatraPDF nie został znaleziony";
            return false;
        }
    }
    
    qDebug() << "Znaleziono SumatraPDF:" << sumatraPath;
    
    // Spróbuj uruchomić SumatraPDF w trybie embedded (jeśli to możliwe)
    // Niestety SumatraPDF nie ma łatwego API do embeddingu, więc na razie
    // wyświetlimy ładną informację o dostępności
    
    QFileInfo fileInfo(filePath);
    m_previewLabel->setText(QString(
        "<div style='text-align: center; padding: 25px; font-family: Segoe UI, Arial, sans-serif;'>"
        "<div style='background: linear-gradient(135deg, #2196F3 0%, #21CBF3 100%); color: white; padding: 20px; border-radius: 12px; margin-bottom: 25px; box-shadow: 0 4px 15px rgba(33, 150, 243, 0.3);'>"
        "<h2 style='margin: 0; font-size: 20px; font-weight: 600;'>📄 %1</h2>"
        "<p style='margin: 8px 0 0 0; opacity: 0.95; font-size: 14px;'>%2 KB • %3</p>"
        "</div>"
        
        "<div style='background: #e8f5e8; padding: 20px; border-radius: 12px; margin: 20px 0; border-left: 5px solid #4CAF50; box-shadow: 0 2px 10px rgba(0,0,0,0.1);'>"
        "<div style='display: flex; align-items: center; justify-content: center; margin-bottom: 15px;'>"
        "<span style='font-size: 24px; margin-right: 10px;'>✅</span>"
        "<h3 style='color: #2e7d32; margin: 0; font-size: 18px; font-weight: 600;'>SumatraPDF dostępny!</h3>"
        "</div>"
        "<p style='color: #2e7d32; margin: 0; line-height: 1.6; font-size: 14px;'>"
        "Wykryto SumatraPDF w systemie. To świetny, lekki czytnik PDF!<br>"
        "<strong>Kliknij dwukrotnie na pliku</strong> aby otworzyć w SumatraPDF."
        "</p>"
        "</div>"
        
        "<div style='background: #f0f7ff; padding: 18px; border-radius: 10px; border: 1px solid #c3e3ff; margin: 20px 0;'>"
        "<h4 style='color: #1565c0; margin: 0 0 12px 0; font-size: 16px; font-weight: 600;'>💡 Wskazówka</h4>"
        "<p style='color: #1565c0; margin: 0; font-size: 13px; line-height: 1.5;'>"
        "Możesz również przeciągnąć pliki PDF bezpośrednio na okno SumatraPDF<br>"
        "lub kliknąć prawym przyciskiem → \"Otwórz za pomocą\" → SumatraPDF"
        "</p>"
        "</div>"
        
        "<div style='margin-top: 25px; padding: 15px;'>"
        "<div style='background: linear-gradient(90deg, #FF6B6B, #4ECDC4); color: white; padding: 12px 24px; border-radius: 25px; display: inline-block; cursor: pointer; font-weight: 600; font-size: 14px; box-shadow: 0 3px 10px rgba(0,0,0,0.2);'>"
        "🚀 Kliknij dwukrotnie na pliku, aby otworzyć"
        "</div>"
        "</div>"
        "</div>"
    ).arg(fileInfo.fileName())
     .arg(fileInfo.size() / 1024)
     .arg(fileInfo.lastModified().toString("dd.MM.yyyy hh:mm")));
    
    m_previewLabel->setStyleSheet(
        "QLabel { "
        "background-color: white; "
        "color: #2c3e50; "
        "font-size: 13px; "
        "padding: 15px; "
        "border: 1px solid #e0e6ed; "
        "border-radius: 15px; "
        "}"
    );
    m_previewScrollArea->setWidget(m_previewLabel);
    
    return true; // Zwracamy true, bo wyświetliliśmy ładną informację
}

#ifdef Q_OS_WIN
// Funkcja do próby użycia Windows Shell Preview
bool PdfViewer::tryWindowsShellPreview(const QString &filePath) {
    qDebug() << "Próbuję Windows Shell Preview";
    
    // Na razie tylko wyświetlamy informację
    // W przyszłości można spróbować IPreviewHandler API
    return false;
}
#endif

// Funkcja do wyświetlenia rozbudowanych informacji o pliku PDF
bool PdfViewer::trySimplePdfInfo(const QString &filePath) {
    qDebug() << "Wyświetlam rozbudowane informacje o PDF";
    
    QFileInfo fileInfo(filePath);
    
    // Sprawdź czy możemy odczytać podstawowe info z pliku
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    // Odczytaj pierwsze 1KB pliku aby sprawdzić czy to PDF
    QByteArray header = file.read(1024);
    file.close();
    
    bool isPdf = header.startsWith("%PDF-");
    QString pdfVersion;
    
    if (isPdf) {
        // Wyciągnij wersję PDF z nagłówka
        QRegularExpression versionRegex("%PDF-(\\d+\\.\\d+)");
        QRegularExpressionMatch match = versionRegex.match(header);
        if (match.hasMatch()) {
            pdfVersion = match.captured(1);
        }
    }
    
    // Stwórz ładną kartę informacyjną o pliku PDF
    m_previewLabel->setText(QString(
        "<div style='padding: 20px; font-family: Segoe UI, Arial, sans-serif; background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%); border-radius: 15px;'>"
        
        // Nagłówek z ikoną i nazwą pliku
        "<div style='background: white; padding: 25px; border-radius: 12px; margin-bottom: 20px; box-shadow: 0 4px 15px rgba(0,0,0,0.1);'>"
        "<div style='text-align: center; margin-bottom: 15px;'>"
        "<div style='background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; width: 80px; height: 80px; border-radius: 50%%; margin: 0 auto 15px; display: flex; align-items: center; justify-content: center; font-size: 32px; box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);'>"
        "📄"
        "</div>"
        "<h2 style='margin: 0; color: #2c3e50; font-size: 18px; font-weight: 600; word-break: break-word;'>%1</h2>"
        "</div>"
        
        // Informacje szczegółowe
        "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px;'>"
        
        "<div style='background: #e3f2fd; padding: 15px; border-radius: 8px; border-left: 4px solid #2196F3;'>"
        "<h4 style='margin: 0 0 8px 0; color: #1976d2; font-size: 14px;'>📊 Rozmiar</h4>"
        "<p style='margin: 0; color: #424242; font-size: 16px; font-weight: 600;'>%2 KB</p>"
        "<p style='margin: 0; color: #757575; font-size: 12px;'>(%3 MB)</p>"
        "</div>"
        
        "<div style='background: #f3e5f5; padding: 15px; border-radius: 8px; border-left: 4px solid #9c27b0;'>"
        "<h4 style='margin: 0 0 8px 0; color: #7b1fa2; font-size: 14px;'>📅 Data modyfikacji</h4>"
        "<p style='margin: 0; color: #424242; font-size: 14px; font-weight: 600;'>%4</p>"
        "<p style='margin: 0; color: #757575; font-size: 12px;'>%5</p>"
        "</div>"
        
        "</div>"
        
        // Informacje o PDF
        "%6"
        
        // Ścieżka do pliku
        "<div style='background: #fff3e0; padding: 15px; border-radius: 8px; margin-bottom: 20px; border-left: 4px solid #ff9800;'>"
        "<h4 style='margin: 0 0 8px 0; color: #f57c00; font-size: 14px;'>📂 Ścieżka</h4>"
        "<p style='margin: 0; color: #424242; font-size: 12px; word-break: break-all; line-height: 1.4;'>%7</p>"
        "</div>"
        
        // Przyciski akcji
        "<div style='text-align: center; margin-top: 25px;'>"
        "<div style='background: linear-gradient(135deg, #4CAF50 0%, #45a049 100%); color: white; padding: 12px 30px; border-radius: 25px; display: inline-block; margin: 5px; cursor: pointer; font-weight: 600; font-size: 14px; box-shadow: 0 4px 15px rgba(76, 175, 80, 0.3);'>"
        "🔗 Kliknij dwukrotnie aby otworzyć"
        "</div>"
        "</div>"
        
        "</div>"
        "</div>"
    ).arg(fileInfo.fileName())
     .arg(fileInfo.size() / 1024)
     .arg(QString::number(fileInfo.size() / 1024.0 / 1024.0, 'f', 2))
     .arg(fileInfo.lastModified().toString("dd.MM.yyyy"))
     .arg(fileInfo.lastModified().toString("hh:mm:ss"))
     .arg(isPdf ? QString(
         "<div style='background: #e8f5e8; padding: 15px; border-radius: 8px; margin-bottom: 15px; border-left: 4px solid #4CAF50;'>"
         "<h4 style='margin: 0 0 8px 0; color: #2e7d32; font-size: 14px;'>✅ Informacje PDF</h4>"
         "<p style='margin: 0; color: #424242; font-size: 14px;'>Wersja PDF: <strong>%1</strong></p>"
         "<p style='margin: 5px 0 0 0; color: #757575; font-size: 12px;'>Plik jest prawidłowym dokumentem PDF</p>"
         "</div>"
     ).arg(pdfVersion.isEmpty() ? "nieznana" : pdfVersion) : QString(
         "<div style='background: #ffebee; padding: 15px; border-radius: 8px; margin-bottom: 15px; border-left: 4px solid #f44336;'>"
         "<h4 style='margin: 0 0 8px 0; color: #c62828; font-size: 14px;'>⚠️ Ostrzeżenie</h4>"
         "<p style='margin: 0; color: #424242; font-size: 14px;'>Plik może nie być prawidłowym dokumentem PDF</p>"
         "</div>"
     ))
     .arg(fileInfo.absoluteFilePath()));
    
    m_previewLabel->setStyleSheet(
        "QLabel { "
        "background-color: transparent; "
        "color: #2c3e50; "
        "font-size: 13px; "
        "padding: 0; "
        "border: none; "
        "}"
    );
    m_previewScrollArea->setWidget(m_previewLabel);
    
    return true;
}
