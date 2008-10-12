CC ?= gcc
CFLAGS ?= -Wall -ggdb -ansi -std=c99

GST_LIBS := $(shell pkg-config --libs gstreamer-0.10)
GST_CFLAGS := $(shell pkg-config --cflags gstreamer-0.10)

bins += gst-identify

all: $(bins)

gst-identify: gst-identify.o
gst-identify: CFLAGS := $(CFLAGS) $(GST_CFLAGS)
gst-identify: LIBS := $(GST_LIBS)

# from Lauri Leukkunen's build system
ifdef V
Q = 
P = @printf "" # <- space before hash is important!!!
else
P = @printf "[%s] $@\n" # <- space before hash is important!!!
Q = @
endif

%.o:: %.c
	$(P)CC
	$(Q)$(CC) $(CFLAGS) -MMD -o $@ -c $<

$(bins):
	$(P)LINK
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(Q)find -name '*.o' -o -name '*.d' | xargs rm -f
	$(Q)rm -f $(bins)
