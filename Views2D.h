#ifndef VIEWS2D_H
#define VIEWS2D_H

#include <QWidget>
#include <QMouseEvent>
#include "Rect3D.h"

class BaseViewWidget : public QWidget {
    Q_OBJECT

public:
    explicit BaseViewWidget(QWidget *parent = nullptr, std::vector<Rect3D> *rects = nullptr, std::vector<Rect3D*> *selectedRects = nullptr);

    QWidget *container;
    int selectedIndex;

    /*

    void setRects(std::vector<Rect3D> rects) {
        m_rects = rects;
    }

    std::vector<Rect3D> getRects() {
        return m_rects;
    }

    */

    virtual QRectF getRect(Rect3D& rect) = 0;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void showContextMenu(const QPoint &pos);

    std::vector<Rect3D> *m_rects;
    std::vector<Rect3D*> *m_selectedRects;

private:
    float xOff;
    QPoint m_lastMousePos;
    QPointF m_selectionStart;
    QPointF m_selectionEnd;
    bool m_isSelecting = false;
    float m_scale = 1.0f;            // Zoom factor
    QPointF m_offset = QPointF(0, 0); // Pan / translation offset

    virtual void drawViewText(QPainter& painter) = 0;
    QPointF mapToWorld(const QPointF& widgetPos);

    int m_worldMinX = -100000;  // Arbitrary world limits
    int m_worldMaxX =  100000;
    int m_worldMinY = -100000;
    int m_worldMaxY =  100000;
};

class XYViewWidget : public BaseViewWidget {
    Q_OBJECT

public:
    explicit XYViewWidget(QWidget *parent = nullptr, std::vector<Rect3D> *rects = nullptr, std::vector<Rect3D*> *selectedRects = nullptr);
    QRectF getRect(Rect3D& rect) override;

private:
    void drawViewText(QPainter& painter) override;

};

class YZViewWidget : public BaseViewWidget {
    Q_OBJECT

public:
    explicit YZViewWidget(QWidget *parent = nullptr, std::vector<Rect3D> *rects = nullptr, std::vector<Rect3D*> *selectedRects = nullptr);
    QRectF getRect(Rect3D& rect) override;

private:
    void drawViewText(QPainter& painter) override;

};

class XZViewWidget : public BaseViewWidget {
    Q_OBJECT

public:
    explicit XZViewWidget(QWidget *parent = nullptr, std::vector<Rect3D> *rects = nullptr, std::vector<Rect3D*> *selectedRects = nullptr);
    QRectF getRect(Rect3D& rect) override;

private:
    void drawViewText(QPainter& painter) override;

};

#endif // VIEWS2D_H
