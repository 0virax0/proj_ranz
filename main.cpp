#include "view.h"
#include <QApplication>
#include<vector>
#include<iostream>
#include "model.h"
using std::vector;

template<class T, int d> class D;
template<class T, int d>
class B{
    public:
    class subClass{
        friend class D<T,d>;
        subClass* s;};
    subClass sub;
    void insert(){
        std::cout << "insert" << std::endl;
    }
    B();
};

template<class T, int d>
class D : public B<T,d>{
public:
    static bool v;
    static float f;
    static void fun();
    D(){
        if(!v){
            v=true;
            f=1;
        }
    }
};
template<class T,int d> bool D<T,d>::v = 0;

template class D<int,2>;
template class NearTree<int,2>;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    /*MainWindow w;
    w.show();*/

    NearTree<int, 2> tr;
    NearTree<int,2>::Iterator;
    tr.insert(new int(0), {0,0,0});
    D<int,2> var;
    var.insert();
   //return a.exec();
}
