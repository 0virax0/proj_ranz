#include "particles.h"

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
bool Particle2::serialize(QXmlStreamWriter& xml)const{
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
    float meanTemp = 0, weight = 1.0f;

    for(auto it = neighbours.begin(); it!= neighbours.end(); it++){
        Properties* otherProperties = (*it)->properties;
        float currSqDist = utils::sqDist(properties->position, otherProperties->position);
        //if(currSqDist > ipow(0.0001f,2)){     //salto le particelle esattamente sopra a me
            vector<float> versor = utils::sub(properties->position, otherProperties->position);    //punta a me

            //contact advection
            if(currSqDist < ipow(0.004f, 2)){
                utils::add_side(correctionDir, versor);
                utils::normalize(versor);

                //calcolo la mia componente permìpendicolare rispetto all'altra particella
                vector<float> myPerpendicularVel = utils::mul(versor , utils::dot(versor , newProperties->velocity)*(-1.0f + 0.5f * newProperties->mass));

                //calcolo la componente perpendicolare della velocità dell'altra particella rispetto a questa;
                vector<float> perpendicularVel = utils::mul(versor , utils::dot(versor , otherProperties->velocity) * 0.5f * otherProperties->mass);

                //sommo le quantità di moto
                utils::add_side(perpendicularVel, myPerpendicularVel);

                //per il calcolo dell'urto anelastico
                utils::add_side(newProperties->velocity, perpendicularVel);
            } else utils::normalize(versor);
            //faccio l'advection della pressione
            utils::add_side(newProperties->velocity, utils::mul(versor, (0.000002f * (1.0f/(currSqDist + 1.0f)) * (newProperties->pressure-1.0f)*entropy*0.1f * deltaTime) / properties->mass));

            float distFactor = 1.0f/(currSqDist * 100000 +1.0f);
            meanTemp += otherProperties->temperature * distFactor;
            weight += distFactor;
        //}
    }
    //aggiungo l'accelerazione di gravità
    if(useGravity)utils::add_side(newProperties->velocity, utils::mul({0.0f, -0.981f}, deltaTime));

    //advection della temperatura
    float curr = properties->temperature;
    meanTemp = (meanTemp + curr) / (weight);
    newProperties->temperature = curr - deltaTime * conductivity * (curr - meanTemp);
            //std::cout<<"curr:"<<curr<<","<<"mean:"<<meanTemp<<",newTemp"<<newProperties->temperature<<" ";
    entropy = specific_heat * 10.0f * (newProperties->temperature / newProperties->pressure);  //l'entropia cambia solo per scambio con le altre particelle (newTemperature)
}

bool Particle2::swapState(float deltaTime){
    //faccio in modo che le particelle non si sovrappongano
    utils::mul_side(correctionDir, 20.0f * deltaTime);
    utils::add_side(newProperties->position, correctionDir);

    //applico lo spostamento
    utils::add_side(newProperties->position, utils::mul(newProperties->velocity, deltaTime));

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
           // std::cout<<(isnan(newProperties->position[0])||isnan(newProperties->position[1])? "nanPosition":"");

    //swap
    Particle2::Properties* tmp = newProperties;
    newProperties = properties;
    properties = tmp;
    return true;
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
        float currSqDist = utils::sqDist(properties->position, otherProperties->position);
        //if(currSqDist > ipow(0.0001f,2)){     //salto le particelle esattamente sopra a me
            vector<float> versorParallel = utils::sub(properties->position, otherProperties->position);    //pointing towards me
            utils::normalize(versorParallel);
            utils::rotate90(versorParallel);

            //attrito solo a contatto
            if(currSqDist < ipow(0.004f, 2)){
                //calcolo la componente parallela della velocità dell'altra particella rispetto a questa e vice versa
                float myParallelComponent = utils::dot(versorParallel , properties->velocity);
                float otherParallelComponent = utils::dot(versorParallel , otherProperties->velocity);
                vector<float> parallelVel = utils::mul(versorParallel , myParallelComponent - otherParallelComponent);

                //creo una velocità opposta alla mia(parallela) per applicare l'attrito
                utils::mul_side(parallelVel, friction * deltaTime / properties->mass);
                float magnitude = sqrt(utils::sqLength(parallelVel));
                utils::add_side(properties->velocity, parallelVel);

                //ricalcolo l'energia interna della particella, acquisita per attrito
                entropy += magnitude * 100.0f;
                newProperties->temperature = entropy * 0.1f * newProperties->pressure / specific_heat;
            }
        //}
    }
}
bool Solid::serialize(QXmlStreamWriter& xml)const {
//serializzo proprietà ulteriori
    xml.writeTextElement("friction", QString::number(friction));
    return true;
}

