#include "canvas.h"
#include <QMouseEvent>

Canvas::Canvas(QWidget *parent, Model& mod)
    : QOpenGLWidget(parent), model(mod)
{
    elapsedMs = 0;
    setFixedSize(200, 200);
    setAutoFillBackground(false);
}

void Canvas::animate()
{
    elapsedMs = (elapsedMs + qobject_cast<QTimer*>(sender())->interval());
    deltaTime = qobject_cast<QTimer*>(sender())->interval() / 1000.0f;
    update();
}

void Canvas::mouseMoveEvent(QMouseEvent* event){
    QPoint p= event->pos();
    float w = p.x() / width();
    float h = p.y() / height();
    emit positionChanged({w, h});
}

void Canvas::paintEvent(QPaintEvent* event){
    QPainter painter;
    painter.begin(this);
    painter.fillRect(event->rect(), QBrush(QColor(255, 255, 255))); //sfondo bianco

    //disegno le particelle
    model.update([&painter, this](Particle2* particle){
        vector<int> col = particle->getColor();
        painter.setBrush(QBrush(QColor(col[0], col[1], col[2])));

        int w = static_cast<int>(particle->properties->position[0] * width());
        int h = static_cast<int>(-particle->properties->position[1] * height());
        painter.drawEllipse(w, h, 3, 3);

    }, deltaTime);

    painter.end();
}
