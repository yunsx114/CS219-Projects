#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
//#include <omp.h>

#define CYCLE_TIME 100

// 函数声明
int calculate_vector_int(int* data, int N, int dim);
int calculate_vector_short(short* data, int N, int dim);
int calculate_vector_byte(int8_t* data, int N, int dim);
double calculate_vector_double(double* data, int N, int dim);
float calculate_vector_float(float* data, int N, int dim);

// 时间差计算函数
long time_diff(struct timespec *start, struct timespec *end) {
    if (start && end) {
        return (end->tv_sec - start->tv_sec) * 1e9 + (end->tv_nsec - start->tv_nsec);
    } else {
        printf("wrong time interval parsing\n");
        return 0;
    }
}

int main(int argc, char* argv[]) {

    int num_threads = 1;
    //omp_get_num_procs()
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads <= 0) num_threads = 4;
    }
    omp_set_num_threads(num_threads);

    const char* filename = "randomCase.txt";
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("can't open file");
        return 1;
    }

    int type, dim, N;
    if (fscanf(file, "%d %d %d", &type, &dim, &N) != 3) {
        printf("wrong file header\n");
        fclose(file);
        return 1;
    }

    void* data = NULL;
    long double sum = 0;
    struct timespec start, end;
    long avg_interval = 0;

    switch (type) {
        case 1: { // int
            data = malloc(N * dim * sizeof(int));
            int* flat_data = (int*)data;
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < dim; j++) {
                    if (fscanf(file, "%d", &flat_data[i * dim + j]) != 1) {
                        printf("parse number failed\n");
                        goto cleanup;
                    }
                }
            }
            for (int i = 0; i < CYCLE_TIME; i++) {
                clock_gettime(CLOCK_MONOTONIC, &start);
                sum = calculate_vector_int(flat_data, N, dim);
                clock_gettime(CLOCK_MONOTONIC, &end);
                avg_interval += time_diff(&start, &end);
            }
            avg_interval /= CYCLE_TIME;
            break;
        }
        case 2: { // short
            data = malloc(N * dim * sizeof(short));
            short* flat_data = (short*)data;
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < dim; j++) {
                    if (fscanf(file, "%hd", &flat_data[i * dim + j]) != 1) {
                        printf("parse number failed\n");
                        goto cleanup;
                    }
                }
            }
            for (int i = 0; i < CYCLE_TIME; i++) {
                clock_gettime(CLOCK_MONOTONIC, &start);
                sum = calculate_vector_short(flat_data, N, dim);
                clock_gettime(CLOCK_MONOTONIC, &end);
                avg_interval += time_diff(&start, &end);
            }
            avg_interval /= CYCLE_TIME;
            break;
        }
        case 3: { // unsigned char
            data = malloc(N * dim * sizeof(int8_t));
            int8_t* flat_data = (int8_t*)data;
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < dim; j++) {
                    int tmp;
                    if (fscanf(file, "%d", &tmp) != 1) {
                        printf("parse number failed\n");
                        goto cleanup;
                    }
                    flat_data[i * dim + j] = (int8_t)tmp;
                }
            }
            for (int i = 0; i < CYCLE_TIME; i++) {
                clock_gettime(CLOCK_MONOTONIC, &start);
                sum = calculate_vector_byte(flat_data, N, dim);
                clock_gettime(CLOCK_MONOTONIC, &end);
                avg_interval += time_diff(&start, &end);
            }
            avg_interval /= CYCLE_TIME;
            break;
        }
        case 4: { // double
            data = malloc(N * dim * sizeof(double));
            double* flat_data = (double*)data;
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < dim; j++) {
                    if (fscanf(file, "%lf", &flat_data[i * dim + j]) != 1) {
                        printf("parse number failed\n");
                        goto cleanup;
                    }
                }
            }
            for (int i = 0; i < CYCLE_TIME; i++) {
                clock_gettime(CLOCK_MONOTONIC, &start);
                sum = calculate_vector_double(flat_data, N, dim);
                clock_gettime(CLOCK_MONOTONIC, &end);
                avg_interval += time_diff(&start, &end);
            }
            avg_interval /= CYCLE_TIME;
            break;
        }
        case 5: { // float
            data = malloc(N * dim * sizeof(float));
            float* flat_data = (float*)data;
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < dim; j++) {
                    if (fscanf(file, "%f", &flat_data[i * dim + j]) != 1) {
                        printf("parse number failed\n");
                        goto cleanup;
                    }
                }
            }
            for (int i = 0; i < CYCLE_TIME; i++) {
                clock_gettime(CLOCK_MONOTONIC, &start);
                sum = calculate_vector_float(flat_data, N, dim);
                clock_gettime(CLOCK_MONOTONIC, &end);
                avg_interval += time_diff(&start, &end);
            }
            avg_interval /= CYCLE_TIME;
            break;
        }
        default:
            printf("unsupported type: %d\n", type);
            goto cleanup;
    }

    printf("sum = %Lf\n", sum);
    printf("Calculations finished in avg %ld nano second\n", avg_interval);
    printf("Calculations finished in avg %.6f millisecond\n", avg_interval / 1e6);

