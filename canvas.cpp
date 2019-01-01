#include "canvas.h"

Canvas::Canvas(QWidget *parent)
    : QOpenGLWidget(parent)
{
    elapsedMs = 0;
    setFixedSize(200, 200);
    setAutoFillBackground(false);
}

void Canvas::animate()
{
    elapsedMs = (elapsedMs + qobject_cast<QTimer*>(sender())->interval());
    deltaTime = qobject_cast<QTimer*>(sender())->interval();
    update();
}

void Canvas::paintEvent(QPaintEvent* event){
    QPainter painter;
    painter.begin(this);

    //paint things

    painter.end();
}
