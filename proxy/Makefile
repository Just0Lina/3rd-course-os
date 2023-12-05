
LIBS=-lcurl


all: clean
	gcc -Wall -g  launch.c proxy.c cache.c http_request.c $(LIBS)
	sudo ./a.out

clean:
	@rm -rf a.out
