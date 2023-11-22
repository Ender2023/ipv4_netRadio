CC			?=	gcc
MKDIR		:=	@mkdir -p
ECHO		:=	@echo
RM			:=	@rm -rf
C_FLAGS		?=  -Wall -O0 -g
SUFFIX		?=	.elf

#specify target name
TARGET_NAME	:=	server
TARGET_ELF	:=	$(TARGET_NAME)$(SUFFIX)

#get current directory
CUR_DIR		:=	$(shell pwd)

#user source files search directory
SRC_DIR		:=	$(CUR_DIR) \

C_FILES		:=	$(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.c))

VPATH		:=	$(SRC_DIR)

#user include files search directory
INC_DIR		:=	$(CUR_DIR) \
				$(CUR_DIR)/../include \

H_FILES		:=	$(foreach dir, $(INC_DIR), $(wildcard $(dir)/*.h))

INC_FLAGS	:=	$(foreach dir, $(INC_DIR), -I$(dir))

#user output files directory
OUT_DIR		?=	$(CUR_DIR)/build

OBJS		:=	$(patsubst %.c, $(OUT_DIR)/%.o, $(notdir $(C_FILES)))

#user flags for compiler
USER_FLAGS	:=	-pthread
USER_FLAGS	+=	$(INC_FLAGS)

C_FLAGS		+=	$(USER_FLAGS)

.PHONY:	all create_build clean test

all:$(OUT_DIR)/$(TARGET_ELF) | create_build

$(OUT_DIR)/$(TARGET_ELF):$(OBJS) | create_build
ifeq ($(VERBOSE),0)
	$(ECHO) "	LD	$<"
endif
	$(CC) $^ -o $@ $(C_FLAGS)

$(OUT_DIR)/%.o:%.c $(H_FILES) | create_build
ifeq ($(VERBOSE),0)
	$(ECHO) "	CC	$<"
endif
	$(CC) -c $< -o $@ $(C_FLAGS)

create_build:
ifeq ($(wildcard $(OUT_DIR)),)
	$(MKDIR) $(OUT_DIR)
endif

clean:
ifneq ($(wildcard $(OUT_DIR)),)
	$(RM) $(OUT_DIR)
else
	$(ECHO) "nothing to do with target $(TARGET_NAME)"
endif

test:
	$(ECHO)	"TARGET_ELF:\n$(TARGET_ELF)\n"
	$(ECHO)	"CUR_DIR:\n$(CUR_DIR)\n"
	$(ECHO)	"SRC_DIR:\n$(SRC_DIR)\n"
	$(ECHO)	"OUT_DIR:\n$(OUT_DIR)\n"
	$(ECHO)	"C_FILES:\n$(C_FILES)\n"
	$(ECHO)	"OBJS:\n$(OBJS)\n"
	$(ECHO)	"C_FLAGS:\n$(C_FLAGS)\n"
