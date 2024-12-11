#include <assert.h>
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
    char *filename  = NULL;
    FILE *f;

    //Find and set imagename and filename from arguments
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--image") == 0 && i+1 < argc) {
            imagename = argv[i+1];
            i++;
        } else if (strcmp(argv[i], "--file") == 0 && i+1 < argc) {
            filename = argv[i+1];
            i++;
        }
    }

    //If no image or file is given, exit
    if (imagename == NULL || filename == NULL) {
        fprintf(stderr, "usage: cat360fs --image <imagename> " \
            "--file <filename in image>");
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
    
    //Array for FAT data
    int fat_data[sb.num_blocks];
    //Set file pointer to block 1
    fseek(f, sb.block_size, SEEK_SET);
    //Read FAT continuously into our array (int index = entry = 4 bytes)
    fread(fat_data, SIZE_FAT_ENTRY, sb.num_blocks, f);
    //Convert to little-endian
    for(int j = 0; j < sb.num_blocks; j++){
        fat_data[j] = ntohl(fat_data[j]);
    }
    
    //Array for directory entries
    directory_entry_t det[MAX_DIR_ENTRIES];
    //Set file pointer to start of DET
    fseek(f, sb.block_size*sb.dir_start, SEEK_SET);
    
    //Read each directory entry into array 
    fread(&det, SIZE_DIR_ENTRY, MAX_DIR_ENTRIES, f);
    
    //Iterate through root directory
    for(int j = 0; j < MAX_DIR_ENTRIES; j++){
        
        //If a directory entry is the file we're looking for; proceed
        if(det[j].status == DIR_ENTRY_NORMALFILE && strcmp(det[j].filename, filename) == 0){
            
            //Convert directory entry data to litte-endian 
            det[j].start_block = ntohl(det[j].start_block);
            det[j].num_blocks = ntohl(det[j].num_blocks);
            det[j].file_size = ntohl(det[j].file_size);
            //Initialize necessary variables
            int cur_block = det[j].start_block;
            char buf[sb.block_size];
            int bytes_read;
            int bytes_left = det[j].file_size;
            
            //Seek to start block
            fseek(f, sb.block_size*det[j].start_block, SEEK_SET);
            
            //Iterate through blocks assigned to this file
            for(int k = 0; k < det[j].num_blocks; k++){
                
                if(bytes_left > sb.block_size){
                    //When we need to read the whole block
                    bytes_read = fread(&buf, 1, sb.block_size, f);
                    fwrite(buf, 1, bytes_read, stdout);
                    bytes_left -= bytes_read;
                }
                else{
                    //When we read the last block
                    bytes_read = fread(&buf, 1, bytes_left, f);
                    fwrite(buf, 1, bytes_left, stdout);
                }
                //Navigate to next block
                fseek(f, sb.block_size*fat_data[cur_block], SEEK_SET);
                cur_block = fat_data[cur_block];
            }
            
            break;
        }
        else if(j == sb.dir_blocks){
            printf("File not found!\n");
        }
        
    }
    

    fclose(f);
    return 0; 
}
