export CFLAGS = -std=c99 -g
export ASMFLAGS =
export CC = gcc
export CXX = g++
export LD = gcc
export ASM = nasm
export LINKFLAGS =
export LIBS =

export TARGET = i686-elf
export TARGET_ASM = nasm
export TARGET_ASMFLAGS =
export TARGET_CFLAGS = -std=c99 -g #-O2
export TARGET_CPPFLAGS = -std=c++17 -fno-exceptions -fno-rtti  #-O2
export TARGET_CC = $(TARGET)-gcc
export TARGET_CXX = $(TARGET)-g++
export TARGET_LD = $(TARGET)-gcc
export TARGET_LINKFLAGS =
export TARGET_LIBS =

export BUILD_DIR = $(abspath build)
export WORKSPACE_DIR = $(abspath .)
export SRC_DIR = src
export CD_ISO = $(BUILD_DIR)/cd
export IMG_BUILDER = python3 buildScripts/buildImage.py
export IMG_INSTALLER = imginstall.mk
export IMG_MOUNT = build/tmount
export ROOT_FILES = root

TOOLCHAIN_PREFIX = $(abspath toolchain/$(TARGET))
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)

BINUTILS_VERSION = 2.41
BINUTILS_URL = https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz

GCC_VERSION = 11.4.0
GCC_URL = https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz

export TOOLCHAIN_LIBS = $(TOOLCHAIN_PREFIX)/lib/gcc/$(TARGET)/$(GCC_VERSION)
