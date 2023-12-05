TARGET = thread-example
SRCS = thread.c

CC = gcc
RM = rm
CFLAGS = -g -Wall
LIBS = -lpthread
INCLUDE_DIR = "."

# Список имен файлов без расширениu
EXAMPLES = thread thread1_2 thread1_2_d thread1_3 thread1_4 thread1_5 thread1_6 thread1_7_u1

all: ${TARGET} ${EXAMPLES}

# Цель для сборки каждого из примеров
${EXAMPLES}: %: %.c clean
	${CC} ${CFLAGS} $< -o $@ && ./$@

clean:
	${RM} -f *.o ${TARGET} ${EXAMPLES}