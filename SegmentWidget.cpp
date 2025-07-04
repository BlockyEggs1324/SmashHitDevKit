#include "SegmentWidget.h"
#include "MainWindow.h"
#include "TextureLoader.h"
#include "TextureExtractor.h"
#include <GL/glu.h>

// Template function to check if a value is in the container
template <typename T, typename U>
bool contains(const T& container, const U& value) {
    return std::find(container->begin(), container->end(), value) != container->end();
}

float roundToNearest005(float value) {
    return std::round(value / 0.05f) * 0.05f;
}

glm::vec3 toVec3(QVector3D vec) {
    return glm::vec3(vec.x(), vec.y(), vec.z());
}

SegmentWidget::SegmentWidget(QWidget *parent, std::vector<Rect3D> *rects, std::vector<Rect3D*> *selectedRects) : QOpenGLWidget(parent), m_drawWireframe(false), m_drawFaces(true), m_gameView(false), m_drawColour(true), m_cameraPosition(0.0f, 0.0f, 0.0f),
    m_cameraYaw(0.0f), m_cameraPitch(0.0f), m_gameViewPosition(1.0f), m_isDragging(false), m_parent(parent), m_useShader(true), m_selectedRects(selectedRects), m_rects(rects) {
    m_cameraSpeed = 0.1f;
    m_mouseSensitivity = 1.0f;  // Adjust this for faster/slower rotation
    setFocusPolicy(Qt::StrongFocus);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&SegmentWidget::update));
    timer->start(4); // ~250 FPS (1000ms / 250 ≈ 4ms)

    if (m_rects != nullptr) {
        m_rects->push_back(Rect3D(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f));
    }

    m_glToggle = true;
    m_cameraFov = 70.0f;

    //m_roomProgram = createShaderProgram("room");

}

QOpenGLTexture* SegmentWidget::loadTexture(QString filename) {
    QImage image(filename);

    qDebug() << "Loaded texture from image:" << filename;

    if (!context()->isValid()) {
        qWarning() << "OpenGL context is not valid!";
    } else {
        qWarning() << "OpenGL context is valid.";
    }

    int tile = 25;

    int tilesPerRow = 8;
    int tileWidth = 128;
    int tileHeight = 128;

    int col = tile % tilesPerRow;

    int row = tile / tilesPerRow;

    QRect tileRect(col * tileWidth, row * tileHeight, 64, 64);

    QImage newTile = image.copy(tileRect);

    if (newTile.isNull()) {
        qWarning() << "Failed to copy tile from image:" << filename;
        return nullptr;
    }

    tileTex = new QOpenGLTexture(newTile.mirrored());

    tileTex->setMinificationFilter(QOpenGLTexture::Nearest);

    return tileTex;

}

void SegmentWidget::loadTileTexture() {

    if (!QFile("tiles.jpg").exists()) extractMTXFile(m_rootDir + "/gfx/tiles.png.mtx");

    tileTex = loadTexture("tiles.jpg");

}

void SegmentWidget::setRootDir(QString rootDir) {
    m_rootDir = rootDir;
}

void SegmentWidget::setFov(int value) {
    m_cameraFov = value;

    if (context()) resizeGL(width(), height());

}

void SegmentWidget::setSens(float value) {
    m_mouseSensitivity = value;
}

void SegmentWidget::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearDepth(1.0);
    glDepthMask(GL_TRUE); // Ensure depth writing

    qDebug() << "Loading room program!";

    m_roomProgram = createShaderProgram("room");

    qDebug() << "Loading clear program!";
    m_clearProgram = createShaderProgram("clear");
    m_basicProgram = createShaderProgram("basic");

    loadTileTexture();
}

void SegmentWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);  // Set the viewport to the window size
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    projectionMatrix = glm::perspective(glm::radians(m_cameraFov),
                                        (float)w/(float)h,
                                        0.1f,  // Note: matches legacy far plane
                                        1000.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

glm::mat4 SegmentWidget::getViewMatrix() {
    glm::vec3 camPos = toVec3(m_cameraPosition);
    glm::vec3 camFront = glm::normalize(getCameraFront());
    glm::vec3 up(0, 1, 0);
    return glm::lookAt(camPos, camPos + camFront, up);
}

void SegmentWidget::updateProjectionMatrix() {
    float aspect = float(width()) / std::max(1.0f, float(height()));
    projectionMatrix = glm::perspective(glm::radians(m_cameraFov), aspect, 0.1f, 1000.0f);
}

QMatrix4x4 SegmentWidget::getMVP(const QMatrix4x4& model) {
    QMatrix4x4 projection;
    projection.perspective(m_cameraFov, float(width()) / float(height()), 0.1f, 1000.0f);

    QMatrix4x4 view;
    QVector3D front;
    front.setX(-cosf(qDegreesToRadians(m_cameraYaw)) * cosf(qDegreesToRadians(m_cameraPitch)));
    front.setY(-sinf(qDegreesToRadians(m_cameraPitch)));
    front.setZ(-sinf(qDegreesToRadians(m_cameraYaw)) * cosf(qDegreesToRadians(m_cameraPitch)));
    front.setZ(-front.z());
    front.normalize();

    QVector3D cameraTarget = m_cameraPosition - front;
    QVector3D up(0.0f, 1.0f, 0.0f);

    if (m_gameView) {
        QVector3D eye(0, 0, -m_gameViewPosition); // Camera moves along Z
        QVector3D target = eye + QVector3D(0, 0, -1); // Look forward (toward -Z)
        view.lookAt(eye, target, up);
    } else {
        view.lookAt(m_cameraPosition, cameraTarget, up);
    }

    //qDebug() << view;

    return projection * view * model;
}

void SegmentWidget::paintGL() {

    QPainter painter(this);
    painter.beginNativePainting();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearDepth(1.0);
    glDepthMask(GL_TRUE); // Ensure depth writing

    glEnable(GL_TEXTURE_2D);
    //glLoadIdentity();
    glLoadMatrixf(glm::value_ptr(projectionMatrix));

    if (m_useShader) {

        m_model = QMatrix4x4();

        if (m_gameView) m_model.translate(0, -1, m_gameViewPosition);
        else m_model.translate(-m_cameraPosition.x(), -m_cameraPosition.y(), -m_cameraPosition.z());

        //model.rotate(m_cameraYaw)
        //m_model.scale(1.0f, 1.0f, 1.0f);

        //getCameraFront()

        // Setup uniforms:
        QMatrix4x4 mvp = getMVP(m_model);

        // === Render Fog (Clear Pass) ===
        m_clearProgram->bind();
        m_clearProgram->setUniformValue("uMvpMatrix", mvp);
        m_clearProgram->setUniformValue("uLowerFog", QVector4D(lowerFogColour[0], lowerFogColour[1], lowerFogColour[2], lowerFogColour[3]));
        m_clearProgram->setUniformValue("uUpperFog", QVector4D(upperFogColour[0], upperFogColour[1], upperFogColour[2], upperFogColour[3]));

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        // Fullscreen quad setup
        float fullscreenQuad[] = { -1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, 1.f };
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), fullscreenQuad, GL_STATIC_DRAW);

        int posAttrib = m_clearProgram->attributeLocation("aPosition");
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableVertexAttribArray(posAttrib);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDeleteBuffers(1, &vbo);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glDepthMask(GL_TRUE);

        // === Basic Pass ===
        m_basicProgram->bind();

        QVector2D texCoords[] = {
            QVector2D(0.0f, 0.0f), // bottom-left
            QVector2D(1.0f, 0.0f), // bottom-right
            QVector2D(0.0f, 1.0f), // top-left
            QVector2D(1.0f, 1.0f)  // top-right
        };

        int texCoordLocation = m_basicProgram->attributeLocation("aTexCoord");
        m_basicProgram->enableAttributeArray(texCoordLocation);
        m_basicProgram->setAttributeArray(texCoordLocation, GL_FLOAT, texCoords, 2);

        QVector4D color = QVector4D(1.0, 0.0, 1.0, 1.0);

        int colorLoc = m_basicProgram->attributeLocation("aColor");
        m_basicProgram->enableAttributeArray(colorLoc);
        m_basicProgram->setAttributeArray(colorLoc, &color);

        QVector3D vertices[] = {
            {-1, -1, 0}, {1, -1, 0}, {-1, 1, 0}, {1, 1, 1}
        };

        int posLoc = m_basicProgram->attributeLocation("aPosition");
        m_basicProgram->enableAttributeArray(posLoc);
        m_basicProgram->setAttributeArray(posLoc, GL_FLOAT, vertices, 3);

        // === Room Pass ===
        //m_roomProgram->bind();

        //tileTex->bind();

        //m_roomProgram->setUniformValue("uMvpMatrix", mvp);
        //m_roomProgram->setUniformValue("uColor", QVector4D(1, 1, 1, 1)); // White
        //m_roomProgram->setUniformValue("uLowerFog", QVector4D(lowerFogColour[0], lowerFogColour[1], lowerFogColour[2], lowerFogColour[3]));
        //m_roomProgram->setUniformValue("uUpperFog", QVector4D(upperFogColour[0], upperFogColour[1], upperFogColour[2], upperFogColour[3]));
        //m_roomProgram->setUniformValue("uTexture0", 0);

    } else {
        if (m_gameView) {
            glRotatef(0.0f,  1.0f, 0.0f, 0.0f);
            glRotatef(0.0f,   0.0f, 1.0f, 0.0f);
            glTranslatef(0.0f, -1.0f, m_gameViewPosition);
        } else {
            // Apply rotation first
            glRotatef(-m_cameraPitch, 1.0f, 0.0f, 0.0f);  // Pitch (X axis)
            glRotatef(-m_cameraYaw,   0.0f, 1.0f, 0.0f);  // Yaw (Y axis)

            // Then move the scene inversely to simulate camera movement
            glTranslatef(-m_cameraPosition.x(), -m_cameraPosition.y(), -m_cameraPosition.z());
        }
    }



    m_model = QMatrix4x4();

    m_model.translate(-m_cameraPosition.x(), -m_cameraPosition.y(), -m_cameraPosition.z());
    m_model.rotate(-m_cameraPitch, 1.0f, 0.0f, 0.0f);
    m_model.rotate(-m_cameraPitch, 1.0f, 0.0f, 0.0f);

    QMatrix4x4 mvp = getMVP(m_model);

    if (m_drawFaces) {
        // Draw filled cubes
        for (const Rect3D& cube : *m_rects) {
            bool isSelected = contains(m_selectedRects, &cube); // Check if the pointer to cube is in the selected rects
            if (m_useShader) drawCubeNew(cube, isSelected); // Draw filled cubes, depending on whether selected
            else drawCube(cube, isSelected);

            if (isSelected) {
                drawCubeOutline(cube);
            }
        }
    }

    if (m_drawWireframe) {
        // Now draw outlines for the cubes
        for (const Rect3D& cube : *m_rects) {
            drawCubeOutline(cube); // Draw only the outlines
        }
    }

    if (m_drawDebugRay) {
        drawDebugRay();
    }

    // Now draw text

    glUseProgram(0); // Unbind any shader program
    glDisable(GL_TEXTURE_2D); // Disable texture mapping for text rendering
    glDisable(GL_DEPTH_TEST); // Disable depth testing for 2D text rendering

    painter.endNativePainting();
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16));
    painter.drawText(10, 25, "3D View");
    painter.end();

    if (hasFocus()) handleInput();

}

