#include "RoomLoader.h"
#include <QFileInfo>
#include <QPair>
#include <regex>

QString extractFileName(const QString& roomPath) {

    // Get QFileInfo to extract the file name without path
    QFileInfo fileInfo(roomPath);
    QString fileNameWithoutExtension = fileInfo.baseName();  // This removes the extension

    // Convert back to QString and return
    return fileNameWithoutExtension;
}

// Function to load room file via QFileDialog
Room Loader::LoadRoom(const QString& roomPath, const QString& rootDir) {
    Room room;
    room.name = extractFileName(roomPath);

    // Open the Lua file using QFileDialog
    QFile file(roomPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open Lua file: " << roomPath;
        return room; // Return empty room in case of error
    }

    // Read the content of the Lua file
    QTextStream in(&file);
    QString luaContent = in.readAll();

    // Parse the Lua content into Room data structure
    ParseLuaFile(luaContent, room, rootDir);

    return room;
}


std::string extractSegment(const std::string& luaLine) {
    // Updated regex to ignore extra arguments in mgSegment
    std::regex segmentRegex(R"((?:mgSegment|confSegment)\(\"([^\"]+)\"(?:,[^)]*)?\))");
    std::smatch match;

    if (std::regex_search(luaLine, match, segmentRegex)) {
        std::string tempStr = match[1].str();
        return tempStr;
        //return QString::fromStdString(tempStr); // Returns the segment name (e.g., "basic/basic/start")
    }

    return "";
}

std::vector<float> extractFogColors(const std::string& luaLine) {
    std::vector<float> colors;

    // Regex to match mgFogColor with various numbers (including decimals and negatives)
    std::regex fogRegex(R"(mgFogColor\(\s*([-+]?\d*\.?\d+)\s*,\s*([-+]?\d*\.?\d+)\s*,\s*([-+]?\d*\.?\d+)\s*(?:,\s*([-+]?\d*\.?\d+)\s*,\s*([-+]?\d*\.?\d+)\s*,\s*([-+]?\d*\.?\d+)\s*)?\))");
    std::smatch match;

    if (std::regex_search(luaLine, match, fogRegex)) {
        // First 3 parameters are required (RGB)
        for (size_t i = 1; i <= 3; ++i) {
            colors.push_back(std::stof(match[i].str()));
        }

        // Optional parameters (typically 3 more for full fog definition)
        for (size_t i = 4; i < match.size() && match[i].matched; ++i) {
            colors.push_back(std::stof(match[i].str()));
        }
    }

    return colors;
}

// Adds the segments to the room
void Loader::ParseLuaFile(const QString& luaContent, Room& room, const QString& rootDir) {
    std::vector<Segment> tempSegments;  // Temporary storage for segments
    Segment startSegment;  // For start.xml
    Segment doorSegment;   // For door.xml

    // Split luaContent into lines
    std::istringstream stream(luaContent.toStdString());
    std::string line;

    while (std::getline(stream, line)) {
        if (line.find("confSegment") != std::string::npos || line.find("mgSegment") != std::string::npos) {
            // Extract the segment path from the line
            std::string segmentPath = extractSegment(line);
            bool canLoad = true;

            if (segmentPath.size() >= 5 && segmentPath.substr(segmentPath.size() - 5) == "start") {
                startSegment = loadLevelSegment(rootDir, QString::fromStdString("/segments/" + segmentPath + ".xml"), true);
                canLoad = false;
            }
            else if (segmentPath.size() >= 4 && segmentPath.substr(segmentPath.size() - 4) == "door") {
                doorSegment = loadLevelSegment(rootDir, QString::fromStdString("/segments/" + segmentPath + ".xml"), true);
                canLoad = false;
            }
            else if (canLoad && !segmentPath.empty()) {
                Segment newSegment = loadLevelSegment(rootDir, QString::fromStdString("/segments/" + segmentPath + ".xml"), true);
                tempSegments.push_back(newSegment);
            }
        }
        if (line.find("mgFogColor") != std::string::npos) {
            std::vector<float> fog = extractFogColors(line);

            // Create and assign std::array objects
            room.lowerFog = {fog[0], fog[1], fog[2], 1.0f};
            room.upperFog = {fog[3], fog[4], fog[5], 1.0f};
        }
    }

    float currentOffset = 0.0f;

    // Add start segment first
    startSegment.offset = currentOffset;
    //qDebug() << "Start segment is" << startSegment.name;
    room.segments.push_back(startSegment);
    currentOffset += startSegment.size.z();

    // Add all middle segments
    for (Segment& seg : tempSegments) {
        seg.offset = currentOffset;
        //qDebug() << "Segment:" << seg.name << "has offset:" << seg.offset;
        room.segments.push_back(seg);
        currentOffset += seg.size.z();
        //qDebug() << currentOffset;

    }

    // Add door segment last
    doorSegment.offset = currentOffset;
    //qDebug() << "Door Segment Offset" << doorSegment.offset;
    room.segments.push_back(doorSegment);

    //qDebug() << "Final checking!";

    for (Segment& seg : room.segments) {
        qDebug() << "Segment:" << seg.name << "has offset:" << seg.offset;
    }
}


