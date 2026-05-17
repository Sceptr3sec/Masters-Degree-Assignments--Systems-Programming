# Welcome to My Tar
***

## Task

## This project was completed with a partner
my_tar is a command to manipulate tape archive. The first option to tar is a mode indicator from the following list:

-c Create a new archive containing the specified items.
-r Like -c, but new entries are appended to the archive. The -f option is required.
-t List archive contents to stdout.
-u Like -r, but new entries are added only if they have a modification date newer than the corresponding entry in the archive. The -f option is required.
-x Extract to disk from the archive. If a file with the same name appears more than once in the archive, each copy will be extracted, with later copies overwriting (replacing) earlier copies.
In -c, -r, or -u mode, each specified file or directory is added to the archive in the order specified on the command line. By default, the contents of each directory are also archived.

Unless specifically stated otherwise, options are applicable in all operating modes:

-f file Read the archive from or write the archive to the specified file. The filename can be standard input or standard output.
my_tar will not handle file inside subdirectory.

## Description
I implemented several helper functions for handling low-level operations such as reading and writing file data (robust read & write).

For create mode (-c):
I created helper functions for read and write to be called later on in code. 
    I used the lstat() system call to gather file metadata without following symbolic links. This allows symbolic links to be archived as links rather than as the files they point to.

    Each file, directory, or symlink is passed to build_header_for_path(), which constructs a valid 512-byte TAR header according to the ustar format.

    For regular files, the files data is read using robust_read() and written directly to the archive with robust_write().

    The program then writes zero-padding to align each entry to the next 512-byte boundary.

    Directories are handled recursively using archive_path(), ensuring all subdirectories and files are added.

    After processing all entries, two zero blocks (1024 bytes total) are written to mark the end of the archive, as required by the TAR specification.

For list mode (-l):
In list mode, the program displays the names of all files stored in an existing archive:
    The program reads one 512-byte header block at a time using robust_read().

    It verifies header integrity by recomputing the checksum and comparing it to the stored value.

    The filename is reconstructed by combining the prefix and name fields.

    If a file entry is encountered, its data is skipped by calculating how many 512-byte blocks the file occupies and advancing the file descriptor accordingly.

    The process continues until two consecutive zero blocks are read, signaling the end of the archive.

For extract mode (-x):
In extract mode, the program restores files, directories, and symbolic links from an existing archive.
    Each header is read and verified just like in list mode.

    Depending on the typeflag, the program:

    Creates directories recursively with mkdir_p().

    Recreates symbolic links using symlink().

    Extracts regular files by reading their data blocks with robust_read() and writing them to disk with robust_write().

    File permissions and modification times are restored using chmod() and utime().

    After each files data is processed, any padding bytes are skipped to align to the next header.

## Installation
No outside packages are required for installation. All packages are in the source code files.
Compile `my_tar.c` with GCC or Clang.
Makefile - *.c - *.h

## Usage
To run the executable program after compilation:
./my_tar argument1 argument2