void SegmentWidget::drawDebugRay() {
    if (!m_drawDebugRay) return;

    // Save current state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // Draw thick yellow line
    glLineWidth(3.0f);
    glColor3f(1.0f, 1.0f, 0.0f);

    glBegin(GL_LINES);
    glVertex3f(m_debugRayStart.x, m_debugRayStart.y, m_debugRayStart.z);
    glVertex3f(m_debugRayEnd.x, m_debugRayEnd.y, m_debugRayEnd.z);
    glEnd();

    // Draw small marker at end
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glVertex3f(m_debugRayEnd.x, m_debugRayEnd.y, m_debugRayEnd.z);
    glEnd();

    // Restore state
    glPopAttrib();
}

void SegmentWidget::handleInput() {
    float forwardSpeed = m_cameraSpeed;
    float rightSpeed = m_cameraSpeed;

    // Convert yaw to radians
    float radYaw = qDegreesToRadians(m_cameraYaw);

    // Calculate the movement direction relative to the camera's yaw
    QVector3D forwardDirection(cos(radYaw), 0.0f, -sin(radYaw));  // Forward direction
    QVector3D rightDirection(sin(radYaw), 0.0f, cos(radYaw));     // Right direction

    if (m_gameView) {
        if (m_pressedKeys.contains(Qt::Key_W)) {
            m_gameViewPosition += m_cameraSpeed;  // Move forward
            //qDebug() << "Moving to" << m_gameViewPosition;
        }

        if (m_pressedKeys.contains(Qt::Key_S)) {
            m_gameViewPosition -= m_cameraSpeed;  // Move backwards
            //qDebug() << "Moving to" << m_gameViewPosition;
        }
    } else {

        if (m_useShader) {

            // Forward direction based on the camera yaw and pitch
            QVector3D front;
            front.setX(-cosf(qDegreesToRadians(m_cameraYaw)) * cosf(qDegreesToRadians(m_cameraPitch)));
            front.setY(-sinf(qDegreesToRadians(m_cameraPitch)));
            front.setZ(-sinf(qDegreesToRadians(m_cameraYaw)) * cosf(qDegreesToRadians(m_cameraPitch)));
            front.setZ(-front.z());
            front.normalize();

            // Right direction is perpendicular to the forward direction and the "up" vector
            QVector3D rightDirection = QVector3D::crossProduct(front, QVector3D(0.0f, 1.0f, 0.0f)).normalized();

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

        } else {

            if (m_pressedKeys.contains(Qt::Key_W)) {
                m_cameraPosition -= rightDirection * rightSpeed;  // Move forward
            }

            if (m_pressedKeys.contains(Qt::Key_S)) {
                m_cameraPosition += rightDirection * rightSpeed;  // Move backwards
            }

            if (m_pressedKeys.contains(Qt::Key_A)) {
                m_cameraPosition -= forwardDirection * forwardSpeed;  // Move left
            }

            if (m_pressedKeys.contains(Qt::Key_D)) {
                m_cameraPosition += forwardDirection * forwardSpeed;  // Move right
            }
        }

        if (m_pressedKeys.contains(Qt::Key_Q) || m_pressedKeys.contains(Qt::Key_Control)) {
            m_cameraPosition += QVector3D(0, -m_cameraSpeed, 0);   // Move down
        }
        if (m_pressedKeys.contains(Qt::Key_E) || m_pressedKeys.contains(Qt::Key_Shift)) {
            m_cameraPosition += QVector3D(0, m_cameraSpeed, 0);    // Move up
        }
    }
}

bool SegmentWidget::intersects(const glm::vec3& rayDir, const glm::vec3& rayOrigin, const Rect3D& cube) {
    // Define the AABB bounds
    glm::vec3 cubeMin(cube.x() - cube.width(),
                      cube.y() - cube.height(),
                      cube.z() - cube.depth());
    glm::vec3 cubeMax(cube.x() + cube.width(),
                      cube.y() + cube.height(),
                      cube.z() + cube.depth());

    // Initial t values (entire ray)
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    // Test each axis
    for (int i = 0; i < 3; ++i) {
        if (std::abs(rayDir[i]) < 1e-6) {
            // Ray parallel to this axis - check if origin is outside bounds
            if (rayOrigin[i] < cubeMin[i] || rayOrigin[i] > cubeMax[i]) {
                return false;
            }
        } else {
            // Compute intersection distances
            float invDir = 1.0f / rayDir[i];
            float t1 = (cubeMin[i] - rayOrigin[i]) * invDir;
            float t2 = (cubeMax[i] - rayOrigin[i]) * invDir;

            // Ensure t1 is the nearer intersection
            if (t1 > t2) std::swap(t1, t2);

            // Update global tMin and tMax
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            // Early exit if ray misses
            if (tMin > tMax) return false;
        }
    }

    // If we get here, there's an intersection
    return true;
}

