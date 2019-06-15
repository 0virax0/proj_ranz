#ifndef MODEL_H
#define MODEL_H
#include <vector>
#include <math.h>
#include <QString>
#include <QXmlStreamWriter>
#include <iostream>
#include "particles.h"
#include "tree.h"
using std::vector;


//Model
class Model{
public:
    using tree = NearTree<Particle2, 2>;
    enum particle_type {
        Water, Ice, Steam, Fire, GunPowder
    };
    tree* container;
    static int numParticles;

    bool insert(const vector<float>& pos, particle_type t)const; //inserisco in posizione cartesiana particelle di tipo t;
    vector<int> getParticleColor(particle_type type)const;
    template<class Lambda>  //outParticle prende un puntatore a Particle2 grazie al quale può leggere lo stato di ogni particella nel container
    bool update(Lambda outParticle, float deltaTime);
    bool clear();
    bool save()const;
    bool restore();
    void setGravity(bool use)const;

    Model();
    ~Model();
private:
    tree* next_container;

    static bool _update(Tree<Particle2,2>::Iterator it, tree* cont, float deltaTime);
};
template<class Lambda>
bool Model::update(Lambda outParticle, float deltaTime){
    int numP = 0;
    //faccio l'advection in ogni particella
    _update(container->root(), container, deltaTime);

    //faccio il detach chiamando anche outParticle
    container->detach(*next_container,
                      [&outParticle, deltaTime, &numP](Particle2*& thisParticle)->vector<float>{
                           numP += 1;   //conto le particelle
                           ////std::cout<< thisParticle->properties->position[0]<<" ";
                           thisParticle->swapState(deltaTime);   //le nuove proprietà vengono portate su
                           if(thisParticle->substitute != nullptr) {    //sostituisco con la nuova particella se serve
                               Particle2* old = thisParticle;
                               thisParticle = thisParticle->substitute;
                               delete old;
                           }
                           outParticle(thisParticle);   //invoco la funzione che utilizza le particle aggiornate nella View
                           return thisParticle->properties->position;
                      } );
    numParticles = numP;

    //porto su il nuovo albero aggiornato
   auto tmp = container;
   container = next_container;
   next_container = tmp;

   return true;
}


#endif // MODEL_H
