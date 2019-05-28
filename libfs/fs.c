#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF
#define NULL_INDEX 0xFFFF

struct superBlock{
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
    char filename[16];
    uint32_t fileSize;
    uint16_t firstIndex;
    uint8_t padding[10];
};
struct fileDescriptor{ 
    int offset;
    struct rootElement *root;
};
/* TODO: Phase 1 */
struct superBlock superblock;
uint16_t *fat;
struct rootElement rootDir[FS_FILE_MAX_COUNT];
struct fileDescriptor fileD[FS_OPEN_MAX_COUNT];
int count = 0;
int fs_mount(const char *diskname)
{
    if (block_disk_open(diskname) < 0)
    {
        return -1;
    }
    if(block_read(0,((void*)&superblock))<0){
        return -1;
    }
    fat = malloc(superblock.numFatBlock * BLOCK_SIZE);

    if(block_read(2,((void*)&rootDir)) < 0){
        return -1;
    }

    // int firstIndex = 0;
    for (int i = 1; i <= superblock.numFatBlock; i++)
    {
        //read and write on the block
        block_read(i,fat+(i-1)*BLOCK_SIZE);
    }
    return 0;

    /* TODO: Phase 1 */
}

int fs_umount(void)
{
    /* TODO: Phase 1 */
    if (block_disk_count() < 0)
    {
        return -1;
    }
    block_write(0, (&superblock));
    //block_write(1, fat);
    for (int i = 1; i <= superblock.numFatBlock; i++)
    {
        block_write(i, fat + (i-1)*BLOCK_SIZE);
    }
    block_write(superblock.rootElementIndex, rootDir);
    block_disk_close();
    free(fat);
    return 0;
}

int fs_info(void)
{
    /* TODO: Phase 1 */
    if (block_disk_count() < 0)
    {
        return -1;
    }
    // printf("Signature: %s\n", (char *)&superblock.Signature);
    printf("FS Info:\n");
    printf("total_blk_count=%d\n", superblock.numTotalBlock);
    printf("fat_blk_count=%d\n",superblock.numFatBlock);
    printf("rdir_blk=%d\n", superblock.rootElementIndex);
    printf("data_blk=%d\n", superblock.dataIndex);
    printf("data_blk_count=%d\n", superblock.numDataBlock);
    int fatcount = 0;
    int rootcount = 0;
    for (int i=0; i<superblock.numDataBlock; i++) {
        if (fat[i] == 0) {
            fatcount++;
        }
    }
    for (int i=0; i<FS_FILE_MAX_COUNT; i++) {
        if (rootDir[i].filename[0] == '\0') {
            rootcount++;
        }
    }
    printf("fat_free_ratio=%d/%d\n",fatcount,superblock.numDataBlock);
    printf("rdir_free_ratio=%d/%d\n",rootcount,FS_FILE_MAX_COUNT);

    return 0;
}

int fs_create(const char *filename)
{
    /* TODO: Phase 2 */
    int emptyIndex = -1;
    for(int i=0; i<=FS_FILE_MAX_COUNT; i++){
        if (rootDir[i].filename[0] == '\0')
        {
            emptyIndex = i;
            break;
        }else{
            if(strcmp(rootDir[i].filename,filename) == 0){
                return -1;
            }
        }
    }
    if(emptyIndex == -1){
        return -1;
    }
    strcpy(rootDir[emptyIndex].filename,filename);
    rootDir[emptyIndex].fileSize = 0;
    rootDir[emptyIndex].firstIndex = FAT_EOC;
    return 0;
}

