# Welcome to My Ls
***

## Task
For each operand that names a file of a type other than directory, my_ls displays its name as well as any requested, associated information. For each operand that names a file of type directory, my_ls displays the names of files contained within that directory, as well as any requested, associated information.

If no operands are given, the contents of the current directory are displayed. If more than one operand is given, non-directory operands are displayed first; directory and non-directory operands are sorted separately and in lexicographical order.

The following options are available:

-a Include directory entries whose names begin with a dot (.).
-t Sort by time modified (most recently modified first) before sorting the operands by lexicographical order.

## Description
1. Parse command line options
2. If no files specified, list current directory
3. Otherwise:
   - Separate files from directories using lstat()
   - Sort files and directories separately  
   - Display files first, then directories
   - For multiple directories, show directory name headers

## Installation
make

## Usage
./my_ls.c argument1 argument2