#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "disk.h"

//Helper function for prints
char *month_to_string(short m) {
    switch(m) {
    case 1: return "Jan";
    case 2: return "Feb";
    case 3: return "Mar";
    case 4: return "Apr";
    case 5: return "May";
    case 6: return "Jun";
    case 7: return "Jul";
    case 8: return "Aug";
    case 9: return "Sep";
    case 10: return "Oct";
    case 11: return "Nov";
    case 12: return "Dec";
    default: return "?!?";
    }
}

//Helper function for prints
void unpack_datetime(unsigned char *time, short *year, short *month, 
    short *day, short *hour, short *minute, short *second)
{
    assert(time != NULL);

    memcpy(year, time, 2);
    *year = htons(*year);

    *month = (unsigned short)(time[2]);
    *day = (unsigned short)(time[3]);
    *hour = (unsigned short)(time[4]);
    *minute = (unsigned short)(time[5]);
    *second = (unsigned short)(time[6]);
}


int main(int argc, char *argv[]) {
    superblock_entry_t sb;
    int  i;
    char *imagename = NULL;
    FILE *f;

    //Find and set imagename from arguments
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--image") == 0 && i+1 < argc) {
            imagename = argv[i+1];
            i++;
        }
    }

    //If no image is given, exit
    if (imagename == NULL){
        fprintf(stderr, "usage: ls360fs --image <imagename>\n");
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
    
    //Array for DET data
    directory_entry_t det[MAX_DIR_ENTRIES];
    
    //Set file pointer to start of DET
    fseek(f, sb.block_size*sb.dir_start, SEEK_SET);
    
    //Read each directory entry into array (64 bytes)
    fread(&det, SIZE_DIR_ENTRY, MAX_DIR_ENTRIES, f);
    
    for(int j = 0; j < MAX_DIR_ENTRIES; j++){
        //Only print information for files
        if(det[j].status == DIR_ENTRY_NORMALFILE){
            short year, month, day, hour, minute, second;
            //Convert data to little-endian
            det[j].start_block = ntohl(det[j].start_block);
            det[j].num_blocks = ntohl(det[j].num_blocks);
            det[j].file_size = ntohl(det[j].file_size);
            //Date/time is stored on disk with the form YYYYMMDDHHMMSS
            //Unpack for printing purposes
            unpack_datetime(det[j].modify_time, &year, &month, 
                            &day, &hour, &minute, &second);
            printf("%8d %d-%s-%d %d:%02d:%02d %s\n", det[j].file_size, year, month_to_string(month), day, hour, minute, second, det[j].filename);
        }
        
    }
    
    fclose(f);
    return 0; 
}
