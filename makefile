# makefile for GNU make (automatically generated by makemake 08-Kwi-12, 13:25:47)
# NOTE: indent with TAB in GNU makefiles!
# PROJECT #
# paths are relative to the project directory (current directory during make)

COMPILE_FILE=printf "\033[K\033[0;33mCompiling \033[1;33m$<\033[0;33m...\033[0m\n"
TARGET_DONE=printf "\033[K\033[0;32mTarget \"$@\" successfully done.\033[0m\n"
LINKING=printf "\033[K\033[1;34mLinking project \"$@\"... \033[0m\n"
NOTMORPHOS=printf "\033[K\033[1;91mTarget \"$@\" is not supported in non-MorphOS enviroment. \033[0m\n"

OS = $(shell uname -s)

NIL = /dev/null 2>&1
ifeq ($(OS),MorphOS)
	NIL = NIL:
endif

OUTFILE = kwakwa

OUTDIR  = bin/
OBJDIR  = o/
DEPSDIR = deps/

PROJECT = $(OUTDIR)$(OUTFILE)

# COMPILER #
CC = ppc-morphos-gcc-11
CWARNS = -Wall -Wno-pointer-sign
CDEFS  = -D__AMIGADATE__=\"\($(shell date "+%d.%m.%Y")\)\" -DAROS_ALMOST_COMPATIBLE -D__MORPHOS_SHAREDLIBS -D__GITHASH__=\"$(shell git log -1 --format=%H)\"
CFLAGS = -O3 -noixemul
CLIBS  = -I$(DEPSDIR)libnsgif -I$(DEPSDIR)ftp -I$(DEPSDIR)magicbeacon

COMPILE = $(CC) $(TARGET) $(CWARNS) $(CDEFS) $(CFLAGS) $(CLIBS)

# LINKER #
LD = ppc-morphos-gcc-11

LWARNS =
LDEFS  =
LFLAGS = -noixemul -L$(DEPSDIR)libnsgif
LIBS   = -lvstring -lnsgif

LINK   = $(LD) $(TARGET) $(LWARNS) $(LDEFS) $(LFLAGS)

# flex
FLEX = flex

# target 'all' (default target)
all: $(DEPSDIR)libnsgif/libnsgif.a $(PROJECT)

# dependecies
$(DEPSDIR)libnsgif/libnsgif.a:
	@make -C deps/libnsgif
	@$(TARGET_DONE)

# target 'compiler' (compile target)
$(OBJDIR)title_class.c.o: title_class.c globaldefines.h title_class.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)title_class.c.o title_class.c

$(OBJDIR)application.c.o: application.c globaldefines.h application.h mainwindow.h prefswindow.h contactslist.h support.h talkwindow.h \
 descwindow.h editconwindow.h smallsbar.h ipc.h locale.h translations.h modules.h modulescycle.h kwakwa_api/defs.h \
 kwakwa_api/protocol.h kwakwa_api/defs.h moduleslogwindow.h slaveprocess.h historysql.h historywindow.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)application.c.o application.c

$(OBJDIR)contactslist.c.o: contactslist.c globaldefines.h locale.h translations.h application.h prefswindow.h contactslist.h support.h \
 lexer.h talkwindow.h editconwindow.h kwakwa_api/defs.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)contactslist.c.o contactslist.c

$(OBJDIR)virtualtext.c.o: virtualtext.c globaldefines.h locale.h translations.h virtualtext.h prefswindow.h application.h talktab.h emoticon.h lexer.h \
 support.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)virtualtext.c.o virtualtext.c

$(OBJDIR)talktab.c.o: talktab.c globaldefines.h locale.h translations.h virtualtext.h prefswindow.h contactslist.h support.h \
 application.h inputfield.h logs.h contactinfoblock.h talkwindow.h talktab.h kwakwa_api/defs.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)talktab.c.o talktab.c

$(OBJDIR)inputfield.c.o: inputfield.c globaldefines.h support.h inputfield.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)inputfield.c.o inputfield.c

$(OBJDIR)locale.c.o: locale.c locale.h translations.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)locale.c.o locale.c

