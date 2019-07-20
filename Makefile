NAME = test_blflogger

SOURCE_DIR = src
SOURCES = $(wildcard $(SOURCE_DIR)/*.cpp)
SOURCE_BASENAMES = $(notdir $(SOURCES))

INCLUDE_DIR = $(SOURCE_DIR)
INCLUDES = $(wildcard $(SOURCE_DIR)/*.h)
# CXXFLAGS = -Wall -Werror -Wextra -I$(INCLUDE_DIR)
CXXFLAGS = -I$(INCLUDE_DIR)

# ------------------------------------------------------------------------------#
# This method of directory creation is adapted from:
# https://gist.github.com/maxtruxa/4b3929e118914ccef057f8a05c614b0f

# intermediate directory for generated object files
OBJECT_DIR := .o

# object files, auto generated from source files
OBJECTS := $(patsubst %,$(OBJECT_DIR)/%.o,$(basename $(SOURCE_BASENAMES)))
# ------------------------------------------------------------------------------#


.SUFFIXES: # remove the default suffix rules, because the GNU Make manual states:
# Suffix rules are the old-fashioned way of defining implicit rules for make.
# Suffix rules are obsolete because pattern rules are more general and clearer.
# They are supported in GNU make for compatibility with old makefiles.
#

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
