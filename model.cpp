#include "model.h"
#include <math.h>

//implementazione model
Model::Model(): container(new NearTree<Particle2,2>), next_container(new NearTree<Particle2,2>){}
Model::~Model(){
    delete container;
    delete next_container;
}

bool Model::insert(const vector<float> &pos, Model::particle_type t) {
   //inserisco una sola particella
    switch(t){
    case Water: container->insert(new class Water(pos), pos); break;
    case Ice: container->insert(new class Ice(pos), pos); break;
    case Steam: container->insert(new class Steam(pos), pos); break;
    }
    return true;
}

template<class Lambda>
bool Model::update(Lambda outParticle, float deltaTime){
    //faccio l'advection in ogni particella
    _update(container->root(), container, deltaTime);

    //faccio il detach chiamando anche outParticle
    container->detach(*next_container,
                      [&outParticle](Particle2*& thisParticle)->vector<float>{
                           thisParticle->swapState();   //le nuove proprietà vengono portate su
                           if(thisParticle->substitute != nullptr) {    //sostituisco con la nuova particella se serve
                               Particle2* old = thisParticle;
                               thisParticle = thisParticle->substitute;
                               delete old;
                           }
                           outParticle(thisParticle);   //invoco la funzione che utilizza le particle aggiornate nella View
                           return thisParticle->properties->position;
                      } );

    //porto su il nuovo albero aggiornato
   auto tmp = container;
   container = next_container;
   next_container = tmp;

   return true;
}

bool Model::_update(Tree<Particle2,2>::Iterator it, tree* cont, float deltaTime) {
    if(it == decltype (it)::pastEnd) return true;
    bool retVal=true;

    vector<Particle2*> neighbours;
    cont->findNeighbouring(it, 0.1f, neighbours);   //trovo i vicini
    it->advect(neighbours, deltaTime); //faccio advection a partire dalle mie properties e quelle dei vicini

    //ripeto per i sottoalberi
   for (int i=0; i< Tree<Particle2,2>::Nchild; i++)
       retVal &= _update(it[i], cont, deltaTime);
   return retVal;
}

//implementazione Particle2
Particle2::Properties::Properties() : position(2,0), velocity(2,0) {}
Particle2::Properties::Properties(const vector<float>& pos, const vector<float>& vel, float m, float p, float t): position(pos), velocity(vel), mass(m), pressure(p), temperature(t){}
Particle2::Properties::Properties(const Properties& p): position(p.position), velocity(p.velocity) {}
Particle2::Properties& Particle2::Properties::operator=(const Particle2::Properties& p){
   position = p.position;
   velocity = p.velocity;
   mass = p.mass;
   pressure = p.pressure;
   temperature = p.temperature;
   return *this;
}

Particle2::Particle2() : properties(new Properties), substitute(nullptr), newProperties(new Properties){}
Particle2::Particle2(const vector<float>& pos, const vector<float>& vel, float m, float p, float t): properties(new Properties(pos, vel, m, p, t)), substitute(nullptr), newProperties(new Properties){}
Particle2::Particle2(const Particle2& p) : properties(new Properties(*p.properties)), substitute(nullptr), newProperties(new Properties(*p.newProperties)){}
Particle2::Particle2(QXmlStreamReader & xml): properties(new Properties), substitute(nullptr), newProperties(new Properties)
{
    if(xml.readNextStartElement() && xml.name() == "position"){
        if(xml.readNextStartElement() && xml.name() == "0") properties->position[0] = xml.readElementText().toFloat();
        if(xml.readNextStartElement() && xml.name() == "1") properties->position[1] = xml.readElementText().toFloat();
    }
    if(xml.readNextStartElement() && xml.name() == "velocity"){
        if(xml.readNextStartElement() && xml.name() == "0") properties->velocity[0] = xml.readElementText().toFloat();
        if(xml.readNextStartElement() && xml.name() == "1") properties->velocity[1] = xml.readElementText().toFloat();
    }
    if(xml.readNextStartElement() && xml.name() == "mass") properties->mass = xml.readElementText().toFloat();
    if(xml.readNextStartElement() && xml.name() == "pressure") properties->pressure = xml.readElementText().toFloat();
    if(xml.readNextStartElement() && xml.name() == "temperature") properties->temperature = xml.readElementText().toFloat();
}
Particle2::~Particle2(){
    delete properties;
    delete newProperties;
}
bool Particle2::serialize(QXmlStreamWriter& xml){
    xml.writeStartElement("position");
    xml.writeTextElement("0", QString::number(properties->position[0]));
    xml.writeTextElement("1", QString::number(properties->position[1]));
    xml.writeEndElement();
    xml.writeStartElement("velocity");
    xml.writeTextElement("0", QString::number(properties->velocity[0]));
    xml.writeTextElement("1", QString::number(properties->velocity[1]));
    xml.writeEndElement();
    xml.writeTextElement("mass", QString::number(properties->mass));
    xml.writeTextElement("pressure", QString::number(properties->pressure));
    xml.writeTextElement("temperature", QString::number(properties->temperature));

    return true;
}