glm::vec3 SegmentWidget::getCameraFront() const {
    glm::vec3 front;
    front.x = cos(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    front.y = sin(glm::radians(m_cameraPitch));
    front.z = sin(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    return glm::normalize(front);
}

QVector3D SegmentWidget::getCameraForward() {
    QVector3D forward;
    forward.setX(cos(qDegreesToRadians(m_cameraYaw)) * cos(qDegreesToRadians(m_cameraPitch)));
    forward.setY(sin(qDegreesToRadians(m_cameraPitch)));
    forward.setZ(sin(qDegreesToRadians(m_cameraYaw)) * cos(qDegreesToRadians(m_cameraPitch)));
    return forward.normalized();
}

int SegmentWidget::getSelectedIndex(Rect3D rect) {
    for (size_t i = 0; i < m_rects->size(); i++) {
        if (m_rects->at(i) == rect) {
            return i;
        }
    } return 0;
}

void SegmentWidget::ScreenPosToWorldRay(
    int mouseX, int mouseY,
    int screenWidth, int screenHeight,
    const glm::mat4& ViewMatrix,
    const glm::mat4& ProjectionMatrix,
    glm::vec3& out_origin,
    glm::vec3& out_direction) {

    /*
    QPoint pos = mapFromGlobal(QPoint(mouseX, mouseY));

    mouseX = pos.x();
    mouseY = pos.y();
    */

    // 1. Convert to Normalized Device Coordinates (with Y flip)
    const float ndcX = ((2.0f * mouseX) / screenWidth) - 1.0f;
    const float ndcY = 1.0f - ((2.0f * mouseY) / screenHeight); // Correct Y flip

    // 2. Create ray in clip space (near plane)
    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);

    // 3. Convert to eye space (camera space)
    glm::mat4 invProj = glm::inverse(ProjectionMatrix);
    glm::vec4 rayEye = invProj * rayClip;
    rayEye.z = -1.0f;  // Forward direction in OpenGL
    rayEye.w = 0.0f;   // Direction vector

    // 4. Convert to world space
    glm::mat4 invView = glm::inverse(ViewMatrix);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(invView * rayEye));

    // 5. Set outputs
    out_direction = rayWorld;
    out_origin = glm::vec3(invView[3]); // Extract camera position
}

glm::vec3 SegmentWidget::getRayFromMouse(int mouseX, int mouseY) {
    // Convert mouse coordinates to normalized device coordinates
    float x = (2.0f * mouseX) / width() - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / height();

    // Create ray in clip space
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    viewMatrix = getViewMatrix();

    // Transform to eye space
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // Transform to world space
    glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix) * rayEye);
    return glm::normalize(rayWorld);
}

glm::mat4 SegmentWidget::computeCubeTransform(const Rect3D& cube) {
    // 1. Create translation matrix (position)
    glm::mat4 translation = glm::translate(glm::mat4(1.0f),
                                           glm::vec3(cube.x(), cube.y(), cube.z()));

    // 2. Create scale matrix (dimensions)
    glm::mat4 scale = glm::scale(glm::mat4(1.0f),
                                 glm::vec3(cube.width() * 0.5f,
                                           cube.height() * 0.5f,
                                           cube.depth() * 0.5f));

    // 3. Combine transformations (scale first, then translate)
    return translation * scale;
}

glm::mat4 toMat4(const QMatrix4x4& mat) {
    glm::mat4 result;

    // QMatrix4x4 is row-major, glm::mat4 is column-major
    // But OpenGL expects column-major, so we transpose while copying
    const float* data = mat.constData(); // returns pointer to 16 floats (row-major)

    // Transpose into glm::mat4
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result[col][row] = data[row * 4 + col]; // transpose
        }
    }

    return result;
}

QMatrix4x4 fromMat4(const glm::mat4& mat) {
    QMatrix4x4 result;

    const float* data = glm::value_ptr(mat); // glm::mat4 stores data column-major

    // Transpose to match QMatrix4x4 (row-major)
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result(row, col) = data[col * 4 + row]; // transpose
        }
    }

    return result;
}

void qMatrix4x4ToDoubleArray(const QMatrix4x4& mat, double* out) {
    const float* data = mat.constData();
    for (int i = 0; i < 16; ++i)
        out[i] = static_cast<double>(data[i]);
}

void glmMat4ToDoubleArray(const glm::mat4& mat, double* out)
{
    // glm is column-major, just like OpenGL expects
    for (int i = 0; i < 16; ++i)
        out[i] = static_cast<double>(glm::value_ptr(mat)[i]);
}

QVector3D unprojectRay(const QMatrix4x4 &viewMatrix,
                       const QMatrix4x4 &projectionMatrix,
                       int viewPortWidth, int viewPortHeight,
                       const QPoint &pos) {

    float x = ((2.0f * pos.x()) / viewPortWidth) - 1.0f;
    float y = 1.0f - ((2.0f * pos.y()) / viewPortHeight);

    // Figure out the ray to the screen position
    QVector4D ray = projectionMatrix.inverted() * QVector4D(x, y, 1.0f, 1.0f);
    ray.setZ(-1.0f);
    ray.setW(0.0f);
    ray = viewMatrix.inverted() * ray;
    return ray.toVector3D().normalized();
}

