#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "i_circular_buffer_repository.h"
#include "circular_buffer_infra_file_service.h"
#include "i_circular_buffer.h"

#define MAX_RECORD 1024
#define INDEX_SUFFIX ".ndx"
#define DATA_SUFFIX ".rec"
#define FILE_DB_REPO "../Persistence/FileDB/CircularBuffer/CIRCULAR_BUFFER"

struct RECORD_CB
{
    unsigned long data_length;
    int offset_head;
    int offset_current;
    char data[10]; // new in c99 !!
};

struct circular_buffer
{
    char *tail;
    unsigned long length;
    char *head;
    char *current;
    bool isFull;
};

struct index
{
    long recordStart;
    size_t recordLength;
};

static FILE *index_stream;
static FILE *data_stream;

int ICircularBufferRepository_save(circular_buffer cb)
{
    if (!ICircularBufferRepository_open(FILE_DB_REPO))
        return 0;
    ICircularBufferRepository_append(cb);

    return 1;
}

void ICircularBufferRepository_close(void)
{
    fclose(data_stream);
    fclose(index_stream);
}

static FILE *auxiliary_open(char *prefix, char *suffix)
{
    int prefix_length = strlen(prefix);
    int suffix_length = strlen(suffix);
    char name[prefix_length + suffix_length + 1];
    strncpy(name, prefix, prefix_length);
    strncpy(name + prefix_length, suffix, suffix_length + 1);
    FILE *stream = fopen(name, "r+");
    if (stream == NULL)
        stream = fopen(name, "w+");
    if (stream == NULL)
        perror(name);
    return stream;
}

int ICircularBufferRepository_open(char *name)
{
    data_stream = auxiliary_open(name, DATA_SUFFIX);
    if (data_stream == NULL)
    {
        return 0;
    }
    index_stream = auxiliary_open(name, INDEX_SUFFIX);
    if (index_stream == NULL)
    {
        fclose(data_stream);
        return 0;
    }
    return 1;
}

int ICircularBufferRepository_append(circular_buffer cb)
{
    struct index index;
    // struct RECORD_CB *record = malloc(sizeof(struct RECORD_CB));
    struct RECORD_CB record;
    record.offset_head = (cb->head - cb->tail) / sizeof(*(cb->head));
    record.offset_current = (cb->current - cb->tail) / sizeof(*(cb->current));
    record.data_length = cb->length;
    // printf("head,curr,length %d %d %ld\n", record.offset_head, record.offset_current, record.data_length);
    char *temporay = cb->head - 1;
    int i = record.offset_head - 1;
    while (i >= 0)
    {
        // printf("temp:%d-> ", *(temporay));
        record.data[i] = *(temporay);
        // printf("rec:%d , ", record.data[i]);
        i -= 1;
        temporay--;
    }
    fseek(data_stream, 0L, SEEK_END);
    index.recordStart = ftell(data_stream);
    index.recordLength = sizeof(struct RECORD_CB);
    fwrite(&record, index.recordLength, 1, data_stream);
    struct RECORD_CB testrec;
    // testing for reading capability
    // fseek(data_stream, 0L, SEEK_SET);
    // fread(&testrec, index.recordLength, 1, data_stream);
    // // printf("\ntestrec\n");
    // // printf("head,curr,length %d %d %ld\n", testrec.offset_head, testrec.offset_current, testrec.data_length);
    // // for (int i = 0; i < testrec.offset_head; i++)
    // // {
    // //     printf("rec->%d", testrec.data[i]);
    // // }
    // // ret
    fseek(index_stream, 0L, SEEK_END);
    fwrite(&index, sizeof index, 1, index_stream);
    return 1;
}

circular_buffer CircularBufferRepository_get_nth_cb(int rank)
{
    // printf("stating get data\n");
    struct index index;
    long shift = (rank - 1) * sizeof index;
    fseek(index_stream, shift, SEEK_SET);
    fread(&index, sizeof index, 1, index_stream);
    fseek(data_stream, index.recordStart, SEEK_SET);
    // struct RECORD_CB *myRecord = malloc(sizeof(struct RECORD_CB));
    struct RECORD_CB myRecord;
    fread(&myRecord, index.recordLength, 1, data_stream);
    // printf("head,curr,length %d %d %ld\n", myRecord.offset_head, myRecord.offset_current, myRecord.data_length);
    circular_buffer cb = (circular_buffer)malloc(sizeof(struct circular_buffer));
    cb = CircularBuffer_construct(myRecord.data_length);
    // printf("cb created\n");
    for (int i = 0; i < myRecord.offset_head; i++)
    {
        // printf("rec->%d", myRecord.data[i]);
        CircularBuffer_append_char_at_head(cb, myRecord.data[i]);
    }
    cb->current = cb->tail + myRecord.offset_current;
    printf("loaded current offset pos%d \n", myRecord.offset_current);
    return cb;
}
circular_buffer ICircularBufferRepository_get_nth_cb(int rank)
{
    if (!ICircularBufferRepository_open(FILE_DB_REPO))
        return EXIT_FAILURE;
    return CircularBufferRepository_get_nth_cb(rank);
}