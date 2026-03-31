#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>

#include "smi.h"
#include "errors.h"


void sram_helloworld(SMI_CXT* cxt)
{
    cxt->rw_config->rconfig->rwidth = SMI_8_BITS;
    cxt->rw_config->wconfig->wformat = SMI_RGB565;
    cxt->rw_config->wconfig->wswap = 0;
    cxt->pxldata = 1;
    cxt->pad = 0;
    cxt->intd = 0;
    cxt->intr = 0;
    cxt->intt = 0;

    //uint32_t data32[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', '!', '\0'};
    uint32_t data32[] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, '\0'};
    //uint32_t clearData[] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
    
    printf("Writing\n");
    smi_direct_write_arr(cxt, data32, 0, 12, SMI_ADDR_INC);
    //smi_programmed_write_arr(cxt, data32, 0, 12);
    sleep(0);
    printf("Reading\n");
    int len = 12;
    uint32_t ret[len];

    int len_read = smi_direct_read_arr(cxt, ret, 0, len, SMI_ADDR_INC);
    
    //int len_read = smi_programmed_read_arr(cxt, ret, 1, len);

    if(len_read < 0)
    {
        ERROR("Did not read");
        return;
    }

    for(int i = 0; i < len; i++)
    {
        printf("i=%d ; Address: %x ; Value: %u ; ASCII: %c\n", i, (i % 64), ret[i], ret[i]);
    }
        
}

int testbench_write(SMI_CXT* cxt, size_t len)
{
    int count = 0;
    int val = 0;
    for(size_t i = 0; i < len; i++)
    {
        count += smi_direct_write(cxt, 0xA, 0);
        //smi_direct_read(cxt, &val, 0);
        //if(val == 0xA) count++;
        //val = 0;
    }

    return count;
}



long testbench_read(SMI_CXT* cxt, size_t len, int block_len)
{
    cxt->rw_config->rconfig->rwidth = SMI_8_BITS;
    cxt->rw_config->wconfig->wformat = SMI_RGB565;
    /*
    SMI_XRGB
    SMI_RGB565
    */
    cxt->rw_config->wconfig->wswap = 0;
    cxt->pxldata = 1;
    cxt->pixel_value_mode = 0;
    cxt->pad = 0;
    cxt->intd = 0;
    cxt->intr = 0;
    cxt->intt = 0;

    int count = 0;
    int val = 0;
    uint32_t ret[block_len];
    
    uint32_t ret_single;
    size_t itr = len / block_len;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(size_t i = 0; i < itr; i++)
    {
        count += smi_programmed_read_arr(cxt, ret, 1, block_len);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long total = (end.tv_sec - start.tv_sec) * 1000000000L +
           (end.tv_nsec - start.tv_nsec);
    /*
    double seconds = total / 1e9;
    double mbps = (len / seconds) / (1024.0 * 1024.0);
    double mts  = (len / seconds) / 1e6;

    printf("Total: %ld ns ; %f s\n", total, seconds);
    printf("Throughput: %f MB/s ; %f MT/s\n", mbps, mts);
    printf("Per read: %f ns\n", (double)total / len);
    */
   
    return total;   
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


#define TEST_ITERATIONS 5
#define ITERATIONS 1048576
#define DEVICE "RPI4"
#define METHOD "SMI"
#define WIDTH 8
#define PACK "RGB565"
#define OPTIMISATION "O0"
#define OS 64

void megbyte_load_block_test(SMI_CXT* cxt)
{
    FILE* csv = fopen("read.csv", "ab+");
    if (!csv) {
        perror("Failed to open CSV file");
        return;
    }

    long times[TEST_ITERATIONS];

    for(int block = 8; block <= 1048576; block *= 2)
    {
        long total = 0;
        long time = 0;

        printf(" --- WARMUP ---\n");
        testbench_read(cxt, ITERATIONS, block);
        printf(" --- START ---\n");
        
        for(int i = 0; i < TEST_ITERATIONS; i++)
        {
            time = testbench_read(cxt, ITERATIONS, block);
            times[i] = time;
            total += time;
        }

        double seconds = total / 1e9;
        double average = seconds / TEST_ITERATIONS;
        double transfers_per_sec = ITERATIONS / average;
        double mbps = (transfers_per_sec * (WIDTH / 8.0)) / (1024.0 * 1024.0);
        double mts  = (ITERATIONS / average) / 1e6;

        double sum_sq = 0;
        for(int r = 0; r < TEST_ITERATIONS; r++)
        {
            double run_sec = times[r] / 1e9;
            sum_sq += (run_sec - average) * (run_sec - average);
        }
        double stddev = sqrt(sum_sq / TEST_ITERATIONS);

        printf("\n --- %d FINAL ---\n", block);
        printf("Total: %ld ns ; %f s\n", total, seconds);
        printf("Average: %f\n", average);
        printf("Throughput: %f MB/s ; %f MT/s\n", mbps, mts);
        printf("Std deviation: %f s\n", stddev);

        fprintf(csv, "%s,%d,%s,%s,%d,%s,%d,%.9f,%.6f,%.6f,%.6f\n",
                DEVICE, OS, OPTIMISATION, METHOD, WIDTH, PACK, block, average, stddev, mbps, mts);

    }

    fclose(csv);
}

void megbyte_load_thread_test(SMI_CXT* cxt)
{
    int block = 2048;
    long total = 0;
    long time = 0;
    long times[TEST_ITERATIONS];

    for(int threads = 1; threads <= 256; threads *= 2)
    {

        pthread_t load_threads[threads];

        for(int i = 0; i < threads; i++)
        {
            pthread_create(&load_threads[i], NULL, cpu_load, NULL);
        }

        printf(" --- WARMUP ---\n");
        testbench_read(cxt, ITERATIONS, block);
        printf(" --- START ---\n");
        
        for(int i = 0; i < TEST_ITERATIONS; i++)
        {
            time = testbench_read(cxt, ITERATIONS, block);
            times[i] = time;
            total += time;
        }

        double seconds = total / 1e9;
        double average = seconds / TEST_ITERATIONS;
        double transfers_per_sec = ITERATIONS / average;
        double mbps = (transfers_per_sec * (8.0 / 8.0)) / (1024.0 * 1024.0);
        double mts  = (ITERATIONS / average) / 1e6;

        printf("\n --- %d FINAL ---\n", threads);
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

        total = 0;
    }
    
}