void DeprojectScreenToWorld(
    const glm::vec2& screenPos,
    const glm::ivec4& viewRect, // x, y, width, height
    const glm::mat4& invViewProjMatrix,
    glm::vec3& outWorldOrigin,
    glm::vec3& outWorldDirection)
{
    float pixelX = std::floor(screenPos.x);
    float pixelY = std::floor(screenPos.y);

    // Normalize to [0, 1] range in viewport
    float normalizedX = (pixelX - viewRect.x) / static_cast<float>(viewRect.z);
    float normalizedY = (pixelY - viewRect.y) / static_cast<float>(viewRect.w);

    // Convert to clip space (-1 to 1)
    float clipX = (normalizedX - 0.5f) * 2.0f;
    float clipY = ((1.0f - normalizedY) - 0.5f) * 2.0f;

    glm::vec4 rayStartNDC(clipX, clipY, 1.0f, 1.0f);
    glm::vec4 rayEndNDC(clipX, clipY, 0.01f, 1.0f);

    glm::vec4 rayStartWorld = invViewProjMatrix * rayStartNDC;
    glm::vec4 rayEndWorld = invViewProjMatrix * rayEndNDC;

    if (rayStartWorld.w != 0.0f) rayStartWorld /= rayStartWorld.w;
    if (rayEndWorld.w != 0.0f) rayEndWorld /= rayEndWorld.w;

    glm::vec3 rayStart(rayStartWorld);
    glm::vec3 rayEnd(rayEndWorld);

    glm::vec3 rayDir = glm::normalize(rayEnd - rayStart);

    outWorldOrigin = rayStart;
    outWorldDirection = rayDir;
}

void SegmentWidget::selectCube(const QPoint& mousePos) {

    float x = (2.0f * mousePos.x()) / width() - 1.0f;
    float y = 1.0f - (2.0f * mousePos.y()) / height();
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    viewMatrix = toMat4(getMVP(m_model));

    glm::vec4 rayWorld = glm::inverse(viewMatrix) * rayEye;
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorld));

    glm::vec3 cameraPos = glm::vec3(glm::inverse(viewMatrix)[3]);

    cameraPos = toVec3(m_cameraPosition);

    if (m_useShader) m_debugRayStart = cameraPos * 2.0f;
    else m_debugRayStart = cameraPos;

    auto vm = fromMat4(viewMatrix);
    auto pm = fromMat4(projectionMatrix);

    QVector3D rayDire = unprojectRay(vm, pm, width(), height(), mousePos);

    glm::vec4 viewport = {0, 0, width(), height()};
    glm::vec3 pos = glm::unProject({mousePos.x(), height()-mousePos.y(), 1}, toMat4(getMVP(m_model)), projectionMatrix, viewport);


    m_debugRayEnd = cameraPos + pos * 100.0f;

    //rayDir = toVec3(rayDire);

    //m_debugRayEnd = cameraPos + rayDir * 100.0f;

    //m_debugRayDir = rayDir;

    const glm::vec2 screenPos(mousePos.x(), height() - mousePos.y());
    const glm::ivec4 view(0, 0, width(), height());
    //DeprojectScreenToWorld(screenPos, view, glm::inverse(projectionMatrix * viewMatrix), cameraPos, m_debugRayDir);

    //glm::vec4 viewport(0, 0, width(), height());

    glm::vec3 rayStartNDC = glm::unProject(glm::vec3(screenPos, 0.1f), viewMatrix, projectionMatrix, viewport); // Near plane
    glm::vec3 rayEndNDC   = glm::unProject(glm::vec3(screenPos, 1000.0f), viewMatrix, projectionMatrix, viewport); // Far plane

    // Create ray
    glm::vec3 rayOrigin = rayStartNDC;
    glm::vec3 rayDirection = glm::normalize(rayEndNDC - rayStartNDC);

    //m_debugRayStart = rayOrigin * 2.0f;
    m_debugRayDir = rayDirection;
    //m_debugRayEnd = rayOrigin + rayDirection * 100.0f;

    //glm::unProject(glm::vec3(screenPos, 0.2f), m_model, projectionMatrix, viewport);

    /*

    int x = mousePos.x();
    int y = mousePos.y();

    // Convert mouse position to normalized device coordinates (NDC)
    float ndcX = (2.0f * x) / width() - 1.0f;  // Normalize X to [-1, 1]
    float ndcY = 1.0f - (2.0f * y) / height(); // Normalize Y to [-1, 1] and flip Y

    // Use FOV to adjust the ray's scale. (For example, let's assume FOV is 45 degrees)
    float fov = m_cameraFov;  // Field of view in degrees (adjust as necessary)
    float aspectRatio = (float)width() / height();
    float fovRad = glm::radians(fov);  // Convert to radians

    // Calculate the scaling factor based on the FOV and the aspect ratio
    float tanFovHalf = tan(fovRad / 2.0f);
    float scaleX = tanFovHalf * aspectRatio;
    float scaleY = tanFovHalf;

    // Adjust the ray direction based on the FOV scaling
    QVector3D rayDirectionInScreenSpace(ndcX * scaleX, ndcY * scaleY, -1.0f);
    rayDirectionInScreenSpace = rayDirectionInScreenSpace.normalized();  // Normalize the direction

    // Apply yaw and pitch rotation to the ray direction based on camera orientation
    QMatrix4x4 rotationMatrix;

    // Apply rotations only around the Z and X axes for yaw and pitch respectively
    rotationMatrix.rotate(m_cameraPitch, QVector3D(1.0f, 0.0f, 0.0f));  // Pitch rotation (around X-axis)
    rotationMatrix.rotate(m_cameraYaw - 90.0f, QVector3D(0.0f, 1.0f, 0.0f));    // Yaw rotation (around Y-axis)

    // Apply the rotation to the ray direction
    QVector3D rayDirectionInWorldSpace = rotationMatrix.map(rayDirectionInScreenSpace);

    // Normalize the final direction (make sure it's unit length)
    rayDirectionInWorldSpace = rayDirectionInWorldSpace.normalized();

    // Now that the ray is adjusted, set the ray origin (the camera position)
    glm::vec3 rayOrigin = toVec3(m_cameraPosition);

    // Now the ray direction in world space is ready to be used
    glm::vec3 rayDirection = toVec3(rayDirectionInWorldSpace);  // Convert to your custom type if necessary


    m_debugRayStart = cameraPos * 2.0f;

    m_debugRayEnd = m_debugRayStart + rayDirection * 20.0f;

    */

    m_drawDebugRay = true;

    Rect3D* selectedCube = nullptr;
    bool found = false;

    m_selectedRects->clear();

    // 4. Test intersections
    for(Rect3D& cube : *m_rects) {
        if (intersects(m_debugRayDir, m_debugRayStart, cube)) {
            m_selectedRects->push_back(&cube);
            selectedCube = &cube;
            found = true;
            break;
        }
    }

    auto window = qobject_cast<MainWindow*>(m_parent);
    window->update2D();

    if (found && selectedCube) {
        qDebug() << "Selected cube at: ("
                 << selectedCube->x() << ", "
                 << selectedCube->y() << ", "
                 << selectedCube->z() << ")";
    } else {
        qDebug() << "No cube selected";
    }
}

