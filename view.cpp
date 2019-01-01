#include "view.h"
#include "ui_mainwindow.h"
#include "canvas.h"
#include <QGridLayout>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("ParticleBox"));
    Canvas *openGL = new Canvas(this);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(openGL, 0, 1);
    setLayout(layout);

    //faccio il refresh del canvas 30 volte al secondo
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, openGL, &Canvas::animate);
    timer->start(33);
}

MainWindow::~MainWindow()
{
    delete ui;
}