$(OBJDIR)main.c.o: main.c locale.h translations.h globaldefines.h application.h contactslist.h support.h mainwindow.h descwindow.h \
 prefswindow.h smallsbar.h talkwindow.h editconwindow.h talktab.h virtualtext.h title_class.h inputfield.h contactinfoblock.h \
 percentageslider.h simplestringlist.h modulescycle.h emoticon.h modulesmsglist.h moduleslogwindow.h slaveprocess.h historywindow.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)main.c.o main.c

$(OBJDIR)mainwindow.c.o: mainwindow.c globaldefines.h locale.h translations.h application.h contactslist.h support.h editconwindow.h \
 virtualtext.h mainwindow.h descwindow.h kwakwa_api/defs.h prefswindow.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)mainwindow.c.o mainwindow.c

$(OBJDIR)prefswindow.c.o: prefswindow.c globaldefines.h application.h contactslist.h support.h locale.h translations.h percentageslider.h \
 prefswindow.h modules.h kwakwa_api/protocol.h kwakwa_api/defs.h emoticonstab.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)prefswindow.c.o prefswindow.c

$(OBJDIR)lexer.c.o: lexer.c lexer.h
	@$(COMPILE_FILE)
	@$(COMPILE) -Wno-unused-function -c -o $(OBJDIR)lexer.c.o lexer.c

$(OBJDIR)editconwindow.c.o: editconwindow.c globaldefines.h locale.h translations.h contactslist.h support.h application.h prefswindow.h \
 talkwindow.h logs.h modulescycle.h editconwindow.h kwakwa_api/defs.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)editconwindow.c.o editconwindow.c

$(OBJDIR)smallsbar.c.o: smallsbar.c globaldefines.h smallsbar.h application.h prefswindow.h mainwindow.h talkwindow.h support.h locale.h \
 translations.h kwakwa_api/defs.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)smallsbar.c.o smallsbar.c

$(OBJDIR)support.c.o: support.c support.h globaldefines.h translations.h kwakwa_api/defs.h kwakwa_api/pictures.h locale.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)support.c.o support.c

$(OBJDIR)talkwindow.c.o: talkwindow.c locale.h translations.h globaldefines.h application.h contactslist.h support.h title_class.h \
 prefswindow.h talktab.h talkwindow.h smallsbar.h kwakwa_api/defs.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)talkwindow.c.o talkwindow.c

$(OBJDIR)descwindow.c.o: descwindow.c globaldefines.h locale.h translations.h support.h application.h kwakwa_api/defs.h inputfield.h \
 simplestringlist.h descwindow.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)descwindow.c.o descwindow.c

$(OBJDIR)ipc.c.o: ipc.c kwakwa_api/defs.h globaldefines.h application.h ipc.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)ipc.c.o ipc.c

$(OBJDIR)logs.c.o: logs.c globaldefines.h support.h locale.h translations.h logs.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)logs.c.o logs.c

$(OBJDIR)contactinfoblock.c.o: contactinfoblock.c globaldefines.h talkwindow.h contactslist.h support.h contactinfoblock.h kwakwa_api/defs.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)contactinfoblock.c.o contactinfoblock.c

$(OBJDIR)percentageslider.c.o: percentageslider.c percentageslider.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)percentageslider.c.o percentageslider.c

$(OBJDIR)simplestringlist.c.o: simplestringlist.c globaldefines.h simplestringlist.h kwakwa_api/defs.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)simplestringlist.c.o simplestringlist.c

$(OBJDIR)modulescycle.c.o: modulescycle.c globaldefines.h modules.h support.h modulescycle.h kwakwa_api/defs.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)modulescycle.c.o modulescycle.c

$(OBJDIR)emoticon.c.o:  emoticon.c $(DEPSDIR)libnsgif/libnsgif.h globaldefines.h support.h emoticon.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)emoticon.c.o emoticon.c

$(OBJDIR)modules.c.o: modules.c kwakwa_api/protocol.h kwakwa_api/defs.h globaldefines.h support.h modules.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)modules.c.o modules.c

$(OBJDIR)modulesmsglist.c.o: modulesmsglist.c globaldefines.h modulesmsglist.h kwakwa_api/protocol.h modules.h translations.h locale.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)modulesmsglist.c.o modulesmsglist.c

