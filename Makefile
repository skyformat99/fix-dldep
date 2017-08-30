MODULE := \
	fix-dldep
inc := $(shell ls inc/*)
src := $(shell ls src/*)
CC := g++
TARGET_BIT ?= 32
CC_FLAG := -g -Iinc -Wall -std=c++11 -DBIT=$(TARGET_BIT)
all:$(MODULE)
$(foreach m,$(MODULE),$(eval TARGET := $(m))$(eval DEP := $(src) $(inc))$(eval include build/reg_rule.mk))
.PHONY:clean
clean:
	-@ rm -rf $(MODULE)
