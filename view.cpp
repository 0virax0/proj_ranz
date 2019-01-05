#include "view.h"
#include "ui_mainwindow.h"
#include "canvas.h"
#include <QGridLayout>
#include <QPushButton>
#include <iostream>
#include <QLCDNumber>
#include <QMessageBox>

View::View(Model& mod): model(mod){  }

bool View::insertParticles(vector<float> relativeMousePosition, int particleType){
    if(relativeMousePosition[0]>1.0f ||relativeMousePosition[0]<0.0f ||relativeMousePosition[1]>1.0f ||relativeMousePosition[1]<0.0f ){ std::cerr<<"coordinate in input sbagliate"<<std::endl; return false;}
   //calcolo la posizione adatta al modello
    relativeMousePosition[1] = 1.0f - relativeMousePosition[1];
    return model.insert(relativeMousePosition, static_cast<Model::particle_type>(particleType));
}
bool View::deleteParticles(vector<float> relativeMousePosition){
    if(relativeMousePosition[0]>1.0f ||relativeMousePosition[0]<0.0f ||relativeMousePosition[1]>1.0f ||relativeMousePosition[1]<0.0f ){ std::cerr<<"coordinate in input sbagliate"<<std::endl; return false;}
   //calcolo la posizione adatta al modello
    relativeMousePosition[1] = 1.0f - relativeMousePosition[1];
    model.container->deleteNeighbouring(relativeMousePosition, ipow(0.1f,2));
    return true;
}
bool View::saveParticles(){
   return model.save();
}
bool View::restoreParticles(){
    return model.restore();
}

MainWindow* MainWindow::window = nullptr;
MainWindow::visual_type MainWindow::visual_state = Color;
MainWindow::MainWindow() : view(model), state(painting)
{
    setWindowTitle(tr("ParticleBox"));
    window = this;

    Canvas *openGL = new Canvas(this, model);
    QPushButton* paint_button = new QPushButton("paint", this);
    QPushButton* erase_button = new QPushButton("erase", this);
    LCDCounter = new QLCDNumber(6, this);
    comboBox = new QComboBox(this);
        comboBox->addItem(tr("Water"));
        comboBox->addItem(tr("Ice"));
        comboBox->addItem(tr("Steam"));
        comboBox->addItem(tr("Fire"));
        comboBox->addItem(tr("GunPowder"));
    visualType = new QComboBox(this);
        visualType->addItem(tr("Color"));
        visualType->addItem(tr("Pressure"));
        visualType->addItem(tr("Velocity"));
        visualType->addItem(tr("Temperature"));
    QPushButton* save_button = new QPushButton("save", this);
    QPushButton* restore_button = new QPushButton("restore", this);

    QGridLayout *Glayout = new QGridLayout(this);
    QVBoxLayout *Vlayout = new QVBoxLayout(this);
    Vlayout->addWidget(paint_button);
    Vlayout->addWidget(comboBox);
    Vlayout->addWidget(visualType);
    Vlayout->addWidget(erase_button);
    Vlayout->addWidget(save_button);
    Vlayout->addWidget(restore_button);
    Vlayout->addWidget(LCDCounter);

    Glayout->addWidget(openGL, 0, 0);
    Glayout->addLayout(Vlayout, 0, 1);
    setLayout(Glayout);

    //faccio il refresh del canvas 30 volte al secondo
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, openGL, &Canvas::animate);
    timer->start(33);

    //collego i bottoni
    connect(paint_button, &QPushButton::clicked, this, [this]{this->set_state(painting);});
    connect(erase_button, &QPushButton::clicked, this, [this]{this->set_state(erasing);});
    connect(save_button, &QPushButton::clicked, this, [this]{this->view.saveParticles();});
    connect(restore_button, &QPushButton::clicked, this, [this]{this->view.restoreParticles();});

    //collego la modalità di visualizzazione
    connect(visualType, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this, [this]{MainWindow::visual_state = static_cast<MainWindow::visual_type>(this->visualType->currentIndex());});

    //collego il mouse al canvas
    connect(openGL, SIGNAL(positionChanged(vector<float>)), this, SLOT(brush_moved(vector<float>)));
}

MainWindow::~MainWindow() {}

void MainWindow::show_error(QString error){
            QMessageBox messageBox;
            messageBox.critical(0,"Error",error);
            messageBox.setFixedSize(500,200);
}

void MainWindow::set_state(drawing_state newState){
    state = newState;
    std::cout<<"state changed"<<std::endl;
}

void MainWindow::brush_moved(vector<float> newPos){
    switch(state){
    case painting: view.insertParticles(newPos, comboBox->currentIndex()); break;
    case erasing: view.deleteParticles(newPos); break;
    }
    //update counter
    LCDCounter->display(model.numParticles);
}
