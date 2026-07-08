#include "history.h"
#include <float.h>
 
void history_push(History *h, float value)
{
    if (h->count < HISTORY_LEN) {
        h->samples[h->count] = value;
        h->count++;
    } else {
        h->samples[h->head] = value;
        h->head = (h->head + 1) % HISTORY_LEN;
    }
}

float history_get(const History *h, int age)
{
    if (age < 0 || age >= h->count)
        return 0.0f;
    int idx = (h->head + h->count - 1 - age) % HISTORY_LEN;
    return h->samples[idx];
}
 
float history_max(const History *h)
{
    float m = -FLT_MAX;
    for (int i = 0; i < h->count; i++)
        if (h->samples[i] > m) m = h->samples[i];
    return (m == -FLT_MAX) ? 0.0f : m;
}
 
float history_min(const History *h)
{
    float m = FLT_MAX;
    for (int i = 0; i < h->count; i++)
        if (h->samples[i] < m) m = h->samples[i];
    return (m == FLT_MAX) ? 0.0f : m;
}