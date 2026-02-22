NAME		= webserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98

SRCDIR		= srcs
INCDIR		= includes
OBJDIR		= objs

FILES		= CGI Config Request Response Server ServerManager main
HEADERS		= CGI Config Request Response Server ServerManager

INCS 		= $(addprefix $(INCDIR)/, $(addsuffix .hpp, $(HEADERS)))
SRCS		= $(addprefix $(SRCDIR)/, $(addsuffix .cpp, $(FILES)))
OBJS		= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(FILES)))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re