$(OBJDIR)moduleslogwindow.c.o: moduleslogwindow.c globaldefines.h locale.h translations.h modulesmsglist.h moduleslogwindow.h translations.h locale.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)moduleslogwindow.c.o moduleslogwindow.c

$(OBJDIR)slaveprocess.c.o: slaveprocess.c slaveprocess.h slave.h globaldefines.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)slaveprocess.c.o slaveprocess.c

$(OBJDIR)slave.c.o: slave.c slave.h globaldefines.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)slave.c.o slave.c

$(OBJDIR)http.c.o: http.c http.h globaldefines.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)http.c.o http.c

$(OBJDIR)timeslider.c.o: timeslider.c timeslider.h globaldefines.h locale.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)timeslider.c.o timeslider.c

$(OBJDIR)pictureview.c.o: pictureview.c globaldefines.h support.h pictureview.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)pictureview.c.o pictureview.c

$(OBJDIR)minmaxslider.c.o: minmaxslider.c minmaxslider.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)minmaxslider.c.o minmaxslider.c

$(OBJDIR)fileview.c.o: fileview.c fileview.h globaldefines.h support.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)fileview.c.o fileview.c

$(OBJDIR)parseftpurl.c.o: parseftpurl.c globaldefines.h parseftpurl.h support.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)parseftpurl.c.o parseftpurl.c

$(OBJDIR)ftp.c.o: ftp.c ftp.h globaldefines.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)ftp.c.o ftp.c

$(OBJDIR)emoticonstab.c.o: emoticonstab.c emoticonstab.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)emoticonstab.c.o emoticonstab.c

$(OBJDIR)historywindow.c.o: historywindow.c historywindow.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)historywindow.c.o historywindow.c

$(OBJDIR)tabtitle.c.o: tabtitle.c tabtitle.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)tabtitle.c.o tabtitle.c

$(OBJDIR)historycontactslist.c.o: historycontactslist.c historycontactslist.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)historycontactslist.c.o historycontactslist.c

$(OBJDIR)historyconversationslist.c.o: historyconversationslist.c historyconversationslist.h
	@$(COMPILE_FILE)
	@$(COMPILE) -c -o $(OBJDIR)historyconversationslist.c.o historyconversationslist.c

OBJS = $(OBJDIR)application.c.o $(OBJDIR)contactslist.c.o $(OBJDIR)locale.c.o $(OBJDIR)main.c.o $(OBJDIR)mainwindow.c.o $(OBJDIR)descwindow.c.o \
 $(OBJDIR)prefswindow.c.o $(OBJDIR)smallsbar.c.o $(OBJDIR)support.c.o $(OBJDIR)talktab.c.o $(OBJDIR)talkwindow.c.o $(OBJDIR)editconwindow.c.o \
 $(OBJDIR)virtualtext.c.o $(OBJDIR)lexer.c.o $(OBJDIR)title_class.c.o $(OBJDIR)ipc.c.o $(OBJDIR)logs.c.o $(OBJDIR)inputfield.c.o \
 $(OBJDIR)contactinfoblock.c.o $(OBJDIR)percentageslider.c.o $(OBJDIR)simplestringlist.c.o $(OBJDIR)emoticon.c.o \
 $(OBJDIR)modules.c.o $(OBJDIR)modulescycle.c.o $(OBJDIR)modulesmsglist.c.o $(OBJDIR)moduleslogwindow.c.o $(OBJDIR)slaveprocess.c.o \
 $(OBJDIR)slave.c.o $(OBJDIR)http.c.o $(OBJDIR)timeslider.c.o $(OBJDIR)pictureview.c.o $(OBJDIR)minmaxslider.c.o $(OBJDIR)fileview.c.o \
 $(OBJDIR)parseftpurl.c.o $(OBJDIR)ftp.c.o $(OBJDIR)emoticonstab.c.o $(OBJDIR)historywindow.c.o $(OBJDIR)tabtitle.c.o $(OBJDIR)historycontactslist.c.o \
 $(OBJDIR)historyconversationslist.c.o

