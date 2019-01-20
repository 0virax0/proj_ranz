#ifndef VIEW_H
#define VIEW_H

#include <QMainWindow>
#include "model.h"
#include <QComboBox>
#include <QLCDNumber>

using std::vector;

class View : QObject{
    Q_OBJECT

public slots:
   bool insertParticles(vector<float> relativeMousePosition, int particleType)const;
   bool deleteParticles(vector<float> relativeMousePosition)const;
   bool clear()const;
   bool saveParticles()const;
   bool restoreParticles()const;
   QColor staticParticleColor(int particleType)const;
public:
   Model& model;
   View(Model& mod);
};

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow();

    Model model;
    View view;

    QComboBox* comboBox;
    QComboBox* visualType;
    QLCDNumber* LCDCounter;

    enum drawing_state{painting, erasing};
    enum visual_type{Color, Pressure, Velocity, Temperature };
    void show_error(QString)const;
    static visual_type visual_state;

public slots:
    void set_state(drawing_state newState);
    void brush_moved(vector<float> newPos)const;
private:
    //ricorda lo stato del pennello
    drawing_state state;
};

#endif // VIEW_H
