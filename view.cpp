#include "view.h"
#include "ui_mainwindow.h"
#include "canvas.h"
#include <QGridLayout>
#include <QPushButton>
#include <iostream>
#include <QLCDNumber>
#include <QMessageBox>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>

View::View(Model& mod): model(mod){  }

bool View::insertParticles(vector<float> relativeMousePosition, int particleType)const{
    if(relativeMousePosition[0]>1.0f ||relativeMousePosition[0]<0.0f ||relativeMousePosition[1]>1.0f ||relativeMousePosition[1]<0.0f ){ std::cerr<<"coordinate in input sbagliate"<<std::endl; return false;}
   //calcolo la posizione adatta al modello
    relativeMousePosition[1] = 1.0f - relativeMousePosition[1];
    return model.insert(relativeMousePosition, static_cast<Model::particle_type>(particleType));
}
bool View::deleteParticles(vector<float> relativeMousePosition)const{
    if(relativeMousePosition[0]>1.0f ||relativeMousePosition[0]<0.0f ||relativeMousePosition[1]>1.0f ||relativeMousePosition[1]<0.0f ){ std::cerr<<"coordinate in input sbagliate"<<std::endl; return false;}
   //calcolo la posizione adatta al modello
    relativeMousePosition[1] = 1.0f - relativeMousePosition[1];
    model.container->deleteNeighbouring(relativeMousePosition, ipow(0.1f,2));
    return true;
}
bool View::saveParticles()const{
   return model.save();
}
bool View::restoreParticles()const{
    return model.restore();
}
bool View::clear()const{
    return model.clear();
}
QColor View::staticParticleColor(int particleType)const{
    vector<int> col = model.getParticleColor(static_cast<Model::particle_type>(particleType));
    return QColor(col[0], col[1], col[2]);
}

MainWindow::visual_type MainWindow::visual_state = Color;
MainWindow::MainWindow() : view(model), state(painting)
{
    setWindowTitle(tr("ParticleBox"));

    Canvas *openGL = new Canvas(this, model);
    QPushButton* paint_button = new QPushButton("paint", this);
    paint_button->setCheckable(true);
    paint_button->setChecked(true);
    QPushButton* erase_button = new QPushButton("erase", this);
    erase_button->setCheckable(true);
    QCheckBox *useGravity = new QCheckBox("Use Gravity", this);
    useGravity->setCheckState(Qt::Checked);
    QLabel *label0 = new QLabel(this);
    label0->setText("Numero Particelle");
    label0->setMaximumHeight(10);
    LCDCounter = new QLCDNumber(6, this);
    LCDCounter->setMaximumHeight(50);
    QLCDNumber* LCDframes = new QLCDNumber(3, this);
    LCDframes->setMaximumHeight(50);
    QLabel *label1 = new QLabel(this);
    label1->setText("Frames Per Secondo");
    label1->setMaximumHeight(10);
    QLabel *label2 = new QLabel(this);
    comboBox = new QComboBox(this);
        comboBox->addItem(tr("Water"));
        comboBox->addItem(tr("Ice"));
        comboBox->addItem(tr("Steam"));
        comboBox->addItem(tr("Fire"));
        comboBox->addItem(tr("GunPowder"));
    //setto lo sfondo delle opzioni basato sul colore statico della particella
    comboBox->setItemData(0, QBrush(view.staticParticleColor(0)), Qt::BackgroundRole);
    comboBox->setItemData(1, QBrush(view.staticParticleColor(1)), Qt::BackgroundRole);
    comboBox->setItemData(2, QBrush(view.staticParticleColor(2)), Qt::BackgroundRole);
    comboBox->setItemData(3, QBrush(view.staticParticleColor(3)), Qt::BackgroundRole);
    comboBox->setItemData(4, QBrush(view.staticParticleColor(4)), Qt::BackgroundRole);

    visualType = new QComboBox(this);
        visualType->addItem(tr("Color"));
        visualType->addItem(tr("Pressure"));
        visualType->addItem(tr("Velocity"));
        visualType->addItem(tr("Temperature"));
    QPushButton* clear_button = new QPushButton("clear all", this);
    QPushButton* save_button = new QPushButton("save", this);
    QPushButton* restore_button = new QPushButton("restore", this);

    QGridLayout *Glayout = new QGridLayout(this);
    QVBoxLayout *Vlayout = new QVBoxLayout(this);

    Vlayout->addWidget(paint_button);
    Vlayout->addWidget(erase_button);
    Vlayout->addWidget(comboBox);
    Vlayout->addWidget(visualType);
    Vlayout->addWidget(useGravity);
    Vlayout->addWidget(clear_button);
    Vlayout->addWidget(save_button);
    Vlayout->addWidget(restore_button);
    Vlayout->addWidget(label0);
    Vlayout->addWidget(LCDCounter);
    Vlayout->addWidget(label1);
    Vlayout->addWidget(LCDframes);
    Vlayout->addWidget(label2);

    Glayout->addWidget(openGL, 0, 0);
    Glayout->addLayout(Vlayout, 0, 1);
    setLayout(Glayout);

    //faccio il refresh del canvas 30 volte al secondo
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, openGL, &Canvas::animate);
    connect(timer, &QTimer::timeout, this, [LCDframes, openGL]{LCDframes->display(1.0f/openGL->deltaTime);});   //mostro i frames al secondo
    timer->start(33);

    //collego i bottoni
    connect(paint_button, &QPushButton::clicked, this, [this, erase_button]{this->set_state(painting); erase_button->setChecked(false);});
    connect(erase_button, &QPushButton::clicked, this, [this, paint_button]{this->set_state(erasing); paint_button->setChecked(false);});
    connect(useGravity, &QCheckBox::clicked, this, [this, useGravity]{this->model.setGravity(useGravity->checkState());});
    connect(clear_button, &QPushButton::clicked, this, [this]{this->view.clear();});
    connect(save_button, &QPushButton::clicked, this, [this]{this->view.saveParticles();});
    connect(restore_button, &QPushButton::clicked, this, [this]{this->view.restoreParticles();});

    //collego la modalità di visualizzazione
    connect(visualType, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this, [this]{MainWindow::visual_state = static_cast<MainWindow::visual_type>(this->visualType->currentIndex());});

    //collego il mouse al canvas
    connect(openGL, SIGNAL(positionChanged(vector<float>)), this, SLOT(brush_moved(vector<float>)));
}

MainWindow::~MainWindow() {}

void MainWindow::show_error(QString error)const{
            QMessageBox messageBox;
            messageBox.critical(0,"Error",error);
            messageBox.setFixedSize(500,200);
}

void MainWindow::set_state(drawing_state newState){
    state = newState;
    std::cout<<"state changed"<<std::endl;
}

void MainWindow::brush_moved(vector<float> newPos)const{
    switch(state){
    case painting: view.insertParticles(newPos, comboBox->currentIndex()); break;
    case erasing: view.deleteParticles(newPos); break;
    }
    //update counter
    LCDCounter->display(model.numParticles);
}
