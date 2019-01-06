#include "canvas.h"
#include <QMouseEvent>
#include <iostream>
#include "view.h"
#include <math.h>

Canvas::Canvas(QWidget *parent, Model& mod)
    : QOpenGLWidget(parent), model(mod)
{
    elapsedTimer.start();
    lastElapsed = 0;
    setFixedSize(1000, 1000);
    setAutoFillBackground(false);
}

void Canvas::animate()
{
    deltaTime = (elapsedTimer.elapsed() - lastElapsed) /1000.0f;
    lastElapsed = elapsedTimer.elapsed();
    update();
}

void Canvas::mouseMoveEvent(QMouseEvent* event){
    QPoint p= event->pos();
    float w = p.x() / static_cast<float>(width());
    float h = p.y() / static_cast<float>(height());
    emit positionChanged({w, h});
}

void Canvas::paintEvent(QPaintEvent* event){
    QPainter painter;
    painter.begin(this);
    painter.fillRect(event->rect(), QBrush(QColor(255, 255, 255))); //sfondo bianco

    QPen circlePen = QPen(Qt::NoPen);

    //disegno le particelle
    model.update([&painter, this, &circlePen](Particle2* particle){
        //visualize color based on visual state
        vector<int> col(3, 0);
        switch(MainWindow::visual_state){
        case MainWindow::Color: { col = particle->getColor(); break;}
        case MainWindow::Pressure:{
            int pressure = particle->properties->pressure * 5;
            col = {pressure ,0, 255 - pressure};
        break;}
        case MainWindow::Velocity: {
            int vel = (particle->properties->velocity[0]+ particle->properties->velocity[0])* 1000;
            col = {vel*4/10 ,vel*4/10, vel};
        break;}
        case MainWindow::Temperature: {
            int Temp = particle->properties->temperature / 4;
            col = {Temp ,0, 255 - Temp};
        break;}
        }
        painter.setBrush(QBrush(QColor(std::min(255, std::max(0, col[0])), std::min(255, std::max(0, col[1])), std::min(255, std::max(0, col[2])))));
        painter.setPen(circlePen);

        int w = static_cast<int>(particle->properties->position[0] * width());
        int h = static_cast<int>(height()-particle->properties->position[1] * height());
        painter.drawEllipse(w, h, 5, 5);

    }, deltaTime);

    painter.end();
}
