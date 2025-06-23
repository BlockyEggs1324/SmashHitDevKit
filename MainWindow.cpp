#include "MainWindow.h"
#include "qapplication.h"

#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QToolBar>
#include <QDockWidget>
#include <QListWidget>
#include <QProxyStyle>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_option(ViewOption::Select) {

    this->setWindowState(Qt::WindowStates::enum_type::WindowMaximized);

    prefs = loadPrefs();

    xyView = new XYViewWidget(this, &m_rects, &m_selectedRects, &m_option);
    xzView = new XZViewWidget(this, &m_rects, &m_selectedRects, &m_option);
    yzView = new YZViewWidget(this, &m_rects, &m_selectedRects, &m_option);

    segmentWidget = new SegmentWidget(this, &m_rects, &m_selectedRects);  // 3D view widget

    segmentWidget->setRootDir(prefs.m_rootDir);
    segmentWidget->setFov(prefs.m_fov);
    segmentWidget->setSens(prefs.m_sensitivity);

    outliner = new QTreeWidget;
    outliner->setColumnCount(1); // Only one column for display
    outliner->setHeaderHidden(true); 

    fileMenu = menuBar()->addMenu("&File");
    editMenu = menuBar()->addMenu("&Edit");
    viewMenu = menuBar()->addMenu("&View");
    toolsMenu = menuBar()->addMenu("&Tools");

    qDebug() << prefs.m_theme;

    setTheme(prefs.m_theme);

    QString style = "QMenu::item { padding:2px 20px 2px 20px; border:1px solid transparent; }"
                    "QMenu::item:selected { border-color:rgba(168, 201, 233, 170); background:rgba(111, 183, 255, 170); }"
                    "QMenu::separator { height:2px; background:palette(base); margin-left:5px; margin-right:5px; }"
                    "QMenu::icon:checked { background-color:qlineargradient(x1:0,y1:1,x2:0,y2:0,stop:0 rgba(25,25,25,127),stop:1 rgba(53,53,53,75));"
                    "border:1px solid palette(highlight);border-radius:2px; }";

    fileMenu->setStyleSheet(style);
    editMenu->setStyleSheet(style);
    viewMenu->setStyleSheet(style);
    toolsMenu->setStyleSheet(style);

    // File Menu

    QMenu *newMenu = new QMenu("&New", this);
    newMenu->setStyleSheet(style);
    fileMenu->addMenu(newMenu);

    // New submenu

    QAction *newSegmentAction = new QAction("New &Segment", this);
    newMenu->addAction(newSegmentAction);

    QAction *newRoomAction = new QAction("New &Room", this);
    newMenu->addAction(newRoomAction);
    connect(newRoomAction, &QAction::triggered, this, &MainWindow::newRoom);

    // Open submenu

    QMenu *openMenu = new QMenu("&Open", this);
    openMenu->setStyleSheet(style);
    fileMenu->addMenu(openMenu);

    QAction *loadSegmentAction = new QAction("Open &Segment", this);
    openMenu->addAction(loadSegmentAction);
    connect(loadSegmentAction, &QAction::triggered, this, &MainWindow::loadSegmentFromFile);

    QAction *loadRoomAction = new QAction("Open &Room", this);
    openMenu->addAction(loadRoomAction);
    connect(loadRoomAction, &QAction::triggered, this, &MainWindow::loadRoomFromFile);

    QAction *loadLevelAction = new QAction("Open &Level", this);
    openMenu->addAction(loadLevelAction);
    connect(loadLevelAction, &QAction::triggered, this, &MainWindow::loadLevelFromFile);

    QAction *loadGameAction = new QAction("Open All &Game Levels", this);
    openMenu->addAction(loadGameAction);
    connect(loadGameAction, &QAction::triggered, this, &MainWindow::loadGame);

    fileMenu->addSeparator();

    QAction *exitButton = new QAction("E&xit", this);
    fileMenu->addAction(exitButton);
    connect(exitButton, &QAction::triggered, this, &MainWindow::close);

    // Edit Menu

    QAction *undoButton = new QAction("&Undo", this);
    editMenu->addAction(undoButton);
    connect(undoButton, &QAction::triggered, this, &MainWindow::close);

    QAction *redoButton = new QAction("&Redo", this);
    editMenu->addAction(redoButton);
    connect(redoButton, &QAction::triggered, this, &MainWindow::close);

    QAction *copyButton = new QAction("&Copy", this);
    editMenu->addAction(copyButton);
    connect(copyButton, &QAction::triggered, this, &MainWindow::close);

    QAction *pasteButton = new QAction("&Paste", this);
    editMenu->addAction(pasteButton);
    connect(pasteButton, &QAction::triggered, this, &MainWindow::close);

    editMenu->addSeparator();

    QAction *prefsButton = new QAction("&Preferences", this);
    editMenu->addAction(prefsButton);
    connect(prefsButton, &QAction::triggered, this, &MainWindow::openPrefs);

    // View Menu

    toggleWireframeButton = new QAction("Show &Outline", this);
    viewMenu->addAction(toggleWireframeButton);
    toggleWireframeButton->setCheckable(true);
    connect(toggleWireframeButton, &QAction::toggled, this, &MainWindow::setWireframe);

    toggleFacesButton = new QAction("Show &Faces", this);
    viewMenu->addAction(toggleFacesButton);
    toggleFacesButton->setCheckable(true);
    connect(toggleFacesButton, &QAction::toggled, this, &MainWindow::setFaces);

    toggleColoured = new QAction("Show &Coloured", this);
    viewMenu->addAction(toggleColoured);
    toggleColoured->setCheckable(true);
    connect(toggleColoured, &QAction::toggled, this, &MainWindow::setColoured);

    toggleGameView = new QAction("Show &Game View", this);
    viewMenu->addAction(toggleGameView);
    toggleGameView->setCheckable(true);
    connect(toggleGameView, &QAction::toggled, this, &MainWindow::setGameView);

    // Tools Menu

    QAction *soundBrowser = new QAction("&Sound Browser", this);
    toolsMenu->addAction(soundBrowser);
    connect(soundBrowser, &QAction::triggered, this, &MainWindow::openSoundBrowser);
    
    QAction *textureBrowser = new QAction("&Template Browser", this);

    // Main Stuff

    QDockWidget *outlinerDock = new QDockWidget("Outliner", this);
    outlinerDock->setWidget(outliner);
    addDockWidget(Qt::LeftDockWidgetArea, outlinerDock);

    // Left column (3D over YZ)
    QSplitter *leftColumn = new QSplitter(Qt::Vertical);

    leftColumn->addWidget(segmentWidget);
    leftColumn->addWidget(xyView);

    leftColumn->setStretchFactor(0, 1); // 3D view takes up more space
    leftColumn->setStretchFactor(1, 1); // YZ view takes up less space

    // Right column (XY over XZ)
    QSplitter *rightColumn = new QSplitter(Qt::Vertical);
    rightColumn->addWidget(xzView);
    rightColumn->addWidget(yzView);
    rightColumn->setStretchFactor(0, 1); // Both views have equal space in the right column
    rightColumn->setStretchFactor(1, 1);

    // Combine the two columns into a main splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(leftColumn);
    mainSplitter->addWidget(rightColumn);
    mainSplitter->setStretchFactor(0, 1); // More space for the left (3D)
    mainSplitter->setStretchFactor(1, 1); // More space for the right (2D)

    // Set the central widget with a layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    //layout->addWidget(outliner);
    layout->addWidget(mainSplitter);
    layout->setContentsMargins(0, 0, 0, 0); // No margins
    setCentralWidget(centralWidget);

    setWindowTitle("Smash Hit DevKit");
    setGeometry(100, 100, 1200, 675);
    //layout->setGeometry(QRect(0, 0, 1200, 675));

    //segmentWidget->setFov(prefs.m_fov);
    //segmentWidget->setSens(prefs.m_sensitivity);

}

