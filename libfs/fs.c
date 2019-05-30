#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF

struct superBlock {
    uint64_t Signature;
    uint16_t numTotalBlock;
    uint16_t rootElementIndex;
    uint16_t dataIndex;
    uint16_t numDataBlock;
    uint8_t numFatBlock;
    uint8_t padding[4079];
};

/* ROOT DIRECTORY */
struct rootElement {
    char filename[16]; /* 16 bytes */
    uint32_t fileSize;
    uint16_t firstIndex;
    uint8_t padding[10];
};

struct fileDescriptor { 
    int offset;
    struct rootElement *root; /* points to one file */
};

struct superBlock superblock;
uint16_t *fat;
struct rootElement rootDir[FS_FILE_MAX_COUNT];
struct fileDescriptor fileD[FS_OPEN_MAX_COUNT];
int count = 0;

int fs_mount(const char *diskname)
{
    if (block_disk_open(diskname) < 0) {
        return -1;
    }
    /* fill struct superblock with diskname's superblock */
    if (block_read(0,((void*)&superblock)) == -1) {
        /* failed to read the block from diskname */
        return -1;
    }
    /* dynamically allocate FAT_array based on num_blocks_FAT */
    fat = malloc(superblock.numFatBlock * BLOCK_SIZE);
    /* fill struct root directory with diskname's root directory */
    if (block_read(2,((void*)&rootDir)) == -1) {
        return -1;
    }
    /* fill out the FAT blocks */
    for (int i = 1; i <= superblock.numFatBlock; i++) {
        block_read(i,fat+(i-1)*BLOCK_SIZE);
    }
    /* if signature failed */
    if (strncmp((char *)&(superblock.Signature), "ECS150FS", 8) != 0) {
        /* no valid file system can be located */
        return -1;
    }
    return 0;
}

int fs_umount(void)
{
    if (block_disk_count() == -1) {
        return -1;
    }
    /* writeback the fat blocks to the blocks */
    for (int i = 1; i <= superblock.numFatBlock; i++) {
        block_write(i, fat + (i-1)*BLOCK_SIZE);
    }
    /* writeback the root directory to the disk */
    block_write(superblock.rootElementIndex, rootDir);
    if (block_disk_close() == -1) {
        /* failed to close the disk */
        return -1;
    }
    free(fat);
    return 0;
}

int fs_info(void)
{
    if (block_disk_count() == -1) {
        return -1;
    }
    /* print lines */
    printf("FS Info:\n");
    printf("total_blk_count=%d\n", superblock.numTotalBlock);
    printf("fat_blk_count=%d\n",superblock.numFatBlock);
    printf("rdir_blk=%d\n", superblock.rootElementIndex);
    printf("data_blk=%d\n", superblock.dataIndex);
    printf("data_blk_count=%d\n", superblock.numDataBlock);
    int fatcount = 0;
    int rootcount = 0;
    /* for free fat counts */
    for (int i=0; i<superblock.numDataBlock; i++) {
        if (fat[i] == 0) {
            fatcount++;
        }
    }
    /* for free root counts */
    for (int i=0; i<FS_FILE_MAX_COUNT; i++) {
        if (rootDir[i].filename[0] == '\0') {
            rootcount++;
        }
    }
    printf("fat_free_ratio=%d/%d\n", fatcount, superblock.numDataBlock);
    printf("rdir_free_ratio=%d/%d\n", rootcount, FS_FILE_MAX_COUNT);
    return 0;
}

int fs_create(const char *filename)
{
    if (block_disk_count() == -1) {
        return -1;
    }
    if (filename == NULL) {
        return -1;
    }
    int emptyIndex = -1;
    for (int i=0; i<=FS_FILE_MAX_COUNT; i++) {
        if (strcmp(rootDir[i].filename, filename) == 0) {
            /* already present in the root dir array. */
            return -1;
        }
        /* if empty entry is not found */
        if (emptyIndex == -1) {
            /* if the first char of filename is '\0', the file is empty */
            if (rootDir[i].filename[0] == '\0') {
                /* empty entry found */
                emptyIndex = i;
            }
        }
    }
    /* if empty, entry is not found */
    if (emptyIndex == -1) {
        return -1;
    }
    /* copy the filename to the root element struct */
    strcpy(rootDir[emptyIndex].filename,filename);
    /* set newly created file size to zero */
    rootDir[emptyIndex].fileSize = 0;
    /* set newly created file first fat block value to be FAT_EOC */
    rootDir[emptyIndex].firstIndex = FAT_EOC;
    return 0;
}

