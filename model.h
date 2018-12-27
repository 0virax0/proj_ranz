#ifndef MODEL_H
#define MODEL_H
#include <vector>
#include <math.h>
#include <QString>
#include <QXmlStreamWriter>
using std::vector;

template<class T, int dim> class NearTree;

template<class T, int dim> //dim (1..8)
class Tree{     //albero n-dimensionale di ricerca
protected:
    class Node{
        friend class Iterator;
        friend class Tree;
        friend class NearTree<T,dim>;
    protected:
        T* data;    //deve essere allocato nello heap
        Node* setChild(Node* n, int index);
        vector<Node*> children;
        vector<float> bounds; //bounds è del tipo [Xmin,Xmax , Ymin,Ymax , ...]
    public:
        vector<float> position;

        Node();
        Node(const Node& n);
        Node(T* d, vector<float> pos);
        Node& operator =(const Node&);  //copio profondamente data, ma non i figli

        ~Node();       //distruggo anche l'oggetto puntato ma non i figli
    };

    Node* r;

    static Node* copy(Node*);
    static void destroy(Node*);
    static bool _findNearest(Node*& n, const vector<float>& pos, Node**& curr, float& currSqDistance, bool onlyLeaves); //ritorno in curr un puntatore al puntatore, con onlyLeaves prendo la foglia più vicina
    //del padre del nodo più vicino ma non sovrapposto alla posizione pos
    bool _del(Node** i);
public:
    class Iterator{
        friend class Tree;
        friend class NearTree<T,dim>;
    protected:
        Node* ptr;  //nodo<T>
        int currIndex;
    public:
        static Iterator pastEnd;

        Iterator();
        Iterator(const Iterator& i);
        explicit Iterator(Node* n, int index = 0);

        bool operator==(const Iterator&) const;
        bool operator!=(const Iterator&) const;
        //se tento di andare pastTheEnd, non modifico ptr, ma ritorno l'iteratore pastEnd dereferenziato
        Iterator operator[](int child);    //ritorno un figlio del nodo e setto currIndex
        Iterator operator++();             //scendo di un livello, nel figlio currIndex (non salgo per evitare il doppio linkaggio)
        Iterator operator++(int);

        T& operator*()const;
        T* operator->()const;

        Iterator& operator =(const Iterator&);
    };
    Iterator root();

    static const int Nchild = pow(2, dim);

    Tree();
    Tree(const Tree& t);    //copia profonda
    Tree& operator=(const Tree& t); //assegnazione profonda

    Iterator insert(T* t, const vector<float> newPos);
    Iterator insert(const Iterator& t);        //sposto un nodo(anche da un altro albero)

    template<class Lambda>
    void detach(Tree& dest, Lambda fn); //stacca tutto l'albero
    template<class Lambda>
    static void detach(Iterator t, Tree& dest, Lambda fn); //stacca un sottoalbero s da t[s], applica la lambda sui nodi e li inserisce in un altro albero, fn deve ritornare una nuova position per il nodo

    Iterator findNearest(const vector<float>& position );  //ritorna pastEnd se l'albero è vuoto o contiene un elemento nell'esatta posizione

    // attenzione con i metodi delete, se un iteratore puntava a un nodo che viene tolto, dereferenziandolo dà segFault
    bool del(); //toglie la radice
    bool del(Iterator d);   //toglie il nodo puntato da d[currIndex]

    ~Tree();    //distruzione profonda
};
template<class T, int dim> typename Tree<T,dim>::Iterator Tree<T,dim>::Iterator::pastEnd = Tree<T,dim>::Iterator();

template<class T, int dim>
class NearTree : public Tree<T, dim>{
public: static const int Nintersec = pow(3, dim);
protected:
    static bool singleConstructed;
    static float intersections[Nintersec][dim]; //definisce tutte le intersezioni tra gli spazi dei figli, ordinate per connettività decrescente (-1, 0 per coordinate condivise, 1)
    static unsigned char interMask[Nintersec][2];

