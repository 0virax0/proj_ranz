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
   bool insertParticles(vector<float> relativeMousePosition, int particleType);
   bool deleteParticles(vector<float> relativeMousePosition);
   bool saveParticles();
   bool restoreParticles();
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
    QLCDNumber* LCDCounter;

    enum drawing_state{painting, erasing};
    void show_error(QString);
    static MainWindow* window;

public slots:
    void set_state(drawing_state newState);
    void brush_moved(vector<float> newPos);
private:
    //ricorda lo stato del pennello
    drawing_state state;
};

#endif // VIEW_H
