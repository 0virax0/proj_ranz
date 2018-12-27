#ifndef SERIALIZER_H
#define SERIALIZER_H
#include<model.h>
#include <QXmlStreamWriter>
class Serializer{
private:
    static bool _serialize(Tree<Particle2, 2>::Iterator, QXmlStreamWriter&);
public:
    static bool serializeTree(Tree<Particle2, 2>& tree);
};

class DeSerializer{
    static bool deSerializeTree(Tree<Particle2, 2>& destination);
};

#endif // SERIALIZER_H
