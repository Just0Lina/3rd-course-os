
LIBS=-lcurl


all: clean
	gcc -Wall -g  launch.c proxy.c cache.c http_request.c $(LIBS)
	sudo ./a.out
	# sudo valgrind --read-var-info=yes --leak-check=full ./a.out

clean:	
	rm -f ../../wget-log*
	@rm -rf a.out
