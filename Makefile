#---------------------------------Variables---------------------------------#

OBJS_DIR	=	.objs
SRCS_DIR	=	sources
HEADER_DIR	=	includes
HEADER_FILE = 	traceroute.h

NAME		=	ft_traceroute
OBJS	=	$(patsubst $(SRCS_DIR)%.c, $(OBJS_DIR)%.o, $(SRCS))
#------------------------------------------------------------------------#

#---------------------------------Sources---------------------------------#
SRCS =		$(SRCS_DIR)/traceroute.c \
			$(SRCS_DIR)/checksum.c \
			$(SRCS_DIR)/send.c \
			$(SRCS_DIR)/echoctl.c \
			$(SRCS_DIR)/parser.c \
			$(SRCS_DIR)/socket.c \
			$(SRCS_DIR)/reply.c \
			$(SRCS_DIR)/print.c \
			$(SRCS_DIR)/probe.c \
			$(SRCS_DIR)/helpers.c \
			$(SRCS_DIR)/timers.c \

			
			
#------------------------------------------------------------------------#

#---------------------------------Compilation & Linking---------------------------------#
CC		=	cc
RM		=	rm -f
CFLAGS	=	-Wall -Wextra -Werror -pthread -g3
INCLUDES = -I $(HEADER_DIR)
#------------------------------------------------------------------------#


#---------------------------------Pretty---------------------------------#
YELLOW	=	\033[1;33m
GREEN	=	\033[1;32m
RESET	=	\033[0m
UP		=	"\033[A"
CUT		=	"\033[K"
#------------------------------------------------------------------------#


all: $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c $(HEADER_DIR)/$(HEADER_FILE)
	@mkdir -p $(@D)
	@echo "$(YELLOW)Compiling [$<]$(RESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@printf $(UP)$(CUT)

$(NAME): $(OBJS) Makefile
	@echo "$(YELLOW)Compiling [$<]$(RESET)"
	@$(CC) $(CFLAGS) $(OBJS) $(INCLUDES) -o $@ -lm
	@printf $(UP)$(CUT)
# 	sudo setcap cap_net_raw+ep ./$(NAME)
	@printf $(UP)$(CUT)
	@echo "$(GREEN)$(NAME) compiled!$(RESET)"

clean:
	@echo "$(YELLOW)cleaning files$(RESET)"
	@$(RM) $(OBJS) rm -rf $(OBJS_DIR)
	@printf $(UP)$(CUT)
	@echo "$(GREEN)$(NAME) files deleted !$(RESET)"

fclean:	clean
	@echo "$(YELLOW)cleaning files$(RESET)"
	@$(RM) $(NAME)
	@printf $(UP)$(CUT)
	@echo "$(GREEN)$(NAME) executable deleted !$(RESET)"

re:	fclean $(NAME)

.PHONY:	all clean fclean re bonus FORCE