void MainWindow::setTheme(QString theme) {

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, Qt::white);
    lightPalette.setColor(QPalette::AlternateBase, QColor(225, 225, 225));
    lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
    lightPalette.setColor(QPalette::ToolTipText, Qt::black);
    lightPalette.setColor(QPalette::Text, Qt::black);
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::BrightText, Qt::red);
    lightPalette.setColor(QPalette::Link, QColor(0, 120, 215));
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    QPalette darkPurplePalette;
    darkPurplePalette.setColor(QPalette::Window, QColor(43, 23, 43));
    darkPurplePalette.setColor(QPalette::WindowText, QColor(147, 6, 246));
    darkPurplePalette.setColor(QPalette::Base, QColor(25, 3, 25));
    darkPurplePalette.setColor(QPalette::AlternateBase, QColor(43, 43, 43));
    darkPurplePalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPurplePalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPurplePalette.setColor(QPalette::Text, QColor(147, 6, 246));
    darkPurplePalette.setColor(QPalette::Button, QColor(43, 43, 43));
    darkPurplePalette.setColor(QPalette::ButtonText, QColor(147, 6, 246));
    darkPurplePalette.setColor(QPalette::BrightText, Qt::red);
    darkPurplePalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPurplePalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPurplePalette.setColor(QPalette::HighlightedText, Qt::black);

    QPalette swampPalette;
    swampPalette.setColor(QPalette::Window, QColor(0, 32, 39));
    swampPalette.setColor(QPalette::WindowText, Qt::white);
    swampPalette.setColor(QPalette::Base, QColor(0, 16, 20));
    swampPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    swampPalette.setColor(QPalette::ToolTipBase, Qt::white);
    swampPalette.setColor(QPalette::ToolTipText, Qt::white);
    swampPalette.setColor(QPalette::Text, Qt::white);
    swampPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    swampPalette.setColor(QPalette::ButtonText, Qt::white);
    swampPalette.setColor(QPalette::BrightText, Qt::red);
    swampPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    swampPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    swampPalette.setColor(QPalette::HighlightedText, Qt::black);

    if (theme == "System Theme") {
        QApplication::setPalette(QPalette());
    } else if (theme == "Light") {
        QApplication::setPalette(lightPalette);
    } else if (theme == "Dark") {
        QApplication::setPalette(darkPalette);
    } else if (theme == "Dark Purple") {
        QApplication::setPalette(darkPurplePalette);
    } else if (theme == "Swamp") {
        QApplication::setPalette(swampPalette);
    }

    QString style = "QMenu::item { padding:2px 20px 2px 20px; border:1px solid transparent; }"
                    "QMenu::item:selected { border-color:rgba(168, 201, 233, 170); background:rgba(111, 183, 255, 170); }"
                    "QMenu::separator { height:2px; background:palette(base); margin-left:5px; margin-right:5px; }"
                    "QMenu::icon:checked { background-color:qlineargradient(x1:0,y1:1,x2:0,y2:0,stop:0 rgba(25,25,25,127),stop:1 rgba(53,53,53,75));"
                    "border:1px solid palette(highlight);border-radius:2px; }";

    fileMenu->setStyleSheet(style);
    editMenu->setStyleSheet(style);
    viewMenu->setStyleSheet(style);
}

void MainWindow::openPrefs() {
    PreferencesDialog pDialog(this, prefs);
    int result = pDialog.exec();
    if (result == PreferencesDialog::Accepted) {
        prefs = pDialog.prefs;

        setTheme(prefs.m_theme);

        savePrefs();

        segmentWidget->setRootDir(prefs.m_rootDir);
        segmentWidget->setFov(prefs.m_fov);
        segmentWidget->setSens(prefs.m_sensitivity);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}

MainWindow::~MainWindow() {
    delete ui;
}
