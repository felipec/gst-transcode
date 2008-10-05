GST_LIBS=`pkg-config --libs gstreamer-0.10 gstreamer-base-0.10`
GST_CFLAGS=`pkg-config --cflags gstreamer-0.10 gstreamer-base-0.10`

CC=gcc
CFLAGS=-Wall -ggdb -ansi -std=c99

binary=gst-transcode

all: $(binary)

$(binary): main.o
$(binary): CFLAGS := $(CFLAGS) $(GST_CFLAGS) -I$(KERNEL)/arch/arm/plat-omap/include
$(binary): LIBS := $(GST_LIBS)

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
	$(Q)$(CC) $(CFLAGS) -Wp,-MMD,$(dir $@).$(notdir $@).d -o $@ -c $<

$(binary):
	$(P)LINK
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	find -name '*.o' -delete
	rm -f $(binary)
