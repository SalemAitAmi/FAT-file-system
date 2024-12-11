#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "disk.h"

int main(int argc, char *argv[]) {
    superblock_entry_t sb;
    int  i;
    char *imagename = NULL;
    FILE  *f;

    //Find and set imagename from arguments 
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--image") == 0 && i+1 < argc) {
            imagename = argv[i+1];
            i++;
        }
    }

    //If no image is given, exit
    if (imagename == NULL){
        fprintf(stderr, "usage: stat360fs --image <imagename>\n");
        exit(1);
    }
    
    //Set file pointer
    f = fopen(imagename, "r");
    //Check if file exists; exit otherwise
    if(f == NULL){
        fprintf(stderr, "File not found!\n");
        exit(1);
    }
    //Read continuously into packed struct
    fread(&sb, sizeof(sb), 1, f);
    
    //Convert superblock data to little-endian
    sb.block_size = ntohs(sb.block_size);
    sb.num_blocks = ntohl(sb.num_blocks);
    sb.fat_start = ntohl(sb.fat_start);
    sb.fat_blocks = ntohl(sb.fat_blocks);
    sb.dir_start = ntohl(sb.dir_start);
    sb.dir_blocks = ntohl(sb.dir_blocks);
    
    //Initialize counters and an array for the FAT
    int free = 0;
    int resv = 0;
    int alloc = 0;
    int fat_data[sb.num_blocks];
    
    //Set file pointer to block 1
    fseek(f, sb.block_size, SEEK_SET);
    //Read FAT continuously into our array (int index = 1 entry = 4 bytes)
    fread(fat_data, SIZE_FAT_ENTRY, sb.num_blocks, f);
    
    //Read through FAT array and increment counters
    for(int j = 0; j < sb.num_blocks; j++){
        //Convert to little-endian before we inspect data
        fat_data[j] = ntohl(fat_data[j]);
        if(fat_data[j] == FAT_AVAILABLE){
            free++;
        }
        else if(fat_data[j] == FAT_RESERVED){
            resv++;
        }
        else{
            alloc++;
        }
        
    }
    
    
    //Print statements 
    printf("%s (%s)\n", sb.magic, imagename+7);
    printf("-----------------------------------------\n");
    printf("  Bsz  Bcnt  FATst  FATcnt  DIRst  DIRcnt\n");
    printf("  %2d  %2d  %5d  %6d  %5d  %6d\n", 
           sb.block_size, sb.num_blocks, sb.fat_start, 
           sb.fat_blocks, sb.dir_start, sb.dir_blocks);
    printf("-----------------------------------------\n");
    printf("  Free  Resv  Alloc\n");
    printf("  %2d  %4d  %5d\n", free, resv, alloc);
    
    fclose(f);
    return 0; 
}
