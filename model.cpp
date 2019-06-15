#include "model.h"
#include <math.h>
#include <iostream>
#include "serializer.h"

//implementazione model
Model::Model(): container(new NearTree<Particle2,2>), next_container(new NearTree<Particle2,2>){}
Model::~Model(){
    delete container;
    delete next_container;
}
int Model::numParticles = 0;

bool Model::insert(const vector<float> &pos, particle_type t)const {
   //inserisco una sola particella
    switch(t){
    case Water: container->insert(new class Water(pos), pos); break;
    case Ice: container->insert(new class Ice(pos), pos); break;
    case Steam: container->insert(new class Steam(pos), pos); break;
    case Fire: container->insert(new class Fire(pos), pos); break;
    case GunPowder: container->insert(new class GunPowder(pos), pos); break;
    }
    return true;
}

vector<int> Model::getParticleColor(particle_type type)const{
    switch(type){
    case Water: return Water::color;
    case Ice: return Ice::color;
    case Steam: return Steam::color;
    case Fire: return Fire::color;
    case GunPowder: return GunPowder::color;
    }
    return {0,0,0};
}

bool Model::_update(Tree<Particle2,2>::Iterator it, tree* cont, float deltaTime) {
    if(it == decltype (it)::pastEnd) return true;
    bool retVal=true;

    vector<Particle2*> neighbours;
    cont->findNeighbouring(it, ipow(0.03f,2), neighbours);   //trovo i vicini
    //std::cout<<neighbours.size()<<" ";
    it->advect(neighbours, deltaTime); //faccio advection a partire dalle mie properties e quelle dei vicini

    //ripeto per i sottoalberi
   for (unsigned int i=0; i< Tree<Particle2,2>::Nchild; i++)
       retVal &= _update(it[i], cont, deltaTime);
   return retVal;
}
bool Model::clear(){
    delete container;
    container = new NearTree<Particle2,2>;
    return true;
}
bool Model::save()const{
   return Serializer::serializeTree(*container);
}
bool Model::restore(){
    delete container;
    container = new NearTree<Particle2,2>;
   return DeSerializer::deSerializeTree(*container);
}
void Model::setGravity(bool use)const{
    Particle2::useGravity = use;
}

