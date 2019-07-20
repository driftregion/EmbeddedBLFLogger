NAME = test_blflogger

SOURCE_DIR = src
SOURCES = $(wildcard $(SOURCE_DIR)/*.cpp) test.cpp
SOURCE_BASENAMES = $(notdir $(SOURCES))

INCLUDE_DIR = $(SOURCE_DIR)
INCLUDES = $(wildcard $(SOURCE_DIR)/*.h)
# CXXFLAGS = -Wall -Werror -Wextra -I$(INCLUDE_DIR)
CXXFLAGS = -I$(INCLUDE_DIR)

.SUFFIXES: # remove the default suffix rules, because the GNU Make manual states:
# Suffix rules are the old-fashioned way of defining implicit rules for make.
# Suffix rules are obsolete because pattern rules are more general and clearer.
# They are supported in GNU make for compatibility with old makefiles.

all: $(NAME)

$(NAME): $(SOURCES) $(INCLUDES)
	make -C can_common
	$(CXX) $^ -I$(INCLUDE_DIR) -Ican_common/src -Lcan_common -lcan_common -lz -o $@

clean:
	/bin/rm -f $(NAME)

fclean: clean
	/bin/rm -f $(NAME)

re: clean all
	
debug: CXXFLAGS += -g
debug: fclean all	
