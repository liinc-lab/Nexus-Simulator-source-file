#
# Makefile
#

PREFIX			=	/usr/local
INSTALL_DIR		=	$(PREFIX)/bin
LIB_DIR			=	$(PREFIX)/lib

OUTPUT			=	nexus
#OUTPUT			=	nexus1.0

BUILD_PREFIX		=	/tmp/nexus-1.0-build
PACKAGE_NAME		=	nexus-1.0-rh71
PACKAGE_FILE		=	$(PACKAGE_NAME).tar.gz


all:
	cd src; $(MAKE) OUTPUT=$(OUTPUT) all

install: all
	test -d $(INSTALL_DIR) || mkdir -p $(INSTALL_DIR)
	cp -p src/$(OUTPUT) $(INSTALL_DIR)/
	@chmod 755 $(INSTALL_DIR)/$(OUTPUT)
	test -d $(LIB_DIR) || mkdir -p $(LIB_DIR)
	cp -rp lib/nexus $(LIB_DIR)/nexus
	@chmod 755 $(LIB_DIR)/nexus

package:
	$(MAKE) PREFIX=$(BUILD_PREFIX)/$(PACKAGE_NAME) install
	tar zcfC $(PACKAGE_FILE) $(BUILD_PREFIX) $(PACKAGE_NAME)
	rm -fr $(BUILD_PREFIX)

clean:
	cd src; $(MAKE) OUTPUT=$(OUTPUT) clean
	-rm -f *~
#	-rm -f $(PACKAGE_FILE)
