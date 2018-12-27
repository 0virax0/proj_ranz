#include "model.h"

Particle2::Properties::Properties() : position(2,0), velocity(2,0) {}
Particle2::Properties::Properties(const Properties& p): position(p.position), velocity(p.velocity) {}

Particle2::Particle2() : properties(new Properties), newProperties(new Properties){}
Particle2::Particle2(const Particle2& p) : properties(new Properties(*p.properties)), newProperties(new Properties(*p.newProperties)){}
Particle2::Particle2(QXmlStreamReader & xml): properties(new Properties), newProperties(new Properties)
{
    if(xml.readNextStartElement() && xml.name() == "position"){
        if(xml.readNextStartElement() && xml.name() == "0") properties->position[0] = xml.readElementText().toFloat();
        if(xml.readNextStartElement() && xml.name() == "1") properties->position[1] = xml.readElementText().toFloat();
    }
    if(xml.readNextStartElement() && xml.name() == "velocity"){
        if(xml.readNextStartElement() && xml.name() == "0") properties->velocity[0] = xml.readElementText().toFloat();
        if(xml.readNextStartElement() && xml.name() == "1") properties->velocity[1] = xml.readElementText().toFloat();
    }
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
    xml.writeTextElement("pressure", QString::number(properties->pressure));
    xml.writeTextElement("temperature", QString::number(properties->temperature));

    return true;
}

//implementazione solid
solid::solid(QXmlStreamReader& xml): Particle2 (xml){
   //letture aggiuntive per finire la costruzione di solid
}
void solid::advect(std::vector<Particle2>) {

}
bool solid::serialize(QXmlStreamWriter& xml) {
    Particle2::serialize(xml);  //serializzo il sottooggetto
//serializzo proprietà ulteriori
    return true;
}