void Particle2::advect(const vector<Particle2*>& neighbours, float deltaTime){
    newProperties = properties;
    float meanTemp = 0, weight = 1;

    for(auto it = neighbours.begin(); it!= neighbours.end(); it++){
        Properties* otherProperties = (*it)->properties;
        float currSqDist = sqDist(properties->position, otherProperties->position);
        vector<float> versor = sub(properties->position, otherProperties->position);    //pointing towards me
        normalize(versor);

        //contact advection
        if(currSqDist < ipow(0.01f, 2)){
            //calcolo la componente perpendicolare della velocità dell'altra particella rispetto a questa;
            vector<float> perpendicularVel = mul(versor , dot(versor , otherProperties->velocity));

            //per il calcolo dell'urto anelastico
            mul_side(perpendicularVel, otherProperties->mass);
            add_side(newProperties->velocity, perpendicularVel);
        }
        //aggiungo l'accelerazione di gravità
        add_side(newProperties->velocity, mul({0.0f, -0.981f}, deltaTime));

        //faccio l'advection della pressione
        add_side(newProperties->velocity, mul(versor, (0.0001f * (1.0f/currSqDist) * (otherProperties->pressure - 1.0f) * deltaTime) / properties->mass));

        float distFactor = 1/(currSqDist * 10000 +1);
        meanTemp += otherProperties->temperature * distFactor;
        weight += distFactor;
    }
    //advection della temperatura
    float curr = properties->temperature;
    meanTemp = (meanTemp + curr) / (weight);
    newProperties->temperature = curr - deltaTime * conductivity * (curr - meanTemp)/100.0f;
    entropy = specific_heat * (newProperties->temperature / properties->pressure);  //l'entropia cambia solo per scambio con le altre particelle (newTemperature)
}

bool Particle2::swapState(){
    Particle2::Properties* tmp = newProperties;
    newProperties = properties;
    properties = tmp;
    return true;
}

vector<float> Particle2::add(const vector<float>& v1, const vector<float>& v2){
    return {v1[0]+v2[0], v1[1]+v2[1]};
}
void add_side(vector<float>& v1, const vector<float>& v2){
    v1[0] += v2[0];
    v1[1] += v2[1];
}
vector<float> Particle2::sub(const vector<float>& v1, const vector<float>& v2){
    return {v1[0]-v2[0], v1[1]-v2[1]};
}
void sub_side(vector<float>& v1, const vector<float>& v2){
    v1[0] -= v2[0];
    v1[1] -= v2[1];
}
vector<float> Particle2::mul(const vector<float>& v1, float scalar){
    return {v1[0]*scalar, v1[1]*scalar};
}
void mul_side(vector<float>& v1, float scalar){
    v1[0] *= scalar;
    v1[1] *= scalar;
}
float Particle2::sqDist(const vector<float>& v1, const vector<float>& v2){
    return (v1[0]-v2[0])*(v1[0]-v2[0]) + (v1[1]-v2[1])*(v1[1]-v2[1]);
}
float Particle2::sqLength(const vector<float>& v1){
    return v1[0]*v1[0] + v1[1]*v1[1];
}
void Particle2::normalize(vector<float>& v1){
    float length = sqrt(sqLength(v1));
    v1[0] /= length;
    v1[1] /= length;
}
float Particle2::dot(const vector<float>& v1, const vector<float>& v2){
    return v1[0]*v2[0] + v1[1]*v2[1];
}

//implementazione solid
Solid::Solid(float fric): friction(fric){}
Solid::Solid(QXmlStreamReader& xml): Particle2 (xml){
   //letture aggiuntive per finire la costruzione di solid
    if(xml.readNextStartElement() && xml.name() == "friction") friction = xml.readElementText().toFloat();
}
void Solid::advect(const vector<Particle2*>& neighbours, float deltaTime) {

}
bool Solid::serialize(QXmlStreamWriter& xml) {
    Particle2::serialize(xml);  //serializzo il sottooggetto

//serializzo proprietà ulteriori
    xml.writeTextElement("friction", QString::number(friction));
    return true;
}

//implementazione gas
void Gas::advect(const vector<Particle2*>& neighbours, float deltaTime){
    if(neighbours.size() >0){
        //calcolo la nuova pressione basandomi sulla vicinanza alle altre particelle, l'entropia conserva
        float area = 2;
        for(auto it = neighbours.begin(); it!= neighbours.end(); it++){
            float sqdist = sqDist(properties->position, (*it)->properties->position);
            if(sqdist<area)  area = sqdist;
        }
        float newPressure = 1.0f + 0.000001f * properties->temperature * entropy / area;
        newProperties->pressure = newPressure;
        newProperties->temperature = entropy * newPressure / specific_heat;

    }else newProperties->pressure = 1.0f;
}

//implementazione ice
vector<float> Ice::color{0.3f, 0.3f, 1.0f};
Ice::Ice(const vector<float>& pos): Particle2 (pos, {0.0f, 0.0f, 0.0f}, 0.93f, 0.0f, -1.0f), Solid (0.04f){}
Ice::Ice(QXmlStreamReader& xml): Particle2 (xml), Solid(xml){/*letture aggiuntive*/ }
Ice::~Ice() {}

void Ice::advect(const vector<Particle2*>& neighbours, float deltaTime) {}
vector<float> Ice::getColor(){
    return color;
}
bool Ice::serialize(QXmlStreamWriter& xml){
    xml.writeStartElement("Ice");
    Solid::serialize(xml);
    //ulteriori sottooggetti

    //ulteriori campi di Ice

    xml.writeEndElement();
    return true;
}
