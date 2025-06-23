#ifndef SEGMENTWIDGET_H
#define SEGMENTWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QWidget>
#include <QTimer>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <QVBoxLayout>
#include <QKeyEvent>
#include "Rect3D.h"
#include <QMainWindow>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QVector3D>
#include <QMatrix4x4>
#include <GL/glu.h>
#include <QtOpenGL>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <glm/glm.hpp>    // For core GLM types like glm::vec3
#include <glm/gtc/matrix_transform.hpp> // For matrix transformations
#include <glm/gtc/type_ptr.hpp>  // For type pointer (e.g., glm::value_ptr)
#include <glm/gtc/matrix_inverse.hpp> // For matrix inversion if needed

namespace glm {
    inline QString to_string(const glm::vec2& v) {
        return QString("vec2(%1, %2)").arg(v.x).arg(v.y);
    }

    inline QString to_string(const glm::vec3& v) {
        return QString("vec3(%1, %2, %3)").arg(v.x).arg(v.y).arg(v.z);
    }

    inline QString to_string(const glm::vec4& v) {
        return QString("vec4(%1, %2, %3, %4)").arg(v.x).arg(v.y).arg(v.z).arg(v.w);
    }

    inline QString to_string(const glm::mat4& m) {
        return QString(
                   "mat4("
                   " %1, %2, %3, %4,"
                   " %5, %6, %7, %8,"
                   " %9, %10, %11, %12,"
                   " %13, %14, %15, %16"
                   " )"
                   ).arg(m[0][0]).arg(m[1][0]).arg(m[2][0]).arg(m[3][0])
            .arg(m[0][1]).arg(m[1][1]).arg(m[2][1]).arg(m[3][1])
            .arg(m[0][2]).arg(m[1][2]).arg(m[2][2]).arg(m[3][2])
            .arg(m[0][3]).arg(m[1][3]).arg(m[2][3]).arg(m[3][3]);
    }
}

class SegmentWidget : public QOpenGLWidget, public QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    explicit SegmentWidget(QWidget *parent = nullptr, std::vector<Rect3D> *rects = nullptr, std::vector<Rect3D*> *selectedRects = nullptr);

    std::vector<Rect3D> getRects();

    void setRects(const std::vector<Rect3D>& newRects);
    void loadTileTexture();

    void setFov(int value);
    void setSens(float value);
    void setRootDir(QString rootDir);

    bool m_drawWireframe;
    bool m_drawFaces;
    bool m_gameView;
    bool m_drawColour;
    bool m_useShader;

    std::array<float, 4> lowerFogColour = {0.4f, 0.0f, 0.5f, 1.0f};
    std::array<float, 4> upperFogColour = {1.3f, 0.9f, 0.6f, 1.0f};

protected:
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

    void handleInput();

    bool intersects(const glm::vec3& ray, const glm::vec3& rayOrigin, const Rect3D& cube);

    glm::vec3 getCameraFront() const;

    QVector3D getCameraForward();

    int getSelectedIndex(Rect3D rect);

    void selectCube(const QPoint& mousePos);

    void keyPressEvent(QKeyEvent *event) override;

    void keyReleaseEvent(QKeyEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QVector3D m_cameraPosition;  // Camera position in 3D space
    float m_cameraYaw;           // Camera rotation (yaw)
    float m_cameraPitch;         // Camera rotation (pitch)
    float m_cameraSpeed;         // Camera movement speed
    float m_cameraFov;
    float m_gameViewPosition;
    float m_mouseSensitivity;    // Mouse sensitivity for rotation
    bool m_isDragging;           // Whether the right mouse button is being held down
    QPoint m_lastMousePos;       // Last position of the mouse
    bool m_glToggle;
    QWidget *m_parent;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;

    std::vector<Rect3D*> *m_selectedRects;

    std::vector<Rect3D> *m_rects;
    QSet<int> m_pressedKeys;

    QString m_rootDir;

    glm::vec3 rayWorld;

    bool m_drawDebugRay = true;
    glm::vec3 m_debugRayStart;
    glm::vec3 m_debugRayDir;
    glm::vec3 m_debugRayEnd;
    QMatrix4x4 m_model;

    glm::mat4 computeCubeTransform(const Rect3D& cube);

    glm::mat4 getViewMatrix();

    glm::vec3 getRayFromMouse(int mouseX, int mouseY);

    void ScreenPosToWorldRay(
        int mouseX, int mouseY,
        int screenWidth, int screenHeight,
        const glm::mat4& ViewMatrix,
        const glm::mat4& ProjectionMatrix,
        glm::vec3& out_origin,
        glm::vec3& out_direction);

    QOpenGLTexture *tileTex;

    GLuint m_tileTexture;
    QOpenGLTexture *loadTexture(QString filename);
    const unsigned char *tileData;

    // Fog shader stuff
    QOpenGLShaderProgram *m_roomProgram;
    QOpenGLShaderProgram *m_clearProgram;
    QOpenGLShaderProgram *m_basicProgram;

    GLuint loadShaderFromFile(const QString& path, GLenum type);
    QOpenGLShaderProgram *createShaderProgram(const QString& path);

    QMatrix4x4 getMVP(const QMatrix4x4& model);

    void updateProjectionMatrix();

    void drawDebugRay();

    void drawCubeSpecial(const Rect3D& cubeRect);

    void drawCubeNew(const Rect3D& rect, bool selected);

    void drawCube(const Rect3D& cubeRect, bool selected = false);

    void drawCubeOutline(const Rect3D& cubeRect);

};

#endif // SEGMENTWIDGET_H