int fs_delete(const char *filename)
{
    /* TODO: Phase 2 */
    int fileIndex = -1;
    int temp = 0;
    int currentIndex = 0;
    for(int i=0; i<=FS_FILE_MAX_COUNT; i++){
        if (strcmp(rootDir[i].filename,filename) == 0)
        {
            fileIndex = i;
            break;
        }
    }
    if(fileIndex == -1){
        return -1; // can not find the file
    }
    rootDir[fileIndex].filename[0] = '\0';
    rootDir[fileIndex].fileSize = 0;
    currentIndex = rootDir[fileIndex].firstIndex;
    if(currentIndex == FAT_EOC)
    {
        fat[currentIndex] = 0;
    }else{
        while(fat[currentIndex] != FAT_EOC){
            temp = fat[currentIndex];
            fat[currentIndex] = 0;
            currentIndex = temp;
        }
        fat[currentIndex] = 0;
    }
    rootDir[fileIndex].firstIndex = NULL_INDEX; //debate on EOC or 0
    return 0;
}

int fs_ls(void)
{
    /* TODO: Phase 2 */
    if (block_disk_count() < 0)
    {
        return -1;
    }
    printf("FS Ls:\n");
    for(int i=0; i<=FS_FILE_MAX_COUNT; i++){
        if(rootDir[i].filename[0] != '\0'){
            printf("File %s, Size: %d, Data Block: %d\n", rootDir[i].filename, rootDir[i].fileSize, rootDir[i].firstIndex);
        }
    }
    return 0;
}

int fs_open(const char *filename)
{
    /* TODO: Phase 3 */
    if(count == FS_OPEN_MAX_COUNT || filename == NULL || block_disk_count() < 0){
        return -1;
    }

    int fileIndex = -1;
    int emptyIndex = -1;

    for(int i=0; i<=FS_FILE_MAX_COUNT; i++){
        if (strcmp(rootDir[i].filename,filename) == 0)
        {
            fileIndex = i;
            break;
        }
    }
    if(fileIndex == -1){
        return -1; // can not find the file
    }
    for(int j=0; j<=FS_OPEN_MAX_COUNT; j++){
        if ((fileD[j].root == NULL))
        {
            emptyIndex = j;
            break;
        }
    }
    if(emptyIndex == -1){
        return -1; // can not find empty fileD slots
    }
    fileD[emptyIndex].offset = 0;
    fileD[emptyIndex].root = &rootDir[fileIndex];
    count++;
    return emptyIndex;
}

int fs_close(int fd)
{
    /* TODO: Phase 3 */
    if(fileD[fd].root == NULL || fd>FS_OPEN_MAX_COUNT){
        return -1;
    }
    fileD[fd].offset = 0;
    fileD[fd].root = NULL;
    count--;
    return 0;
}

int fs_stat(int fd)
{
    /* TODO: Phase 3 */
    if(!fileD[fd].root){
        return -1;
    }
    return fileD[fd].root->fileSize;
}

