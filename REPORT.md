# **ECS 150**
---

## **Project 4:** File system
---

### Implementation for fs.c
In our filesystem creation, we have a struct named superBlock to contain the internals in the first block. Then we have a struct named rootElement that will be array of struct for the entire root directory. Then also a fat array with dynamiclly allocated in mounting process. Also we have a struct for file descriptor to store the offset and the pointer to its root directory.

**fs_mount:**
We dynamiclly allocating new space for fat array with total number of fat block * BLOCK_SIZE. Then we used the given disk.c function called block_read() and pass the 0 for the superblock, 2 for the root directery and look through the numfatblock and read it one by one. 

**fs_umount:**
In our unmount, we need to unmount currently mounted file system and  also write back what we have changed then close the underlying virtual disk files and free the fat array.

**fs_info**
Simply printing what is contained in our super block struct and also loop through the total number of data block to find the avaliable fat block space and count them in order to find the fat free ratio. And as for the root directory free ratio, loop through the FS_FILE_MAX_COUNT to find the roodDir with empty filenames, then calculate the ratio based on the counts. 

**fs_create**
In our create a file function, first of all, we loop through the root directory to find an empty space and ge the index of it, and return -1 if the filename already exsits. Then we used strcpy() to set the filemame to the root director within that index. Then also set the fileSize to 0, and first index to FAT_EOC since there is no content for the file yet.

**fs_delete:**
For deleting the file, is the similar process which is undo what we did in create. Which loop through the root directory to find the file, and get the index and change the filename and size and the desginated first index of the root directory. In addition, we need to find the fat block and data block and delete them as well. First we find the fat[currentIndex] to check if it is FAT_EOC, if not then loop through to unchain the fat block in a while loop by setting the fat blocks to 0. 

**fs_ls:**
For printing the files that we have in our root directory, we check if the first charater of the filename is '\0' and if it is not, that means that root directory is taken by a file, then we print the file and size and the data block number. 

**fs_open:**
For opening a file, first we loop through the root director to find the file by filename, if the filename can not be found, then return -1.  When a file is being open, it will create a file descriptor and when the file descriptor is valid until the file is closed. the Then loopthrough the file descriptor array to find a avaliable space then remember the index and link the previously found file root element to the file descriptor. Set the offset of the fd[element] to 0 and pointer to the root element that found by the filename and return the index of the file descriptor. 

**fs_close**
For closer a file, simply removing the file descriptor array with associated fd, and set the offset to 0 and set the the file pointer  to NULL which was originally pointing to a opened file. 

**fs_stat**
We simply returned the file descritor array with given fd to its filesize. 


**fs_write/fs_read**
We have definded local_buffer in order to store the target datablock in order to read the data starting from the offset. In order to find the data block that we want to start reading, we did the file offset/BLOCK_SIZE to be steps_to_target which we iterated through a helper function block_iter that takes the steps and the first block of the file which is blk_itr. Then we also have a local_offset for where to start read in that found target block. Then we start reading from the sum of the index that the data block stars and blk_itr, into the local buffer. We start reading from the found offset of the local buffer into buf. 



### Overview



### Resources
+ Information/code provided on lecture slides
+ The GNU C Library


