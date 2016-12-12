all:
	gcc -Wall y86emul.c -lm -o y86emul
test:
	./y86emul prog1.y86
	./y86emul prog3.y86
test1:
	./y86emul prog2.y86
clean:
	rm -rf y86emul 