void SegmentWidget::keyPressEvent(QKeyEvent *event) {
    m_pressedKeys.insert(event->key());

    if (event->key() == Qt::Key_F1) {
        m_drawFaces = !m_drawFaces;
        auto window = qobject_cast<MainWindow*>(m_parent);
        window->toggleFacesButton->setChecked(m_drawFaces);
    }
    if (event->key() == Qt::Key_F2) {
        m_drawWireframe = !m_drawWireframe;
        auto window = qobject_cast<MainWindow*>(m_parent);
        window->toggleWireframeButton->setChecked(m_drawWireframe);
    }
    if (event->key() == Qt::Key_F3) {
        m_drawColour = !m_drawColour;
        auto window = qobject_cast<MainWindow*>(m_parent);
        window->toggleColoured->setChecked(m_drawColour);
    }
    if (event->key() == Qt::Key_F6) {
        m_gameView = !m_gameView;
        auto window = qobject_cast<MainWindow*>(m_parent);
        window->toggleGameView->setChecked(m_gameView);
    }
    if (event->key() == Qt::Key_F7) {
        if (!m_rects->empty()) {

            const Rect3D* minZCube = nullptr;
            const Rect3D* maxZCube = nullptr;

            for (const auto& cube : *m_rects) {
                if (!minZCube || cube.position().z() < minZCube->position().z()) {
                    minZCube = &cube;
                }
                if (!maxZCube || cube.position().z() > maxZCube->position().z()) {
                    maxZCube = &cube;
                }
            }

            if (minZCube && maxZCube) {
                float zDistance = std::abs(maxZCube->position().z() - minZCube->position().z());

                m_cameraSpeed = zDistance / (30.0f * 240.0f);
                qDebug() << "Distance along Z axis between furthest and closest cubes:" << zDistance;
            }

        }

    }
    if (event->key() == Qt::Key_F8) {
        if (m_gameView) {
            m_gameViewPosition = 0.0f;
        } else {
            m_cameraPosition = QVector3D(0.0f, 0.0f, 0.0f);
            m_cameraYaw = 0.0f;
            m_cameraPitch = 0.0f;
        }
        
    }
}

void SegmentWidget::keyReleaseEvent(QKeyEvent *event) {
    m_pressedKeys.remove(event->key());
}

void SegmentWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPoint mousePos = event->pos(); // Get mouse position
        selectCube(mousePos); // Call function to select cube
    }
    if (event->button() == Qt::RightButton) {
        QPoint center = rect().center();  // Get the center of the widget
        QCursor::setPos(mapToGlobal(center));
        m_isDragging = true;
        setCursor(Qt::BlankCursor);  // Hide the cursor during mouse movement
    }
}

void SegmentWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        m_isDragging = false;
        setCursor(Qt::ArrowCursor);  // Show the cursor when released
    }
}

void SegmentWidget::wheelEvent(QWheelEvent *event) {
    // Get the amount of scrolling (delta)
    int delta = event->angleDelta().y();  // y() gives vertical scroll, x() for horizontal

    // Depending on the scroll direction, do something
    if (delta > 0) {
        if (m_cameraSpeed <= 0.06f) {
            m_cameraSpeed = 0.05f;
        } else if (m_cameraSpeed <= 10.0f) {
            m_cameraSpeed = roundToNearest005(m_cameraSpeed + 0.05f);
        }
    } else if (delta < 0) {
        if (m_cameraSpeed >= 0.04f) {
            m_cameraSpeed = roundToNearest005(m_cameraSpeed - 0.05f);
        } else {
            m_cameraSpeed = 0.01f;
        }
    }

    // Optionally, you can process the event to prevent further handling
    event->accept();  // This marks the event as handled
}

void SegmentWidget::mouseMoveEvent(QMouseEvent *event) {
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

QOpenGLShaderProgram *SegmentWidget::createShaderProgram(const QString& name) {
    QOpenGLShaderProgram *program = new QOpenGLShaderProgram();

    QString vertPath = QString(":/res/shaders/%1.vert").arg(name);
    QString fragPath = QString(":/res/shaders/%1.frag").arg(name);

    if (!program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertPath))
        qWarning() << "Vertex shader failed:" << program->log();

    if (!program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragPath))
        qWarning() << "Fragment shader failed:" << program->log();

    if (!program->link())
        qWarning() << "Shader linking failed:" << program->log();

    return program;
}

glm::vec3 operator+(const QVector3D& left, const glm::vec3& right) {
    glm::vec3 newVec = toVec3(left);
    return newVec + right;
}

