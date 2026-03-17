#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

#include "../deps/gpio.h"

int smi_data_pins[] = {8, 9, 10, 11, 12, 13, 14, 15};
int smi_addr_pins[] = {5, 4, 3, 2, 1, 0};
int smi_oe = 6;
int smi_we = 7;

void gpio_data_mode(MEM_MAP regs, bool read)
{
    int mode = read ? GPIO_IN : GPIO_OUT;

    for(int i = 0; i < 8; i++)
    {
        gpio_mode(regs, smi_data_pins[i], mode);
    }
}

void gpio_addr_mode(MEM_MAP regs)
{
    for(int i = 0; i < 6; i++)
    {
        gpio_mode(regs, smi_addr_pins[i], GPIO_OUT);
    }
}

void set_address(MEM_MAP regs, uint8_t addr)
{
    int val = 0;
    for(int i = 0; i < 6; i++)
    {
        val = (addr >> i) & 0x01;
        gpio_set(regs, smi_addr_pins[i], val);
    }
}

uint8_t read_byte(MEM_MAP regs)
{
    uint8_t ret = 0;

    for(int i = 0; i < 8; i++)
    {
        ret |= gpio_read(regs, smi_data_pins[i]) << i;
    }

    return ret;
}

void gpio_hello_world(MEM_MAP regs)
{
    uint8_t val = 0;
 
    gpio_mode(regs, smi_oe, GPIO_OUT);    
    gpio_data_mode(regs, true);
    gpio_addr_mode(regs);

    gpio_set(regs, smi_oe, 0);

    for(int i = 0; i < 12; i++)
    {
        set_address(regs, i);
        val = read_byte(regs);
        printf("Value: %u ; %c\n", val, val);
    }
    
    gpio_set(regs, smi_oe, 1);
}

void* cpu_load(void* args)
{
    volatile double x = 0;

    while(1)
    {
        for(int i = 0; i < 1e9; i++)
        {
            x += sqrt(i);
        }
    }

    return NULL;
}

long diff_ns(struct timespec start, struct timespec end)
{
    return (end.tv_sec - start.tv_sec) * 1000000000L +
           (end.tv_nsec - start.tv_nsec);
}

static inline void delay_ns(long ns)
{
    struct timespec start, now;

    clock_gettime(CLOCK_MONOTONIC, &start);

    do {
        clock_gettime(CLOCK_MONOTONIC, &now);
    } while ((now.tv_sec - start.tv_sec)*1000000000L +
             (now.tv_nsec - start.tv_nsec) < ns);
}

#define ITERATIONS 1e6
long megabyte_test(MEM_MAP regs)
{
    uint8_t val = 0;
    set_address(regs, 0);
    gpio_set(regs, smi_oe, 0);
    gpio_set(regs, smi_we, 1);
    sleep(1);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(int i = 0; i < ITERATIONS; i++)
    {
        val = read_byte(regs);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    gpio_set(regs, smi_oe, 1);

    long total = diff_ns(start, end);
    double seconds = total / 1e9;
    double mbps = (ITERATIONS / seconds) / (1024.0 * 1024.0);
    double mts  = (ITERATIONS / seconds) / 1e6;

    printf("Total: %ld ns ; %f s\n", total, seconds);
    printf("Throughput: %f MB/s ; %f MT/s\n", mbps, mts);
    printf("Per read: %f ns\n", (double)total / ITERATIONS);
    return total;
}

#define THREADS 128
#define TEST_ITERATIONS 5

void megabyte_load_test(MEM_MAP regs)
{
    pthread_t load_threads[THREADS];

    for(int i = 0; i < THREADS; i++)
    {
        pthread_create(&load_threads[i], NULL, cpu_load, NULL);
    }

    long total = 0;
    long time = 0;
    long times[TEST_ITERATIONS];

    printf(" --- WARMUP ---\n");
    megabyte_test(regs);
    printf(" --- START ---\n");

    for(int i = 0; i < TEST_ITERATIONS; i++)
    {
        time = megabyte_test(regs);
        times[i] = time;
        total += time;
    }

    double seconds = total / 1e9;
    double average = seconds / TEST_ITERATIONS;
    double mbps = (ITERATIONS / average) / (1024.0 * 1024.0);
    double mts  = (ITERATIONS / average) / 1e6;

    printf("\n --- FINAL ---\n");
    printf("Total: %ld ns ; %f s\n", total, seconds);
    printf("Average: %f\n", average);
    printf("Throughput: %f MB/s ; %f MT/s\n", mbps, mts);
    
    
    double sum_sq = 0;
    for(int r = 0; r < TEST_ITERATIONS; r++)
    {
        double run_sec = times[r] / 1e9;
        sum_sq += (run_sec - average) * (run_sec - average);
    }
    double stddev = sqrt(sum_sq / TEST_ITERATIONS);
    printf("Std deviation: %f s\n", stddev);
}


int main()
{
    MEM_MAP regs;
    uint8_t val = 0;

    map_segment(&regs, GPIO_BASE, PAGE_SIZE);
    gpio_mode(regs, smi_oe, GPIO_OUT);    
    gpio_data_mode(regs, true);
    gpio_addr_mode(regs);

    megabyte_load_test(regs);

    return 0;
}