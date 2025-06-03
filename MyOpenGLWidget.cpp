#include "MyOpenGLWidget.h"
#include "QEvent"
#include "qevent.h"
#include <QImage>
#include <QTimer>
#include <QFile>
#include <QOpenGLShader>

MyOpenGLWidget::MyOpenGLWidget(QWidget* parent)
    : QOpenGLWidget(parent), texture1(nullptr), texture2(nullptr) {

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&MyOpenGLWidget::update));
    timer->start(4); // ~250 FPS (1000ms / 250 ≈ 4ms)

    // Initialize camera to look at origin from positive Z
    m_cameraPosition = QVector3D(0.0f, 0.0f, 3.0f);
    m_cameraYaw = -90.0f;  // Pointing along negative Z
    m_cameraPitch = 0.0f;
    m_cameraSpeed = 0.05f;
    m_mouseSensitivity = 0.1f;

}

MyOpenGLWidget::~MyOpenGLWidget() {
    makeCurrent();
    delete texture1;
    delete texture2;
    doneCurrent();
}

void MyOpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);

    // Load textures
    texture1 = new QOpenGLTexture(QImage("tiles.jpg").mirrored());
    texture2 = new QOpenGLTexture(QImage(":/tex2.png").mirrored());

    texture1->setMinificationFilter(QOpenGLTexture::Linear);
    texture1->setMagnificationFilter(QOpenGLTexture::Linear);
    texture2->setMinificationFilter(QOpenGLTexture::Linear);
    texture2->setMagnificationFilter(QOpenGLTexture::Linear);

    // Vertex shader
    shader.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                   R"(#version 330 core
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec2 texcoord;
        out vec2 TexCoord;
        uniform mat4 uMvpMatrix;
        void main() {
            gl_Position = uMvpMatrix * vec4(position, 0.0, 1.0);
            TexCoord = texcoord;
        })");

    // Fragment shader
    shader.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                   R"(#version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D u_texture;
        void main() {
            FragColor = texture(u_texture, TexCoord);
        })");

    if (!shader.link()) {
        qDebug() << "Shader link error:" << shader.log();
    }

    // Geometry 1 - Triangle
    float vertices1[] = {
        // Positions        // Texture Coords
        -0.9f, -0.5f, 0.0f, 0.0f, 1.0f,
        -0.1f, -0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.5f, 0.0f
    };

    // Geometry 2 - Triangle
    float vertices2[] = {
        0.1f, -0.5f, 0.0f, 0.0f, 1.0f,
        0.9f, -0.5f, 0.0f, 1.0f, 1.0f,
        0.5f,  0.5f, 0.0f, 0.5f, 0.0f
    };

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

QMatrix4x4 MyOpenGLWidget::getMVP(const QMatrix4x4& model) {
    QMatrix4x4 projection;
    projection.perspective(60.0f, float(width()) / float(height()), 0.1f, 1000.0f);

    QMatrix4x4 view;
    QVector3D front;
    front.setX(-cosf(qDegreesToRadians(m_cameraYaw)) * cosf(qDegreesToRadians(m_cameraPitch)));
    front.setY(-sinf(qDegreesToRadians(m_cameraPitch)));
    front.setZ(-sinf(qDegreesToRadians(m_cameraYaw)) * cosf(qDegreesToRadians(m_cameraPitch)));
    front.setZ(-front.z());
    front.normalize();

    QVector3D cameraTarget = m_cameraPosition - front;
    QVector3D up(0.0f, 1.0f, 0.0f);

    view.lookAt(m_cameraPosition, cameraTarget, up);


    //qDebug() << view;

    return projection * view * model;
}

void MyOpenGLWidget::paintGL() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.bind();

    QMatrix4x4 model;
    model.translate(m_cameraPosition);
    qDebug() << model;
    qDebug() << m_cameraPosition;

    // Set up simple orthographic projection
    QMatrix4x4 projection;
    projection.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    shader.setUniformValue("uMvpMatrix", getMVP(model));

    vbo1.bind();
    shader.enableAttributeArray(0);
    shader.enableAttributeArray(1);
    shader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    shader.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    // Draw first triangle with first texture
    texture1->bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    shader.release();

    handleInput();
}

void MyOpenGLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void MyOpenGLWidget::keyPressEvent(QKeyEvent *event) {
    m_pressedKeys.insert(event->key());
}

void MyOpenGLWidget::keyReleaseEvent(QKeyEvent *event) {
    m_pressedKeys.remove(event->key());
}

