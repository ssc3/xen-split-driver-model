all : server_test.c client_test.c data.h
	@echo Building..
	@gcc -D_GNU_SOURCE -g -o bin/server server_test.c -lrt
	@gcc -g -pthread -o bin/client client_test.c -lrt -lm
	@echo Done!
	@echo Now run

.PHONY : clean

clean :
	@rm bin/server
	@rm bin/client
	@echo Cleaned!

