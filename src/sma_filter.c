#include "sma_filter.h"

void sma_filter_init(sma_filter_t *filter, int *buffer, int size, int initial_value)
{
    filter->buffer = buffer;
    filter->size = size;

    for (int i = 0; i < size; i++)
        filter->buffer[i] = initial_value;
}

int sma_filter_update(sma_filter_t *filter, int new_value)
{
    for (int i = 0; i < filter->size - 1; i++)
        filter->buffer[i] = filter->buffer[i + 1];

    filter->buffer[filter->size - 1] = new_value;

    long sum = 0;
    for (int i = 0; i < filter->size; i++)
        sum += filter->buffer[i];

    return (int)(sum / filter->size);
}