    static void interCombinations(unsigned int zeroes, unsigned int indexX, vector<int> tmp, vector<vector<int>>& res);
    static void findNrecursive(typename Tree<T,dim>::Node*& n, const vector<float>& targetPos, float maxDistanceSq, std::vector<typename Tree<T,dim>::Node**>& v);
public:
    NearTree(); //costruisce solo la prima volta i membri statici per questa istanza di NearTree

    void findNeighbouring(typename Tree<T,dim>::Iterator target, float maxDistanceSq, std::vector<T*>& v);
    void deleteNeighbouring(const vector<float>& targetPos, float maxDistanceSq);
};
template<class T, int dim> bool NearTree<T,dim>::singleConstructed = false;
template<class T, int dim> float NearTree<T,dim>::intersections[NearTree::Nintersec][dim];
template<class T, int dim> unsigned char NearTree<T,dim>::interMask[NearTree::Nintersec][2];

class Particle2{
public:
   class Properties{
   public:
        vector<float> position;
        vector<float> velocity;
        float pressure;
        float temperature;

        Properties();
        Properties(const Properties& p);
   };
   Properties* properties;

   virtual void advect(std::vector<Particle2>)=0;
   virtual bool swapState();
   virtual bool serialize(QXmlStreamWriter&);

   Particle2(); //create properties in the heap
   Particle2(const Particle2&);
   Particle2(QXmlStreamReader&); //create from deserialization
   virtual ~Particle2();

protected:
   Properties* newProperties;

};

class solid : public virtual Particle2{
public:
   void advect(std::vector<Particle2>) override;
   bool serialize(QXmlStreamWriter&) override;

   solid(QXmlStreamReader&);
};
class liquid : public virtual Particle2{};
class gas : public virtual Particle2{};
class explosive : public virtual Particle2{};
class corrosive : public virtual Particle2{};

//implementazione Node
template<class T, int dim>
Tree<T,dim>::Node::Node(): data(nullptr), children( Nchild, nullptr), bounds(2*dim, 0) , position(dim,0){ }
template<class T, int dim>
Tree<T,dim>::Node::Node(const Node& n): data(new T(*n.data)), children(n.children), bounds(n.bounds), position(n.position){  }
template<class T, int dim>
Tree<T,dim>::Node::Node(T* d, vector<float> pos):data(d), children(Nchild, nullptr), bounds(2*dim, 0), position(pos) {}
template<class T, int dim>
typename Tree<T,dim>::Node& Tree<T,dim>::Node::operator =(const typename Tree<T,dim>::Node& n){
    position = n.position;
    data = new T(*n.data);
    children(dim, nullptr);
    bounds(n->bounds);
    return *this;
}
template<class T, int dim>
Tree<T,dim>::Node::~Node(){delete data;}
template<class T, int dim>
typename Tree<T,dim>::Node* Tree<T,dim>::Node::setChild(Node* n, int index){
   children[index] = n;
   return n;
}

//implementazione Tree::Iterator
template<class T, int dim>
Tree<T,dim>::Iterator::Iterator() : ptr(nullptr), currIndex(0){}
template<class T, int dim>
Tree<T,dim>::Iterator::Iterator(const Iterator& i) : ptr(i.ptr), currIndex(i.currIndex){}
template<class T, int dim>
Tree<T,dim>::Iterator::Iterator(Node* n, int index) : ptr(n), currIndex(index){}

template<class T, int dim>
bool Tree<T,dim>::Iterator::operator==(const typename Tree<T,dim>::Iterator& it) const{return ptr==it.ptr && (ptr==nullptr || currIndex==it.currIndex);}
template<class T, int dim>
bool Tree<T,dim>::Iterator::operator!=(const typename Tree<T,dim>::Iterator& it ) const{return !(ptr==it.ptr && (ptr==nullptr || currIndex==it.currIndex));}

