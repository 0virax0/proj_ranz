#include "serializer.h"
#include "model.h"
#include<QXmlStreamWriter>
#include<QFile>
#include<iostream>

bool Serializer::serializeTree(Tree<Particle2, 2> &tree)
{
    QString fileName ="./data/field.xml";
    QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            std::cerr << "Error: Cannot write file "
                      << qPrintable(fileName) << ": "
                      << qPrintable(file.errorString()) << std::endl;
            return false;
        }
    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    //percorro l'albero e lo serializzo
    xmlWriter.writeStartElement("Particles");
    bool serState = _serialize(tree.root(), xmlWriter);

    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();
    file.close();
    if (file.error() || serState) {
        std::cerr << "Error: Cannot serialize in file "
                  << qPrintable(fileName) << ": "
                  << qPrintable(file.errorString()) << std::endl;
        return false;
    }
    return true;
}

bool Serializer::_serialize(Tree<Particle2, 2>::Iterator it, QXmlStreamWriter& xml)
{
   if(it == decltype(it)::pastEnd) return true;
   bool retVal = true;
   retVal &= it->serialize(xml);

   for (int i=0; i< Tree<Particle2,2>::Nchild; i++)
       retVal &= _serialize(it[i], xml);
   return retVal;
}

bool DeSerializer::deSerializeTree(Tree<Particle2, 2>& destination){
    QString fileName ="./data/field.xml";
    QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            std::cerr << "Error: Cannot write file "
                      << qPrintable(fileName) << ": "
                      << qPrintable(file.errorString()) << std::endl;
            return false;
        }

    QXmlStreamReader xmlReader(&file);
    while(!xmlReader.atEnd() && !xmlReader.hasError()) {
        if(xmlReader.readNext() == QXmlStreamReader::StartElement){
            if( xmlReader.name() == "Ice") {    //controllo il tag e decido quale particella costruire
                Particle2* p = new Ice(xmlReader);
                destination.insert(p, p->properties->position);
            }
        }
    }
    file.close();
    if (file.error() || xmlReader.hasError()) {
        std::cerr << "Error: Cannot deSerialize file "
                  << qPrintable(fileName) << ": "
                  << qPrintable(file.errorString()) << std::endl;
        return false;
    }
    return true;
}
