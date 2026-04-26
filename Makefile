EE_BIN = ps2snes2005_boot.elf

CORE_DIR = .
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
        $(SRC_DIR)/apu_blargg.c \
        $(CORE_DIR)/libretro.c


COMM_SRCS = \
 $(COMM_DIR)/compat/compat_posix_string.c \
 $(COMM_DIR)/compat/compat_strcasestr.c \
 $(COMM_DIR)/compat/compat_strl.c \
 $(COMM_DIR)/compat/fopen_utf8.c \
 $(COMM_DIR)/encodings/encoding_utf.c \
 $(COMM_DIR)/file/file_path.c \
 $(COMM_DIR)/file/file_path_io.c \
 $(COMM_DIR)/streams/file_stream.c \
 $(COMM_DIR)/streams/file_stream_transforms.c \
 $(COMM_DIR)/string/stdstring.c \
 $(COMM_DIR)/time/rtime.c \
 $(COMM_DIR)/vfs/vfs_implementation.c

COMM_OBJS = $(COMM_SRCS:.c=.o)

BASE_OBJS = \
        ps2boot/app/main.o \
        ps2boot/video/video.o \
        ps2boot/video/video_core.o \
        ps2boot/video/video_menu.o \
        ps2boot/video/video_debug.o \
        ps2boot/debug/debug_font.o \
        ps2boot/video/video_color.o \
        ps2boot/video/launcher_video.o \
        ps2boot/input/input.o \
        ps2boot/menu/menu.o \
        ps2boot/audio/audio.o \
        ps2boot/audio/audio_backend_audsrv.o \
        ps2boot/storage/disc.o \
        ps2boot/ui/select_menu/select_menu.o \
        ps2boot/ui/select_menu/actions/actions.o \
        ps2boot/ui/select_menu/render/render.o \
        ps2boot/ui/select_menu/pages/pages.o \
        ps2boot/ui/select_menu/font/font.o \
        ps2boot/ui/launcher/launcher.o \
        ps2boot/ui/launcher/actions/actions.o \
        ps2boot/ui/launcher/state/state.o \
        ps2boot/ui/launcher/render/render.o \
        ps2boot/ui/launcher/font/font.o \
        ps2boot/ui/launcher/pages/pages.o \
        ps2boot/ui/launcher/logo/logo.o \
        ps2boot/ui/launcher/logo/logo_data.o \
        ps2boot/ui/launcher/browser/browser.o \
        ps2boot/ui/launcher/browser/devices.o \
        ps2boot/ui/launcher/browser/scan.o \
        ps2boot/ui/launcher/browser/sort.o \
    ps2boot/irx/audsrv_blob.o \
    ps2boot/irx/usb_irx_blob.o \
        $(CORE_SRCS:.c=.o) \
  $(COMM_OBJS)

ROM_LOADER_OBJS = \
        ps2boot/rom_loader/rom_loader.o \
        ps2boot/rom_loader/rom_loader_util.o \
        ps2boot/rom_loader/rom_zip.o \
        ps2boot/rom_loader/vendor/miniz/miniz.o \
        ps2boot/rom_loader/vendor/miniz/miniz_tdef.o \
        ps2boot/rom_loader/vendor/miniz/miniz_tinfl.o \
        ps2boot/rom_loader/vendor/miniz/miniz_zip.o

APP_OBJS = \
        ps2boot/app/app_game.o \
        ps2boot/app/app_save.o \
 ps2boot/app/app_state.o \
 ps2boot/app/app_transition.o \
 ps2boot/app/frontend_config.o \
        ps2boot/app/app_launcher.o \
        ps2boot/app/app_runtime.o \
        ps2boot/app/app_overlay.o \
        ps2boot/app/app_boot.o \
        ps2boot/app/app_callbacks.o

