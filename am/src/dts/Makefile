DTS_FILE=$(AM_HOME)/am/src/dts/$(ARCH).dts
ifneq ($(DTS_FILE), $(wildcard $(DTS_FILE)))
	DTS_FILE=$(AM_HOME)/am/src/dts/$(PLATFORM).dts
endif


DTS = $(shell cat $(DTS_FILE))
COMMA:= ,
DEVICE_VAL = $(subst $(COMMA), ,$(device))
DEVICE_NAME = $(word 1, $(DEVICE_VAL))
DEVICE_ADDR = $(word 2, $(DEVICE_VAL))

CFLAGS_FROM_DTS = CFLAGS += -D$(addsuffix _ADDR, $(DEVICE_NAME))=$(DEVICE_ADDR)

$(foreach device, $(DTS),$(eval $(CFLAGS_FROM_DTS)))


.PHONY: test