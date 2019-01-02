#include "view.h"
#include "ui_mainwindow.h"
#include "canvas.h"
#include <QGridLayout>
#include <QPushButton>
#include <iostream>

View::View(Model& mod): model(mod){  }

bool View::insertParticles(vector<float> relativeMousePosition, int particleType){
    if(relativeMousePosition[0]>1.0f ||relativeMousePosition[0]<0.0f ||relativeMousePosition[1]>1.0f ||relativeMousePosition[1]>0.0f ){ std::cerr<<"coordinate in input sbagliate"; return false;}
   //calcolo la posizione adatta al modello
    relativeMousePosition[1] = 1.0f - relativeMousePosition[1];
    return model.insert(relativeMousePosition, static_cast<Model::particle_type>(particleType));
}
bool View::deleteParticles(vector<float> relativeMousePosition){
    if(relativeMousePosition[0]>1.0f ||relativeMousePosition[0]<0.0f ||relativeMousePosition[1]>1.0f ||relativeMousePosition[1]>0.0f ){ std::cerr<<"coordinate in input sbagliate"; return false;}
   //calcolo la posizione adatta al modello
    relativeMousePosition[1] = 1.0f - relativeMousePosition[1];
    model.container->deleteNeighbouring(relativeMousePosition, ipow(0.01f,2));
    return true;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), view(model), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("ParticleBox"));
    Canvas *openGL = new Canvas(this, model);
    QPushButton* paint_button = new QPushButton("paint", this);
    QPushButton* erase_button = new QPushButton("erase", this);
    comboBox = new QComboBox(this);
        comboBox->addItem(tr("Water"));
        comboBox->addItem(tr("Ice"));
        comboBox->addItem(tr("Steam"));
        comboBox->addItem(tr("Fire"));
        comboBox->addItem(tr("GunPowder"));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(openGL, 0, 0);
    layout->addWidget(paint_button, 0, 1);
    layout->addWidget(erase_button, 1, 1);
    layout->addWidget(comboBox, 1, 2);
    setLayout(layout);

    //faccio il refresh del canvas 30 volte al secondo
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, openGL, &Canvas::animate);
    timer->start(33);

    //collego i bottoni
    connect(paint_button, &QPushButton::clicked, this, [this]{this->set_state(painting);});
    connect(erase_button, &QPushButton::clicked, this, [this]{this->set_state(erasing);});
    
    //collego il mouse al canvas
    connect(openGL, SIGNAL(positionChanged(vector<float>)), this, SLOT(brush_moved(vector<float>)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::set_state(drawing_state newState){
    state = newState;
}

void MainWindow::brush_moved(vector<float> newPos){
    switch(state){
    case painting: view.insertParticles(newPos, comboBox->currentIndex()); break;
    case erasing: view.deleteParticles(newPos); break;
    }
}