SELECT_MENU_OBJS = \
        ps2boot/ui/select_menu/actions/main.o \
        ps2boot/ui/select_menu/actions/video.o \
        ps2boot/ui/select_menu/state/state.o \
        ps2boot/ui/select_menu/actions/game.o \
        ps2boot/ui/select_menu/pages/main.o \
        ps2boot/ui/select_menu/pages/video.o \
        ps2boot/ui/select_menu/pages/game.o \
        ps2boot/ui/select_menu/pages/common.o \
        ps2boot/ui/select_menu/pages/icons.o \
        ps2boot/ui/select_menu/pages/layout.o

LAUNCHER_OBJS = \
        ps2boot/ui/launcher/pages/main.o \
        ps2boot/ui/launcher/pages/browser.o \
        ps2boot/ui/launcher/pages/options.o \
        ps2boot/ui/launcher/pages/credits.o \
        ps2boot/ui/launcher/actions/main.o \
        ps2boot/ui/launcher/actions/browser.o \
        ps2boot/ui/launcher/actions/options.o \
        ps2boot/ui/launcher/browser/browser_state.o \
        ps2boot/ui/launcher/browser/open.o \
        ps2boot/ui/launcher/browser/nav.o \
        ps2boot/ui/launcher/font/browser_font.o \
        ps2boot/ui/launcher/background/background.o \
        ps2boot/ui/launcher/background/bg_ntsc_data.o \
        ps2boot/ui/launcher/background/bg_pal_data.o

VIDEO_EXTRA_OBJS = \
        ps2boot/video/video_ui.o \
        ps2boot/video/video_launcher_target.o

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
EE_CFLAGS += -O3 -G0 -DLAGFIX -DLOAD_FROM_MEMORY -DUSE_BLARGG_APU
EE_CFLAGS += -Ips2boot -Ips2boot/rom_loader -Ips2boot/rom_loader/vendor -Ips2boot/rom_loader/vendor/miniz -DMINIZ_NO_ARCHIVE_WRITING_APIS
EE_CFLAGS += -Ips2boot/app -Ips2boot/audio -Ips2boot/video -Ips2boot/input -Ips2boot/menu -Ips2boot/storage -Ips2boot/debug -Ips2boot/irx -Ips2boot/ui/font
EE_CFLAGS += -Ips2boot/ui/select_menu -Ips2boot/ui/select_menu/state -Ips2boot/ui/select_menu/render -Ips2boot/ui/select_menu/pages -Ips2boot/ui/select_menu/actions -Ips2boot/ui/select_menu/font

HOT_CORE_OBJS = \
	$(SRC_DIR)/apu_blargg.o \
	$(SRC_DIR)/ppu.o \
	$(SRC_DIR)/tile.o \
	$(SRC_DIR)/cpuops.o \
	$(SRC_DIR)/cpuexec.o

$(HOT_CORE_OBJS): EE_CFLAGS += -O3 -fomit-frame-pointer

HOT_FRONTEND_OBJS = \
	ps2boot/video/video_core.o \
	ps2boot/app/app_callbacks.o \
	ps2boot/app/app_overlay.o

$(HOT_FRONTEND_OBJS): EE_CFLAGS += -O3 -fomit-frame-pointer

EE_LIBS += -L$(PS2SDK)/ports/lib -lps2_drivers  -ldebug -lpacket -ldraw -lgraph -ldma -lpad -lmtap -lmc -lpatches -liopreboot -laudsrv -lcdvd

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

