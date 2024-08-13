#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdint.h>

#define N_UINTS         512
#define N_WRITES        262144

#define RAM1_ADDR       0x00000000
#define BRAM_ADDR       0xC0000000

void write_to_DMA(int fd, off_t base_addr, const void *data, size_t size) {
    if (pwrite(fd, data, size, base_addr) == -1) {
        perror("pwrite");
        exit(EXIT_FAILURE);
    }
}

void read_from_DMA(int fd, off_t base_addr, void *buffer, size_t size) {
    if (pread(fd, buffer, size, base_addr) == -1) {
        perror("pread");
        exit(EXIT_FAILURE);
    }
}


void write_axi_lite_32b(void *addr, uint32_t offset, uint32_t *value, uint32_t vector_elemnts){

    int i;
    void *virt_addr;

    for (i = 0; i < vector_elemnts; i++){
        virt_addr = addr + offset + sizeof(uint32_t)*i;
        *((uint32_t *) virt_addr) = value[i];
    }
}

void read_axi_lite_32b(void *addr, uint32_t offset, uint32_t *value, uint32_t vector_elemnts){

    int i;
    void *virt_addr;

    for (i = 0; i < vector_elemnts; i++){
        virt_addr = addr + offset + sizeof(uint32_t)*i;
        value[i] = *((uint32_t *) virt_addr);
        usleep(500);
        printf("Index %i\n", i);
    }

}

int main() {

    int fd;
    uint32_t i, j;

    uint64_t *read_data = malloc(N_UINTS * sizeof(uint64_t));
    uint64_t *write_data = malloc(N_UINTS * sizeof(uint64_t));
    uint32_t n_errors =0;

    
    int fd_h2c = open("/dev/xdma0_h2c_0", O_RDWR);
    if (fd_h2c == -1) {
        perror("open /dev/xdma0_h2c_0");
        return EXIT_FAILURE;
    }

    int fd_c2h = open("/dev/xdma0_c2h_0", O_RDWR);
    if (fd_c2h == -1) {
        perror("open /dev/xdma0_c2h_0");
        close(fd_h2c);
        return EXIT_FAILURE;
    }

    for (i = 0; i < N_UINTS; i++)
        write_data[i] = i; 

    for (i = 0; i < N_WRITES; i++){
        write_to_DMA(fd_h2c, RAM1_ADDR + i * N_UINTS * sizeof(uint64_t), write_data, N_UINTS * sizeof(uint64_t));
    }
    
    for (j = 0; j < N_WRITES; j++){
        read_from_DMA(fd_c2h, RAM1_ADDR + i * N_UINTS * sizeof(uint64_t), read_data, N_UINTS * sizeof(uint64_t));
        
        for (i = 0; i < N_UINTS; i++){
            if (write_data[i] != read_data[i]){
                n_errors++;
            }
        }  
    }    

    printf("END TEST num errors %u of %u -> %f%%\n", n_errors, N_WRITES*N_UINTS, 1.0*n_errors/(N_WRITES*N_UINTS));

    return 0;
}