# Makefile for project

all: xyproj3 xyproj1 xyproj2 

xyproj1: xyproj1.c
	gcc xyproj1.c -Wall -Wextra -Werror -pedantic  -pthread -lX11 -o xyproj1

xyproj2: xyproj2.c
	gcc xyproj2.c -Wall -Wextra  -pedantic  -pthread -lX11 -o xyproj2


xyproj3: xyproj3.c
	gcc xyproj3.c -Wall -Wextra -pedantic -pthread -lX11 -lm -o xyproj3


clean:
	rm -f xyproj1 
	rm -f xyproj2
	rm -f xyproj3
