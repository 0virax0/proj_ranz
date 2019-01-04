#include "serializer.h"
#include "model.h"
#include<QXmlStreamWriter>
#include<QFile>
#include<iostream>
#include "view.h"

bool Serializer::serializeTree(Tree<Particle2, 2> &tree)
{
    QString fileName ="./field.xml";
    QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            std::cerr << "Error: Cannot write file "
                      << qPrintable(fileName) << ": "
                      << qPrintable(file.errorString()) << std::endl;
           MainWindow::window->show_error("Error: Cannot write file "+fileName + ": " + file.errorString());
            return false;
        }
    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("Particles");
    //percorro l'albero e lo serializzo
    bool serState = _serialize(tree.root(), xmlWriter);

    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();
    file.close();
    if (file.error() || !serState) {
        std::cerr << "Error: Cannot serialize in file "
                  << qPrintable(fileName) << ": "
                  << qPrintable(file.errorString()) << std::endl;
           MainWindow::window->show_error("Error: Cannot serialize in file "+fileName + ": " + file.errorString());
        return false;
    }
    return true;
}

bool Serializer::_serialize(Tree<Particle2, 2>::Iterator it, QXmlStreamWriter& xml)
{
   if(it == decltype(it)::pastEnd) return true;
   bool retVal = true;
   retVal &= it->serialize(xml);

   for (unsigned int i=0; i< Tree<Particle2,2>::Nchild; i++)
       retVal &= _serialize(it[i], xml);
   return retVal;
}

bool DeSerializer::deSerializeTree(Tree<Particle2, 2>& destination){
    QString fileName ="./field.xml";
    QFile file(fileName);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            std::cerr << "Error: Cannot read file "
                      << qPrintable(fileName) << ": "
                      << qPrintable(file.errorString()) << std::endl;
           MainWindow::window->show_error("Error: Cannot read file "+fileName + ": " + file.errorString());
            return false;
        }

    QXmlStreamReader xmlReader(&file);
    while(!xmlReader.atEnd() ) {
        if(xmlReader.readNextStartElement() ){
            if( xmlReader.name() == "Ice") {    //controllo il tag e decido quale particella costruire
                Particle2* p = new Ice(xmlReader);
                destination.insert(p, p->properties->position);
            }else if( xmlReader.name() == "Water") {
                Particle2* p = new Water(xmlReader);
                destination.insert(p, p->properties->position);
            }else if( xmlReader.name() == "Steam") {
                Particle2* p = new Steam(xmlReader);
                destination.insert(p, p->properties->position);
            }else if( xmlReader.name() == "Fire") {
                Particle2* p = new Fire(xmlReader);
                destination.insert(p, p->properties->position);
            }else if( xmlReader.name() == "GunPowder") {
                Particle2* p = new GunPowder(xmlReader);
                destination.insert(p, p->properties->position);
            }
        }
    }
    file.close();
    if (file.error()) {
        std::cerr << "Error: Cannot deSerialize file "
                  << qPrintable(fileName) << ": "
                  << qPrintable(file.errorString()) << std::endl;
           MainWindow::window->show_error("Error: Cannot deserialize file "+fileName + ": " + file.errorString());
        return false;
    }
    return true;
}
