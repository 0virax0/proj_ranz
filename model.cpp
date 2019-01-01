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
                      [&outParticle, deltaTime](Particle2*& thisParticle)->vector<float>{
                           thisParticle->swapState(deltaTime);   //le nuove proprietà vengono portate su
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
Particle2::Particle2(const vector<float>& pos, const vector<float>& vel, float m, float p, float t, float cond, float spec_heat): properties(new Properties(pos, vel, m, p, t)), substitute(nullptr), newProperties(properties),
    conductivity(cond), specific_heat(spec_heat), entropy(specific_heat * t/p){}
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
    correctionDir = {0,0};
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
            add_side(correctionDir, versor);

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

bool Particle2::swapState(float deltaTime){
    //sposto la particella secondo la nuova velocity, stando attento a non sovrappormi con le particelle con le quali collido
    normalize(correctionDir);  //prendo la direzione correttiva dalla somma
    float errorMagnitude = dot(newProperties->velocity, correctionDir);
    if(errorMagnitude < 0.0f){
        //non rispetto la direzione corretta, devo eliminare la componente erronea
        mul_side(correctionDir, -errorMagnitude);
        add_side(newProperties->velocity, correctionDir);
    }
    //applico lo spostamento
    add_side(newProperties->position, mul(newProperties->velocity, deltaTime));

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
void Particle2::rotate90(vector<float>& v1){
   float tmp = v1[1];
   v1[1] = -v1[0];
   v1[0] = tmp;
}

//implementazione solid
Solid::Solid(float fric): friction(fric){}
Solid::Solid(QXmlStreamReader& xml){
   //letture aggiuntive per finire la costruzione di solid
    if(xml.readNextStartElement() && xml.name() == "friction") friction = xml.readElementText().toFloat();
}
void Solid::advect(const vector<Particle2*>& neighbours, float deltaTime) {
    //nel solido aggiungo l'attrito
    for(auto it = neighbours.begin(); it!= neighbours.end(); it++){
        Properties* otherProperties = (*it)->properties;
        float currSqDist = sqDist(properties->position, otherProperties->position);
        vector<float> versorParallel = sub(properties->position, otherProperties->position);    //pointing towards me
        normalize(versorParallel);
        rotate90(versorParallel);

        //attrito solo a contatto
        if(currSqDist < ipow(0.01f, 2)){
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
            if(sqdist<area)  area = sqdist;
        }
        float newPressure = 1.0f + 0.000001f * properties->temperature * entropy / area;
        newProperties->pressure = newPressure;
        newProperties->temperature = entropy * newPressure / specific_heat;

    }else newProperties->pressure = 1.0f;

    //i gas più leggeri dell'aria vanno verso l'alto
    add_side(newProperties->velocity, mul({0.0f, 0.981f + (newProperties->mass - 0.1f) * (newProperties->temperature - 293.0f) * 0.01f}, deltaTime));

}
bool Gas::serialize(QXmlStreamWriter& xml) {
//serializzo proprietà ulteriori
    return true;
}

//implementazione explosive
Explosive::Explosive(float thresh_t, float thresh_p, float explosion_pres): threshold_temp(thresh_t), threshold_pressure(thresh_p), explosion_pressure(explosion_pres){}
Explosive::Explosive(QXmlStreamReader& xml){
   //letture aggiuntive per finire la costruzione di explosive
    if(xml.readNextStartElement() && xml.name() == "threshold_temp") threshold_temp = xml.readElementText().toFloat();
    if(xml.readNextStartElement() && xml.name() == "threshold_pressure") threshold_pressure = xml.readElementText().toFloat();
    if(xml.readNextStartElement() && xml.name() == "explosion_pressure") explosion_pressure = xml.readElementText().toFloat();
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
vector<float> Ice::color{0.3f, 0.3f, 1.0f};
Ice::Ice(const vector<float>& pos, const vector<float>& vel): Particle2 (pos, vel, 0.93f, 1.0f, 272.0f, 20.0f, 0.5f), Solid (0.04f), currColor(color){}
Ice::Ice(QXmlStreamReader& xml): Particle2 (xml), Solid(xml){/*letture aggiuntive*/ }
Ice::~Ice() {}

void Ice::advect(const vector<Particle2*>& neighbours, float deltaTime) {
    Particle2::advect(neighbours, deltaTime);
    Solid::advect(neighbours, deltaTime);

    //sciolgo il ghiaccio
    if(newProperties->temperature > 273.0f){
       substitute = new Water(properties->position, properties->velocity);
    }
}
vector<float> Ice::getColor(){
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
vector<float> Water::color{0.3f, 0.3f, 1.0f};
Water::Water(const vector<float>& pos, const vector<float>& vel): Particle2 (pos, vel, 0.95f, 1.0f, 290.0f, 20.0f, 1.0f), Liquid (), currColor(color){}
Water::Water(QXmlStreamReader& xml): Particle2 (xml), Liquid(xml){/*letture aggiuntive*/ }
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
vector<float> Water::getColor(){
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
vector<float> Steam::color{0.7f, 0.7f, 1.0f};
Steam::Steam(const vector<float>& pos, const vector<float>& vel): Particle2 (pos, vel, 0.2f, 1.0f, 373.0f, 5.0f, 0.2f), Gas (), currColor(color){}
Steam::Steam(QXmlStreamReader& xml): Particle2 (xml), Gas(xml){/*letture aggiuntive*/ }
Steam::~Steam() {}

void Steam::advect(const vector<Particle2*>& neighbours, float deltaTime) {
    Particle2::advect(neighbours, deltaTime);
    Gas::advect(neighbours, deltaTime);

    //tramuto in acqua
    if(newProperties->temperature < 372.0f){
       substitute = new Water(properties->position, properties->velocity);
    }
}
vector<float> Steam::getColor(){
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
vector<float> Fire::color{1.0f, 0.3f, 0.3f};
Fire::Fire(const vector<float>& pos, const vector<float>& vel, float start_pressure): Particle2 (pos, vel, 0.1f, start_pressure, 1000.0f, 1.0f, 0.5f), Gas(), currColor(color), materialLeft(1.0f){}
Fire::Fire(QXmlStreamReader& xml): Particle2 (xml), Gas(xml){
    if(xml.readNextStartElement() && xml.name() == "materialLeft") materialLeft = xml.readElementText().toFloat();
}
Fire::~Fire() {}

vector<float> Fire::getColor(){
    return currColor;
}
void Fire::advect(const vector<Particle2*>& neighbours, float deltaTime){
    Particle2::advect(neighbours, deltaTime);
    Gas::advect(neighbours, deltaTime);

    //man mano il combustibile esaurisce e il fuoco si spegne
    if(materialLeft > 0){
        materialLeft -= 0.2f * deltaTime;
        entropy += 30.0f * deltaTime;
        newProperties->temperature = entropy * newProperties->pressure / specific_heat;
        currColor = mul(color, newProperties->temperature/1000.0f);
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
vector<float> GunPowder::color{0.1f, 0.1f, 0.1f};
GunPowder::GunPowder(const vector<float>& pos, const vector<float>& vel): Particle2 (pos, vel, 0.53f, 1.0f, 279.0f, 5.0f, 0.3f), Solid (0.6f), Explosive (480.0f, 3.0f, 20.0f), currColor(color){}
GunPowder::GunPowder(QXmlStreamReader& xml): Particle2 (xml), Solid(xml), Explosive (xml){/*letture aggiuntive*/ }
GunPowder::~GunPowder() {}

void GunPowder::advect(const vector<Particle2*>& neighbours, float deltaTime){
   Particle2::advect(neighbours, deltaTime);
   Solid::advect(neighbours, deltaTime);
   Explosive::advect(neighbours, deltaTime);
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
