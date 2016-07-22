RPMBUILD_DIR = $(PWD)/rpmbuild
MODULE_DIR   = $(PWD)/module
APP_DIR      = $(PWD)/app
LIB_DIR      = $(PWD)/lib
DOC_DIR      = $(PWD)/doc

include $(MODULE_DIR)/Makefile.inc
include $(APP_DIR)/Makefile.inc
include $(LIB_DIR)/Makefile.inc

TARGET_SRC_DIR=    /tmp/AnB/src
TARGET_MODULE_DIR= /tmp/AnB
TARGET_APP_DIR=    /tmp/AnB/test
TARGET_LIB_DIR=    /tmp/AnB/devel
TARGET_DOC_DIR=    /tmp/AnB/doc

SPEC_FILE= PhotonAnB.spec

RPM_BUILD_MACRO += --define "_topdir $(RPMBUILD_DIR)"
RPM_BUILD_MACRO += --define "MODULE_DIR $(MODULE_DIR)"
RPM_BUILD_MACRO += --define "APP_DIR $(APP_DIR)"
RPM_BUILD_MACRO += --define "LIB_DIR $(LIB_DIR)"
RPM_BUILD_MACRO += --define "SRC_DIR $(PWD)"
RPM_BUILD_MACRO += --define "DOC_DIR $(DOC_DIR)"

RPM_BUILD_MACRO += --define "MODULE_NAME $(MODULE_NAME)"
RPM_BUILD_MACRO += --define "APP_NAME $(APP_NAME)" 
RPM_BUILD_MACRO += --define "LIB_NAME $(LIB_NAME)" 

RPM_BUILD_MACRO += --define "TARGET_SRC_DIR $(TARGET_SRC_DIR)"
RPM_BUILD_MACRO += --define "TARGET_MODULE_DIR $(TARGET_MODULE_DIR)"
RPM_BUILD_MACRO += --define "TARGET_APP_DIR $(TARGET_APP_DIR)"
RPM_BUILD_MACRO += --define "TARGET_LIB_DIR $(TARGET_LIB_DIR)"
RPM_BUILD_MACRO += --define "TARGET_DOC_DIR $(TARGET_DOC_DIR)"

RPM_FLAGS= -ba

RPM_BUILD= rpmbuild

SUDO= sudo

all:
	cd "$(MODULE_DIR)"; make 
	cd "$(LIB_DIR)";    make MODULE_PATH="$(MODULE_DIR)"
	cd "$(APP_DIR)";    make LIB_LIST="$(LIB_DIR)/$(LIB_NAME).a" LIB_PATH="$(LIB_DIR)"
    
module:
	cd "$(MODULE_DIR)"; make 
	
lib:
	cd "$(LIB_DIR)";    make MODULE_PATH="$(MODULE_DIR)"

app:
	cd "$(APP_DIR)";    make LIB_LIST="$(LIB_DIR)/$(LIB_NAME).a" LIB_PATH="$(LIB_DIR)"

install: all
	cd "$(MODULE_DIR)"; make install
    
remove:
	cd "$(MODULE_DIR)"; make remove
    
purge:    
	cd "$(MODULE_DIR)"; make clean
	cd "$(LIB_DIR)";    make clean
	cd "$(APP_DIR)";    make clean
    
clean:
	cd "$(MODULE_DIR)"; make clean
	cd "$(LIB_DIR)";    make clean
	cd "$(APP_DIR)";    make clean
	
	$(SUDO) rm -rf "$(RPMBUILD_DIR)"

	rm -f rpm
    
rpm: 
	$(SUDO) $(RPM_BUILD) $(RPM_BUILD_MACRO) $(RPM_FLAGS) $(SPEC_FILE)
	
