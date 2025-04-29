#include "Views2D.h"
#include <QMenu>
#include <QPainter>
#include <QWheelEvent>

// Template function to check if a value is in the container
template <typename T, typename U>
bool contains(const T& container, const U& value) {
    return std::find(container->begin(), container->end(), value) != container->end();
}

BaseViewWidget::BaseViewWidget(QWidget *parent, std::vector<Rect3D> *rects, std::vector<Rect3D*> *selectedRects)
    : QWidget(parent), m_rects(rects), m_selectedRects(selectedRects), m_lastMousePos(0, 0) {
    setMouseTracking(true);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void BaseViewWidget::showContextMenu(const QPoint &pos) {
    QMenu contextMenu(this);

    contextMenu.addAction("Delete", this, [=]() {
        if (!m_selectedRects->empty()) {
            for (Rect3D* selected : *m_selectedRects) {
                auto it = std::find(m_rects->begin(), m_rects->end(), *selected);
                if (it != m_rects->end()) {
                    m_rects->erase(it);
                }
            }
            m_selectedRects->clear();
            update();
        }
    });

    contextMenu.exec(mapToGlobal(pos));
}

void BaseViewWidget::paintEvent(QPaintEvent *event) {

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    qDebug() << m_offset;

    // Apply your current zoom and offset
    painter.translate(m_offset);
    painter.scale(m_scale, m_scale);

    // Grid settings
    int gridSize = 100;  // Grid size in world units

    float width = m_worldMaxX - m_worldMinX;
    float height = m_worldMaxY - m_worldMinY;

    float expandX = width * 0.25f;
    float expandY = height * 0.25f;

    QRect bgRect(
        m_worldMinX - expandX,
        m_worldMinY - expandY,
        width + 2 * expandX,
        height + 2 * expandY
    );

    qDebug() << m_scale;

    if (m_scale <= 0.05) gridSize = 1000;
    else if (m_scale <= 0.01) gridSize = 10000;

    // Snap the start points to the grid
    int startX = (m_worldMinX / gridSize) * gridSize;
    int startY = (m_worldMinY / gridSize) * gridSize;

    painter.fillRect(bgRect, Qt::black);

    QPen gridPen(Qt::gray, 1);
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);
    //gridPen.setWidth(0.01 * gridSize);
    gridPen.setWidth(1);

    // Vertical lines
    for (int x = startX; x <= m_worldMaxX; x += gridSize) {
        if (x == startX || x == m_worldMaxX) {
            gridPen.setColor(QColor(100, 46, 0));
        } else if ((x / gridSize) % 10 == 0) {
            gridPen.setColor(QColor(200, 200, 200));
        } else {
            gridPen.setColor(QColor(100, 100, 100));
        }
        painter.setPen(gridPen);
        painter.drawLine(QPointF(x, m_worldMinY), QPointF(x, m_worldMaxY));
    }

    // Horizontal lines
    for (int y = startY; y <= m_worldMaxY; y += gridSize) {
        if (y == startY || y == m_worldMaxY) {
            gridPen.setColor(QColor(100, 46, 0));
        } else if ((y / gridSize) % 10 == 0) {
            gridPen.setColor(QColor(200, 200, 200));
        } else if ((y / gridSize) % 20 == 0) {
            gridPen.setColor(QColor(100, 46, 0));
        } else {
            gridPen.setColor(QColor(100, 100, 100));
        }
        painter.setPen(gridPen);
        painter.drawLine(QPointF(m_worldMinX, y), QPointF(m_worldMaxX, y));
    }

    QPen rectPen = QPen(Qt::green, 3);

    rectPen.setCosmetic(true);

    painter.setPen(rectPen);
    painter.setBrush(QColor(0, 0, 0, 0));  // Semi-transparent fill

    for (Rect3D& rect : *m_rects) {

        QRectF rectF = getRect(rect);

        QRectF scaled = QRectF(
            rectF.x() * 100,
            rectF.y() * 100,
            rectF.width() * 100,
            rectF.height() * 100
        );

        xOff = 20 / m_scale;

        xOff = std::clamp(xOff, 0.0f, 50.0f);

        if (contains(m_selectedRects, &rect)) {
            rectPen = QPen(Qt::red, 3);
        } else {
            rectPen = QPen(Qt::green, 3);
        }

        rectPen.setCosmetic(true);

        painter.setPen(rectPen);

        qDebug() << "Is cosmetic?" << rectPen.isCosmetic();

        painter.drawRect(scaled);
        painter.drawLine(scaled.center() - QPointF(xOff, xOff), scaled.center() + QPointF(xOff, xOff));
        painter.drawLine(scaled.center() - QPointF(-xOff, xOff), scaled.center() + QPointF(-xOff, xOff));
    }

    // Draw selection rectangle if active
    if (m_isSelecting) {
        painter.setPen(QPen(Qt::yellow, 1, Qt::DashLine));
        qDebug() << painter.pen().isCosmetic();
        painter.setBrush(QColor(150, 150, 150, 150));  // Semi-transparent fill

        auto snapToGrid = [gridSize](const QPointF& point) {
            int x = std::round(point.x() / static_cast<float>(gridSize)) * gridSize;
            int y = std::round(point.y() / static_cast<float>(gridSize)) * gridSize;
            return QPoint(x, y);
        };

        QPoint snappedStart = snapToGrid(m_selectionStart);
        QPoint snappedEnd = snapToGrid(m_selectionEnd);

        QRect selectionRect = QRect(snappedStart, snappedEnd).normalized();
        painter.drawRect(selectionRect);
    }

    drawViewText(painter);
}

void BaseViewWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {

        for (Rect3D& rect : *m_rects) {

            QRectF rectF = getRect(rect);

            QRectF scaled = QRectF(
                rectF.x() * 100,
                rectF.y() * 100,
                rectF.width() * 100,
                rectF.height() * 100
            );

            xOff = 20 / m_scale;

            xOff = std::clamp(xOff, 0.0f, 50.0f);

            QRectF XRect = QRectF(scaled.center() - QPointF(xOff, xOff), scaled.center() + QPointF(xOff, xOff));

            qDebug() << "XRect:" << XRect;
            qDebug() << "Mouse pos:" << mapToWorld(event->pos());

            if (XRect.contains(mapToWorld(event->pos()))) {
                m_selectedRects->push_back(&rect);
            }

        }

        // Transform mouse position to world space (considering zoom)
        QPointF worldPos = mapToWorld(event->pos());

        m_selectionStart = worldPos;  // Store in world coordinates
        m_selectionEnd = worldPos;    // Initialize selection at the start
        m_isSelecting = true;
        update();  // Trigger repaint


    } else if (event->button() == Qt::RightButton) {
        showContextMenu(event->pos());
    }
}

void BaseViewWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_isSelecting) {
        // Transform mouse position to world space (considering zoom)
        QPointF worldPos = mapToWorld(event->pos());

        m_selectionEnd = worldPos;  // Update selection to the current position
        update();  // Trigger repaint
    }
}

void BaseViewWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_isSelecting) {
        // Transform mouse position to world space (considering zoom)
        QPointF worldPos = mapToWorld(event->pos());

        m_selectionEnd = worldPos;  // Finalize selection position
        m_isSelecting = false;
        update();  // Final repaint

        // Calculate the selection rectangle based on normalized positions
        QRectF selectedRect = QRectF(m_selectionStart, m_selectionEnd).normalized();
        qDebug() << "Selected area:" << selectedRect;
    }
}

// Function to map from widget coordinates to world coordinates considering zoom
QPointF BaseViewWidget::mapToWorld(const QPointF& widgetPos) {
    float worldX = (widgetPos.x() - m_offset.x()) / m_scale;
    float worldY = (widgetPos.y() - m_offset.y()) / m_scale;
    return QPointF(worldX, worldY);
}

float clamp(float value, float min, float max) {
    if (value < min) value = min;
    if (value > max) value = max;

    return value;
}