template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::Iterator::operator[](int child){
    Node* n = ptr->children[child];
    if(n) currIndex = child;
    return Iterator(n);
}
template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::Iterator::operator++(){
    Node* n = ptr->children[currIndex];
    if(!n) return pastEnd;
    ptr = n;
    currIndex = 0;
    return *this;
}
template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::Iterator::operator++(int){
    Node* n = ptr->children[currIndex];
    if(!n) return pastEnd;
    Iterator i = *this;
    ptr = n;
    currIndex = 0;
    return i;
}

template<class T, int dim>
T& Tree<T,dim>::Iterator::operator*()const{
    return *(ptr->data);
}
template<class T, int dim>
T* Tree<T,dim>::Iterator::operator->()const{
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
typename Tree<T,dim>::Iterator Tree<T,dim>::insert(T* t, const vector<float> newPos)
{
    Node* n = new Node(t, newPos);
    return insert(Iterator(n));
}
template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::insert(const Iterator& t){
    Node* n = t.ptr;
    Iterator p = root();

    //inizializzo i bounds
    vector<float> standardBounds(2*dim, 0);
    for(unsigned int i=0; i<2*dim; i+=2){
       standardBounds[i] = 0.0f;
       standardBounds[i+1] = 1.0f;
    }

    if(p==Iterator::pastEnd){
        r = n;
        n->bounds = standardBounds;
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
              index += pow(2, i); //se sono strettamente maggiore al pivot nella dimensione considerata setto la bitmask in modo da puntare il figlio corretto
        nP = p[index]; //seleziono il sottoalbero
    }

    //setto i nuovi bounds
    for(unsigned int d=0; d<dim; d++){ //ogni dimensione
        unsigned char isAfterD = static_cast<unsigned char>(index);
        isAfterD &= 1<<d; //bitmask

        if(isAfterD){
            n->bounds[2*d] = p.ptr->position[d]; //dMin
            n->bounds[2*d+1] = p.ptr->bounds[2*d+1]; //dMax
        }else{
            n->bounds[2*d] = p.ptr->bounds[2*d]; //dMin
            n->bounds[2*d+1] = p.ptr->position[d]; //dMax
        }
    }

    return Iterator(p.ptr->setChild(n,index));
}

template<class T, int dim>
typename Tree<T,dim>::Node* Tree<T,dim>::copy(Node* r){
    if(!r)return nullptr;

    Node* n = new Node(r);
    for(int i=0; i<(Nchild); i++){
        n->children[i] = copy(r->children[i]);
    }
    return n;
}
template<class T, int dim>
void Tree<T,dim>::destroy(Node* r){
   if(!r)return;

   for(int i=0; i<(Nchild); i++){
       destroy(r->children[i]);
   }
   delete r;
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
    destroy(r);
}

template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::root(){
    return Iterator(r);
}

template<class T, int dim>
template<class Lambda>
void Tree<T,dim>::detach(Tree& dest, Lambda fn){
    Iterator curr = root();
    for(int i=0; i<(Nchild); i++){
        curr.currIndex = i;
        detach(curr, dest, fn);
    }
    r = nullptr;
    curr.ptr->position = fn(curr);
    dest.insert(curr);
}
template<class T, int dim>
template<class Lambda>
void Tree<T,dim>::detach(Iterator t, Tree& dest, Lambda fn){
    if(t == Iterator::pastEnd )return;
    Iterator curr = t[t.currIndex];
    if(curr == Iterator::pastEnd) return;

    //prima stacco i figli poi il sottoalbero
    int startIndex = curr.currIndex;
    curr.currIndex=0;
    for(int i=0; i<(Nchild); i++){
       curr.currIndex = i;
       detach(curr, dest, fn);
    }
    t.ptr->children[startIndex]=nullptr;
    curr.ptr->position = fn(curr);
    dest.insert(curr);
}

template<class T, int dim>
bool Tree<T,dim>::_findNearest(Node*& n, const vector<float>& pos, Node**& curr, float& currSqDistance, bool onlyLeaves){
    if(!n)return false;
    bool retVal = false;

    bool skip = false;
    if(onlyLeaves) //se ho almeno un figlio skippo
        for(unsigned int i=0; i<(Nchild); i++) skip |= n->children[i] != nullptr;
    if(!skip){
    //controllo la mia distanza quadratica
        float sqDist = 0;
        for(int i=0; i<dim; i++) sqDist += (n->position[i] - pos[i]) * (n->position[i] - pos[i]);
        if(sqDist != 0.0f && sqDist < currSqDistance){ //se la distanza è 0 sono proprio sul nodo
            curr = &n;
            currSqDistance = sqDist;

            retVal = true;
        }
    }
    //stimo la minima distanza quadratica di un sottoalbero dai suoi bounds, per capire se continuare la ricerca
    for(unsigned int c=0; c<(Nchild); c++){ //ogni sottoalbero
        Node* child = n->children[c];
        if(child){ //ho il figlio da analizzare

           float minSqDistance = 0;
           for(unsigned int d=0; d<dim; d++){ //per ogni dimensione sommo la componente^2 alla distanza solo se la supero
              float min = child->bounds[2*d] - pos[d];
              float max = child->bounds[2*d+1] - pos[d];
              if(min > 0) minSqDistance += min * min;
              if(max < 0) minSqDistance += max * max;
           }
           if(minSqDistance < currSqDistance) retVal |= _findNearest(n->children[c], pos, curr, currSqDistance, onlyLeaves);
        }
    }
    return retVal;
}
template<class T, int dim>
typename Tree<T,dim>::Iterator Tree<T,dim>::findNearest(const vector<float>& position ){
    Node** nearestNodePtr;
    float maxDistance = 2;  //maggiore della massima distanza possibile (sqrt(2))
    bool found = _findNearest(r, position, nearestNodePtr, maxDistance, false);

    if(!found) return Iterator::pastEnd;
    return Iterator(*nearestNodePtr);
}

template<class T, int dim>
bool Tree<T,dim>::del(){
    return r!=nullptr && _del(&r);
}
template<class T, int dim>
bool Tree<T,dim>::del(typename Tree<T,dim>::Iterator d){
    return d != Iterator::pastEnd && _del(&(d.ptr->children[d.currIndex]));
}
template<class T, int dim>
bool Tree<T,dim>::_del(Node** i){
    if(!i || !(*i)) return false;   //non ho nulla da rimuovere

    Node* n = *i;
    //controllo se il nodo da togliere ha figli
    bool hasChildren = false;
    for(unsigned int i=0; i<(Nchild); i++) hasChildren |= n->children[i] != nullptr;
    if(!hasChildren){
        //tolgo il nodo direttamente
        *i = nullptr;
        delete n;
        return true;
    }
    //scambio con il nodo foglia più vicino nel sottoalbero
    Node** nearestNodePtr;
    float dist = 2; //maggiore della massima distanza possibile
    _findNearest(n, n->position, nearestNodePtr, dist, true);
    (*nearestNodePtr)->children = n->children; //il nuovo nodo ha gli stessi figli
    *i = *nearestNodePtr; //il padre ora punta a lui
    *nearestNodePtr = nullptr; //rimuovo il vecchio ptr alla foglia
    delete n;
    return true;
}

//implementazione nearTree
template<class T, int dim>
NearTree<T,dim>::NearTree(){
    if(singleConstructed) return;
    singleConstructed = true;

//costruisco la matrice instersections
    vector<vector<int>> res;
    for(int i=3; i>=0; i--) interCombinations(i, 0, vector<int>(dim,0), res); //costruisco tutte le combinazioni con (i) zeri
    //inserisco
    for(unsigned int y=0; y<Nintersec; y++){
        unsigned char nMask = static_cast<unsigned char>(0);
        unsigned char cMask = static_cast<unsigned char>(0);
        for(unsigned int x=0; x<dim; x++){
           intersections[y][x] = res[y][x];

           //neutralizer mask (-1 -> 1, 1 -> 1, 0 -> 0)
           switch(res[y][x]){
           case -1: nMask |= 1<<x; break;
           case  1: nMask |= 1<<x; break;
           }

           //comparison mask (-1 -> 0, 1 -> 1, 0 -> 0)
           switch(res[y][x]){
           case 1: cMask |= 1<<x;  break;
           }
        }
        interMask[y][0] = nMask;
        interMask[y][1] = cMask;
    }
}

template<class T, int dim>
void NearTree<T,dim>::interCombinations(unsigned int zeroes, unsigned int indexX, vector<int> tmp, vector<vector<int>>& res){
    if(indexX == dim){
        res.push_back(tmp); //ho completato una riga, la salvo
        return;
    }
    if(zeroes == dim-indexX){ //devo aggiungere solo zeri per completare la riga
        tmp[indexX] = 0;
        return interCombinations(zeroes-1, indexX+1, tmp, res);
    }

    if(zeroes>0){
        tmp[indexX]=0;
        interCombinations(zeroes-1, indexX+1, tmp, res);
    }
    tmp[indexX] = -1;   interCombinations(zeroes, indexX+1, tmp, res);
    tmp[indexX] =  1;   interCombinations(zeroes, indexX+1, tmp, res);

}
template<class T, int dim>
void NearTree<T,dim>::findNrecursive(typename Tree<T,dim>::Node*& n, const vector<float>& targetPos, float maxDistanceSq, std::vector<typename Tree<T,dim>::Node**>& v){
    if(!n)return;

    //controllo tutte le intersezioni in ordine di decrescente connettività, fino a che non ne trovo una che giace a una distanza minore di maxDistance
    //a quel punto cerco i sottoalberi toccati dall'intersezione e li controllo.
    unsigned int interIndex = 0;
    bool found = false;
    while(!found && interIndex < Nintersec){
        float distAcc = 0;
        for(unsigned int i=0; i< dim; i++){ //calcolo la distanza
           int interComponent = intersections[interIndex][i];
           float posComponent = 0;

           if(interComponent == 0) posComponent = n->position[i];
           else if(interComponent == -1) posComponent = n->bounds[2*i];
           else posComponent = n->bounds[2*i + 1];

           posComponent -= targetPos[i];
           distAcc += posComponent * posComponent;
        }

        if(distAcc < maxDistanceSq) found = true;
        else interIndex++;
    }
    if(found){
        //trovo sottoalberi corrispondenti
        for(int c=0; c< Tree<T,dim>::Nchild; c++){
            unsigned char indexMask = static_cast<unsigned char>(c);
            indexMask &= interMask[interIndex][0]; //neutralizzo i valori dove in intermask ho 0
            if(indexMask == interMask[interIndex][1]) //vedo se matcha
               findNrecursive(n->children[c], targetPos, maxDistanceSq, v); //cerco nel sottoalbero
        }
        //mi inserisco in v solo ora perchè voglio che v sia in postordine
        if(interIndex == 0) v.push_back(&n);    //con indice 0 indico l'intersezione con maggiore connettività ovvero il nodo stesso
    }
}

template<class T, int dim>
void NearTree<T,dim>::findNeighbouring(typename Tree<T,dim>::Iterator target, float maxDistanceSq, std::vector<T*>& v){
    typename Tree<T,dim>::Node*& n = target.ptr;

    vector<typename Tree<T,dim>::Node**> t; //conterrà i nodi da eliminare partendo dalle foglie, in modo da avere meno nodi da spostare
    findNrecursive(Tree<T,dim>::r, Tree<T,dim>::r->position, maxDistanceSq, t);

    for(auto it=t.begin(); it!=t.end(); it++){
        if(*(*it) != n){    //escludo il nodo passato per parametro
           v.push_back((**it)->data);
        }
    }
}
template<class T, int dim>
void NearTree<T,dim>::deleteNeighbouring(const vector<float>& targetPos, float maxDistanceSq){
    vector<typename Tree<T,dim>::Node**> v; //conterrà i nodi da eliminare partendo dalle foglie, in modo da avere meno nodi da spostare
    findNrecursive(Tree<T,dim>::r, targetPos, maxDistanceSq, v);

    for(auto it = v.begin(); it!=v.end(); it++) Tree<T,dim>::_del(*it);
}

#endif // MODEL_H