void MyOpenGLWidget::handleInput() {
    float forwardSpeed = m_cameraSpeed;
    float rightSpeed = m_cameraSpeed;

    // Convert yaw to radians
    float radYaw = qDegreesToRadians(m_cameraYaw);

    // Calculate the movement direction relative to the camera's yaw
    QVector3D forwardDirection(cos(radYaw), 0.0f, -sin(radYaw));  // Forward direction
    QVector3D rightDirection(sin(radYaw), 0.0f, cos(radYaw));     // Right direction


    // Forward direction based on the camera yaw and pitch
    QVector3D front;
    front.setX(-cosf(qDegreesToRadians(m_cameraYaw)) * cosf(qDegreesToRadians(m_cameraPitch)));
    front.setY(-sinf(qDegreesToRadians(m_cameraPitch)));
    front.setZ(-sinf(qDegreesToRadians(m_cameraYaw)) * cosf(qDegreesToRadians(m_cameraPitch)));
    front.setZ(-front.z());
    front.normalize();

    // Right direction is perpendicular to the forward direction and the "up" vector
    //QVector3D rightDirection = QVector3D::crossProduct(front, QVector3D(0.0f, 1.0f, 0.0f)).normalized();

    // Now move based on keys
    if (m_pressedKeys.contains(Qt::Key_W)) {
        m_cameraPosition -= front * forwardSpeed;  // Move forward
    }

    if (m_pressedKeys.contains(Qt::Key_S)) {
        m_cameraPosition += front * forwardSpeed;  // Move backward
    }

    if (m_pressedKeys.contains(Qt::Key_A)) {
        m_cameraPosition += rightDirection * rightSpeed;  // Move left
    }

    if (m_pressedKeys.contains(Qt::Key_D)) {
        m_cameraPosition -= rightDirection * rightSpeed;  // Move right
    }


    if (m_pressedKeys.contains(Qt::Key_Q) || m_pressedKeys.contains(Qt::Key_Control)) {
        m_cameraPosition += QVector3D(0, -m_cameraSpeed, 0);   // Move down
    }
    if (m_pressedKeys.contains(Qt::Key_E) || m_pressedKeys.contains(Qt::Key_Shift)) {
        m_cameraPosition += QVector3D(0, m_cameraSpeed, 0);    // Move up
    }

}

void MyOpenGLWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        QPoint center = rect().center();  // Get the center of the widget
        QCursor::setPos(mapToGlobal(center));
        m_isDragging = true;
        setCursor(Qt::BlankCursor);  // Hide the cursor during mouse movement
    }
}

void MyOpenGLWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        m_isDragging = false;
        setCursor(Qt::ArrowCursor);  // Show the cursor when released
    }
}

void MyOpenGLWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_isDragging) {
        // Get the movement of the mouse (delta)
        QPoint center = rect().center();  // Get the center of the widget
        QPoint delta = event->pos() - center;  // Calculate movement relative to the center
        m_lastMousePos = event->pos();

        // Update yaw and pitch based on mouse movement
        m_cameraYaw -= delta.x() * m_mouseSensitivity / 10;  // Horizontal movement (yaw)
        m_cameraPitch -= delta.y() * m_mouseSensitivity / 10;  // Vertical movement (pitch)

        // Clamp pitch to prevent flipping
        if (m_cameraPitch > 89.0f) m_cameraPitch = 89.0f;
        if (m_cameraPitch < -89.0f) m_cameraPitch = -89.0f;

        // Normalize yaw to keep it in the 0-360 range
        m_cameraYaw = std::fmod(m_cameraYaw + 360.0f, 360.0f);

        // Reset cursor to the center of the widget
        QCursor::setPos(mapToGlobal(center));
    }
}

void MyOpenGLWidget::makeObject() {
    // Create just two textures
    texture1 = new QOpenGLTexture(QImage(":/tiles.jpg").mirrored());
    texture2 = new QOpenGLTexture(QImage(":/tex2.png").mirrored());

    // Two triangles (6 vertices)
    QList<GLfloat> vertData = {
        // Triangle 1
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f,  // bottom-left
        0.5f, -0.5f, 0.0f,  1.0f, 1.0f,  // bottom-right
        0.0f,  0.5f, 0.0f,  0.5f, 0.0f,  // top-center

        // Triangle 2
        0.1f, -0.5f, 0.0f,  0.0f, 1.0f,
        0.9f, -0.5f, 0.0f,  1.0f, 1.0f,
        0.5f,  0.5f, 0.0f,  0.5f, 0.0f
    };

    vbo1.create();
    vbo1.bind();
    vbo1.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));
}
