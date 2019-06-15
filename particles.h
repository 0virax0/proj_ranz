#ifndef PARTICLES_H
#define PARTICLES_H
#include <vector>
#include <QString>
#include <QXmlStreamWriter>
#include <math.h>
#include "utils.h"

using std::vector;


class Particle2{    //la quantità di materia è costante
public:
   class Properties{
   public:
        vector<float> position;
        vector<float> velocity;
        float mass;
        float pressure;
        float temperature;

        Properties();
        Properties(const vector<float>& pos, const vector<float>& vel, float m, float p, float t);
        Properties(const Properties& p);
        Properties& operator=(const Properties& p);
   };
   Properties* properties;

   Particle2* substitute; //segnala che occorre sostituire la particella con quella puntata

   static bool useGravity;

   virtual void advect(const vector<Particle2*>& neighbours, float deltaTime);
   bool swapState(float deltaTime);
   virtual vector<int> getColor()const =0;
   virtual bool serialize(QXmlStreamWriter&)const;

   Particle2(); //create properties in the heap
   Particle2(const vector<float>& pos, const vector<float>& vel, float m, float p, float t, float cond, float spec_heat);
   Particle2(const Particle2&);
   Particle2(QXmlStreamReader&); //create from deserialization
   virtual ~Particle2();

protected:
   Properties* newProperties;
   float conductivity;  // K/s da 0 a 100K
   float specific_heat;  // Joule/K
   float entropy;  // Joule per ragiungerla

   vector<float> correctionDir;

};

class Solid : public virtual Particle2{
public:
   float friction;

   void advect(const vector<Particle2*>& neighbours, float deltaTime) override;
   bool serialize(QXmlStreamWriter&)const override;

   Solid(float fric);
   Solid(QXmlStreamReader&);
};
class Liquid : public virtual Particle2{
   public:
   void advect(const vector<Particle2*>& neighbours, float deltaTime) override;
   bool serialize(QXmlStreamWriter&)const override;

   Liquid();
   Liquid(QXmlStreamReader&);
};
class Gas : public virtual Particle2{
public:
   void advect(const vector<Particle2*>& neighbours, float deltaTime) override;
   bool serialize(QXmlStreamWriter&)const override;

   Gas();
   Gas(QXmlStreamReader&);
};
class Explosive : public virtual Particle2{
public:
   float threshold_temp;
   float threshold_pressure;
   float explosion_pressure;

   void advect(const vector<Particle2*>& neighbours, float deltaTime) override;
   bool serialize(QXmlStreamWriter&)const override;

   Explosive(float thresh_t, float thresh_p, float explosion_pres);
   Explosive(QXmlStreamReader&);
};

//classi concrete
class Water : public Liquid{
public:
    static vector<int> color;
    vector<int> currColor;

    void advect(const vector<Particle2*>& neighbours, float deltaTime) override;
    vector<int> getColor()const override;
    bool serialize(QXmlStreamWriter&)const override;

    Water(const vector<float>& pos, const vector<float>& vel = {0,0});
    Water(QXmlStreamReader&);
    ~Water() override;
};
class Ice : public Solid{
public:
    static vector<int> color;
    vector<int> currColor;

    void advect(const vector<Particle2*>& neighbours, float deltaTime) override;
    vector<int> getColor()const override;
    bool serialize(QXmlStreamWriter&)const override;

    Ice(const vector<float>& pos, const vector<float>& vel = {0,0});
    Ice(QXmlStreamReader&);
    ~Ice() override;
};
class Steam : public Gas{
public:
    static vector<int> color;
    vector<int> currColor;

    void advect(const vector<Particle2*>& neighbours, float deltaTime) override;
    vector<int> getColor()const override;
    bool serialize(QXmlStreamWriter&)const override;

    Steam(const vector<float>& pos, const vector<float>& vel = {0,0});
    Steam(QXmlStreamReader&);
    ~Steam() override;
};

class GunPowder : public Solid, public Explosive{
public:
    static vector<int> color;
    vector<int> currColor;

    void advect(const vector<Particle2*>& neighbours, float deltaTime) override;
    vector<int> getColor()const override;
    bool serialize(QXmlStreamWriter&)const override;

    GunPowder(const vector<float>& pos, const vector<float>& vel = {0,0});
    GunPowder(QXmlStreamReader&);
    ~GunPowder() override;
};

class Fire : public Gas{
public:
    static vector<int> color;
    vector<int> currColor;

    void advect(const vector<Particle2*>& neighbours, float deltaTime) override;
    vector<int> getColor()const override;
    bool serialize(QXmlStreamWriter&)const override;

    Fire(const vector<float>& pos, const vector<float>& vel = {0,0}, float start_pressure = 1.0f);
    Fire(QXmlStreamReader&);
    ~Fire() override;
protected:
    float materialLeft;
};

#endif // PARTICLES_H
