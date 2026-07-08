#ifndef HISTORY_H
#define HISTORY_H
 
#define HISTORY_LEN 60
 
typedef struct {
    float samples[HISTORY_LEN];
    int   head;
    int   count;
} History;
 
void  history_push(History *h, float value);
float history_get(const History *h, int age);
float history_max(const History *h);
float history_min(const History *h);
 
#endif