A scalar pipelined processor with a 256B instruction cache (I$) and a 256B data cache (D$) and 16 8-bit Registers.
It is a five-stage instruction pipeline: IF-ID-EX-MEM-WB
The instructions are to be of a fixed length of 2 bytes each.

All the input files are in the input folder, 
ICache.txt - stores instructions in 1 byte each line.
DCache.txt- stores initial memory data.
RF.txt - stores initial Registers value.

output files will be generated in the output folder.
DCache.txt- stores final memory data.
Output.txt - stores the final statistics.

Compile the main.cpp on a any C++ compiler.
The final statistics will be printed in the Output.txt and the final memory data will be present in the DCache.txt in the output folder.