BUILD_STEP = $(SHELL) -ec 'kind="$$STEP_KIND"; tool="$$REAL_TOOL"; src=""; out=""; prev=""; \
for arg in "$$@"; do \
  if [ "$$prev" = "-o" ]; then out="$$arg"; prev=""; continue; fi; \
  case "$$arg" in \
    -o) prev="-o" ;; \
    *.c|*.cc|*.cpp|*.cxx|*.s|*.S) src="$$arg" ;; \
  esac; \
done; \
label="$$src"; \
[ -n "$$label" ] || label="$$out"; \
[ -n "$$label" ] || label="$$kind"; \
log=$$(mktemp); \
start_ns=$$(date +%s%N); \
if "$$tool" "$$@" > /dev/null 2>"$$log"; then rc=0; else rc=$$?; fi; \
end_ns=$$(date +%s%N); \
elapsed=$$(awk "BEGIN { printf \"%.2fs\", ($$end_ns-$$start_ns)/1000000000 }"); \
first=$$(sed -n "1{s/[[:space:]]\\+/ /g;s/^ //;s/ $$//;p;}" "$$log"); \
if [ $$rc -eq 0 ]; then \
  if [ -s "$$log" ]; then \
    printf "%s %s -> [ WARN - %s%s%s ]\n" "$$kind" "$$label" "$$elapsed" "$${first:+ - }" "$$first"; \
  else \
    printf "%s %s -> [ OK - %s ]\n" "$$kind" "$$label" "$$elapsed"; \
  fi; \
else \
  printf "%s %s -> [ ERROR - %s%s%s ]\n" "$$kind" "$$label" "$$elapsed" "$${first:+ - }" "$$first" >&2; \
  cat "$$log" >&2; \
  rm -f "$$log"; \
  exit $$rc; \
fi; \
rm -f "$$log"' _

EE_CC  = env STEP_KIND=CC  REAL_TOOL="$(REAL_EE_CC)"  $(BUILD_STEP)
EE_CXX = env STEP_KIND=CXX REAL_TOOL="$(REAL_EE_CXX)" $(BUILD_STEP)
EE_AS  = env STEP_KIND=AS  REAL_TOOL="$(REAL_EE_AS)"  $(BUILD_STEP)

.SILENT:
endif

.DEFAULT_GOAL := $(EE_BIN)

# --------------------------------------------------------------
# User-facing make interface.
#
# Preferred (lowercase) form:
#   make                      # compila
#   make v                    # compila com saida detalhada
#   make clean                # remove ELF e .o/.d
#   make rebuild              # clean + build
#   make push  out=<pasta>    # copia o ELF para <pasta>
#   make iso  [roms=<pasta>] [out=<pasta>]
#                             # gera ISO; copia para <pasta> se 'out=' dado
#
# Aliases legados (push-android, push-win, iso-push, WIN_OUT_DIR=,
# ISO_ROMS_DIR=) continuam funcionando para nao quebrar scripts
# existentes mas nao aparecem mais em 'make help'.
# --------------------------------------------------------------

.PHONY: help v push iso iso-push clean rebuild push-android push-win

# Lowercase user knobs (the new public interface). Use 'override'-friendly
# names so they don't conflict with internal vars.
out  ?=
roms ?=

# Map lowercase 'roms=' onto the existing ISO_ROMS_DIR plumbing.
ifneq ($(strip $(roms)),)
override ISO_ROMS_DIR := $(roms)
endif

# Resolve the push destination once.
#   1. 'out=<pasta>'         (preferred)
#   2. WIN_OUT_DIR (legacy)
#   3. ANDROID_OUT_DIR fallback only for the legacy push-android target
PUSH_DEST = $(strip $(if $(strip $(out)),$(out),$(WIN_OUT_DIR)))

help:
	@echo "InfinityStation"
	@echo
	@printf "  %-40s %s\n" "make"                                   "Compila o projeto"
	@printf "  %-40s %s\n" "make v"                                 "Compila com saida detalhada"
	@printf "  %-40s %s\n" "make clean"                             "Remove arquivos gerados"
	@printf "  %-40s %s\n" "make rebuild"                           "Limpa e recompila"
	@printf "  %-40s %s\n" "make push out=<pasta>"                  "Copia o ELF para <pasta>"
	@printf "  %-40s %s\n" "make iso [roms=<pasta>] [out=<pasta>]"  "Gera ISO; se 'out=' dado, copia tambem"
	@printf "  %-40s %s\n" "make help"                              "Mostra esta ajuda"
	@echo
	@printf "  %-40s %s\n" "EE_BIN"   "$(EE_BIN)"
	@printf "  %-40s %s\n" "ISO_OUT"  "$(ISO_OUT)"

