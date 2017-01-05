all: mythread.a

mythread.a: MyThread.o
	ar rcs mythread.a MyThread.o

MyThread.o: MyThread.c
	gcc -c MyThread.c

clean:
	rm MyThread.o