void SegmentWidget::drawCubeNew(const Rect3D& rect, bool selected) {
    glm::vec3 p1 = rect.position() + glm::vec3(-rect.size().x(), -rect.size().y(), -rect.size().z());
    glm::vec3 p2 = rect.position() + glm::vec3( rect.size().x(), -rect.size().y(), -rect.size().z());
    glm::vec3 p3 = rect.position() + glm::vec3( rect.size().x(),  rect.size().y(), -rect.size().z());
    glm::vec3 p4 = rect.position() + glm::vec3(-rect.size().x(),  rect.size().y(), -rect.size().z());
    glm::vec3 p5 = rect.position() + glm::vec3(-rect.size().x(), -rect.size().y(),  rect.size().z());
    glm::vec3 p6 = rect.position() + glm::vec3( rect.size().x(), -rect.size().y(),  rect.size().z());
    glm::vec3 p7 = rect.position() + glm::vec3( rect.size().x(),  rect.size().y(),  rect.size().z());
    glm::vec3 p8 = rect.position() + glm::vec3(-rect.size().x(),  rect.size().y(),  rect.size().z());

    // Each face has 6 vertices (2 triangles) with position, color, and texture coordinates
    // Winding order is COUNTER-CLOCKWISE when viewed from outside
    GLfloat vertices[] = {

        // Front face (p1, p2, p3, p1, p3, p4) - Z-
        p1.x, p1.y, p1.z,  0.0f, 0.0f,
        p2.x, p2.y, p2.z,  1.0f, 0.0f,
        p3.x, p3.y, p3.z,  1.0f, 1.0f,
        p1.x, p1.y, p1.z,  0.0f, 0.0f,
        p3.x, p3.y, p3.z,  1.0f, 1.0f,
        p4.x, p4.y, p4.z,  0.0f, 1.0f,

        // Back face (p6, p5, p8, p6, p8, p7) - Z+
        p6.x, p6.y, p6.z,  0.0f, 0.0f,
        p5.x, p5.y, p5.z,  1.0f, 0.0f,
        p8.x, p8.y, p8.z,  1.0f, 1.0f,
        p6.x, p6.y, p6.z,  0.0f, 0.0f,
        p8.x, p8.y, p8.z,  1.0f, 1.0f,
        p7.x, p7.y, p7.z,  0.0f, 1.0f,

        // Left face (p5, p1, p4, p5, p4, p8) - X-
        p5.x, p5.y, p5.z,  0.0f, 0.0f,
        p1.x, p1.y, p1.z,  1.0f, 0.0f,
        p4.x, p4.y, p4.z,  1.0f, 1.0f,
        p5.x, p5.y, p5.z,  0.0f, 0.0f,
        p4.x, p4.y, p4.z,  1.0f, 1.0f,
        p8.x, p8.y, p8.z,  0.0f, 1.0f,

        // Right face (p2, p6, p7, p2, p7, p3) - X+
        p2.x, p2.y, p2.z,  0.0f, 0.0f,
        p6.x, p6.y, p6.z,  1.0f, 0.0f,
        p7.x, p7.y, p7.z,  1.0f, 1.0f,
        p2.x, p2.y, p2.z,  0.0f, 0.0f,
        p7.x, p7.y, p7.z,  1.0f, 1.0f,
        p3.x, p3.y, p3.z,  0.0f, 1.0f,

        // Top face (p4, p3, p7, p4, p7, p8) - Y+
        p4.x, p4.y, p4.z,  0.0f, 0.0f,
        p3.x, p3.y, p3.z,  1.0f, 0.0f,
        p7.x, p7.y, p7.z,  1.0f, 1.0f,
        p4.x, p4.y, p4.z,  0.0f, 0.0f,
        p7.x, p7.y, p7.z,  1.0f, 1.0f,
        p8.x, p8.y, p8.z,  0.0f, 1.0f,

        // Bottom face (p1, p5, p6, p1, p6, p2) - Y-
        p1.x, p1.y, p1.z,  0.0f, 0.0f,
        p5.x, p5.y, p5.z,  1.0f, 0.0f,
        p6.x, p6.y, p6.z,  1.0f, 1.0f,
        p1.x, p1.y, p1.z,  0.0f, 0.0f,
        p6.x, p6.y, p6.z,  1.0f, 1.0f,
        p2.x, p2.y, p2.z,  0.0f, 1.0f

    };

    m_model = QMatrix4x4();

    if (m_gameView) m_model.translate(0, -1, m_gameViewPosition);
    else m_model.translate(-m_cameraPosition.x(), -m_cameraPosition.y(), -m_cameraPosition.z());
        
    QMatrix4x4 mvp = getMVP(m_model);

    m_basicProgram->setUniformValue("uMvpMatrix", mvp);
    m_basicProgram->setUniformValue("uLowerFog", QVector4D(lowerFogColour[0], lowerFogColour[1], lowerFogColour[2], lowerFogColour[3]));
    m_basicProgram->setUniformValue("uUpperFog", QVector4D(upperFogColour[0], upperFogColour[1], upperFogColour[2], upperFogColour[3]));
    m_basicProgram->setUniformValue("uIsSelected", selected);
    m_basicProgram->setUniformValue("uTexture0", 0);

    m_basicProgram->setAttributeValue("aColor", rect.getColourVector());

    GLuint color = m_basicProgram->attributeLocation("aColor");
    GLuint position = m_basicProgram->attributeLocation("aPosition");
    GLuint texCoord = m_basicProgram->attributeLocation("aTexCoord");

    glActiveTexture(GL_TEXTURE0);
    tileTex->bind();

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //m_basicProgram->enableAttributeArray(color);
    //m_basicProgram->setAttributeBuffer(color, GL_FLOAT, 0, 4, 5 * sizeof(GLfloat)); // color (now 4 floats)
    
    m_basicProgram->enableAttributeArray(position);
    m_basicProgram->setAttributeBuffer(position, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat)); // position
    
    m_basicProgram->enableAttributeArray(texCoord);
    m_basicProgram->setAttributeBuffer(texCoord, GL_FLOAT, 0, 2, 5 * sizeof(GLfloat)); // texcoord

    // Position attribute
    //glEnableVertexAttribArray(position);
    //glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);           // position
    

    // Color attribute
    //glVertexAttribPointer(color, 4, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); // color (now 4 floats)
    //glEnableVertexAttribArray(1);

    //glEnableVertexAttribArray(color);
    //m_basicProgram->setAttributeValue("aColor", rect.getColourVector());

    // Texture coordinate attribute
    //glEnableVertexAttribArray(texCoord);
    //glVertexAttribPointer(texCoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(7 * sizeof(GLfloat))); // texcoord

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); 
    glFrontFace(GL_CCW);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36); // 6 faces × 6 vertices = 36 vertices

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    
}