void BaseViewWidget::wheelEvent(QWheelEvent *event) {
    QPointF mousePos = event->position();         // Mouse position in widget coordinates
    QPointF worldBeforeZoom = (mousePos - m_offset) / m_scale; // Mouse in world space

    // Zoom factor adjustment
    float zoomFactor = 1.15f;
    if (event->angleDelta().y() > 0) {
        if (m_scale < 2)
            m_scale *= zoomFactor;   // Zoom in
    } else {
        if (m_scale > 0.001)
            m_scale /= zoomFactor;   // Zoom out
    }

    // New offset to keep world point under mouse unchanged
    m_offset = mousePos - worldBeforeZoom * m_scale;

    float offX = m_offset.x();
    float offY = m_offset.y();

    m_offset.setX(clamp(offX, m_worldMinX * m_scale * 2, m_worldMaxX * m_scale));
    m_offset.setY(clamp(offY, m_worldMinY * m_scale * 2, m_worldMaxY * m_scale));

    update();  // Trigger redraw
}

// Constructors for derived classes
XYViewWidget::XYViewWidget(QWidget *parent, std::vector<Rect3D> *rects, std::vector<Rect3D*> *selectedRects)
    : BaseViewWidget(parent) {
    // Custom initialization for the XY view
    setWindowTitle("XY View");

    m_rects = rects;
    m_selectedRects = selectedRects;
}

QRectF XYViewWidget::getRect(Rect3D& rect) {
    // Invert the Y-coordinate for the XY view to fix upside down issue
    return QRectF(rect.x() - rect.width(), -rect.y() - rect.height(), rect.width() * 2, rect.height() * 2);
}

void XYViewWidget::drawViewText(QPainter& painter) {
    // Set the pen and font for drawing text
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16));

    // Disable any transformations (scaling) for the font
    painter.setWorldMatrixEnabled(false);

    // Draw the text at the fixed position (10, 30) in the 2D view
    painter.drawText(10, 25, "front (x/y)");

    // End the painting operation
    painter.end();
}

YZViewWidget::YZViewWidget(QWidget *parent, std::vector<Rect3D> *rects, std::vector<Rect3D*> *selectedRects)
    : BaseViewWidget(parent) {
    // Custom initialization for the YZ view
    setWindowTitle("YZ View");

    m_rects = rects;
    m_selectedRects = selectedRects;
}

QRectF YZViewWidget::getRect(Rect3D& rect) {
    return QRectF(
        rect.z() - rect.depth(),    // X = Z
        -rect.y() - rect.height(),  // Y = -Y (flipped)
        rect.depth() * 2,           // Width
        rect.height() * 2           // Height
    );
}

void YZViewWidget::drawViewText(QPainter& painter) {
    // Set the pen and font for drawing text
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16));

    // Disable any transformations (scaling) for the font
    painter.setWorldMatrixEnabled(false);

    // Draw the text at the fixed position (10, 30) in the 2D view
    painter.drawText(10, 25, "side (y/z)");

    // End the painting operation
    painter.end();
}

XZViewWidget::XZViewWidget(QWidget *parent, std::vector<Rect3D> *rects, std::vector<Rect3D*> *selectedRects)
    : BaseViewWidget(parent) {
    // Custom initialization for the XZ view
    setWindowTitle("XZ View");

    m_rects = rects;
    m_selectedRects = selectedRects;
}

QRectF XZViewWidget::getRect(Rect3D& rect) {
    return QRectF(
        rect.x() - rect.width(),   // X position (centered)
        -rect.z() - rect.depth(),  // Y position (Z flipped and centered)
        rect.width() * 2,          // Width
        rect.depth() * 2           // Height
        );
}

void XZViewWidget::drawViewText(QPainter& painter) {
    // Set the pen and font for drawing text
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16));

    // Disable any transformations (scaling) for the font
    painter.setWorldMatrixEnabled(false);

    // Draw the text at the fixed position (10, 30) in the 2D view
    painter.drawText(10, 25, "top (x/z)");

    // End the painting operation
    painter.end();
}
