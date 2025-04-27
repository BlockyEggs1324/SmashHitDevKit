#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include <QString>
#include "RoomLoader.h"

struct Level {
    QString name;
    std::vector<Room> rooms;
};

namespace Loader {
    Level loadLevel(const QString& levelPath, const QString& rootDir, const bool appendStr, const bool useRoot);

    std::vector<Level> loadGame(const QString& gamePath, const QString& rootDir);
}

#endif // LEVELLOADER_H