int fs_delete(const char *filename)
{
    if (block_disk_count() == -1) {
        return -1;
    }
    if (filename == NULL) {
        return -1;
    }
    int fileIndex = -1;
    int temp = 0;
    int currentIndex = 0;
    for (int i=0; i<=FS_FILE_MAX_COUNT; i++) {
        if (strcmp(rootDir[i].filename,filename) == 0) {
            fileIndex = i;
            break;
        }
    }
    if (fileIndex == -1) {
        /* filename not found */
        return -1;
    }
    /* delete the file by inserting the first char of filename with '\0' */
    rootDir[fileIndex].filename[0] = '\0'; /* set to string terminator */
    /* set size of the deleted file to zero */
    rootDir[fileIndex].fileSize = 0; 
    currentIndex = rootDir[fileIndex].firstIndex;
    /* reset fat values */
    while (fat[currentIndex] != FAT_EOC) {
        temp = fat[currentIndex];
        /* set fat block contents to zero (free) */
        fat[currentIndex] = 0;
        currentIndex = temp;
    }
    /* set the last fat bock content to zero */
    fat[currentIndex] = 0;
    rootDir[fileIndex].firstIndex = FAT_EOC;
    return 0;
}

int fs_ls(void)
{
    if (block_disk_count() < 0) {
        return -1;
    }
    printf("FS Ls:\n");
    for (int i=0; i<=FS_FILE_MAX_COUNT; i++) {
        if(rootDir[i].filename[0] != '\0') {
            /* print all not empty filenames, sizes, and data blocks */
            printf("File %s, Size: %d, Data Block: %d\n", rootDir[i].filename, 
            rootDir[i].fileSize, rootDir[i].firstIndex);
        }
    }
    return 0;
}

int fs_open(const char *filename)
{
    if (count == FS_OPEN_MAX_COUNT || filename == NULL || 
    block_disk_count() < 0) {
        return -1;
    }
    int fileIndex = -1;
    int emptyIndex = -1;
    for (int i=0; i<=FS_FILE_MAX_COUNT; i++) {
        if (strcmp(rootDir[i].filename,filename) == 0) {
            /* found the filename: set the index to fileIndex */
            fileIndex = i;
            break;
        }
    }
    if (fileIndex == -1) {
        /* filename not found */
        return -1; 
    }
    for (int j=0; j<=FS_OPEN_MAX_COUNT; j++) {
        if (fileD[j].root == NULL) {
            /* found empty file file descriptor: set index to emptyIndex */
            emptyIndex = j;
            break;
        }
    }
    if (emptyIndex == -1) {
        /* cannot find free */
        return -1; 
    }
    fileD[emptyIndex].offset = 0;
    fileD[emptyIndex].root = &rootDir[fileIndex];
    count++;
    return emptyIndex;
}

int fs_close(int fd)
{
    if (fileD[fd].root == NULL || fd > FS_OPEN_MAX_COUNT) {
        return -1;
    }
    fileD[fd].offset = 0;
    fileD[fd].root = NULL;
    count--;
    return 0;
}

int fs_stat(int fd)
{
    if (!fileD[fd].root) {
        return -1;
    }
    /* prints the file size of the passed file descriptor */
    return fileD[fd].root->fileSize;
}

int fs_lseek(int fd, size_t offset)
{
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }
    if (!fileD[fd].root) {
        return -1;
    }
    /* set the new offset */
    fileD[fd].offset = offset;
    return 0;
}
int findFatSpace(){
    for (int i = 1; i<superblock.numDataBlock; i++) {
        if (fat[i] == 0){
            fat[i] = FAT_EOC;
            return i;
        }
    }
    return -1;
}
int block_iter(int first_blk, int steps_to_target)
{
    int itr = first_blk;
    for (int i = 0; i < steps_to_target; i++) {
        if (itr == FAT_EOC) {
            return -1;
        }
        itr = fat[itr];
    }
    return itr;
}

