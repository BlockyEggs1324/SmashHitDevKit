#ifndef RECT3D_H
#define RECT3D_H

#include <QVector3D>  // Use QVector3D for 3D vectors and points
#include <QRectF>      // Use QRectF for 2D rectangles
#include <GL/gl.h>
#include <QOpenGLFunctions>

class Rect3D {
public:
    // Constructor to initialize the position and size of the 3D rectangle
    Rect3D(const QVector3D& position, const QVector3D& size)
        : m_position(position), m_size(size) {}

    Rect3D(const float& ax, const float& ay, const float& az, const float& awidth, const float& aheight, const float& adepth)
        : m_position(ax, ay,az), m_size(awidth, aheight, adepth) {}

    // Getters
    QVector3D position() const { return m_position; }
    QVector3D size() const { return m_size; }

    float x() const { return m_position.x(); }
    float y() const { return m_position.y(); }
    float z() const { return m_position.z(); }

    float width() const { return m_size.x(); }
    float height() const { return m_size.y(); }
    float depth() const { return m_size.z(); }

    // Setters
    void setPosition(const QVector3D& position) { m_position = position; }
    void setSize(const QVector3D& size) { m_size = size; }
    void setColour(const std::array<GLfloat, 3> colour) {m_colour = colour;}

    std::array<GLfloat, 3> getColour() const {
        return m_colour;
    }

    // Translate the rectangle (move it in 3D space)
    void translate(const QVector3D& translation) {
        m_position += translation;
    }

    // Scale the rectangle
    void scale(const QVector3D& scaleFactor) {
        m_size *= scaleFactor;
    }

    // Get the XY rect
    QRectF getXYRect() {
        return QRectF(m_position.x(), m_position.y(), m_size.x(), m_size.y());
    }

    // Get the XZ rect
    QRectF getXZRect() {
        return QRectF(m_position.x(), m_position.z(), m_size.x(), m_size.z());
    }

    // Get the YZ rect
    QRectF getYZRect() {
        return QRectF(m_position.y(), m_position.z(), m_size.y(), m_size.z());
    }

    // Check if a point is inside the 3D rectangle (AABB check)
    bool contains(const QVector3D& point) const {
        return (point.x() >= m_position.x() && point.x() <= m_position.x() + m_size.x()) &&
               (point.y() >= m_position.y() && point.y() <= m_position.y() + m_size.y()) &&
               (point.z() >= m_position.z() && point.z() <= m_position.z() + m_size.z());
    }

    bool operator==(const Rect3D& other) const {
        return m_position == other.m_position && m_size == other.m_size;
    }

    bool operator!=(const Rect3D& other) const {
        return !(*this == other);
    }

    std::array<GLfloat, 3> m_colour = {1.0f, 1.0f, 1.0f};

private:
    QVector3D m_position;  // Position in 3D space (x, y, z)
    QVector3D m_size;      // Size (width, height, depth) in 3D space
};

#endif // RECT3D_H
