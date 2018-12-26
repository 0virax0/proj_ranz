#include "view.h"
#include <QApplication>
#include<vector>
#include<iostream>
#include "model.h"
using std::vector;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    /*MainWindow w;
    w.show();*/

    NearTree<int, 3> t;
    vector<float> v {0.5, 0.5, 0.5};
    int* val = new int(95);
    int* val1 = new int(90);
    t.insert(val, v);
    v[0] = 0;
    t.insert(val1, v);
    auto it = t.root();
    auto it1 = it[0];

    NearTree<int, 3> t1;
    t.detach(t1, [&v](decltype (it) i)->vector<float>{(*i) += 1; return v;});
int i=0;
   //return a.exec();
}
