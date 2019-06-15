#include "utils.h"

vector<float> utils::add(const vector<float>& v1, const vector<float>& v2){
    return {v1[0]+v2[0], v1[1]+v2[1]};
}
void utils::add_side(vector<float>& v1, const vector<float>& v2){
    v1[0] += v2[0];
    v1[1] += v2[1];
}
vector<float> utils::sub(const vector<float>& v1, const vector<float>& v2){
    return {v1[0]-v2[0], v1[1]-v2[1]};
}
void utils::sub_side(vector<float>& v1, const vector<float>& v2){
    v1[0] -= v2[0];
    v1[1] -= v2[1];
}
vector<float> utils::mul(const vector<float>& v1, float scalar){
    return {v1[0]*scalar, v1[1]*scalar};
}
void utils::mul_side(vector<float>& v1, float scalar){
    v1[0] *= scalar;
    v1[1] *= scalar;
}
float utils::sqDist(const vector<float>& v1, const vector<float>& v2){
    return (v1[0]-v2[0])*(v1[0]-v2[0]) + (v1[1]-v2[1])*(v1[1]-v2[1]);
}
float utils::sqLength(const vector<float>& v1){
    return v1[0]*v1[0] + v1[1]*v1[1];
}
void utils::normalize(vector<float>& v1){
    float length = sqrt(sqLength(v1));
    v1[0] /= length;
    v1[1] /= length;
}
float utils::dot(const vector<float>& v1, const vector<float>& v2){
    return v1[0]*v2[0] + v1[1]*v2[1];
}
void utils::rotate90(vector<float>& v1){
   float tmp = v1[1];
   v1[1] = -v1[0];
   v1[0] = tmp;
}
