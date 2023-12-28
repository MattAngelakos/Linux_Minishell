#Name: Matthew Angelakos
#Pledge: I pledge my honor that I have abided by the Stevens Honor System.
CC = gcc
all: minishell_main clean

minishell_main: minishell.o
	$(CC) minishell.o -o minishell

minishell.o: minishell.c
	$(CC) -c -g -Wall -Werror minishell.c

clean:
	rm *.o