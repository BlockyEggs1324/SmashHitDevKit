#ifndef ROOMLOADER_H
#define ROOMLOADER_H

#include <vector>
#include <QFileInfo>
#include "SegmentLoader.h"

struct Room {
    bool pStart = true;  // Whether the room starts with a start segment.
    bool pEnd = true;    // Whether the room ends with a door segment.
    std::vector<Segment> segments;  // List of possible segments.
    QString name;
    std::array<float, 4> lowerFog;
    std::array<float, 4> upperFog;
};

QString extractFileName(const QString& roomPath);

namespace Loader {
    Room LoadRoom(const QString& roomPath, const QString& rootDir);

    // Function to parse the Lua script to extract segments
    void ParseLuaFile(const QString& luaContent, Room& room, const QString& rootDir);
}

#endif // ROOMLOADER_H