# link all file(s)
$(PROJECT): $(OBJS)
	@$(LINKING)
	@$(LINK) $(OBJS) -o $(PROJECT) $(LIBS)
	@$(TARGET_DONE)

# any other targets
.PHONY: install

lexer.c: contrib/lexer.l
	@$(COMPILE_FILE)
	@$(FLEX) -i -o$@ $<

strip:
	@strip --strip-unneeded --remove-section=.comment $(PROJECT)
	@$(TARGET_DONE)

clean:
ifeq ($(OS),MorphOS)
	@-rm -rf translations.h $(OUTDIR)catalogs
endif
	@-make clean -C $(DEPSDIR)libnsgif >$(NIL)
	@-rm $(PROJECT) >$(NIL)
	@-rm $(OBJDIR)*.c.o >$(NIL)
	@$(TARGET_DONE)

translations.h: locale/kwakwa.cs
ifeq ($(OS),MorphOS)
	MakeDir ALL $(OUTDIR)catalogs/polski
	SimpleCat locale/kwakwa.cs
else
	@$(NOTMORPHOS)
endif

dist: all
ifeq ($(OS),MorphOS)
# delete old archive and directory
	@rm -rf RAM:$(OUTFILE) RAM:$(OUTFILE).lha
# make directory for new one
	@mkdir RAM:$(OUTFILE)
# copy all files from bin/
	@copy >NIL: $(OUTDIR) RAM:$(OUTFILE) ALL
# delete private stuff
	-@rm -rf RAM:$(OUTFILE)/listdump.log
	-@rm -rf RAM:$(OUTFILE)/logs/*
	-@rm -rf RAM:$(OUTFILE)/cache/*
	-@rm -rf RAM:$(OUTFILE)/gfx/emots/*
	-@find RAM:$(OUTFILE) -name *.module* -printf "\"%p\"\n" | xargs rm -rf
# strip executable
	@strip --strip-unneeded --remove-section .comment RAM:$(OUTFILE)/$(OUTFILE) >NIL:
# create cache directories
	-@mkdir RAM:$(OUTFILE)/cache/gui
# create example desclist.cfg
	@echo "KwaKwa rulez!!!\n" >"RAM:$(OUTFILE)/cache/gui/desclist.cfg"
# copy docs
	@copy >NIL: LICENSE doc/kwakwa_eng.guide doc/kwakwa_eng.guide.info doc/kwakwa_pl.guide doc/kwakwa_pl.guide.info doc/kwakwa.readme RAM:$(OUTFILE)/
# make dir for scripts and copy them
	-@mkdir RAM:$(OUTFILE)/scripts
	@copy >NIL: contrib/scripts RAM:$(OUTFILE)/scripts ALL
# delete vcs stuff
	@find RAM:$(OUTFILE) \( -name .svn -o -name .git \) -printf "\"%p\"\n" | xargs rm -rf
#copy default drawer icon
	-@cp SYS:Prefs/Presets/Deficons/def_drawer.info RAM:$(OUTFILE).info
# create archive
	@MOSSYS:C/LHa a -r -a -e RAM:$(OUTFILE).lha RAM:$(OUTFILE) RAM:$(OUTFILE).info >NIL:
# be happy ;-)
	@echo "Build dist package in <RAM:$(OUTFILE).lha> is done."
	@$(TARGET_DONE)
else
	@$(NOTMORPHOS)
endif

dump:
ifeq ($(OS),MorphOS)
	@rm -rf RAM:$(OUTFILE).dump
	@objdump -dC $(OUTDIR)/$(OUTFILE) >RAM:$(OUTFILE).dump
	@$(TARGET_DONE)
else
	@$(NOTMORPHOS)
endif

install:
ifeq ($(OS),MorphOS)
	@copy bin/kwakwa SYS:Applications/KwaKwa/ >NIL:
	@copy ALL bin/catalogs/ SYS:Applications/KwaKwa/catalogs/ >NIL:
	@$(TARGET_DONE)
else
	@$(NOTMORPHOS)
endif

RELEASE: all
	@$(TARGET_DONE)

DEBUG: COMPILE += -D__DEBUG__ -D__DEBUG_SQL_
DEBUG: LIBS += -ldebug
DEBUG: all
	@$(TARGET_DONE)
