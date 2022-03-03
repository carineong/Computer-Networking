CC=gcc  #compiler
TARGET = router agent

default:   
	$(CC) -o router router.c common.c common.h		
	$(CC) -o agent agent.c common.c common.h

# agent.o:	
# 	$(CC) -c agent.c common.h -o agent.o
clean:
	rm -f router.o agent.o