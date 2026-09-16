#pragma once

typedef struct {
    int *buffer;
    int size;
} sma_filter_t;

void sma_filter_init(sma_filter_t *filter, int *buffer, int size, int initial_value);
int  sma_filter_update(sma_filter_t *filter, int new_value);