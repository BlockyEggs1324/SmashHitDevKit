#include "SegmentWidget.h"
#include "mainwindow.h"

// Template function to check if a value is in the container
template <typename T, typename U>
bool contains(const T& container, const U& value) {
    return std::find(container->begin(), container->end(), value) != container->end();
}

SegmentWidget::SegmentWidget(QWidget *parent, std::vector<Rect3D> *rects, std::vector<Rect3D*> *selectedRects) : QOpenGLWidget(parent), m_drawWireframe(false), m_drawFaces(true), m_gameView(false), m_drawColour(true), m_cameraPosition(0.0f, 0.0f, 5.0f),
    m_cameraYaw(0.0f), m_cameraPitch(0.0f), m_gameViewPosition(1.0f), m_isDragging(false), m_parent(parent), m_useShader(true), m_selectedRects(selectedRects), m_rects(rects) {
    m_cameraSpeed = 0.1f;
    m_mouseSensitivity = 0.1f;  // Adjust this for faster/slower rotation
    setFocusPolicy(Qt::StrongFocus);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&SegmentWidget::update));
    timer->start(4); // ~240 FPS (1000ms / 240 ≈ 4ms)

    if (m_rects != nullptr) {
        m_rects->push_back(Rect3D(10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f));
    }

    m_glToggle = true;
    m_cameraFov = 70.0f;    
}

void SegmentWidget::setFov(int value) {
    m_cameraFov = value;

    resizeGL(rect().width(), rect().height());
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

    m_shaderProgram = createShaderProgram("fog.vert", "fog.frag");
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
        view.lookAt(QVector3D(0, -1, m_gameViewPosition), // Camera moves
                    QVector3D(0, -1, 0),                   // Target stays fixed at Z=0
                    up);
    } else {
        view.lookAt(m_cameraPosition, cameraTarget, up);
    }

    return projection * view * model;
}

void SegmentWidget::drawFogOverlay() {
    float distance = 1000.0f; // Half size (cube is 2000x2000x2000 around camera)

    float x = m_cameraPosition.x();
    float y = m_cameraPosition.y();
    float z = m_cameraPosition.z();

    float half = distance / 2.0f;

    // Each face will be a thin box (very small thickness)

    // Positive X face
    Rect3D x1(
        QVector3D(x + half, y, z),
        QVector3D(0.1f, distance, distance) // thin in X, full in YZ
    );

    // Negative X face
    Rect3D x2(
        QVector3D(x - half, y, z),
        QVector3D(0.1f, distance, distance)
    );

    // Positive Y face
    Rect3D y1(
        QVector3D(x, y + half, z),
        QVector3D(distance, 0.1f, distance) // thin in Y, full in XZ
     );

    // Negative Y face
    Rect3D y2(
        QVector3D(x, y - half, z),
        QVector3D(distance, 0.1f, distance)
    );

    // Positive Z face
    Rect3D z1(
        QVector3D(x, y, z + half),
        QVector3D(distance, distance, 0.1f) // thin in Z, full in XY
    );

    // Negative Z face
    Rect3D z2(
        QVector3D(x, y, z - half),
        QVector3D(distance, distance, 0.1f)
    );

    drawCubeSpecial(x1);
    drawCubeSpecial(x2);
    drawCubeSpecial(y1);
    drawCubeSpecial(y2);
    drawCubeSpecial(z1);
    drawCubeSpecial(z2);

}

void SegmentWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    //glLoadIdentity();
    glLoadMatrixf(glm::value_ptr(projectionMatrix));

    if (m_useShader) {

        glUseProgram(m_shaderProgram);

        QMatrix4x4 model;

        if (m_gameView) model.translate(0, -1, m_gameViewPosition);
        else model.translate(-m_cameraPosition.x(), -m_cameraPosition.y(), -m_cameraPosition.z());

        //model.rotate(m_cameraYaw)
        model.scale(1.0f, 1.0f, 1.0f);

        //getCameraFront()

        // Setup uniforms:
        QMatrix4x4 mvp = getMVP(model); // however you compute this
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uMvpMatrix"), 1, GL_FALSE, mvp.constData());

        glUniform4f(glGetUniformLocation(m_shaderProgram, "uLowerFog"), lowerFogColour[0], lowerFogColour[1], lowerFogColour[2], lowerFogColour[3]); // your lower fog color
        glUniform4f(glGetUniformLocation(m_shaderProgram, "uUpperFog"), upperFogColour[0], upperFogColour[1], upperFogColour[2], upperFogColour[3]); // your upper fog color

        GLuint yourTextureID;
        glGenTextures(1, &yourTextureID);
        glBindTexture(GL_TEXTURE_2D, yourTextureID);

        // Create a single white pixel
        uint8_t whitePixel[4] = { 255, 255, 0, 255 };

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Bind your texture to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, yourTextureID);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "uTexture0"), 0);

        //drawFogOverlay();

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



    if (m_drawFaces) {
        // Draw filled cubes
        for (const Rect3D& cube : *m_rects) {
            bool isSelected = contains(m_selectedRects, &cube); // Check if the pointer to cube is in the selected rects
            drawCube(cube, isSelected); // Draw filled cubes, depending on whether selected
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

    QPainter painter(this);
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
            qDebug() << "Moving";
        }

        if (m_pressedKeys.contains(Qt::Key_S)) {
            m_gameViewPosition -= m_cameraSpeed;  // Move backwards
            qDebug() << "Moving";
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


glm::vec3 SegmentWidget::toVec3(QVector3D vec) const {
    return glm::vec3(vec.x(), vec.y(), vec.z());
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

void SegmentWidget::selectCube(const QPoint& mousePos) {

    int x = mousePos.x();
    int y = mousePos.y();

    // Convert mouse position to normalized device coordinates (NDC)
    float ndcX = (2.0f * x) / width() - 1.0f;  // Normalize X to [-1, 1]
    float ndcY = 1.0f - (2.0f * y) / height(); // Normalize Y to [-1, 1] and flip Y

    // Use FOV to adjust the ray's scale. (For example, let's assume FOV is 45 degrees)
    float fov = 45.0f;  // Field of view in degrees (adjust as necessary)
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

    m_debugRayStart = rayOrigin;
    m_debugRayEnd = rayOrigin + rayDirection * 100.0f;
    m_drawDebugRay = true;

    Rect3D* selectedCube = nullptr;
    bool found = false;

    m_selectedRects->clear();

    // 4. Test intersections
    for(Rect3D& cube : *m_rects) {
        if (intersects(rayDirection, rayOrigin, cube)) {
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
    if (event->key() == Qt::Key_F8) {
        m_gameViewPosition = 0.0f;
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
        if (m_cameraSpeed <= 0.05f) {
            m_cameraSpeed = 0.05f;
        } else if (m_cameraSpeed <= 10.0f) {
            m_cameraSpeed += 0.05f;
        }
    } else if (delta < 0) {
        if (m_cameraSpeed >= 0.05f) {
            m_cameraSpeed -= 0.05f;
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
        m_cameraYaw -= delta.x() * m_mouseSensitivity;  // Horizontal movement (yaw)
        m_cameraPitch -= delta.y() * m_mouseSensitivity;  // Vertical movement (pitch)

        // Clamp pitch to prevent flipping
        if (m_cameraPitch > 89.0f) m_cameraPitch = 89.0f;
        if (m_cameraPitch < -89.0f) m_cameraPitch = -89.0f;

        // Normalize yaw to keep it in the 0-360 range
        m_cameraYaw = std::fmod(m_cameraYaw + 360.0f, 360.0f);

        // Reset cursor to the center of the widget
        QCursor::setPos(mapToGlobal(center));

        update();  // Request a repaint after camera rotation
    }
}

GLuint SegmentWidget::loadShaderFromFile(const QString& path, GLenum type) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Shader file not found:" << path;
        return 0;
    }

    QByteArray shaderSource = file.readAll();
    const char* src = shaderSource.constData();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        qDebug() << "Shader compile error:" << log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint SegmentWidget::createShaderProgram(const QString& vertexPath, const QString& fragmentPath) {
    GLuint vertexShader = loadShaderFromFile(vertexPath, GL_VERTEX_SHADER);
    GLuint fragmentShader = loadShaderFromFile(fragmentPath, GL_FRAGMENT_SHADER);
    if (!vertexShader || !fragmentShader) return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        qDebug() << "Shader link error:" << log;
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

void SegmentWidget::drawCubeSpecial(const Rect3D& cubeRect) {

    float x0 = cubeRect.x() - cubeRect.width();
    float x1 = cubeRect.x() + cubeRect.width();
    float y0 = cubeRect.y() - cubeRect.height();
    float y1 = cubeRect.y() + cubeRect.height();
    float z0 = cubeRect.z() - cubeRect.depth();
    float z1 = cubeRect.z() + cubeRect.depth();

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);

    glBegin(GL_QUADS);

    glColor3f(1.0f, 1.0f, 1.0f);

    glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
    glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
    glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);

    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);
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
            glColor3f(cubeRect.m_colour[0], cubeRect.m_colour[1], cubeRect.m_colour[2]);

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
        glColor3f(
            cubeRect.m_colour[0] * 0.7f,
            cubeRect.m_colour[1] * 0.7f,
            cubeRect.m_colour[2] * 0.7f
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