cleanup:
    if (data) free(data);
    fclose(file);
    return 0;
}


int calculate_vector_int(int* data, int N, int dim) {
    int sum = 0;
    if (!data) {
        printf("empty data passing\n");
        return sum;
    }
    #pragma omp parallel for reduction(+:sum) schedule(guided)
    for (int i = 1; i <= N/2 ; i++) {
        int offset1 = (2 * i - 2) * dim;
        int offset2 = (2 * i - 1) * dim;

        for (int j = 0; j < dim; j++) {
            sum += data[offset1 + j] * data[offset2 + j];
        }
    }
    return sum;
}

int calculate_vector_short(short* data, int N, int dim) {
    int sum = 0;
    if (!data) {
        printf("empty data passing\n");
        return sum;
    }
    #pragma omp parallel for reduction(+:sum) schedule(guided)
    for (int i = 1; i <= N/2 ; i++) {
        int offset1 = (2 * i - 2) * dim;
        int offset2 = (2 * i - 1) * dim;        
        for (int j = 0; j < dim; j++) {
            sum += data[offset1 + j] * data[offset2 + j];
        }
    }
    return sum;
}

int calculate_vector_byte(int8_t* data, int N, int dim) {
    int sum = 0;
    if (!data) {
        printf("empty data passing\n");
        return sum;
    }
    #pragma omp parallel for reduction(+:sum) schedule(guided)
    for (int i = 1; i <= N/2 ; i++) {
        int offset1 = (2 * i - 2) * dim;
        int offset2 = (2 * i - 1) * dim;        
        for (int j = 0; j < dim; j++) {
            sum += data[offset1 + j] * data[offset2 + j];
        }
    }
    return sum;
}

double calculate_vector_double(double* data, int N, int dim) {
    double sum = 0.0;
    if (!data) {
        printf("empty data passing\n");
        return sum;
    }

    #pragma omp parallel for reduction(+:sum) schedule(dynamic)
    for (int i = 1; i <= N/2 ; i++) {
        int offset1 = (2 * i - 2) * dim;
        int offset2 = (2 * i - 1) * dim;        
        for (int j = 0; j < dim; j++) {
            sum += data[offset1 + j] * data[offset2 + j];
        }
    }
    return sum;
}

float calculate_vector_float(float* data, int N, int dim) {
    float sum = 0.0f;
    if (!data) {
        printf("empty data passing\n");
        return sum;
    }

    #pragma omp parallel for reduction(+:sum) schedule(dynamic)
    for (int i = 1; i <= N/2 ; i++) {
        int offset1 = (2 * i - 2) * dim;
        int offset2 = (2 * i - 1) * dim;
        for (int j = 0; j < dim; j++) {
            sum += data[offset1 + j] * data[offset2 + j];
        }
    }
    return sum;
}
/**
 * FOR TEST USE
 *  int max_threads = 32;
    num_threads = 1;
    omp_set_num_threads(num_threads);
    float base_time = 0;
    while(num_threads <= max_threads){
        for (int i = 0; i < CYCLE_TIME; i++) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            sum = calculate_vector_float(flat_data, N, dim);
            clock_gettime(CLOCK_MONOTONIC, &end);
            avg_interval += time_diff(&start, &end);
        }
        avg_interval /= CYCLE_TIME;
        if(num_threads==1){
            base_time = (float)avg_interval;
        }
        printf("Thread num = %d, time(ns) = %ld, ratio = %lf\n",num_threads,avg_interval,(float)avg_interval/base_time);
        num_threads++;
        omp_set_num_threads(num_threads);
    }
 */