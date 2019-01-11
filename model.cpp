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

bool Model::insert(const vector<float> &pos, particle_type t) {
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

vector<int> Model::getParticleColor(int particleType){
    particle_type type = static_cast<particle_type>(particleType);
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
    cont->findNeighbouring(it, ipow(0.02f,2), neighbours);   //trovo i vicini
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
bool Model::save(){
   return Serializer::serializeTree(*container);
}
bool Model::restore(){
    delete container;
    container = new NearTree<Particle2,2>;
   return DeSerializer::deSerializeTree(*container);
}
void Model::setGravity(bool use){
    Particle2::useGravity = use;
}

//implementazione Particle2
bool Particle2::useGravity = true;
Particle2::Properties::Properties() : position(2,0), velocity(2,0) {}
Particle2::Properties::Properties(const vector<float>& pos, const vector<float>& vel, float m, float p, float t): position(pos), velocity(vel), mass(m), pressure(p), temperature(t){}
Particle2::Properties::Properties(const Properties& p): position(p.position), velocity(p.velocity), mass(p.mass), pressure(p.pressure), temperature(p.temperature) {}
Particle2::Properties& Particle2::Properties::operator=(const Particle2::Properties& p){
   position = p.position;
   velocity = p.velocity;
   mass = p.mass;
   pressure = p.pressure;
   temperature = p.temperature;
   return *this;
}

Particle2::Particle2() : properties(new Properties), substitute(nullptr), newProperties(new Properties){}
Particle2::Particle2(const vector<float>& pos, const vector<float>& vel, float m, float p, float t, float cond, float spec_heat): properties(new Properties(pos, vel, m, p, t)), substitute(nullptr), newProperties(new Properties(*properties)),
    conductivity(cond), specific_heat(spec_heat), entropy(specific_heat * t/p){}
Particle2::Particle2(const Particle2& p) : properties(new Properties(*p.properties)), substitute(nullptr), newProperties(new Properties(*p.newProperties)){}
Particle2::Particle2(QXmlStreamReader & xml): properties(new Properties), substitute(nullptr), newProperties(new Properties)
{
    while(xml.readNextStartElement()){
        if(xml.name() == "position"){
           while(xml.readNextStartElement()){
                if(xml.name() == "a") properties->position[0] = xml.readElementText().toFloat();
                else if(xml.name() == "b") properties->position[1] = xml.readElementText().toFloat();
            }
        }
        else if(xml.name() == "velocity"){
            while(xml.readNextStartElement()){
                if(xml.name() == "a") properties->velocity[0] = xml.readElementText().toFloat();
                else if(xml.name() == "b") properties->velocity[1] = xml.readElementText().toFloat();
            }
        }
        else if(xml.name() == "mass") properties->mass = xml.readElementText().toFloat();
        else if(xml.name() == "pressure") properties->pressure = xml.readElementText().toFloat();
        else if(xml.name() == "temperature") properties->temperature = xml.readElementText().toFloat();
        else if(xml.name() == "conductivity") conductivity = xml.readElementText().toFloat();
        else if(xml.name() == "specific_heat") specific_heat = xml.readElementText().toFloat();
        else if(xml.name() == "entropy"){ entropy = xml.readElementText().toFloat(); break;}
    }
}
Particle2::~Particle2(){
    delete properties;
    delete newProperties;
}
bool Particle2::serialize(QXmlStreamWriter& xml){
    xml.writeStartElement("position");
    xml.writeTextElement("a", QString::number(properties->position[0]));
    xml.writeTextElement("b", QString::number(properties->position[1]));
    xml.writeEndElement();
    xml.writeStartElement("velocity");
    xml.writeTextElement("a", QString::number(properties->velocity[0]));
    xml.writeTextElement("b", QString::number(properties->velocity[1]));
    xml.writeEndElement();
    xml.writeTextElement("mass", QString::number(properties->mass));
    xml.writeTextElement("pressure", QString::number(properties->pressure));
    xml.writeTextElement("temperature", QString::number(properties->temperature));
    xml.writeTextElement("conductivity", QString::number(conductivity));
    xml.writeTextElement("specific_heat", QString::number(specific_heat));
    xml.writeTextElement("entropy", QString::number(entropy));

    return true;
}

void Particle2::advect(const vector<Particle2*>& neighbours, float deltaTime){
    *newProperties = *properties;   //copio le proprietà in quelle nuove
    correctionDir = {0,0};
    float meanTemp = 0, weight = 1;

    for(auto it = neighbours.begin(); it!= neighbours.end(); it++){
        Properties* otherProperties = (*it)->properties;
        float currSqDist = sqDist(properties->position, otherProperties->position);
        if(currSqDist > ipow(0.001f,2)){     //salto le particelle esattamente sopra a me
            vector<float> versor = sub(properties->position, otherProperties->position);    //punta a me

            //contact advection
            if(currSqDist < ipow(0.003f, 2)){
                add_side(correctionDir, versor);
                normalize(versor);

                //calcolo la mia componente permìpendicolare rispetto all'altra particella
                vector<float> myPerpendicularVel = mul(versor , dot(versor , newProperties->velocity)*(-1.0f + 0.5f * newProperties->mass));

                //calcolo la componente perpendicolare della velocità dell'altra particella rispetto a questa;
                vector<float> perpendicularVel = mul(versor , dot(versor , otherProperties->velocity) * 0.5f * otherProperties->mass);

                //sommo le quantità di moto
                add_side(perpendicularVel, myPerpendicularVel);

                //per il calcolo dell'urto anelastico
                add_side(newProperties->velocity, perpendicularVel);
            } else normalize(versor);
            //faccio l'advection della pressione
            add_side(newProperties->velocity, mul(versor, (0.00002f * (1.0f/currSqDist) * (otherProperties->pressure - 1.0f) * deltaTime) / properties->mass));

            float distFactor = 1/(currSqDist * 1000000 +1);
            meanTemp += otherProperties->temperature * distFactor;
            weight += distFactor;
        }
    }
    //aggiungo l'accelerazione di gravità
    if(useGravity)add_side(newProperties->velocity, mul({0.0f, -0.981f}, deltaTime));

    //advection della temperatura
    float curr = properties->temperature;
    meanTemp = (meanTemp + curr) / (weight);
    newProperties->temperature = curr - deltaTime * conductivity * (curr - meanTemp);
            //std::cout<<"curr:"<<curr<<","<<"mean:"<<meanTemp<<",newTemp"<<newProperties->temperature<<" ";
    entropy = specific_heat * (newProperties->temperature / properties->pressure);  //l'entropia cambia solo per scambio con le altre particelle (newTemperature)
}

bool Particle2::swapState(float deltaTime){
    //faccio in modo che le particelle non si sovrappongano
    mul_side(correctionDir, 10.0f * deltaTime);
    add_side(newProperties->position, correctionDir);

    //applico lo spostamento
    add_side(newProperties->position, mul(newProperties->velocity, deltaTime));

    //controllo di essere dentro i confini
    if(newProperties->position[0]<0.005f){
        newProperties->position[0] = 0.005f;
        if(newProperties->velocity[0]<0) newProperties->velocity[0] = 0;
    }
    if(newProperties->position[0]>1.0f){
        newProperties->position[0] = 1.0f;
        if(newProperties->velocity[0]>0) newProperties->velocity[0] = 0;
    }
    if(newProperties->position[1]<0.005f){
        newProperties->position[1] = 0.005f;
        if(newProperties->velocity[1]<0) newProperties->velocity[1] = 0;
    }
    if(newProperties->position[1]>1.0f){
        newProperties->position[1] = 1.0f;
        if(newProperties->velocity[1]>0) newProperties->velocity[1] = 0;

    }
            std::cout<<(isnan(newProperties->position[0])||isnan(newProperties->position[1])? "nanPosition":"");

    //swap
    Particle2::Properties* tmp = newProperties;
    newProperties = properties;
    properties = tmp;
    return true;
}

vector<float> Particle2::add(const vector<float>& v1, const vector<float>& v2){
    return {v1[0]+v2[0], v1[1]+v2[1]};
}
void Particle2::add_side(vector<float>& v1, const vector<float>& v2){
    v1[0] += v2[0];
    v1[1] += v2[1];
}
vector<float> Particle2::sub(const vector<float>& v1, const vector<float>& v2){
    return {v1[0]-v2[0], v1[1]-v2[1]};
}
void Particle2::sub_side(vector<float>& v1, const vector<float>& v2){
    v1[0] -= v2[0];
    v1[1] -= v2[1];
}
vector<float> Particle2::mul(const vector<float>& v1, float scalar){
    return {v1[0]*scalar, v1[1]*scalar};
}
void Particle2::mul_side(vector<float>& v1, float scalar){
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
void Particle2::rotate90(vector<float>& v1){
   float tmp = v1[1];
   v1[1] = -v1[0];
   v1[0] = tmp;
}

//implementazione solid
Solid::Solid(float fric): friction(fric){}
Solid::Solid(QXmlStreamReader& xml){
   //letture aggiuntive per finire la costruzione di solid
    while(xml.readNextStartElement()){
        if(xml.name() == "friction") {friction = xml.readElementText().toFloat();break;}
    }
}
void Solid::advect(const vector<Particle2*>& neighbours, float deltaTime) {
    //nel solido aggiungo l'attrito
    for(auto it = neighbours.begin(); it!= neighbours.end(); it++){
        Properties* otherProperties = (*it)->properties;
        float currSqDist = sqDist(properties->position, otherProperties->position);
        if(currSqDist > ipow(0.001f,2)){     //salto le particelle esattamente sopra a me
            vector<float> versorParallel = sub(properties->position, otherProperties->position);    //pointing towards me
            normalize(versorParallel);
            rotate90(versorParallel);

            //attrito solo a contatto
            if(currSqDist < ipow(0.003f, 2)){
                //calcolo la componente parallela della velocità dell'altra particella rispetto a questa e vice versa
                float myParallelComponent = dot(versorParallel , properties->velocity);
                float otherParallelComponent = dot(versorParallel , otherProperties->velocity);
                vector<float> parallelVel = mul(versorParallel , myParallelComponent - otherParallelComponent);

                //creo una velocità opposta alla mia(parallela) per applicare l'attrito
                mul_side(parallelVel, friction * deltaTime / properties->mass);
                float magnitude = sqrt(sqLength(parallelVel));
                add_side(properties->velocity, parallelVel);

                //ricalcolo l'energia interna della particella, acquisita per attrito
                entropy += magnitude * 100.0f;
                newProperties->temperature = entropy * newProperties->pressure / specific_heat;
            }
        }
    }
}
bool Solid::serialize(QXmlStreamWriter& xml) {
//serializzo proprietà ulteriori
    xml.writeTextElement("friction", QString::number(friction));
    return true;
}

//implementazione liquid
Liquid::Liquid(){}
Liquid::Liquid(QXmlStreamReader& xml){ /*letture aggiuntive per finire la costruzione di liquid*/ }
void Liquid::advect(const vector<Particle2*>& neighbours, float deltaTime){ }
bool Liquid::serialize(QXmlStreamWriter& xml) {
//serializzo proprietà ulteriori
    return true;
}

//implementazione gas
Gas::Gas(){}
Gas::Gas(QXmlStreamReader& xml){ /*letture aggiuntive per finire la costruzione di gas*/ }
void Gas::advect(const vector<Particle2*>& neighbours, float deltaTime){
    if(neighbours.size() >0){
        //calcolo la nuova pressione basandomi sulla vicinanza alle altre particelle, l'entropia conserva
        float area = 2;
        for(auto it = neighbours.begin(); it!= neighbours.end(); it++){
            float sqdist = sqDist(properties->position, (*it)->properties->position);
            if(sqdist > ipow(0.07f,2))     //salto le particelle esattamente sopra a me
            if(sqdist<area)  area = sqdist;
        }
        float newPressure = 1.0f + 0.00001f * entropy / area ;
        newProperties->pressure = newPressure;
        newProperties->temperature = entropy * newPressure / specific_heat;
            //if(isnan(newPressure))std::cout<<"area:"<<area<<" ";

    }else newProperties->pressure = 1.0f;

    //i gas più leggeri dell'aria vanno verso l'alto
    add_side(newProperties->velocity, mul({0.0f, 0.981f + (newProperties->temperature - 293.0f) * 0.0005f / (newProperties->mass * 0.5f)}, deltaTime));

}
bool Gas::serialize(QXmlStreamWriter& xml) {
//serializzo proprietà ulteriori
    return true;
}

//implementazione explosive
Explosive::Explosive(float thresh_t, float thresh_p, float explosion_pres): threshold_temp(thresh_t), threshold_pressure(thresh_p), explosion_pressure(explosion_pres){}
Explosive::Explosive(QXmlStreamReader& xml){
   //letture aggiuntive per finire la costruzione di explosive
    while(xml.readNextStartElement()){
        if(xml.name() == "threshold_temp") threshold_temp = xml.readElementText().toFloat();
        else if(xml.name() == "threshold_pressure") threshold_pressure = xml.readElementText().toFloat();
        else if(xml.name() == "explosion_pressure"){ explosion_pressure = xml.readElementText().toFloat(); break;}
    }
}
void Explosive::advect(const vector<Particle2*>& neighbours, float deltaTime) {
    //in caso di superamento di uno threshold scambio la particella con fuoco
    if(newProperties->pressure > threshold_pressure || newProperties->temperature > threshold_temp){
       substitute = new Fire(properties->position, properties->velocity, explosion_pressure);
    }
}
bool Explosive::serialize(QXmlStreamWriter& xml) {
//serializzo proprietà ulteriori
    xml.writeTextElement("threshold_temp", QString::number(threshold_temp));
    xml.writeTextElement("threshold_pressure", QString::number(threshold_pressure));
    xml.writeTextElement("explosion_pressure", QString::number(explosion_pressure));
    return true;
}

//implementazione ice
vector<int> Ice::color{100, 100, 255};
Ice::Ice(const vector<float>& pos, const vector<float>& vel): Particle2 (pos, vel, 0.93f, 1.0f, 272.0f, 20.0f, 0.5f), Solid (0.04f), currColor(color){}
Ice::Ice(QXmlStreamReader& xml): Particle2 (xml), Solid(xml), currColor(color){/*letture aggiuntive*/ }
Ice::~Ice() {}

void Ice::advect(const vector<Particle2*>& neighbours, float deltaTime) {
    Particle2::advect(neighbours, deltaTime);
    Solid::advect(neighbours, deltaTime);

    //sciolgo il ghiaccio
    if(newProperties->temperature > 273.0f){
       substitute = new Water(properties->position, properties->velocity);
    }
}
vector<int> Ice::getColor(){
    return currColor;
}
bool Ice::serialize(QXmlStreamWriter& xml){
    xml.writeStartElement("Ice");
    Particle2::serialize(xml);
    Solid::serialize(xml);
    //ulteriori sottooggetti

    //ulteriori campi di Ice

    xml.writeEndElement();
    return true;
}

//implementazione Water
vector<int> Water::color{100, 100, 255};
Water::Water(const vector<float>& pos, const vector<float>& vel): Particle2 (pos, vel, 0.95f, 1.0f, 290.0f, 20.0f, 1.0f), Liquid (), currColor(color){}
Water::Water(QXmlStreamReader& xml): Particle2 (xml), Liquid(xml), currColor(color){/*letture aggiuntive*/ }
Water::~Water() {}

void Water::advect(const vector<Particle2*>& neighbours, float deltaTime) {
    Particle2::advect(neighbours, deltaTime);
    Liquid::advect(neighbours, deltaTime);

    //trasformo in ghiaccio
    if(newProperties->temperature < 273.0f){
       substitute = new Ice(properties->position, properties->velocity);
    //trasformo in vapore
    }else if(newProperties->temperature > 373.0f){
       substitute = new Steam(properties->position, properties->velocity);
    }
}
vector<int> Water::getColor(){
    return currColor;
}
bool Water::serialize(QXmlStreamWriter& xml){
    xml.writeStartElement("Water");
    Particle2::serialize(xml);
    Liquid::serialize(xml);
    //ulteriori sottooggetti

    //ulteriori campi di Water

    xml.writeEndElement();
    return true;
}

//implementazione steam
vector<int> Steam::color{200, 200, 255};
Steam::Steam(const vector<float>& pos, const vector<float>& vel): Particle2 (pos, vel, 0.2f, 1.0f, 383.0f, 5.0f, 0.2f), Gas (), currColor(color){}
Steam::Steam(QXmlStreamReader& xml): Particle2 (xml), Gas(xml), currColor(color){/*letture aggiuntive*/ }
Steam::~Steam() {}

void Steam::advect(const vector<Particle2*>& neighbours, float deltaTime) {
    Particle2::advect(neighbours, deltaTime);
    Gas::advect(neighbours, deltaTime);

    //tramuto in acqua
    if(newProperties->temperature < 372.0f){
       substitute = new Water(properties->position, properties->velocity);
    }
}
vector<int> Steam::getColor(){
    return currColor;
}
bool Steam::serialize(QXmlStreamWriter& xml){
    xml.writeStartElement("Steam");
    Particle2::serialize(xml);
    Gas::serialize(xml);
    //ulteriori sottooggetti

    //ulteriori campi di Steam

    xml.writeEndElement();
    return true;
}

//implementazione fire
vector<int> Fire::color{255, 100, 100};
Fire::Fire(const vector<float>& pos, const vector<float>& vel, float start_pressure): Particle2 (pos, vel, 0.1f, start_pressure, 1000.0f, 1.0f, 0.5f), Gas(), currColor(color), materialLeft(1.0f){}
Fire::Fire(QXmlStreamReader& xml): Particle2 (xml), Gas(xml), currColor(color){
    while(xml.readNextStartElement()){
        if(xml.name() == "materialLeft") materialLeft = xml.readElementText().toFloat();
    }
}
Fire::~Fire() {}

vector<int> Fire::getColor(){
    return currColor;
}
void Fire::advect(const vector<Particle2*>& neighbours, float deltaTime){
    Particle2::advect(neighbours, deltaTime);
    Gas::advect(neighbours, deltaTime);

    //man mano il combustibile esaurisce e il fuoco si spegne
    if(materialLeft > 0){
        materialLeft -= 0.1f * deltaTime;
        entropy += 15.0f * deltaTime;
        float newTemp = entropy * newProperties->pressure / specific_heat;
        newProperties->temperature = newTemp;
        currColor[0] = static_cast<int>(materialLeft * 300.0f);
        currColor[1] = static_cast<int>(materialLeft * 110.0f);
        currColor[2] = static_cast<int>(materialLeft * 0.0f);
    }
}
bool Fire::serialize(QXmlStreamWriter& xml){
    xml.writeStartElement("Fire");
    Particle2::serialize(xml);
    Gas::serialize(xml);
    //ulteriori sottooggetti

    xml.writeTextElement("materialLeft", QString::number(materialLeft));
    xml.writeEndElement();
    return true;
}

//implementazione gunpowder
vector<int> GunPowder::color{20, 20, 20};
GunPowder::GunPowder(const vector<float>& pos, const vector<float>& vel): Particle2 (pos, vel, 0.53f, 1.0f, 279.0f, 5.0f, 0.3f), Solid (0.6f), Explosive (480.0f, 3.0f, 20.0f), currColor(color){}
GunPowder::GunPowder(QXmlStreamReader& xml): Particle2 (xml), Solid(xml), Explosive (xml), currColor(color){/*letture aggiuntive*/ }
GunPowder::~GunPowder() {}

void GunPowder::advect(const vector<Particle2*>& neighbours, float deltaTime){
   Particle2::advect(neighbours, deltaTime);
   Solid::advect(neighbours, deltaTime);
   Explosive::advect(neighbours, deltaTime);
}
vector<int> GunPowder::getColor(){
    return currColor;
}
bool GunPowder::serialize(QXmlStreamWriter& xml){
    xml.writeStartElement("GunPowder");
    Particle2::serialize(xml);
    Solid::serialize(xml);
    Explosive::serialize(xml);

    //ulteriori campi

    xml.writeEndElement();
    return true;
}