int fs_lseek(int fd, size_t offset)
{
    /* TODO: Phase 3 */
    if(fd < 0 || fd >= FS_OPEN_MAX_COUNT) 
    {
        return -1;
    }
    if(!fileD[fd].root){
        return -1;
    }
    fileD[fd].offset = offset;
    return 0;
}
int findFatSpace(){
    for(int i = 1; i<superblock.numDataBlock; i++){
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
    if(fd < 0 || fd >= FS_OPEN_MAX_COUNT) 
    {
        return -1;
    }
    int next;
    int buffer_index = 0;
    int temp_counter = 0;
    uint8_t localBuf[BLOCK_SIZE];
    // 1: be able to start from any offset within fd
    int steps_to_target = fileD[fd].offset / BLOCK_SIZE;
    // 2: compute new offset within starting block
    /* start reading byte within the local data block */
    int local_offset = fileD[fd].offset % BLOCK_SIZE;
    // 3: be able to read over block boundaries
    // 	  i.e. when end of block is hit, read "next" (via FAT) into localBuf
    // data blk index
    int blk_itr = block_iter(fileD[fd].root->firstIndex, steps_to_target);
    if(blk_itr == FAT_EOC){
        blk_itr = findFatSpace(); // We know fat[blk_itr]=EOC by dafault.
        fileD[fd].root->firstIndex = blk_itr;
    }
    block_read(blk_itr + superblock.dataIndex, (void*)localBuf);

    for (int j = local_offset; buffer_index< count && j < BLOCK_SIZE; buffer_index++, j++) {
        localBuf[j] = ((uint8_t *)buf)[buffer_index];
        temp_counter++;
    }
    block_write(blk_itr + superblock.dataIndex, (void*)localBuf);
    while (temp_counter < count) {
        next = next_block(blk_itr);
        if(next == -1){
            next = findFatSpace();
            fat[blk_itr] = next;    //fat[next]=FAT_EOC by default
            // fileD[fd].root->firstIndex = next;

        }
        blk_itr = next;
        block_read(next + superblock.dataIndex, (void*)localBuf);
        for (int j = 0; buffer_index < count && j < BLOCK_SIZE; buffer_index++, j++) {
            localBuf[j] = ((uint8_t *)buf)[buffer_index];
            temp_counter++;
        }
        block_write(blk_itr + superblock.dataIndex, (void*)localBuf);
    }
    // if(fileD[fd].root.size - local_offset > count){
    // 	fileD[fd].root->size += buffer_index;
    // }
    if(fileD[fd].offset + temp_counter > fileD[fd].root->fileSize ){
        fileD[fd].root->fileSize += temp_counter + fileD[fd].offset - fileD[fd].root->fileSize;
    }
    fileD[fd].offset += temp_counter;
    return temp_counter;
}

// int fs_read(int fd, void *buf, size_t count)
// {
// 	/* TODO: Phase 4 */
// 	if(fd < 0 || fd >= FS_OPEN_MAX_COUNT) 
//     {
//         return -1;
//     }
// 	// int start = fileD[fd].offset;
// 	// int end = fileD[fd].offset + count;
// 	int blocktoread[10];
// 	int iterate = fileD[fd].root->firstIndex;
// 	for(int i = 0; i<10;i++){
// 		if(fat[iterate == FAT_EOC]){
// 			blocktoread[i] = iterate;
// 			break;
// 		}else{
// 			blocktoread[i] = iterate;
// 			iterate = fat[iterate];
// 		}
// 	}


int fs_read(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */

    int next;
    int buffer_index = 0;
    int temp_counter = count;
    uint8_t localBuf[BLOCK_SIZE];
    // 1: be able to start from any offset within fd
    int steps_to_target = fileD[fd].offset / BLOCK_SIZE;
    // 2: compute new offset within starting block
    /* start reading byte within the local data block */
    int local_offset = fileD[fd].offset % BLOCK_SIZE;
    // 3: be able to read over block boundaries
    // 	  i.e. when end of block is hit, read "next" (via FAT) into localBuf
    // data blk index
    int blk_itr = block_iter(fileD[fd].root->firstIndex, steps_to_target);
    block_read(blk_itr + superblock.dataIndex, (void*)localBuf);

    for (int j = local_offset; buffer_index < count && j < BLOCK_SIZE; buffer_index++, j++) {
        ((uint8_t *)buf)[buffer_index] = localBuf[j];
        temp_counter--;
    }
    while (temp_counter > 0) {
        next = next_block(blk_itr);
        blk_itr = next;
        block_read(next + superblock.dataIndex, (void*)localBuf);
        for (int j = 0; buffer_index < count && j < BLOCK_SIZE; buffer_index++, j++) {
            ((uint8_t *)buf)[buffer_index] = localBuf[j];
            temp_counter--;
        }
    }	
    // for (int i = 0, j = local_offset; i < count && j < BLOCK_SIZE; i++, j++) {
    // 	((uint8_t *)buf)[i] = localBuf[j];
    // 	temp_counter--;
    // }

    // /* read the offset block from disk to local buffer */
    // block_read(SP.data_start_index + fileD[fd].file->index, (void*)localBuf);
    // for (int i = 0, j = fileD[fd].offset; i < count && j < BLOCK_SIZE - fileD[fd].offset; i++, j++) {
    // 	((uint8_t *)buf)[i] = localBuf[j];
    // 	// buf[i]
    // *(buf + i*sizeof(one element))
    // 
    //
    return buffer_index;
}

