#ifndef UTILS_H
#define UTILS_H
#include <vector>
#include <math.h>
using std::vector;

template<class T>
constexpr T ipow(T n, int pow){ //potenze di interi a compile time
    return pow == 0 ? 1 : n * ipow(n, pow-1);
}


class utils
{
public:
    //metodi per gestire vettori a 2 dimensioni
    static  vector<float> add(const vector<float>& v1, const vector<float>& v2);
    static  void add_side(vector<float>& v1, const vector<float>& v2);
    static  vector<float> sub(const vector<float>& v1, const vector<float>& v2);
    static  void sub_side(vector<float>& v1, const vector<float>& v2);
    static  vector<float> mul(const vector<float>& v1, float scalar);
    static  void mul_side(vector<float>& v1, float scalar);
    static  float sqDist(const vector<float>& v1, const vector<float>& v2);
    static  float sqLength(const vector<float>& v1);
    static  void normalize(vector<float>& v1);
    static  float dot(const vector<float>& v1, const vector<float>& v2);
    static  void rotate90(vector<float>& v1);
};

#endif // UTILS_H
