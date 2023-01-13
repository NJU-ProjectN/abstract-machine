DTS_FILE=$(AM_HOME)/am/src/dts/$(ARCH).dts
ifneq ($(DTS_FILE), $(wildcard $(DTS_FILE)))
	DTS_FILE=$(AM_HOME)/am/src/dts/$(PLATFORM).dts
endif


DTS = $(shell cat $(DTS_FILE))
COMMA:= ,
DSRCS = $(subst $(COMMA), ,$(firstword $(DTS)))
DADDR = $(wordlist 2, $(words $(DTS)), $(DTS))
DEVICE_VAL = $(subst $(COMMA), ,$(device))
DEVICE_NAME = $(word 1, $(DEVICE_VAL))
DEVICE_ADDR = $(word 2, $(DEVICE_VAL))
DSRC_VAL = $(subst :, ,$(dsrc))
DSRC_NAME = $(word 1, $(DSRC_VAL))
DSRC_TYPE = $(word 2, $(DSRC_VAL))

CFLAGS_FROM_DTS = CFLAGS += -D$(addsuffix _ADDR, $(DEVICE_NAME))=$(DEVICE_ADDR)
AM_SRCS_FROM_DTS = DEVICE_SRCS += device/$(DSRC_NAME)/$(DSRC_NAME)-$(DSRC_TYPE).c
$(foreach device, $(DADDR), $(eval $(CFLAGS_FROM_DTS)))
$(foreach dsrc, $(DSRCS), $(eval $(AM_SRCS_FROM_DTS)))


.PHONY: test