int next_block(int crr_blk) {
    if (fat[crr_blk] == FAT_EOC){
        return -1;
    }
    return fat[crr_blk];

}
int fs_write(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }
    int next;
    int buffer_index = 0;
    int temp_counter = 0;
    uint8_t localBuf[BLOCK_SIZE];
    /* number of steps from the first datablock of the file 
    to target datablock */
    int steps_to_target = fileD[fd].offset / BLOCK_SIZE;
    /* find the offset of the local buffer (where to start to read) */
    int local_offset = fileD[fd].offset % BLOCK_SIZE;
    /* find the target data block */
    int blk_itr = block_iter(fileD[fd].root->firstIndex, steps_to_target);
    /* edge case - first write operation begins over a block boundary */
    if (local_offset == 0 && blk_itr == FAT_EOC && 
    fileD[fd].root->firstIndex != FAT_EOC) {
        /* find a free fat space */
        int fatspace = findFatSpace();
        /* set preblock to be the block before the target block */
        int preblock = block_iter(fileD[fd].root->firstIndex, 
        steps_to_target-1);
        if (preblock > superblock.numDataBlock) {
            printf("something concerning happened");
        }
        fat[preblock] = fatspace;
        blk_itr = fatspace;
    }
    /* if the first block of the file is FAT_EOC */
    if (fileD[fd].root->firstIndex == FAT_EOC) { 
        /* We know fat[blk_itr] = EOC by dafault. */
        blk_itr = findFatSpace();
        fileD[fd].root->firstIndex = blk_itr;
    }
    /* read the target datablock into into localBuf */
    block_read(blk_itr + superblock.dataIndex, (void*)localBuf);
    for (int j = local_offset; buffer_index< count && j < BLOCK_SIZE; 
    buffer_index++, j++) {
        /* write into localBuf from the offset index j based on buf */
        localBuf[j] = ((uint8_t *)buf)[buffer_index];
        temp_counter++;
    }
    /* writeback to the disk */
    block_write(blk_itr + superblock.dataIndex, (void*)localBuf);
    /* while there are more characters to be written into localBuf */
    while (temp_counter < count) {
        next = next_block(blk_itr);
        /* if there is no next */
        if (next == -1) {
            next = findFatSpace();
            fat[blk_itr] = next; 
        }
        blk_itr = next;
        /* update local buffer */
        block_read(next + superblock.dataIndex, (void*)localBuf);
        for (int j = 0; buffer_index < count && j < BLOCK_SIZE; 
        buffer_index++, j++) {
            /* write into localBuf from the offset index j based on buf */
            localBuf[j] = ((uint8_t *)buf)[buffer_index];
            temp_counter++;
        }
        /* writeback to the disk */
        block_write(blk_itr + superblock.dataIndex, (void*)localBuf);
    }
    /* if the write operation increases the size of the file, increate the filesize */
    if (fileD[fd].offset + temp_counter > fileD[fd].root->fileSize ) {
        /* increase filesize */
        fileD[fd].root->fileSize += temp_counter + fileD[fd].offset - 
        fileD[fd].root->fileSize;
    }
    /* update file offset */
    fileD[fd].offset += temp_counter;
    /* return bytes written to the file */
    return temp_counter;
}

int fs_read(int fd, void *buf, size_t count)
{
    int next;
    int buffer_index = 0;
    int temp_counter = count;
    uint8_t localBuf[BLOCK_SIZE];
    /* number of steps from the first datablock of the file 
    to target datablock */
    int steps_to_target = fileD[fd].offset / BLOCK_SIZE;
    /* find the offset of the local buffer (where to start to read) */
    int local_offset = fileD[fd].offset % BLOCK_SIZE;
    /* find the target data block */
    int blk_itr = block_iter(fileD[fd].root->firstIndex, steps_to_target);
    /* read the target data block into the local buffer */
    block_read(blk_itr + superblock.dataIndex, (void*)localBuf);
    for (int j = local_offset; buffer_index < count && j < BLOCK_SIZE; 
    buffer_index++, j++) {
        /* read localBuf from the local offset into the buf*/
        ((uint8_t *)buf)[buffer_index] = localBuf[j];
        temp_counter--;
    }
    /* while there are more bytes to be read (hit the end of the datablock)*/
    while (temp_counter > 0) {
        /* go to the next datablock */
        next = next_block(blk_itr);
        /* update blk_itr */
        blk_itr = next;
        /* update localBuf with the new next datablock */
        block_read(next + superblock.dataIndex, (void*)localBuf);
        for (int j = 0; buffer_index < count && j < BLOCK_SIZE; 
        buffer_index++, j++) {
            /* keep reading into buf from where it was left at the end 
            of the datablock */
            ((uint8_t *)buf)[buffer_index] = localBuf[j];
            temp_counter--;
        }
    }	
    /* return total bytes read */
    return buffer_index;
}

