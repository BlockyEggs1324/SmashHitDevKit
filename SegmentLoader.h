#ifndef SEGMENTLOADER_H
#define SEGMENTLOADER_H

#include <QFile>
#include <QDomDocument>
#include <QDebug>
#include <QMessageBox>
#include "Rect3D.h"

struct Box {
    QVector3D size;
    QVector3D pos;
    bool hidden;
    QString templateType;
    std::array<GLfloat, 3> colour = {1.0f, 1.0f, 1.0f};
};

struct Obstacle {
    QVector3D pos;
    bool hidden;
    QString type;
    QString templateType;
    int mode;
};

struct Segment {
    QVector3D size;
    QString templateType;
    QString name;
    float offset;
    std::vector<Box> boxes;
    std::vector<Obstacle> obstacles;
};

namespace Loader {
    Segment loadLevelSegment(const QString& rootDir, const QString& filename, const bool useRootDir);

    std::vector<Rect3D> getRects(const std::vector<Box>& boxes);
}

#endif // SEGMENTLOADER_H
