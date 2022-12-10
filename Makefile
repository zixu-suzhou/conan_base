PROJECT_SOURCE_DIR ?= $(abspath ./)
BUILD_DIR ?= $(PROJECT_SOURCE_DIR)/build
INSTALL_DIR ?= $(BUILD_DIR)/install
CONAN_PKG_DIR ?= $(BUILD_DIR)/conan_package
DEPLOY_DIR ?= $(BUILD_DIR)/deploy
NUM_JOB ?= 8

all:
	@echo nothing special

lint: lintcpp lintcmake
lintcpp:
	python3 -m mdk_tools.cli.cpp_lint .
lintcmake:
	python3 -m mdk_tools.cli.cmake_lint .

clean:
	cd $(BUILD_DIR) && ls | grep -v conan_data | xargs rm -rf

PACKAGE_NAME ?= sample
RUN_IN_BUILD_ENV_BATCH_ARGS := --includes u16 u18 u20 devcar orin mdc qnx --
ifneq ($(MF_SYSTEM_ROOT_DIR), )
    MF_SYSTEM_ROOT := ${MF_SYSTEM_ROOT_DIR}
endif
    MF_SYSTEM_ROOT ?= ./mf_system

include ${MF_SYSTEM_ROOT}/package/utils.mk
BUILD_ENVS_JSON := ${MF_SYSTEM_ROOT}/package/build_envs.json
RUN_IN_BUILD_ENV_PY := ${MF_SYSTEM_ROOT}/package/run_in_build_env.py

CMAKE_EXTRA_ARGS ?= # 留作接口，勿用
CMAKE_ARGS ?= \
        -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
        $(CMAKE_EXTRA_ARGS)

prepare: default_prepare
.PHONY: prepare

build: default_build
.PHONY: build

package: default_package
.PHONY: package

upload: package default_upload
.PHONY:upload

deploy: default_deploy
.PHONY:deploy



