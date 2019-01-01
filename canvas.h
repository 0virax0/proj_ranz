#ifndef CANVAS_H
#define CANVAS_H
#include <QPainter>
#include <QTimer>
#include <QOpenGLWidget>

class Canvas : public QOpenGLWidget{
    Q_OBJECT

public:
    Canvas(QWidget *parent);

public slots:
    void animate();

protected:
    void paintEvent(QPaintEvent *event) override;
private:
    int elapsedMs;
    float deltaTime;    //in seconds
};

#endif // CANVAS_H
