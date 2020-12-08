#build an executable named detector from Asst3.c
CC = gcc
CFLAGS = -Wall -o
TARGET = KKJserver
NAME = Asst3.c

all: $(TARGET)

$(TARGET): $(NAME)
	$(CC) $(CFLAGS) $(TARGET) $(NAME) 
clean:
	rm $(TARGET)