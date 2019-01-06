#ifndef CANVAS_H
#define CANVAS_H
#include <QPainter>
#include <QTimer>
#include <QOpenGLWidget>
#include "model.h"
#include <QElapsedTimer>

using std::vector;

class Canvas : public QOpenGLWidget{
    Q_OBJECT

public:
    Canvas(QWidget *parent, Model& mod);
    Model& model;
    float deltaTime;    //in seconds

public slots:
    void animate();
signals:
    //passa coordinate da 0 a 1 con asse y invertito
    void positionChanged(vector<float> newPos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent* event)override;
private:
    QElapsedTimer elapsedTimer;
    int lastElapsed;
};

#endif // CANVAS_H
