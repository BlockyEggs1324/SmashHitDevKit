#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QTreeWidget>
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

        newPrefs.m_fov = settings.value("3D.fov", 75.0f).toFloat();
        newPrefs.m_sensitivity = settings.value("3D.sensitivity", 1.0f).toFloat();

        return newPrefs;
    }

    void savePrefs() {
        QSettings settings("settings.ini", QSettings::Format::IniFormat);

        settings.setValue("Editor.rootDir", prefs.m_rootDir);

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

    // Create the 3D segment widget
    SegmentWidget *segmentWidget; // 3D view widget

    std::vector<Rect3D> m_rects;
    std::vector<Rect3D*> m_selectedRects;

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
