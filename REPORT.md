# **ECS 150**
---

## **Project 4:** File system
---

### Implementation for fs.c
In our filesystem creation, we have a struct named superBlock to contain the
information of the disk . Then we have a struct rootElement that will be an
array of struct for the entire root directory. Then also a fat array with
dynamically allocated in the mounting process. Also, we have a struct for
a file descriptor to store the offset and the pointer to its root
directory.

**fs_mount:**
We are dynamically allocating new space for a fat array with the total number
of `fat block * BLOCK_SIZE`. Then we used the given disk.c function called
`block_read()` and pass the 0 for the superblock, 2 for the root directory and
look through the numfatblock and read fat section that contains one or more fat
blocks. 

**fs_umount:**
In our unmount, we need to unmount currently mounted file system and also write
back what we have changed then close the underlying virtual disk files and free
the fat array.

**fs_info**
Simply printing what is contained in our superblock struct and also loop
through the total number of the data block to find the available fat block
space and count them in order to find the fat free ratio. And as for the root
directory free ratio, loop through the `FS_FILE_MAX_COUNT` to find the roodDir
with empty filenames, then calculate the ratio based on the counts. 

**fs_create**
In our create a file function, first of all, we loop through the root directory
to find an empty space and get the index of it and `return -1` if the filename
already exists. Then, we used `strcpy()` to set the filename to the root
directory within that index. Then also set the fileSize to 0, and the first
index to FAT_EOC since there is no content for the file yet.

**fs_delete:**
For deleting the file, is the similar process which undoes what we did in
creation. Which loop through the root directory to find the file, and get
the index and change the filename and size and the designated first index of
the root directory. In addition, we need to find the fat block and data block
and delete them as well. First, we find the `fat[currentIndex]` to check if it
is `FAT_EOC`, if not then loop through to unchain the fat block in a while loop
by setting the fat blocks to 0. 

**fs_ls:**
For printing the files that we have in our root directory, we check if the
first character of the filename is `'\0'`, and if it is not, that means that
root directory is taken by a file, then we print the file and size and the
data block number. 

**fs_open:**
For opening a file, first, we loop through the root directory to find the file
by filename, if the filename cannot be found, then `return -1`.  When a file is
being opened, it will create a file descriptor and the file descriptor is valid
until the file is closed. then through the file descriptor array to find an
available space then remember the index and link the previously found file root
element to the file descriptor. Set the offset of the `fd[element]` to 0 and
pointer to the root element that found by the filename and returns the index of
the file descriptor. 

**fs_close**
For closing a file, simply removing the file descriptor array with associated
fd, and set the offset to 0 and set the file pointer to NULL, which was
originally pointing to an opened file. 

**fs_stat**
We simply returned the file descriptor array with given fd to its file size. 


**fs_write/fs_read**
We have defined local_buffer in order to store the target data block. To find
the data block that we want to start reading, we did the file
`offset/BLOCK_SIZE` to be steps_to_target, which iterates through a helper
function `block_iter()` that takes the steps and the first block of the file
named blk_itr. Then, we have a local_offset variable that tells us where to
start reading at found target datablock. Then, we start reading from the sum of
the index that the data block stars and blk_itr, into the local buffer. We
start reading from the found offset of the local buffer into buf. There are
different cases that involved in the writing operation. First of all, it can
be a new file just created with offset is 0, and we initialized the new block
with `findfatpace()` function that finds the first avaliable slot. Then start
writing and if it meets the end of the edge of this block, and there are more
bytes to write, then it will find another empty fat space link that to the
previous datablock. Secondly, if the file has some data, we start to write
from the offset which is the end of the existing file. We iterate to the end
of the file with `block_iter()` function and begin writing from there. In
addition, we also covered the edge-case for writing operation that begins over
a block boundary. In order to cover this edge-case, we find a new space and
start writing from the newly created datablock. At the end of write operation,
we update the offset in the file descriptor and also update the file size in
the root directory if the newly added size expands the original size. And
return the temp_counter which is how many bytes we wrote. Finally, for reading
operation we just need to read from the local buffer instead of writing in.
Also, we do not need to create new datablocks either.


### Resources
+ Information/code provided on lecture slides
+ The GNU C Library


