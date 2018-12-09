#ifndef MODEL_H
#define MODEL_H
#include <vector>
template<class T, int dim>
class Tree{     //albero n-dimensionale di ricerca
protected:
    class Node{
    public:
        T* data;
        float position[dim];
        Node* children[dim];

        Node();
        Node(const Node& n);
        Node(T* d);

        ~Node();
    };
public:
    class Iterator{
    private:
        Node* ptr;  //nodo<T>
    public:
        Iterator* pastEnd;

        Iterator();
        Iterator(const Iterator& i);

        bool operator==(const Iterator&) const;
        bool operator!=(const Iterator&) const;
        //se tento di andare pastTheEnd, non modifico ptr, ma ritorno l'iteratore pastEnd dereferenziato
        Iterator& operator[](int child);    //navigo tra i figli del seguente nodo
        Iterator& operator++();             //scendo di un livello (non salgo per evitare il doppio linkaggio)

        T* operator*();                     //ritorno il T* per renderlo nullable
    };
    protected: Iterator r; /*primo nodo radice*/ public:
    Iterator& root();

    Tree();
    Tree(const Tree& t);    //copia profonda
    Tree& operator=(const Tree& t); //assegnazione profonda

    Iterator insert(const T& t, const float newPos[dim]);
    Iterator move(Iterator m, const float newPos[dim]);
    bool del(Iterator d);
    Iterator findNearest(const float Pos[dim]);

    ~Tree();    //distruzione profonda
};

template<class T, int dim>
class NearTree : public Tree<T, dim>{
protected:
    static std::vector<T>& findNrecursive(std::vector<T>& v, float targetPos[dim], int maxDistance, typename Tree<T,dim>::Iterator it, const float bounds[dim]);
public:
    std::vector<T>& findNeighbouring(std::vector<T>& v, float targetPos[dim], int maxDistance, typename Tree<T,dim>::Iterator it = Tree<T,dim>::root()) const;
};

class Particle2{
public:
   class Properties{
   public:
        float position[2];
        float velocity[2];
        float pressure;
        float temperature;
   };
   Properties* properties;



   Particle2(); //create properties in the heap
   Particle2(const Particle2&);
   virtual ~Particle2();

protected:
   Properties* newProperties;

};

#endif // MODEL_H
