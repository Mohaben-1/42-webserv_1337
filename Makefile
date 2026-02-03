NAME		= webserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98

SRCDIR		= srcs
INCDIR		= includes
OBJDIR		= objs

SRCS		= $(wildcard $(SRCDIR)/*.cpp)
OBJS		= $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re