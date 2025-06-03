#ifndef MYOPENGLWIDGET_H
#define MYOPENGLWIDGET_H

#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>

class MyOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    MyOpenGLWidget(QWidget* parent = nullptr);
    ~MyOpenGLWidget();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    //void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QOpenGLShaderProgram shader;
    QOpenGLTexture* texture1;
    QOpenGLTexture* texture2;

    QOpenGLBuffer vbo1;

    QVector3D m_cameraPosition;
    float m_cameraYaw, m_cameraPitch;
    float m_cameraSpeed;
    float m_mouseSensitivity;    // Mouse sensitivity for rotation
    bool m_isDragging;           // Whether the right mouse button is being held down
    QPoint m_lastMousePos;       // Last position of the mouse

    QSet<int> m_pressedKeys;

    void handleInput();
    QMatrix4x4 getMVP(const QMatrix4x4& model);
    void makeObject();

};

/*

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <QImage>
#include <QDebug>

class BgfxWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    BgfxWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent), m_textureHandle(BGFX_INVALID_HANDLE) {}
    ~BgfxWidget() {
        bgfx::shutdown();  // Clean up bgfx when widget is destroyed
    }

protected:
    void initializeGL() override {
        // Initialize OpenGL functions for Qt
        initializeOpenGLFunctions();

        // Initialize bgfx
        bgfx::renderFrame();
        bgfx::Init bgfxInit;
        bgfxInit.type = bgfx::RendererType::Count;  // Automatically choose renderer
        bgfxInit.resolution.width = width();
        bgfxInit.resolution.height = height();
        bgfxInit.resolution.reset = BGFX_RESET_VSYNC;
        bgfx::init(bgfxInit);

        // Set up bgfx view clear options (clear color, depth)
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

        // Load a texture
        m_textureHandle = loadTexture(":/textures/your_texture.png");  // Update with your texture file path

        // Set up any other resources like shaders, vertex buffers, etc.
    }

    void resizeGL(int w, int h) override {
        // Handle window resizing for bgfx
        bgfx::reset(w, h, BGFX_RESET_VSYNC);
    }

    void paintGL() override {
        // Clear the screen and start a new frame
        bgfx::touch(0);

        // Example: Set up a simple identity matrix for rendering
        float mtx[16];
        bx::mtxIdentity(mtx);

        // Bind texture to texture unit 0
        bgfx::setTexture(0, s_texColor, m_textureHandle);

        // Submit rendering command to bgfx (you'd replace this with your actual draw logic)
        bgfx::submit(0);

        // End the current frame
        bgfx::frame();
    }

    void closeEvent(QCloseEvent *event) override {
        // Ensure proper cleanup of bgfx when the widget is closed
        bgfx::shutdown();
        QOpenGLWidget::closeEvent(event);
    }

private:
    bgfx::TextureHandle m_textureHandle;

    bgfx::TextureHandle loadTexture(const QString &filename) {
        // Load the texture from file using Qt
        QImage image(filename);
        if (image.isNull()) {
            qWarning() << "Failed to load image:" << filename;
            return BGFX_INVALID_HANDLE;
        }

        qDebug() << "Loaded texture from image:" << filename;

        // Create bgfx texture from QImage data
        bgfx::TextureHandle textureHandle = BGFX_INVALID_HANDLE;
        bgfx::ImageContainer imageContainer;
        imageContainer.data = image.bits();
        imageContainer.width = image.width();
        imageContainer.height = image.height();
        imageContainer.stride = image.width() * image.depth();
        imageContainer.format = bgfx::TextureFormat::BGRA8;

        textureHandle = bgfx::createTexture2D(image.width(), image.height(), false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_NONE, nullptr);

        return textureHandle;
    }
};
*/

#endif // MYOPENGLWIDGET_H