void SegmentWidget::drawCube(const Rect3D& cubeRect, bool selected) {
    float x0 = cubeRect.x() - cubeRect.width();
    float x1 = cubeRect.x() + cubeRect.width();
    float y0 = cubeRect.y() - cubeRect.height();
    float y1 = cubeRect.y() + cubeRect.height();
    float z0 = cubeRect.z() - cubeRect.depth();
    float z1 = cubeRect.z() + cubeRect.depth();

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);

    glBegin(GL_QUADS);

    if (!selected) {

        if (m_drawColour) {

            auto colour = cubeRect.getColour();

            glColor3f(colour[0], colour[1], colour[2]);

            glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
            glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
            glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
            glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
            glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
            glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
        } else {
            glColor3f(1, 0, 0); glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
            glColor3f(0, 1, 0); glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
            glColor3f(0, 0, 1); glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
            glColor3f(1, 1, 0); glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
            glColor3f(1, 0, 1); glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
            glColor3f(0, 1, 1); glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
        }

    } else {
        glColor3f(1.0, 0.0, 0.0);

        glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
        glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
        glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
        glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
        glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
        glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
    }

    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Draw grid lines (if m_drawColour is true)
    if (m_drawColour) {

        auto colour = cubeRect.getColour();
        glColor3f(
            colour[0] * 0.7f,
            colour[1] * 0.7f,
            colour[2] * 0.7f
            );

        glLineWidth(2.0f);  // <-- Make the grid lines thicker (default is 1.0)

        glBegin(GL_LINES);

        // Grid on Front and Back faces (XY plane at Z = z1 and Z = z0)
        for (float x = std::ceil(x0); x <= x1; x += 1.0f) {
            glVertex3f(x, y0, z1); glVertex3f(x, y1, z1);  // Front
            glVertex3f(x, y0, z0); glVertex3f(x, y1, z0);  // Back
        }
        for (float y = std::ceil(y0); y <= y1; y += 1.0f) {
            glVertex3f(x0, y, z1); glVertex3f(x1, y, z1);  // Front
            glVertex3f(x0, y, z0); glVertex3f(x1, y, z0);  // Back
        }

        // Grid on Left and Right faces (YZ plane at X = x0 and X = x1)
        for (float y = std::ceil(y0); y <= y1; y += 1.0f) {
            glVertex3f(x0, y, z0); glVertex3f(x0, y, z1);  // Left
            glVertex3f(x1, y, z0); glVertex3f(x1, y, z1);  // Right
        }
        for (float z = std::ceil(z0); z <= z1; z += 1.0f) {
            glVertex3f(x0, y0, z); glVertex3f(x0, y1, z);  // Left
            glVertex3f(x1, y0, z); glVertex3f(x1, y1, z);  // Right
        }

        // Grid on Top and Bottom faces (XZ plane at Y = y1 and Y = y0)
        for (float x = std::ceil(x0); x <= x1; x += 1.0f) {
            glVertex3f(x, y1, z0); glVertex3f(x, y1, z1);  // Top
            glVertex3f(x, y0, z0); glVertex3f(x, y0, z1);  // Bottom
        }
        for (float z = std::ceil(z0); z <= z1; z += 1.0f) {
            glVertex3f(x0, y1, z); glVertex3f(x1, y1, z);  // Top
            glVertex3f(x0, y0, z); glVertex3f(x1, y0, z);  // Bottom
        }

        glEnd();
    }
}


void SegmentWidget::drawCubeOutline(const Rect3D& cubeRect) {
    float x0 = cubeRect.x() - cubeRect.width();
    float x1 = cubeRect.x() + cubeRect.width();
    float y0 = cubeRect.y() - cubeRect.height();
    float y1 = cubeRect.y() + cubeRect.height();
    float z0 = cubeRect.z() - cubeRect.depth();
    float z1 = cubeRect.z() + cubeRect.depth();

    glLineWidth(1.0f);

    // Set line color for the outline
    glBegin(GL_LINES);

    glColor3f(1.0f, 1.0, 0.0f); // Yellow for the outline

    // Front face edges
    glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1);
    glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z1); glVertex3f(x0, y0, z1);

    // Back face edges
    glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
    glVertex3f(x0, y1, z0); glVertex3f(x0, y0, z0);

    // Connecting edges between front and back faces
    glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1);
    glVertex3f(x0, y1, z0); glVertex3f(x0, y1, z1);
    glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);

    glEnd();
}