# 'make v' just re-invokes make with V=1 set so the underlying recipe
# echoes the full compiler invocation. Equivalent to 'make V=1'.
v:
	@$(MAKE) V=1 $(EE_BIN)

push: $(EE_BIN)
	@if [ -z "$(PUSH_DEST)" ]; then \
		echo "ERRO: passe 'out=<pasta>'. Exemplos:"; \
		echo '  make push out=/sdcard/ps2'; \
		echo '  make push out="/mnt/c/Users/SeuNome/Desktop/ps2"'; \
		exit 1; \
	fi
	@mkdir -p "$(PUSH_DEST)"
	@cp -fv "$(EE_BIN)" "$(PUSH_DEST)/"

# Legacy aliases. Kept working but no longer shown by 'make help'.
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
	@find ps2boot $(SRC_DIR) $(COMM_DIR) -type f \
	    \( -name '*.o' -o -name '*.d' \) -delete 2>/dev/null || true
	@rm -f "$(CORE_DIR)"/libretro.o "$(CORE_DIR)"/libretro.d
	@echo "[clean] ok"

rebuild: clean
	@$(MAKE) V=$(V) $(EE_BIN)


# ---- ISO helpers ----
ISO_ROOT_DIR ?= dist/iso_root
ISO_OUT ?= dist/InfinityStation-roms-test.iso
ISO_LABEL ?= INFSTATION
ISO_BOOT ?= SNESBOOT.ELF
ISO_VMODE ?= NTSC
ISO_ROMS_DIR ?=

.PHONY: iso-check iso-root iso iso-push

iso-check:
	@command -v xorriso >/dev/null 2>&1 || { \
		echo "ERRO: xorriso nao encontrado."; \
		echo "Instale com: apt install xorriso"; \
		exit 1; \
	}

iso-root: $(EE_BIN) iso-check
	@rm -rf "$(ISO_ROOT_DIR)"
	@mkdir -p "$(ISO_ROOT_DIR)/ROMS"
	@cp "$(EE_BIN)" "$(ISO_ROOT_DIR)/$(ISO_BOOT)"
	@printf '%s\n' \
		"BOOT2 = cdrom0:\\$(ISO_BOOT);1" \
		"VER = 1.00" \
		"VMODE = $(ISO_VMODE)" > "$(ISO_ROOT_DIR)/SYSTEM.CNF"
	@if [ -n "$(ISO_ROMS_DIR)" ]; then \
		if [ ! -d "$(ISO_ROMS_DIR)" ]; then \
			echo "ERRO: ISO_ROMS_DIR nao existe: $(ISO_ROMS_DIR)"; \
			exit 1; \
		fi; \
		find "$(ISO_ROMS_DIR)" -maxdepth 1 -type f \
			\( -iname '*.smc' -o -iname '*.sfc' -o -iname '*.swc' -o -iname '*.fig' -o -iname '*.zip' \) \
			-exec cp -f {} "$(ISO_ROOT_DIR)/ROMS/" \; ; \
	else \
		echo "[iso-root] ISO_ROMS_DIR vazio; ISO sera criada sem ROMs"; \
	fi

iso: iso-root
	@mkdir -p "$$(dirname "$(ISO_OUT)")"
	@xorriso -as mkisofs \
		-V "$(ISO_LABEL)" \
		-sysid PLAYSTATION \
		-iso-level 2 \
		-full-iso9660-filenames \
		-o "$(ISO_OUT)" \
		"$(ISO_ROOT_DIR)"
	@echo "[iso] $(ISO_OUT)"
	@if [ -n "$(strip $(out))" ]; then \
		mkdir -p "$(out)"; \
		cp -f "$(ISO_OUT)" "$(out)/"; \
		echo "[iso] copiada para $(out)/"; \
	fi

# Legacy alias. 'make iso-push' still works for existing scripts; if
# 'out=' is not provided, falls back to ANDROID_OUT_DIR like before.
iso-push:
	@$(MAKE) iso out="$(if $(strip $(out)),$(out),$(ANDROID_OUT_DIR))"

