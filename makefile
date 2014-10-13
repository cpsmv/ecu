#
# Locating the root directory
#
ROOT=./

#
# Device and EVM definitions
#
DEVICE=am335x
EVM=bbblack

#
# Include the makefile definitions. This contains compiler, linker and
# archiver definitions and options
#
include ${ROOT}/build/armv7a/gcc/makedefs

#
# Target Directories that need to be built
#
DIRS=${DRIVERS_BLD} ${PLATFORM_BLD} ${SYSCONFIG_BLD} ${UTILITY_BLD}

#
# The application directory and name
#
APPDIR=src
APPNAME=testapp

#
# Where the application will be loaded to. This is required to generate
# image with Header (Load Address and Size)
#
IMG_LOAD_ADDR = 0x80000000

#
# Application Location
#
APP=${ROOT}/$(APPDIR)/
APP_BIN=${ROOT}/ECU

#
# Application source files
#
APP_SRC=$(APP)/*.c

#
# Required library files
#
APP_LIB=-ldrivers  \
	-lutils    \
	-lplatform \
	-lsystem_config

#
# Rules for building the application and library
#
all: debug release

debug:
	make TARGET_MODE=debug lib
	make TARGET_MODE=Debug bin

release:
	make TARGET_MODE=release lib
	make TARGET_MODE=Release bin

lib:
	@for i in ${DIRS};				\
	do						\
		if [ -f $${i}/makefile ] ;		    \
		then					  \
			make $(TARGET_MODE) -C $${i} || exit $$?; \
		fi;					   \
	done;


bin:
	$(CC)  $(CFLAGS) $(APP_SRC)
	@mkdir -p $(TARGET_MODE)/
	@mv *.o* $(TARGET_MODE)/
	$(LD) ${LDFLAGS} ${LPATH} -o $(TARGET_MODE)/$(APPNAME).out \
          -Map $(TARGET_MODE)/$(APPNAME).map $(TARGET_MODE)/*.o* \
          $(APP_LIB) -lc -lgcc $(APP_LIB) $(RUNTIMELIB) -T build.lds
	@mkdir -p $(APP_BIN)/$(TARGET_MODE)
	@cp $(TARGET_MODE)/$(APPNAME).out $(APP_BIN)/$(TARGET_MODE)/$(APPNAME).out
	$(BIN) $(BINFLAGS) $(APP_BIN)/$(TARGET_MODE)/$(APPNAME).out \
               $(APP_BIN)/$(TARGET_MODE)/$(APPNAME).bin 
	cd $(ROOT)/tools/ti_image/; gcc tiimage.c -o a.out; cd - 
	       $(ROOT)/tools/ti_image/a.out $(IMG_LOAD_ADDR) NONE \
               $(APP_BIN)/$(TARGET_MODE)/$(APPNAME).bin \
               $(APP_BIN)/$(TARGET_MODE)/$(APPNAME)_ti.bin; rm -rf $(ROOT)/tools/ti_image/a.out;


#
# Rules for cleaning
#
clean:
	@rm -rf Debug Release $(APP_BIN)/Debug $(APP_BIN)/Release

clean+: clean
	@make TARGET_MODE=clean lib


