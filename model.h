#ifndef MODEL_H
#define MODEL_H
#include <vector>
template<class T, int dim>
class Tree{     //albero n-dimensionale di ricerca
protected:
    class Node{
        friend class Iterator;
        friend class Tree;
    protected:
        T* data;
        Node* setChild(Node* n, int index);
        Node* children[2^dim];
    public:
        float position[dim];

        Node();
        Node(const Node& n);
        Node(T* d);

        ~Node();
    };
public:
    class Iterator{
    private:
        Node* ptr;  //nodo<T>
        int currIndex;
    public:
        static Iterator* pastEnd = Iterator();

        Iterator();
        Iterator(const Iterator& i);
        Iterator(const Node& n);

        bool operator==(const Iterator&) const;
        bool operator!=(const Iterator&) const;
        //se tento di andare pastTheEnd, non modifico ptr, ma ritorno l'iteratore pastEnd dereferenziato
        Iterator operator[](int child);    //ritorno un figlio del nodo e setto currIndex
        Iterator operator++();             //scendo di un livello, nel figlio currIndex (non salgo per evitare il doppio linkaggio)
        Iterator operator++(int);

        T& operator*();
        T* operator->();

        Iterator& operator =(const Iterator&);
    };
    protected: Node r; /*primo nodo radice*/ public:
    Iterator& root();

    Tree();
    Tree(const Tree& t);    //copia profonda
    Tree& operator=(const Tree& t); //assegnazione profonda

    Iterator insert(const T& t, const float newPos[dim]);
    Iterator update();  //aggiorno le posizioni di tutti i nodi
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

   virtual void advect(std::vector<Particle2>)=0;
   virtual bool swapState();

   Particle2(); //create properties in the heap
   Particle2(const Particle2&);
   virtual ~Particle2();

protected:
   Properties* newProperties;

};

class solid : public virtual Particle2{
   virtual void advect(std::vector<Particle2>);
};
class liquid : public virtual Particle2{};
class gas : public virtual Particle2{};
class explosive : public virtual Particle2{};
class corrosive : public virtual Particle2{};

//implementazione tree

template<class T, int dim>
Tree<T,dim>::Node::Node():data(nullptr), position({0}){ }
template<class T, int dim>
Tree<T,dim>::Node::Node(const Node& n){  }
template<class T, int dim>
Tree<T,dim>::Node::Node(T* d){}
template<class T, int dim>
Tree<T,dim>::Node::~Node(){}

template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::insert(const T &t, const float newPos[dim])
{
    Node* n = new Node(&t);
    Iterator p = root();
    if(p==Iterator::pastEnd){
        r = n;
       return Iterator(r);
    }
    //navigo tra i rami e inserisco non appena trovo spazio
    Iterator nP = p;
    int index=0;
    while(nP != Iterator::pastEnd){
        p = nP; //scendo nel sottoalbero
        //trovo indice
        index=0;
        for(int i=0; i<dim;i++)
           if(newPos[i] > p.ptr->position[i])
              index += 2^i; //se sono strettamente maggiore al pivot nella dimensione considerata setto la bitmask in modo da puntare il figlio corretto
        nP = p[index]; //seleziono il sottoalbero
    }
    return Iterator(p.ptr->setChild(n,index));
}
#endif // MODEL_H
