HOST_CC		:=	gcc

C_FLAGS		:=	-Wall -O0 -g

MAKE_FLAGS	:=	-f Makefile --no-print-directory

TOP_DIR		:=	$(shell pwd)

INSTALL_DIR	:=	$(TOP_DIR)/install

SUB_DIR		:=	$(TOP_DIR)/client \
				$(TOP_DIR)/server

SUFFIX		:=	.elf

OUT_DIR		:=	./build

ifeq ("$(origin V)","command line")
    VERBOSE_OPT	:=	$(V)
endif
    
ifndef VERBOSE_OPT
    VERBOSE		:=	0
else
    VERBOSE		:=	1
endif

ifeq ($(VERBOSE),1)
    Q			:=	
else
    Q			:=	@
endif

CC				:=	$(Q)$(HOST_CC)

MKDIR			:=	$(Q)mkdir -p
RM				:=	$(Q)rm -rf

ECHO			:=	@echo
CP				:=	cp
MAKE			:=	make

export CC MKDIR RM OUT_DIR SUFFIX C_FLAGS VERBOSE

.PHONY:	all install distclean test

all:
	$(Q)for dir in $(SUB_DIR); do $(MAKE) -C $$dir $(MAKE_FLAGS) $@; done

distclean:
	$(Q)for dir in $(SUB_DIR); do $(MAKE) -C $$dir $(MAKE_FLAGS) clean; done
	$(RM) $(INSTALL_DIR)

install:
ifeq ($(wildcard $(INSTALL_DIR)),)
	$(MKDIR) $(INSTALL_DIR)
endif
	$(Q)for dir in $(SUB_DIR); do $(CP) $$dir/$(OUT_DIR)/*.elf $(INSTALL_DIR); done

test:
	$(ECHO)	"CC:\n$(CC)\n"
	$(ECHO)	"TOP_DIR:\n$(TOP_DIR)\n"
	$(ECHO)	"INSTALL_DIR:\n$(INSTALL_DIR)\n"
	$(ECHO)	"SUB_DIR:\n$(SUB_DIR)\n"
