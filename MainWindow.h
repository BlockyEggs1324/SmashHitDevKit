#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QListWidget>
#include <QComboBox>
#include <QTreeWidget>
#include <QDir>
#include <QStringList>
#include <QBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include "Views2D.h"
#include "SegmentWidget.h"
#include "SegmentLoader.h"
#include "RoomLoader.h"
#include "LevelLoader.h"
#include "PreferencesDialog.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}

QT_END_NAMESPACE

class SelectSeg : public QDialog {
    Q_OBJECT

public:
    SelectSeg(QWidget *parent = nullptr, QString rootDir = "") {
        setWindowTitle("Select Room Segments");
        setModal(true);

        QVBoxLayout *layout = new QVBoxLayout(this);

        selectedXmlFiles = {};

        QFileSystemModel *model = new QFileSystemModel(this);
        model->setRootPath(rootDir + "/segments");
        model->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
        model->setNameFilters(QStringList() << "*.xml");
        model->setNameFilterDisables(false);

        QTreeView *tree = new QTreeView(this);
        tree->setModel(model);
        tree->setRootIndex(model->index(rootDir + "/segments"));
        tree->setSelectionMode(QAbstractItemView::ExtendedSelection); // Allow multiple files
        tree->setSelectionBehavior(QAbstractItemView::SelectItems);
        layout->addWidget(tree);

        // When user selects files
        connect(tree->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=]() {
            QModelIndexList indexes = tree->selectionModel()->selectedIndexes();
            selectedXmlFiles = {};
            for (const QModelIndex &index : indexes) {           
                selectedXmlFiles << model->fileInfo(index);
            }
            qDebug() << "Selected XML files:" << selectedXmlFiles;
        });

        // Buttons
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        // Connect the ok button
        connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &SelectSeg::accept);

        // Connect the Cancel button
        connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &SelectSeg::reject);
        layout->addWidget(buttonBox);
    }

    void resizeEvent(QResizeEvent *event) {
        QDialog::resizeEvent(event);
    }

    ~SelectSeg() override = default;

    QFileInfoList selectedXmlFiles;
};

class NewRoomWindow : public QDialog {
    Q_OBJECT

public:
    NewRoomWindow(QWidget *parent = nullptr, QString rootDir = "") : QDialog(parent), m_rootDir(rootDir) {

        setWindowTitle("New Room");
        setModal(true);

        QVBoxLayout *layout = new QVBoxLayout(this);

        layout->addWidget(new QLabel("Room Name"));

        QLineEdit *nameEdit = new QLineEdit(this);
        layout->addWidget(nameEdit);

        connect(nameEdit, &QLineEdit::textEdited, this, &NewRoomWindow::changeName);

        QListWidget *segBox = new QListWidget(this);
        layout->addWidget(segBox);

        QPushButton *selBut = new QPushButton("Select Segments", this);
        layout->addWidget(selBut);
        connect(selBut, &QPushButton::pressed, this, [=]() {

            SelectSeg *seg = new SelectSeg(this, m_rootDir);
            seg->exec();

            for (QFileInfo segment : seg->selectedXmlFiles) {
                QString name = segment.fileName();
                QString path = segment.filePath();

                qDebug() << name;
                qDebug() << segment.baseName();
                qDebug() << segment.filePath();

                path = path.remove(m_rootDir + "/segments/");

                qDebug() << path;

                path = path.remove(".xml");
                qDebug() << path;

                files << path;
            }

            segBox->addItems(files);

        });


        // Buttons
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        // Connect the Apply button
        connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, [=]() {
            if (m_roomName == "") QMessageBox::warning(this, "Room Name Not Entered", "You need to enter a name for the room");
            else if (files.empty()) QMessageBox::warning(this, "No Segments", "You need at least one segment for the room");
            else {
                accept();
            }
        });

