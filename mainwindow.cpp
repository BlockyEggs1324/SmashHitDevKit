#include "mainwindow.h"

#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QToolBar>
#include <QDockWidget>
#include <QListWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

    prefs = loadPrefs();

    xyView = new XYViewWidget(this, &m_rects, &m_selectedRects);
    xzView = new XZViewWidget(this, &m_rects, &m_selectedRects);
    yzView = new YZViewWidget(this, &m_rects, &m_selectedRects);

    segmentWidget = new SegmentWidget(this, &m_rects, &m_selectedRects);  // 3D view widget

    outliner = new QTreeWidget;
    outliner->setColumnCount(1); // Only one column for display
    outliner->setHeaderHidden(true);

    QMenu *fileMenu = menuBar()->addMenu("&File");
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    QMenu *viewMenu = menuBar()->addMenu("&View");

    QString style = "QMenu::item { padding:2px 20px 2px 20px; border:1px solid transparent; }"
                    "QMenu::item:selected { border-color:rgba(168, 201, 233, 170); background:rgba(111, 183, 255, 170); }"
                    "QMenu::separator { height:2px; background:palette(base); margin-left:5px; margin-right:5px; }"
                    "QMenu::icon:checked { background-color:qlineargradient(x1:0,y1:1,x2:0,y2:0,stop:0 rgba(25,25,25,127),stop:1 rgba(53,53,53,75));"
                    "border:1px solid palette(highlight);border-radius:2px; }";

    fileMenu->setStyleSheet(style);
    editMenu->setStyleSheet(style);
    viewMenu->setStyleSheet(style);

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

    // Main Stuff

    QDockWidget *outlinerDock = new QDockWidget("Outliner", this);
    outlinerDock->setWidget(outliner);
    addDockWidget(Qt::LeftDockWidgetArea, outlinerDock);

    // Left column (3D over YZ)
    QSplitter *leftColumn = new QSplitter(Qt::Vertical);

    leftColumn->addWidget(segmentWidget);
    leftColumn->addWidget(yzView);
    leftColumn->setStretchFactor(0, 1); // 3D view takes up more space
    leftColumn->setStretchFactor(1, 1); // YZ view takes up less space

    // Right column (XY over XZ)
    QSplitter *rightColumn = new QSplitter(Qt::Vertical);
    rightColumn->addWidget(xyView);
    rightColumn->addWidget(xzView);
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
    layout->setGeometry(QRect(0, 0, 1200, 675));

    //segmentWidget->setFov(prefs.m_fov);
    //segmentWidget->setSens(prefs.m_sensitivity);

}

void MainWindow::openPrefs() {
    PreferencesDialog pDialog(this, prefs);
    int result = pDialog.exec();
    if (result == PreferencesDialog::Accepted) {
        prefs = pDialog.prefs;

        savePrefs();

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
