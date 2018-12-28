#include "model.h"

//implementazione model
Model::Model(): container(new NearTree<Particle2,2>), next_container(new NearTree<Particle2,2>){}
Model::~Model(){
    delete container;
    delete next_container;
}

bool Model::insert(const vector<float> &pos, Model::particle_type t) {
    Tree<class Particle2,2>* cont = new NearTree<class Particle2,2>;
    NearTree<int,3> a;
    NearTree<int,3> b;
    a.insert({0,0,0},0);
    a.insert({0,0,0},0);
    b.insert({0,0,0},0);
   //inserisco una sola particella
    switch(t){
    case Water: container->insert(new class Water(pos), pos); break;
    case Ice: container->insert(new class Ice(pos), pos); break;
    case Steam: container->insert(new class Steam(pos), pos); break;
    }
}

bool Model::update() {

}

bool Model::_update(Tree<Particle2,2>::Iterator it)
{

}

//implementazione Particle2
Particle2::Properties::Properties() : position(2,0), velocity(2,0) {}
Particle2::Properties::Properties(const vector<float>& pos, const vector<float>& vel, float m, float p, float t): position(pos), velocity(vel), mass(m), pressure(p), temperature(t){}
Particle2::Properties::Properties(const Properties& p): position(p.position), velocity(p.velocity) {}

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

bool Particle2::swapState(){
    Particle2::Properties* tmp = newProperties;
    newProperties = properties;
    properties = tmp;
    return true;
}

//implementazione solid
Solid::Solid(float fric): friction(fric){}
Solid::Solid(QXmlStreamReader& xml): Particle2 (xml){
   //letture aggiuntive per finire la costruzione di solid
    if(xml.readNextStartElement() && xml.name() == "friction") friction = xml.readElementText().toFloat();
}
void Solid::advect(std::vector<Particle2>) {

}
bool Solid::serialize(QXmlStreamWriter& xml) {
    Particle2::serialize(xml);  //serializzo il sottooggetto

//serializzo proprietà ulteriori
    xml.writeTextElement("friction", QString::number(friction));
    return true;
}

//implementazione ice
const vector<float> Ice::color{0.3f, 0.3f, 1.0f};
Ice::Ice(const vector<float>& pos): Particle2 (pos, {0.0f, 0.0f, 0.0f}, 0.93f, 0.0f, -1.0f), Solid (0.04f){}
Ice::Ice(QXmlStreamReader& xml): Particle2 (xml), Solid(xml){/*letture aggiuntive*/ }

bool Ice::serialize(QXmlStreamWriter& xml){
    xml.writeStartElement("Ice");
    Solid::serialize(xml);
    //ulteriori sottooggetti

    //ulteriori campi di Ice

    xml.writeEndElement();
    return true;
}
