EE_BIN = ps2snes2005_boot.elf

CORE_DIR = ..
SRC_DIR  = $(CORE_DIR)/source
COMM_DIR = $(CORE_DIR)/libretro-common

CORE_SRCS = \
        $(SRC_DIR)/c4.c \
        $(SRC_DIR)/c4emu.c \
        $(SRC_DIR)/cheats2.c \
        $(SRC_DIR)/cheats.c \
        $(SRC_DIR)/clip.c \
        $(SRC_DIR)/cpu.c \
        $(SRC_DIR)/cpuexec.c \
        $(SRC_DIR)/cpuops.c \
        $(SRC_DIR)/data.c \
        $(SRC_DIR)/dma.c \
        $(SRC_DIR)/dsp1.c \
        $(SRC_DIR)/fxemu.c \
        $(SRC_DIR)/fxinst.c \
        $(SRC_DIR)/gfx.c \
        $(SRC_DIR)/getset.c \
        $(SRC_DIR)/globals.c \
        $(SRC_DIR)/memmap.c \
        $(SRC_DIR)/obc1.c \
        $(SRC_DIR)/ppu.c \
        $(SRC_DIR)/sa1.c \
        $(SRC_DIR)/sa1cpu.c \
        $(SRC_DIR)/sdd1.c \
        $(SRC_DIR)/sdd1emu.c \
        $(SRC_DIR)/seta010.c \
        $(SRC_DIR)/seta011.c \
        $(SRC_DIR)/seta018.c \
        $(SRC_DIR)/seta.c \
        $(SRC_DIR)/spc7110.c \
        $(SRC_DIR)/spc7110dec.c \
        $(SRC_DIR)/srtc.c \
        $(SRC_DIR)/tile.c \
        $(SRC_DIR)/apu.c \
        $(SRC_DIR)/soundux.c \
        $(SRC_DIR)/spc700.c \
        $(CORE_DIR)/libretro.c

BASE_OBJS = \
        app/main.o \
        platform/ps2/ps2_video.o \
        platform/ps2/ps2_video_core.o \
        platform/ps2/ps2_video_menu.o \
        platform/ps2/ps2_video_debug.o \
        platform/ps2/ps2_video_color.o \
        platform/ps2/ps2_launcher_video.o \
        platform/ps2/ps2_input.o \
        platform/ps2/ps2_menu.o \
    platform/ps2/ps2_audio.o \
        ui/select_menu/select_menu.o \
        ui/select_menu/select_menu_actions.o \
        ui/select_menu/select_menu_render.o \
        ui/select_menu/select_menu_pages.o \
        ui/select_menu/font/select_menu_font.o \
        ui/launcher/launcher.o \
        ui/launcher/launcher_actions.o \
        ui/launcher/launcher_render.o \
        ui/launcher/launcher_font.o \
        ui/launcher/launcher_pages.o \
        ui/launcher/launcher_logo.o \
        ui/launcher/launcher_logo_data.o \
        ui/launcher/launcher_browser.o \
        ui/launcher/launcher_browser_devices.o \
        ui/launcher/launcher_browser_scan.o \
        ui/launcher/launcher_browser_sort.o \
        assets/smw_blob.o \
    assets/audsrv_blob.o \
        $(CORE_SRCS:.c=.o)

ROM_LOADER_OBJS = \
        rom_loader/rom_loader.o \
        rom_loader/rom_zip.o \
        rom_loader/miniz/miniz.o \
        rom_loader/miniz/miniz_tdef.o \
        rom_loader/miniz/miniz_tinfl.o \
        rom_loader/miniz/miniz_zip.o

APP_OBJS = \
        app/app_game.o \
        app/app_launcher.o \
        app/app_runtime.o \
        app/app_overlay.o \
        app/app_boot.o \
        app/app_callbacks.o

SELECT_MENU_OBJS = \
        ui/select_menu/select_menu_actions_main.o \
        ui/select_menu/select_menu_actions_video.o \
        ui/select_menu/select_menu_actions_game.o \
        ui/select_menu/select_menu_pages_main.o \
        ui/select_menu/select_menu_pages_video.o \
        ui/select_menu/select_menu_pages_game.o \
        ui/select_menu/select_menu_pages_common.o \
        ui/select_menu/select_menu_pages_icons.o \
        ui/select_menu/select_menu_pages_layout.o

LAUNCHER_OBJS = \
        ui/launcher/launcher_pages_main.o \
        ui/launcher/launcher_pages_browser.o \
        ui/launcher/launcher_pages_options.o \
 ui/launcher/launcher_pages_credits.o \
        ui/launcher/launcher_actions_main.o \
        ui/launcher/launcher_actions_browser.o \
        ui/launcher/launcher_actions_options.o \
        ui/launcher/launcher_browser_state.o \
        ui/launcher/launcher_browser_open.o \
        ui/launcher/launcher_browser_nav.o \
        ui/launcher/font/browser_font.o \
        ui/launcher/launcher_background.o \
        ui/launcher/launcher_bg_ntsc_data.o \
        ui/launcher/launcher_bg_pal_data.o

