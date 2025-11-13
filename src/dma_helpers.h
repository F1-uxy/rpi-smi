#ifndef DMA_HELPERS_H
#define DMA_HELPERS_H

/*
 * This file supplies some helpers to test DMA buffers
*/

int check_buf(unsigned char* buf, unsigned int size)
{
    int m = 256;
    int n = 10;
    int i, k;
    int error_count = 0;
    while(--n > 0) 
    {
        for(i = 0; i < size; i = i + m) 
        {
            m = (i+256 < size) ? 256 : (size-i);
            for(k = 0; k < m; k++) 
            {
                buf[i+k] = (k & 0xFF);
            }
            for(k = 0; k < m; k++) 
            {
                if (buf[i+k] != (k & 0xFF)) 
                {
                    error_count++;
                }
            }
        }
    }
    return error_count;
}

int clear_buf(unsigned char* buf, unsigned int size)
{
    int n = 100;
    int error_count = 0;
    while(--n > 0) {
        memset((void*)buf, 0, size);
    }
    return error_count;
}

#endif