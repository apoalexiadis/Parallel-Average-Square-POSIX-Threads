#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//����� ��� ������ ��� �������� ������� (MAX_THREADS), ���� � ������� �� ��� ������ �� ����� ��������� ������ ������, ����� ���� ������ �� ��������� ������� ���������� ��� ���������� ���� ������� ��� ��� ���������� ��� ������������:
// 1. ������ ��� ������ �����: ���� ���� ����������� ������ ���������� ���� ����� ��� ����� CPU. �� ������� ���� � ������� ��� �������, �� ������� ������ �� ���������� ���� ����������� ������ ���, ��������� �� ������������� � ��� �� �������� ��������� ��� ������������.
// 2. ������ ��� overhead ����������� �������: � ���������� ��� � ���������� ��� ������� ���� ������ overhead. ���� � ������� ��� ������� ��������� ���������, �� ����������� ������� ������� ����������� ����� ��� ���������� ����� ��� �������, ���� ���� ���������� ������������ �������.
// 3. ����������� �������������� ��� ���������� (thrashing): �� ������������� ���� ����� ������, �� ������� ������ �� ������ �� ��������� thrashing, ���� ��������� ������������ ������ ���� �������� ������ ��� ������� ���� ���� �������� ��� ������������� �������� ����.
// 4. ������������ ��� ��� ���������� (mutex contention): �� ����� ������ ���������� �� ���������� �������� ���� ���� ���������� ����������, �� �������� ������ ������������� ����� �� ������ ���������� �� ����� ���� �� ���������� ��������. ���� ������� ��� ������������� ��� ���������� ������������.
// 5. ������ ������ ��� ��������� ��������: �� ��������� �����������, � ����� ���������� ������ ������� ������ �� �������� �� ������ ��� ��������� �������� ���� �� ������. ���� ��������� ������ �� ������ ��� ���������� ������������ �������������� ��� �� �������� ����������.
#define MAX_THREADS 8


// ���� ��������� ��� �� �������� ��� ���� �������
typedef struct {
    int *array;       // ������� ���� ������ ��� ���������
    int start;        // ������ ���� ��� �� ����
    int end;          // ������ ���� ��� �� ����
    long long local_sum; // ������ �������� ��� �� ����
} ThreadData;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // ������������ ����������
long long total_sum = 0; // �������� ��������

// ������������ �����������
void* thread_sum(void* );

int main() {
    int n, t, i; // ������������ ����������

    // �������� ��� ������� ��� ��������� ��� ������
    printf("Enter the number of elements (n): ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        fprintf(stderr, "Invalid input for number of elements\n");
        return 1;
    }

    // �������� ��� ������� ��� �������
    printf("Enter the number of threads (t): ");
    if (scanf("%d", &t) != 1 || t <= 0 || t > MAX_THREADS) {
        fprintf(stderr, "Invalid input for number of threads\n");
        return 1;
    }

    // ������� �� �� n ����� ������� ����������� ��� t
    if (n % t != 0) {
        fprintf(stderr, "n must be an integer multiple of t\n");
        return 1;
    }

    // �������� ������ ��� ��� ������
    int *array = (int*) malloc(n * sizeof(int));
    if (array == NULL) {
        perror("Failed to allocate memory");
        return 1;
    }

    // ������� ��� ������ �� �����, ������� �� ��� ��������). �� ���� ��� �� �� i ���� �� ��� ����� ������� ������� �� ����� ��� �� ����� �� �������� ��� ���� ���������� �� ����������� ������ �������
    for (i = 0; i < n; ++i) {
        array[i] = i;
    }

    pthread_t threads[MAX_THREADS]; // ������� ��� �� ������
    ThreadData thread_data[MAX_THREADS]; // ������� ��� �� �������� ��� �������
    int chunk_size = n / t; // ������� ��� �������� ��� �� ������������� ���� ����

    clock_t start_time = clock(); // ������ �������������

    // ���������� ��� �������� ��� �������
    for (i = 0; i < t; ++i) {
        thread_data[i].array = array;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i + 1) * chunk_size;
        thread_data[i].local_sum = 0;

        if (pthread_create(&threads[i], NULL, thread_sum, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            free(array);
            pthread_mutex_destroy(&mutex);
            return 1;
        }
    }

    // ������� ��� ��� ���������� ���� ��� �������
    for (i = 0; i < t; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            free(array);
            pthread_mutex_destroy(&mutex);
            return 1;
        }
    }

    clock_t end_time = clock(); // ���� �������������

    // ����������� ��� �������� ��� ����� ���� ��� ����������
    double average = (double)total_sum / n;
    printf("Average of squares: %.2f\n", average);
    printf("Execution time: %.2f seconds\n", (double)(end_time - start_time));

    free(array); // ����������� ��� ������
    pthread_mutex_destroy(&mutex); // ���������� ��� ����������

    return 0;
}

// ��������� ��� �� ������� ���� ����
void* thread_sum(void* arg) {
    ThreadData *data = (ThreadData*) arg;
    data->local_sum = 0;
    int i;

    // ����������� ��� ������� ����������� ��� ���������� ��� ��������� ��� ������
    for (i = data->start; i < data->end; ++i) {
        data->local_sum += (long long)data->array[i] * data->array[i];
    }

    // �������� ��� ���������� ��� �� ���������� �� �������� ��������
    pthread_mutex_lock(&mutex);
    total_sum += data->local_sum;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