VIDEO_EXTRA_OBJS = \
        platform/ps2/ps2_video_ui.o \
        platform/ps2/ps2_video_launcher_target.o

EXTRA_OBJS = \
        $(ROM_LOADER_OBJS) \
        $(APP_OBJS) \
        $(SELECT_MENU_OBJS) \
        $(LAUNCHER_OBJS) \
        $(VIDEO_EXTRA_OBJS)

EE_OBJS = \
        $(BASE_OBJS) \
        $(EXTRA_OBJS)

EE_INCS += -I$(PS2SDK)/ports/include  -I$(CORE_DIR) -I$(SRC_DIR) -I$(COMM_DIR)/include
EE_CFLAGS += -O2 -G0 -DLOAD_FROM_MEMORY -DLAGFIX
EE_CFLAGS += -I. -Irom_loader -Irom_loader/miniz -DMINIZ_NO_ARCHIVE_WRITING_APIS
EE_CFLAGS += -Iapp -Iplatform/ps2 -Iassets
EE_LIBS += -L$(PS2SDK)/ports/lib -lps2_drivers  -ldebug -lpacket -ldraw -lgraph -ldma -lpad -lpatches -liopreboot -laudsrv

EXTRA_TARGETS = $(EE_BIN)

PS2SDK = $(PS2DEV)/ps2sdk

ANDROID_OUT_DIR ?= /sdcard/ps2
WIN_OUT_DIR ?=

include $(PS2SDK)/samples/Makefile.pref
EE_DBGINFOFLAGS :=
include $(PS2SDK)/samples/Makefile.eeglobal
EE_CFLAGS := $(filter-out -g -gdwarf-2 -gz,$(EE_CFLAGS))

V ?= 0

ifeq ($(V),0)
REAL_EE_CC := $(EE_CC)
REAL_EE_CXX := $(EE_CXX)
REAL_EE_AS := $(EE_AS)

EE_CC = ./tools/build_step.sh CC "$(REAL_EE_CC)"
EE_CXX = ./tools/build_step.sh CXX "$(REAL_EE_CXX)"
EE_AS = ./tools/build_step.sh AS "$(REAL_EE_AS)"

.SILENT:
endif

.DEFAULT_GOAL := $(EE_BIN)

.PHONY: help push push-android push-win clean rebuild
help:
	@echo "InfinityStation / ps2boot"
	@echo
	@printf "  %-20s %s\n" "make" "Compila o projeto"
	@printf "  %-20s %s\n" "make clean" "Remove arquivos gerados"
	@printf "  %-20s %s\n" "make rebuild" "Limpa e recompila"
	@printf "  %-20s %s\n" "make V=1" "Mostra os comandos completos do compilador"
	@printf "  %-20s %s\n" "make help" "Mostra esta ajuda"
	@printf "  %-20s %s\n" "make push" "Copia o ELF para o destino padrao (Android)"
	@printf "  %-20s %s\n" "make push-android" "Copia o ELF para $(ANDROID_OUT_DIR)"
	@printf "  %-20s %s\n" "make push-win" "Copia o ELF para WIN_OUT_DIR"
	@printf "  %-20s %s\n" "make push-win WIN_OUT_DIR=/caminho" "Define a pasta de destino no Windows"
	@echo
	@printf "  %-20s %s\n" "ANDROID_OUT_DIR" "$(ANDROID_OUT_DIR)"
	@printf "  %-20s %s\n" "WIN_OUT_DIR" "$(if $(WIN_OUT_DIR),$(WIN_OUT_DIR),<nao definido>)"
	@printf "  %-20s %s\n" "EE_BIN" "$(EE_BIN)"

push: push-android

push-android: $(EE_BIN)
	@mkdir -p "$(ANDROID_OUT_DIR)"
	@cp -fv "$(EE_BIN)" "$(ANDROID_OUT_DIR)/"

push-win: $(EE_BIN)
	@if [ -z "$(WIN_OUT_DIR)" ]; then \
		echo "ERRO: defina WIN_OUT_DIR. Exemplo:"; \
		echo '  make push-win WIN_OUT_DIR="/mnt/c/Users/SeuNome/Desktop/ps2"'; \
		exit 1; \
	fi
	@mkdir -p "$(WIN_OUT_DIR)"
	@cp -fv "$(EE_BIN)" "$(WIN_OUT_DIR)/"

clean:
	@echo "[clean] removendo binario e objetos..."
	@rm -f "$(EE_BIN)"
	@find . -type f \( -name '*.o' -o -name '*.d' \) -delete
	@echo "[clean] ok"

rebuild: clean
	@$(MAKE) $(EE_BIN)
