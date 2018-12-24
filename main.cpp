#include "view.h"
#include <QApplication>
#include<vector>
#include<iostream>
using std::vector;

template<class T, int dim>
void interCombinations(int zeroes,unsigned int indexX, vector<int>tmp, vector<vector<int>>& res){
    if(indexX == dim)res.push_back(tmp); //ho completato una riga, la salvo

    if(zeroes == dim-indexX){ //devo aggiungere solo zeri per completare la riga
        tmp[indexX] = 0;
        return interCombinations<T,dim>(zeroes-1, indexX+1, tmp, res);
    }

    if(zeroes>0){
        tmp[indexX]=0;
        interCombinations<T,dim>(zeroes-1, indexX+1, tmp, res);
    }
    tmp[indexX] = -1;   interCombinations<T,dim>(zeroes, indexX+1, tmp, res);
    tmp[indexX] =  1;   interCombinations<T,dim>(zeroes, indexX+1, tmp, res);

}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    vector<vector<int>> res;
    for(int i=3; i!=0; i--){
        interCombinations<int, 3>(i, 0, vector<int>(), res);
    }
    //print
    for(auto it=res.begin(); it != res.end(); it++){
        for(auto i=(*it).begin(); i!= (*it).end(); i++){
            std::cout<< *i<< " ";
        }
        std::cout<<std::endl;
    }
   // return a.exec();
}
