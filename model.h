#ifndef MODEL_H
#define MODEL_H
#include <vector>
using std::vector;

template<class T, int dim>
class Tree{     //albero n-dimensionale di ricerca
protected:
    class Node{
        friend class Iterator;
        friend class Tree;
    protected:
        T* data;    //deve essere allocato nello heap
        Node* setChild(Node* n, int index);
        vector<Node*> children;
    public:
        vector<float> position;

        Node();
        Node(const Node& n);
        Node(T* d, vector<float> pos);
        Node& operator =(const Node&);  //copio profondamente data, ma non i figli

        ~Node();       //distruggo anche l'oggetto puntato ma non i figli
    };

    static Node* copy(Node*);
    static void destroy(Node*);
public:
    class Iterator{
    private:
        Node* ptr;  //nodo<T>
        int currIndex;
    public:
        static Iterator* pastEnd = Iterator();

        Iterator();
        Iterator(const Iterator& i);
        explicit Iterator(const Node& n, int index = 0);

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
    Iterator root();

    Tree();
    Tree(const Tree& t);    //copia profonda
    Tree& operator=(const Tree& t); //assegnazione profonda

    Iterator insert(const T& t, const vector<float> newPos);
    Iterator insert(const Iterator& t);        //sposto un nodo(anche da un altro albero)

    template<class Lambda>
    void detach(const Tree& dest, Lambda fn); //stacca tutto l'albero
    template<class Lambda>
    static void detach(const Iterator& t, const Tree& dest, Lambda fn); //stacca un sottoalbero s da t[s], applica la lambda sui nodi e li inserisce in un altro albero, fn deve ritornare una nuova position per il nodo

    bool del(); //svuota l'albero
    bool del(Iterator d);   //elimina il sottoalbero s da d[s]
    Iterator findNearest(const float Pos[dim])const;

    ~Tree();    //distruzione profonda
};

template<class T, int dim>
class NearTree : public Tree<T, dim>{
protected:
    static std::vector<T>& findNrecursive(std::vector<T>& v, const vector<float> targetPos, int maxDistance, typename Tree<T,dim>::Iterator it, const vector<float> bounds);
public:
    std::vector<T>& findNeighbouring(std::vector<T>& v, const vector<float> targetPos, int maxDistance, typename Tree<T,dim>::Iterator it = Tree<T,dim>::root()) const;
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

//implementazione Node
template<class T, int dim>
Tree<T,dim>::Node::Node():position(dim,0), data(nullptr), children( 2^dim, nullptr) { }
template<class T, int dim>
Tree<T,dim>::Node::Node(const Node& n): position(n.position), data(new T(*n.data)), children(n.children){  }
template<class T, int dim>
Tree<T,dim>::Node::Node(T* d, vector<float> pos):position(pos), data(d), children(2^dim, nullptr) {}
template<class T, int dim>
typename Tree<T,dim>::Node& Tree<T,dim>::Node::operator =(const typename Tree<T,dim>::Node& n){
    position = n.position;
    data = new T(*n.data);
    children(dim, nullptr);

    return *this;
}
template<class T, int dim>
Tree<T,dim>::Node::~Node(){delete data;}

//implementazione Tree::Iterator
template<class T, int dim>
Tree<T,dim>::Iterator::Iterator() : ptr(nullptr), currIndex(0){}
template<class T, int dim>
Tree<T,dim>::Iterator::Iterator(const Iterator& i) : ptr(i.ptr), currIndex(i.currIndex){}
template<class T, int dim>
Tree<T,dim>::Iterator::Iterator(const Node& n, int index) : ptr(&n), currIndex(index){}

template<class T, int dim>
bool Tree<T,dim>::Iterator::operator==(const typename Tree<T,dim>::Iterator& it) const{return ptr==it.ptr && (ptr==nullptr || currIndex==it.currIndex);}
template<class T, int dim>
bool Tree<T,dim>::Iterator::operator!=(const typename Tree<T,dim>::Iterator& it ) const{return !(ptr==it.ptr && (ptr==nullptr || currIndex==it.currIndex));}

template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::Iterator::operator[](int child){
    return Iterator(ptr->children[child]);
}
template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::Iterator::operator++(){
    Node* n = ptr->children[currIndex];
    if(!n) return pastEnd;
    ptr = n;
    return *this;
}
template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::Iterator::operator++(int){
    Node* n = ptr->children[currIndex];
    if(!n) return pastEnd;
    Iterator i = *this;
    ptr = n;
    return i;
}

template<class T, int dim>
T& Tree<T,dim>::Iterator::operator*(){
    return *(ptr->data);
}
template<class T, int dim>
T* Tree<T,dim>::Iterator::operator->(){
    return ptr->data;
}

template<class T, int dim>
typename Tree<T,dim>::Iterator& Tree<T,dim>::Iterator::operator=(const Iterator& t){
    ptr = t.ptr;
    currIndex = t.currIndex;
    return *this;
}

//implementazione albero
template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::insert(const T &t, const vector<float> newPos)
{
    Node* n = new Node(&t, newPos);
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
template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::insert(const Iterator& t){
    Node* n = t.currIndex;
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
           if(n->position[i] > p.ptr->position[i])
              index += 2^i; //se sono strettamente maggiore al pivot nella dimensione considerata setto la bitmask in modo da puntare il figlio corretto
        nP = p[index]; //seleziono il sottoalbero
    }
    return Iterator(p.ptr->setChild(n,index));
}

template<class T, int dim>
typename Tree<T,dim>::Node* Tree<T,dim>::copy(Node* r){
    if(!r)return nullptr;

    Node* n = new Node(r);
    for(int i=0; i<(2^dim); i++){
        n->children[i] = copy(r->children[i]);
    }
    return n;
}
template<class T, int dim>
void Tree<T,dim>::destroy(Node* r){
   if(!r)return;

   for(int i=0; i<(2^dim); i++){
       destroy(r->children[i]);
   }
}
template<class T, int dim>
Tree<T,dim>::Tree(): r(nullptr){  }
template<class T, int dim>
Tree<T,dim>::Tree(const Tree& t): r(copy(t)){  }
template<class T, int dim>
Tree<T,dim>& Tree<T,dim>::operator=(const Tree<T,dim>& t){
   if(*this == t) return *this;
    destroy(r);
    r = copy(t);
}
template<class T, int dim>
Tree<T,dim>::~Tree(){
    detroy(r);
}

template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::root(){
    return Iterator(r);
}

template<class T, int dim>
template<class Lambda>
void Tree<T,dim>::detach(const Tree& dest, Lambda fn){
    Iterator curr = root();
    for(int i=0; i<(2^dim); i++){
        curr.currIndex = i;
        detach(curr, dest, fn);
    }
    r = nullptr;
    curr.ptr->position = fn(curr);
    dest.insert(curr);
}
template<class T, int dim>
template<class Lambda>
void Tree<T,dim>::detach(const Iterator& t, const Tree& dest, Lambda fn){
    if(t == Iterator::pastEnd )return;
    Iterator curr = t[t.currIndex];
    if(curr == Iterator::pastEnd) return;

    //prima stacco i figli poi il sottoalbero
    int startIndex = curr.currIndex;
    curr.currIndex=0;
    for(int i=0; i<(2^dim); i++){
       curr.currIndex = i;
       detach(curr, dest, fn);
    }
    t.ptr->children[startIndex]=nullptr;
    curr.ptr->position = fn(curr);
    dest.insert(curr);
}

#endif // MODEL_H
