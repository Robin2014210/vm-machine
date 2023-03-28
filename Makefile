TARGET = vm_riskxvii

CC = gcc

# CFLAGS     = -c -Wall -Wvla -Werror -O0 -g -std=c11
CFLAGS     = -g
# ASAN_FLAGS = -fsanitize=address
SRC        = vm_riskxvii.c
OBJ        = $(SRC:.c=.o)

all:
	gcc -g vm_riskxvii.c


clean:
	rm -f *.o *.obj $(TARGET)
