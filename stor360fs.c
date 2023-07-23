#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "disk.h"


/*
 * Based on http://bit.ly/2vniWNb
 */
void pack_current_datetime(unsigned char *entry) {
    assert(entry);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    unsigned short year   = tm.tm_year + 1900;
    unsigned char  month  = (unsigned char)(tm.tm_mon + 1);
    unsigned char  day    = (unsigned char)(tm.tm_mday);
    unsigned char  hour   = (unsigned char)(tm.tm_hour);
    unsigned char  minute = (unsigned char)(tm.tm_min);
    unsigned char  second = (unsigned char)(tm.tm_sec);

    year = htons(year);

    memcpy(entry, &year, 2);
    entry[2] = month;
    entry[3] = day;
    entry[4] = hour;
    entry[5] = minute;
    entry[6] = second; 
}

//Find next free block in FAT
int next_free_block(int *FAT, int max_blocks) {
    assert(FAT != NULL);

    int i;

    for (i = 0; i < max_blocks; i++) {
        if (FAT[i] == FAT_AVAILABLE) {
            return i;
        }
    }

    return -1;
}


int main(int argc, char *argv[]) {
    superblock_entry_t sb;
    int  i;
    char *imagename  = NULL;
    char *filename   = NULL;
    char *sourcename = NULL;
    FILE *f;

    //Set imagename, filename, and sourcename from arguments
    //filename corresponds to disk image; sourcename corresponds to host path
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--image") == 0 && i+1 < argc) {
            imagename = argv[i+1];
            i++;
        } else if (strcmp(argv[i], "--file") == 0 && i+1 < argc) {
            filename = argv[i+1];
            i++;
        } else if (strcmp(argv[i], "--source") == 0 && i+1 < argc) {
            sourcename = argv[i+1];
            i++;
        }
    }

    //If an argument is missing, exit
    if (imagename == NULL || filename == NULL || sourcename == NULL) {
        fprintf(stderr, "usage: stor360fs --image <imagename> " \
            "--file <filename in image> " \
            "--source <filename on host>\n");
        exit(1);
    }

    //Set file pointer for image (reading and writing)
    f = fopen(imagename, "r+");
    //Check if disk image exists; exit otherwise
    if(f == NULL){
        fprintf(stderr, "File not found!\n");
        exit(1);
    }
    //Set file pointer for source file; if no file is found, exit
    FILE *f_source = fopen(sourcename, "r");
    if(f_source == NULL){
        fprintf(stderr, "Source Not Found!\n");
        exit(1);
    }
    
    //Read continuously into packed struct
    fread(&sb, sizeof(sb), 1, f);
    
    //Convert data from big-endian to little-endian
    sb.block_size = ntohs(sb.block_size);
    sb.num_blocks = ntohl(sb.num_blocks);
    sb.fat_start = ntohl(sb.fat_start);
    sb.fat_blocks = ntohl(sb.fat_blocks);
    sb.dir_start = ntohl(sb.dir_start);
    sb.dir_blocks = ntohl(sb.dir_blocks);
    
    //Array for easy access to FAT data
    int fat_data[sb.num_blocks];
    //Set file pointer to block 1
    fseek(f, sb.block_size, SEEK_SET);
    //Read FAT continuously into our array (each index = 1 entry)
    fread(fat_data, SIZE_FAT_ENTRY, sb.num_blocks, f);
    //Convert to little-endian
    for(int j = 0; j < sb.num_blocks; j++){
        fat_data[j] = ntohl(fat_data[j]);
    }
    
    //Array of directory entries
    directory_entry_t det[MAX_DIR_ENTRIES];
    //Set file pointer to start of DET
    fseek(f, sb.block_size*sb.dir_start, SEEK_SET);
    
    //Read each directory entry into array (64 bytes)
    fread(&det, SIZE_DIR_ENTRY, MAX_DIR_ENTRIES, f);
    
    //Iterate through root directory
    for(int j = 0; j < MAX_DIR_ENTRIES; j++){
        //If the file we want to add already exist, exit
        if(det[j].status == DIR_ENTRY_NORMALFILE && strcmp(det[j].filename, filename) == 0){
            fprintf(stderr, "File Is ALready On Disk!\n");
            exit(1);
        }
        else if(det[j].status == DIR_ENTRY_NORMALFILE){
            det[j].start_block = ntohl(det[j].start_block);
            det[j].num_blocks = ntohl(det[j].num_blocks);
            det[j].file_size = ntohl(det[j].file_size);
        }
    }
    
    //Assign the first available directory entry; If no entry available, exit
    int entry_id = 0;
    for(int j = 0; j < MAX_DIR_ENTRIES; j++){
        if(det[j].status == DIR_ENTRY_AVAILABLE){
            entry_id = j;
            break;
        }
        else if(j == sb.dir_blocks){
            fprintf(stderr, "Disk Full!\n");
            exit(1);
        }
    }
    
    directory_entry_t dir_entry;
    //Set dir_entry (num_blocks/file_size incremented during copying)
    dir_entry.status = DIR_ENTRY_NORMALFILE;
    dir_entry.start_block = next_free_block(fat_data, sb.num_blocks);
    dir_entry.num_blocks = 0;
    dir_entry.file_size = 0;
    pack_current_datetime(dir_entry.create_time);
    pack_current_datetime(dir_entry.modify_time);
    strcpy(dir_entry.filename, filename);
    for(int j = 0; j < sizeof(dir_entry._padding); j++){
        dir_entry._padding[j] = 0xff;
    }
    
    //Initialize variables for loop
    int bytes_read;
    char buf[sb.block_size];
    int cur_block = dir_entry.start_block;
    int next_block = cur_block;
    
    //Seek to start block
    fseek(f, sb.block_size*dir_entry.start_block, SEEK_SET);
    
    while(1){
        //Read up to 1 block from f_source
        bytes_read = fread(&buf, 1, sb.block_size, f_source);
        //printf("Bytes Read: %d\n", bytes_read);
        //Write as many bytes as were read
        fwrite(buf, 1, bytes_read, f);
        //Increment metadata accordingly
        dir_entry.file_size += bytes_read;
        dir_entry.num_blocks += 1;
        
        //If less than 1 block was read, end of file; set last block, break
        if(bytes_read < sb.block_size){
            fat_data[cur_block] = FAT_LASTBLOCK;
            break;
        }
        
        //Assign the next free block
        fat_data[cur_block] = cur_block;
        next_block = next_free_block(fat_data, sb.num_blocks);
        fat_data[cur_block] = next_block;
        cur_block = fat_data[cur_block];
        //Seek to next block
        fseek(f, sb.block_size*cur_block, SEEK_SET);
    }

    //printf("File Size: %d\n", dir_entry.file_size);
    //printf("Blocks allocated: %d\n", dir_entry.num_blocks);
    
    
    //Add new entry to DET
    det[entry_id] = dir_entry;
    
    //Convert data to big-endian
    for(int j = 0; j < MAX_DIR_ENTRIES; j++){
        det[j].start_block = htonl(det[j].start_block);
        det[j].num_blocks = htonl(det[j].num_blocks);
        det[j].file_size = htonl(det[j].file_size);
    }
    
    //Set file pointer to start of DET
    fseek(f, sb.block_size*sb.dir_start, SEEK_SET);
    
    //Rewrite DET with the new entry
    fwrite(&det, SIZE_DIR_ENTRY, MAX_DIR_ENTRIES, f);
    
    
    //Convert to big-endian
    for(int j = 0; j < sb.num_blocks; j++){
        fat_data[j] = htonl(fat_data[j]);
    }
    //Set file pointer to block 1
    fseek(f, sb.block_size, SEEK_SET);
    //Write array into FAT contiguously
    fwrite(fat_data, SIZE_FAT_ENTRY, sb.num_blocks, f);
    
    
    
    fclose(f_source);
    fclose(f);
    return 0; 
}
