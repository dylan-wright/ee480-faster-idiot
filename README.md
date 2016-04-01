# EE480 Assignment 3 - A Faster IDIOT
A pipelined implementation of the IDIOT ISA
## IDIOT scripts
### aik.py
Use for interfacing with Dr. Dietz's AIK cgi. Usage:

`echo "filename" | ./aik.py`

### build.sh
Use for building all idiot programs in the directory. Creates .data.vmem and
.text.vmem Usage:

`./build.sh`

The .gitignore will prevent the vmem's from being added to the repo.
