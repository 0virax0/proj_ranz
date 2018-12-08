#ifndef MODEL_H
#define MODEL_H
#include <vector>
template<class T, int dim>
class Tree{
protected:
    class Node{
    public:
        T* data;
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
protected: Iterator r; //primo nodo radice
public:
    Iterator& root();

    Tree();
    Tree(const Tree& t);    //copia profonda
    Tree& operator=(const Tree& t); //assegnazione profonda

    ~Tree();    //distruzione profonda
};

template<class T, int dim>
class NearTree : public Tree<T, dim>{   //definisce il concetto di distanza euclidea
protected:
    static std::vector<T>& findNrecursive(std::vector<T>& v, float targetPos[dim], int limit, typename Tree<T,dim>::Iterator it);
public:
    std::vector<T>& findNeighbouring(std::vector<T>& v, float targetPos[dim], int limit, typename Tree<T,dim>::Iterator it = Tree<T,dim>::root()) const;
};

#endif // MODEL_H
