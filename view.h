#ifndef VIEW_H
#define VIEW_H

#include <QMainWindow>
#include "model.h"
#include <QComboBox>

using std::vector;

class View : QObject{
    Q_OBJECT

public slots:
   bool insertParticles(vector<float> relativeMousePosition, int particleType);
   bool deleteParticles(vector<float> relativeMousePosition);
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

    enum drawing_state{painting, erasing};

public slots:
    void set_state(drawing_state newState);
    void brush_moved(vector<float> newPos);
private:
    //ricorda lo stato del pennello
    drawing_state state;
};

#endif // VIEW_H
