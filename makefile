CC = g++
OBJ = aicb.o
TARGET = AICB
EMAIL = matthew.l.allen@gmail.com

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	g++ -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o $(TARGET) log.txt 

run: $(TARGET)
	./$(TARGET)

mail:
	mail -s "AICB source" "$(EMAIL)" < aicb.cpp
	mail -s "AICB makefile" "$(EMAIL)" < makefile

gitadd:
	git add aicb.cpp
docs:
	doxygen Doxyfile