//implementazione liquid
Liquid::Liquid(){}
Liquid::Liquid(QXmlStreamReader& xml){ /*letture aggiuntive per finire la costruzione di liquid*/ }
void Liquid::advect(const vector<Particle2*>& neighbours, float deltaTime){ }
bool Liquid::serialize(QXmlStreamWriter& xml)const {
//serializzo proprietà ulteriori
    return true;
}

//implementazione gas
Gas::Gas(){}
Gas::Gas(QXmlStreamReader& xml){ /*letture aggiuntive per finire la costruzione di gas*/ }
void Gas::advect(const vector<Particle2*>& neighbours, float deltaTime){
    if(neighbours.size() >0){
        //calcolo la nuova pressione basandomi sulla vicinanza alle altre particelle, l'entropia conserva
        float area = 2.0f;
        for(auto it = neighbours.begin(); it!= neighbours.end(); it++){
            float sqdist = utils::sqDist(properties->position, (*it)->properties->position);
            if(sqdist > ipow(0.01f,2))     //salto le particelle sopra a me
            if(sqdist<area)  area = sqdist;
        }
        float newPressure = 1.0f + 10.0f / (area * 500000.0f + 1.0f);
        newProperties->pressure = newPressure;
        newProperties->temperature = entropy * 0.1f * newPressure / specific_heat;

    }else newProperties->pressure = 1.0f;

    //i gas più leggeri dell'aria vanno verso l'alto
    utils::add_side(newProperties->velocity, utils::mul({0.0f, 0.981f + (newProperties->temperature ) * 0.0002f / (newProperties->mass * 0.5f)}, deltaTime));

}
bool Gas::serialize(QXmlStreamWriter& xml)const {
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
bool Explosive::serialize(QXmlStreamWriter& xml)const {
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
vector<int> Ice::getColor()const{
    return currColor;
}
bool Ice::serialize(QXmlStreamWriter& xml)const{
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
vector<int> Water::getColor()const{
    return currColor;
}
bool Water::serialize(QXmlStreamWriter& xml)const{
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
vector<int> Steam::getColor()const{
    return currColor;
}
bool Steam::serialize(QXmlStreamWriter& xml)const{
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

vector<int> Fire::getColor()const{
    return currColor;
}
void Fire::advect(const vector<Particle2*>& neighbours, float deltaTime){
    Particle2::advect(neighbours, deltaTime);
    Gas::advect(neighbours, deltaTime);

    //man mano il combustibile esaurisce e il fuoco si spegne
    if(materialLeft > 0){
        materialLeft -= 0.1f * deltaTime;
        entropy += 15.0f * deltaTime;
        float newTemp = entropy * 0.1f * newProperties->pressure / specific_heat;
        newProperties->temperature = newTemp;
        currColor[0] = static_cast<int>(materialLeft * 300.0f);
        currColor[1] = static_cast<int>(materialLeft * 110.0f);
        currColor[2] = static_cast<int>(materialLeft * 0.0f);
    }
}
bool Fire::serialize(QXmlStreamWriter& xml)const{
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
GunPowder::GunPowder(const vector<float>& pos, const vector<float>& vel): Particle2 (pos, vel, 0.53f, 1.0f, 279.0f, 5.0f, 0.3f), Solid (0.6f), Explosive (400.0f, 2.0f, 20.0f), currColor(color){}
GunPowder::GunPowder(QXmlStreamReader& xml): Particle2 (xml), Solid(xml), Explosive (xml), currColor(color){/*letture aggiuntive*/ }
GunPowder::~GunPowder() {}

void GunPowder::advect(const vector<Particle2*>& neighbours, float deltaTime){
   Particle2::advect(neighbours, deltaTime);
   Solid::advect(neighbours, deltaTime);
   Explosive::advect(neighbours, deltaTime);
}
vector<int> GunPowder::getColor()const{
    return currColor;
}
bool GunPowder::serialize(QXmlStreamWriter& xml)const{
    xml.writeStartElement("GunPowder");
    Particle2::serialize(xml);
    Solid::serialize(xml);
    Explosive::serialize(xml);

    //ulteriori campi

    xml.writeEndElement();
    return true;
}
