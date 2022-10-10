
NAME = mini_serv
SRC = mini_serv.c
GO = gcc -Wall -Wextra -Werror #-g3


all :
	@$(GO) $(SRC) -o $(NAME)


fclean :
	@ rm -rf a.out a.out* $(NAME) mini_serv.dSYM