        // Connect the Cancel button
        connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &NewRoomWindow::reject);
        layout->addWidget(buttonBox);

        //setLayout(layout);
    }

    void changeName(const QString& value) {
        m_roomName = value;
    }

    void setLevels(QComboBox *box) {
        QString basePath = m_rootDir + QDir::separator() + "segments";
        QDir baseDir(basePath);

        if (!baseDir.exists()) {
            qDebug() << "Directory does not exist:" << basePath;
            return;
        }

        QStringList xmlFiles;

        QDirIterator it(basePath, QStringList() << "*.xml", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString fullPath = it.next();                        // Full absolute path
            QString relativePath = baseDir.relativeFilePath(fullPath);  // e.g., "subdir/file.xml"
            xmlFiles << relativePath;
            qDebug() << "Found XML file:" << relativePath;
        }

        box->addItems(xmlFiles);
    }

    void resizeEvent(QResizeEvent *event) {
        QDialog::resizeEvent(event);
    }

    ~NewRoomWindow() override = default;

    QStringList files;
    QString song;
    QColor upperFog;
    QColor lowerFog;

private:
    QString m_rootDir;
    QString m_roomName = "";

};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void resizeEvent(QResizeEvent *event);
    ~MainWindow();

    std::array<float, 4> lerpColour(const std::array<float, 4>& cola, const std::array<float, 4>& colb, float t) {
        std::array<float, 4> result;
        for (size_t i = 0; i < 4; ++i) {
            result[i] = cola[i] + (colb[i] - cola[i]) * t;
         }
        return result;
    }

    void tickFogColour() {
        constexpr float duration = 1.5f;
        constexpr float tickInterval = 1.0f / 60.0f; // ~60 FPS

        fogChangeT += tickInterval;
        float t = std::min(fogChangeT / duration, 1.0f);

        segmentWidget->lowerFogColour = lerpColour(lowerFogStart, lowerFogTarget, t);
        segmentWidget->upperFogColour = lerpColour(upperFogStart, upperFogTarget, t);

        segmentWidget->update(); // Redraw the OpenGL widget

        if (t >= 1.0f && fogTimer) {
            fogTimer->stop();
            fogTimer->deleteLater();
            fogTimer = nullptr;
        }
    }



    void startFogChange(const std::array<float, 4>& newLower, const std::array<float, 4>& newUpper) {
        lowerFogStart = segmentWidget->lowerFogColour;
        upperFogStart = segmentWidget->upperFogColour;

        lowerFogTarget = newLower;
        upperFogTarget = newUpper;

        fogChangeT = 0.0f;

        if (fogTimer) {
            fogTimer->stop();
            fogTimer->deleteLater();
        }

        fogTimer = new QTimer(this);
        connect(fogTimer, &QTimer::timeout, this, &MainWindow::tickFogColour);
        fogTimer->start(1000 / 60); // 60 FPS
    }

    void newSegment() {
        QString filePath = QFileDialog::getSaveFileName(this, "New Segment", prefs.m_rootDir, "XML Files (*.xml)");

        QFile newFile(filePath);
    }

    void newRoom() {
        NewRoomWindow newRoom(nullptr, prefs.m_rootDir);

        if (newRoom.exec() == NewRoomWindow::Accepted) {

            QStringList segments = newRoom.files;

            QString luaScript;
            luaScript += "function init()\n";
            luaScript += "\tpStart = mgGetBool(\"start\", true)\n";
            luaScript += "\tpEnd = mgGetBool(\"end\", true)\n\n";

            luaScript += "\tmgFogColor(1, 1, 1, .2, .2, .2)\n";
            luaScript += "\tmgMusic(\"1\")\n";
            luaScript += "\tmgLowPass(0.2)\n";
            luaScript += "\tmgReverb(0.2, 0.5, 0.2)\n";
            luaScript += "\tmgEcho(0.2, 0.5, 0.75, 0.8)\n";
            luaScript += "\tmgGravity(1)\n\n";

            // Generate confSegment lines
            for (int i = 0; i < segments.size(); ++i) {
                if (segments[i] == "brownie/part1/16_2") {
                    luaScript += QString("\tconfSegment(\"%1\", 1)\n").arg(segments[i]);
                } else {
                    luaScript += QString("\tconfSegment(\"%1\", 1)\n").arg(segments[i]);
                }
            }

            luaScript += "\n\tl = 0\n\n";
            luaScript += "\tif pStart then\n";
            luaScript += QString("\t\tl = l + mgSegment(\"basic/basic/start\", -l)\n");
            luaScript += "\tend\n\n";

            luaScript += "\tlocal targetLen = 120\n";

            luaScript += "\twhile l < targetLen do\n";
            luaScript += "\t\ts = nextSegment()\n";
            luaScript += "\t\tl = l + mgSegment(s, -l)\n";
            luaScript += "\tend\n\n";

            luaScript += "\tif pEnd then\n";
            luaScript += "\t\tl = l + mgSegment(\"brownie/part1/door\", -l)\n";
            luaScript += "\tend\n\n";

            luaScript += "\tmgLength(l)\n";
            luaScript += "end\n\n";

            luaScript += "function tick()\n";
            luaScript += "end\n";

            QString fileName = QFileDialog::getSaveFileName(this, "Save Room Lua", prefs.m_rootDir, "Lua Files(*.lua)");
            if (fileName.isEmpty())
                return;

            QFile newRoomFile(fileName);
            if (newRoomFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                newRoomFile.write(luaScript.toUtf8());
                newRoomFile.close();
            }
        }


    }

    void loadSegmentFromFile() {
        QString filePath = QFileDialog::getOpenFileName(this, "Open Segment XML", prefs.m_rootDir, "XML Files (*.xml)");
        if (filePath.isEmpty())
            return;
        currentSegment = Loader::loadLevelSegment(prefs.m_rootDir, filePath, false);

        populateOutliner(outliner, currentSegment);

        m_rects = Loader::getRects(currentSegment.boxes);
    }

    void loadRoomFromFile() {
        QString filePath = QFileDialog::getOpenFileName(this, "Open Room lua", prefs.m_rootDir, "Lua Files (*.lua)");
        if (filePath.isEmpty())
            return;

        currentRoom = Loader::LoadRoom(filePath, prefs.m_rootDir);

        startFogChange(currentRoom.lowerFog, currentRoom.upperFog);

        std::vector<Box> boxes;

        for (Segment& seg : currentRoom.segments) {
            qDebug() << "Segment offset: " << seg.offset;
            for (Box& box : seg.boxes) {
                box.pos += QVector3D(0, 0, -seg.offset);
                boxes.push_back(box);
            }
        }

        qDebug() << "Final segment offset: " << currentRoom.segments.back().offset;

        populateOutliner(outliner, currentRoom);

        m_rects = Loader::getRects(boxes);

    }

    void loadLevelFromFile() {
        QString filePath = QFileDialog::getOpenFileName(this, "Open Level XML", prefs.m_rootDir, "XML Files (*.xml)");
        if (filePath.isEmpty())
            return;

        currentLevel = Loader::loadLevel(filePath, prefs.m_rootDir, false, false);

        float totalOffset = 0.0f;
        std::vector<Box> boxes;

        for (Room& room : currentLevel.rooms) {
            for (Segment& segment : room.segments) {
                // Apply global room+segment offset to boxes
                for (Box& box : segment.boxes) {
                    box.pos += QVector3D(0, 0, -(totalOffset + segment.offset));
                    boxes.push_back(box);
                }
            }

            // Increase total offset by the total room length
            if (!room.segments.empty()) {
                Segment& lastSeg = room.segments.back();
                totalOffset += lastSeg.offset + lastSeg.size.z();
            }
        }

        populateOutliner(outliner, {currentLevel});

        m_rects = Loader::getRects(boxes);

    }

    void loadGame() {

        QMessageBox::StandardButton q;
        q = QMessageBox::question(nullptr, "Confirm", "Confirm action? This may cause huge amounts of lag!", QMessageBox::Yes | QMessageBox::No);

        if (q == QMessageBox::No) return;

        qDebug() << "Confirmed!";

        std::vector<Level> levels = Loader::loadGame("/game.xml", prefs.m_rootDir);

        float totalOffset = 0.0f;
        std::vector<Box> boxes;

        for (Level& level : levels) {
            for (Room& room : level.rooms) {
                for (Segment& segment : room.segments) {

                    for (Box& box : segment.boxes) {
                        // Apply cumulative offset for the entire game's room/segment structure
                        box.pos += QVector3D(0, 0, -totalOffset);
                        boxes.push_back(box);
                    }

                    // Increase totalOffset by segment size
                    totalOffset += segment.size.z();
                }
            }
        }

        qDebug() << "Total game length:" << totalOffset;

        m_rects = Loader::getRects(boxes);

        populateOutliner(outliner, levels);
    }

    void setWireframe(bool checked) {
        segmentWidget->m_drawWireframe = checked;
    }

    void setFaces(bool checked) {
        segmentWidget->m_drawFaces = checked;
    }

    void setColoured(bool checked) {
        segmentWidget->m_drawColour = checked;
    }

    void setGameView(bool checked) {
        segmentWidget->m_gameView = checked;
    }

    void populateOutliner(QTreeWidget* treeWidget, const std::vector<Level>& levels) {

        treeWidget->clear();

        for (const Level& level : levels) {
            // Add the Level item as a root
            QTreeWidgetItem* levelItem = new QTreeWidgetItem(treeWidget);
            levelItem->setText(0, level.name); // Set level name

            // Add rooms under the Level item
            for (const Room& room : level.rooms) {
                QTreeWidgetItem* roomItem = new QTreeWidgetItem(levelItem);
                roomItem->setText(0, room.name); // Set room name

                // Add segments under the Room item
                for (const Segment& segment : room.segments) {
                    QTreeWidgetItem* segmentItem = new QTreeWidgetItem(roomItem);
                    segmentItem->setText(0, segment.name); // Set segment name

                    // Add boxes under the Segment item
                    for (const Box& box : segment.boxes) {
                        QTreeWidgetItem* boxItem = new QTreeWidgetItem(segmentItem);
                        boxItem->setText(0, "Box");

                        // Create a formatted string for position and size
                        QString posText = QString("Pos: [%1, %2, %3]").arg(box.pos.x()).arg(box.pos.y()).arg(box.pos.z());
                        QString sizeText = QString("Size: [%1, %2, %3]").arg(box.size.x()).arg(box.size.y()).arg(box.size.z());
                        QString templateText = QString("Template: %1").arg(box.templateType);

                        QTreeWidgetItem* posItem = new QTreeWidgetItem(boxItem);
                        posItem->setText(0, posText); // Position

                        QTreeWidgetItem* sizeItem = new QTreeWidgetItem(boxItem);
                        sizeItem->setText(0, sizeText); // Size

                        QTreeWidgetItem* templateItem = new QTreeWidgetItem(boxItem);
                        templateItem->setText(0, templateText);
                    }
                }
            }
        }
    }

    void populateOutliner(QTreeWidget* treeWidget, const Room room) {

        treeWidget->clear();

        // Set room as root
        QTreeWidgetItem* roomItem = new QTreeWidgetItem(treeWidget);
        roomItem->setText(0, room.name); // Set level name

        for (const Segment& segment : room.segments) {
            // Add rooms under the Level item
            QTreeWidgetItem* segmentItem = new QTreeWidgetItem(roomItem);
            segmentItem->setText(0, segment.name); // Set segment name

            // Add boxes under the Segment item
            for (const Box& box : segment.boxes) {
                QTreeWidgetItem* boxItem = new QTreeWidgetItem(segmentItem);
                boxItem->setText(0, "Box");

                // Create a formatted string for position and size
                QString posText = QString("Pos: [%1, %2, %3]").arg(box.pos.x()).arg(box.pos.y()).arg(box.pos.z());
                QString sizeText = QString("Size: [%1, %2, %3]").arg(box.size.x()).arg(box.size.y()).arg(box.size.z());
                QString templateText = QString("Template: %1").arg(box.templateType);

                QTreeWidgetItem* posItem = new QTreeWidgetItem(boxItem);
                posItem->setText(0, posText); // Position

                QTreeWidgetItem* sizeItem = new QTreeWidgetItem(boxItem);
                sizeItem->setText(0, sizeText); // Size

                QTreeWidgetItem* templateItem = new QTreeWidgetItem(boxItem);
                templateItem->setText(0, templateText);
            }
        }
    }

    void populateOutliner(QTreeWidget* treeWidget, const Segment& segment) {

        treeWidget->clear();

        // Add the Segment item as a root
        QTreeWidgetItem* segmentItem = new QTreeWidgetItem(treeWidget);
        segmentItem->setText(0, segment.name); // Set level name

        for (const Box& box : segment.boxes) {
            QTreeWidgetItem* boxItem = new QTreeWidgetItem(segmentItem);
            boxItem->setText(0, "Box");

            // Create a formatted string for position and size
            QString posText = QString("Pos: [%1, %2, %3]").arg(box.pos.x()).arg(box.pos.y()).arg(box.pos.z());
            QString sizeText = QString("Size: [%1, %2, %3]").arg(box.size.x()).arg(box.size.y()).arg(box.size.z());
            QString templateText = QString("Template: %1").arg(box.templateType);

            QTreeWidgetItem* posItem = new QTreeWidgetItem(boxItem);
            posItem->setText(0, posText); // Position

            QTreeWidgetItem* sizeItem = new QTreeWidgetItem(boxItem);
            sizeItem->setText(0, sizeText); // Size

            QTreeWidgetItem* templateItem = new QTreeWidgetItem(boxItem);
            templateItem->setText(0, templateText);
        }
    }

    Prefs loadPrefs() {
        QSettings settings("settings.ini", QSettings::Format::IniFormat);
        Prefs newPrefs;

        newPrefs.m_rootDir = settings.value("Editor.rootDir", "C:/").toString();
        newPrefs.m_theme = settings.value("Editor.theme", "System Theme").toString();

        newPrefs.m_fov = settings.value("3D.fov", 75.0f).toFloat();
        newPrefs.m_sensitivity = settings.value("3D.sensitivity", 1.0f).toFloat();

        return newPrefs;
    }

    void savePrefs() {
        QSettings settings("settings.ini", QSettings::Format::IniFormat);

        settings.setValue("Editor.rootDir", prefs.m_rootDir);
        settings.setValue("Editor.theme", prefs.m_theme);

        settings.setValue("3D.fov", prefs.m_fov);
        settings.setValue("3D.sensitivity", prefs.m_sensitivity);
    }

    void update2D() {
        xyView->update();
        xzView->update();
        yzView->update();
    }

    void openPrefs();

    // Menu

    QAction *toggleWireframeButton;
    QAction *toggleFacesButton;
    QAction *toggleColoured;
    QAction *toggleGameView;

private:
    Ui::MainWindow *ui;
    // Create the 2D views widgets
    XYViewWidget *xyView;
    XZViewWidget *xzView;
    YZViewWidget *yzView;

    QTreeWidget* outliner;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *toolsMenu;

    // Create the 3D segment widget
    SegmentWidget *segmentWidget; // 3D view widget

    std::vector<Rect3D> m_rects;
    std::vector<Rect3D*> m_selectedRects;

    ViewOption m_option;

    void setTheme(QString theme);

    Segment currentSegment;
    Room currentRoom;
    Level currentLevel;

    Prefs prefs;

    std::array<float, 4> lowerFogStart;
    std::array<float, 4> upperFogStart;
    std::array<float, 4> lowerFogTarget;
    std::array<float, 4> upperFogTarget;

    float fogChangeT = 0.0f;
    QTimer* fogTimer = nullptr;

};

#endif // MAINWINDOW_H
