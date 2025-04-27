#include "LevelLoader.h"
#include <QFile>
#include <QFileInfo>
#include <qxmlstream.h>

Level Loader::loadLevel(const QString& levelPath, const QString& rootDir, const bool appendStr, const bool useRoot) {
    Level level;
    level.name = extractFileName(levelPath);

    QString filename;

    if (useRoot) filename = rootDir + "/levels/" + levelPath;
    else filename = levelPath;

    if (appendStr) filename += ".xml";

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open Level XML: " << filename;
        return level; // Return empty Level on error
    }

    QXmlStreamReader xml(&file);

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const QString name = xml.name().toString();

            if (name == "room") {
                QString typeAttr = xml.attributes().value("type").toString();

                if (!typeAttr.isEmpty()) {
                    QString luaPath = rootDir + "/rooms/" + typeAttr + ".lua";
                    Room room = LoadRoom(luaPath, rootDir);
                    level.rooms.push_back(room);
                } else {
                    qWarning() << "Room element is missing 'type' attribute!";
                }
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "XML Parse Error:" << xml.errorString();
    }

    return level;
}

std::vector<Level> Loader::loadGame(const QString& gamePath, const QString& rootDir) {
    std::vector<Level> levels;

    QString realPath = rootDir + gamePath;

    QFile file(realPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open game XML: " << realPath;
        return levels; // Return empty Level on error
    }

    QXmlStreamReader xml(&file);

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const auto name = xml.name().toString();

            if (name == "level") {

                QString levelName = xml.attributes().value("name").toString();

                Level newLevel = loadLevel(levelName, rootDir, true, true);

                levels.push_back(newLevel);
            }
        }
    }

    return levels;
}
