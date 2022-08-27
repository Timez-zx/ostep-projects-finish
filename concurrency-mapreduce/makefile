cc = gcc
CFLAG = -pthread -O
target = wordcount
rely = mapreduce.c wordcount.c
header = mapreduce.h
$(target):$(rely) $(header)
	$(cc) $(CFLAG) $(rely) -o $(target)

clean :
	@echo "start clean target and objects"
	rm $(target)

