#include "view.h"
#include <QApplication>
#include<vector>
#include<iostream>
using std::vector;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();


   return a.exec();
}
