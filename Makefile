include .env
export

ifndef VERSION
  VERSION = 0.1.0
endif

ifndef BUILDDIR
  BUILDDIR = ./bin
endif

ifndef SRCDIR
  SRCDIR = ./src
endif

ifndef BINNAME
  BINNAME = gb2gc
endif

all: clean gb2gc

.PHONY: gb2gc
gb2gc: configure checkbuilddir ${BUILDDIR}/main.o ${BUILDDIR}/geom.o ${BUILDDIR}/context.o ${BUILDDIR}/reflist.o ${BUILDDIR}/point.o ${BUILDDIR}/line.o ${BUILDDIR}/arc.o ${BUILDDIR}/cont.o ${BUILDDIR}/milling.o ${BUILDDIR}/mergcont.o ${BUILDDIR}/gerber.o ${BUILDDIR}/excellon.o ${BUILDDIR}/gcode.o ${BUILDDIR}/export.o ${BUILDDIR}/conf.o ${BUILDDIR}/apply_config.o
	gcc  -o ${BUILDDIR}/${BINNAME} ${BUILDDIR}/main.o ${BUILDDIR}/geom.o ${BUILDDIR}/context.o ${BUILDDIR}/reflist.o ${BUILDDIR}/point.o ${BUILDDIR}/line.o ${BUILDDIR}/arc.o ${BUILDDIR}/cont.o ${BUILDDIR}/milling.o ${BUILDDIR}/mergcont.o ${BUILDDIR}/gerber.o ${BUILDDIR}/excellon.o ${BUILDDIR}/gcode.o ${BUILDDIR}/export.o ${BUILDDIR}/conf.o ${BUILDDIR}/apply_config.o -lm

${BUILDDIR}/main.o: checkbuilddir src/main.c
	gcc -o ${BUILDDIR}/main.o -c src/main.c

${BUILDDIR}/geom.o: checkbuilddir src/geom.c
	gcc -o ${BUILDDIR}/geom.o -c src/geom.c

${BUILDDIR}/context.o: checkbuilddir src/context.c
	gcc -o ${BUILDDIR}/context.o -c src/context.c

${BUILDDIR}/reflist.o: checkbuilddir src/reflist.c
	gcc -o ${BUILDDIR}/reflist.o -c src/reflist.c

${BUILDDIR}/point.o: checkbuilddir src/point.c
	gcc -o ${BUILDDIR}/point.o -c src/point.c

${BUILDDIR}/line.o: checkbuilddir src/line.c
	gcc -o ${BUILDDIR}/line.o -c src/line.c

${BUILDDIR}/arc.o: checkbuilddir src/arc.c
	gcc -o ${BUILDDIR}/arc.o -c src/arc.c

${BUILDDIR}/cont.o: checkbuilddir src/cont.c
	gcc -o ${BUILDDIR}/cont.o -c src/cont.c

${BUILDDIR}/milling.o: checkbuilddir src/milling.c
	gcc -o ${BUILDDIR}/milling.o -c src/milling.c

${BUILDDIR}/mergcont.o: checkbuilddir src/mergcont.c
	gcc -o ${BUILDDIR}/mergcont.o -c src/mergcont.c

${BUILDDIR}/gerber.o: checkbuilddir src/gerber.c
	gcc -o ${BUILDDIR}/gerber.o -c src/gerber.c

${BUILDDIR}/excellon.o: checkbuilddir src/excellon.c
	gcc -o ${BUILDDIR}/excellon.o -c src/excellon.c

${BUILDDIR}/gcode.o: checkbuilddir src/gcode.c
	gcc -o ${BUILDDIR}/gcode.o -c src/gcode.c

${BUILDDIR}/export.o: checkbuilddir src/export.c
	gcc -o ${BUILDDIR}/export.o -c src/export.c

${BUILDDIR}/conf.o: checkbuilddir src/conf.c
	gcc -o ${BUILDDIR}/conf.o -c src/conf.c

${BUILDDIR}/apply_config.o: checkbuilddir src/apply_config.c
	gcc -o ${BUILDDIR}/apply_config.o -c src/apply_config.c

.PHONY: configure
configure: clean
	$(shell echo "#define WORKDIR \"${WORKDIR}\"" > ./include/define.h)
	$(shell echo "#define VERSION \"${VERSION}\"" >> ./include/define.h)
	$(shell echo "#define TOOL_D ${TOOL_D}" >> ./include/define.h)
	$(shell echo "#define TOOL_CH_HEIGHT ${TOOL_CH_HEIGHT}" >> ./include/define.h)
	$(shell echo "#define SVG_M ${SVG_M}" >> ./include/define.h)
	$(shell echo "#define SVG_WIDTH ${SVG_WIDTH}" >> ./include/define.h)
	$(shell echo "#define SVG_HEIGHT ${SVG_HEIGHT}" >> ./include/define.h)
	$(shell echo "#define SVG_MARGIN ${SVG_MARGIN}" >> ./include/define.h)
	$(shell echo "#define DRILL_SAFE ${DRILL_SAFE}" >> ./include/define.h)
	$(shell echo "#define DRILL_START ${DRILL_START}" >> ./include/define.h)
	$(shell echo "#define DRILL_DEEP ${DRILL_DEEP}" >> ./include/define.h)
	$(shell echo "#define DRILL_STEP ${DRILL_STEP}" >> ./include/define.h)
	$(shell echo "#define DRILL_FEED ${DRILL_FEED}" >> ./include/define.h)
	$(shell echo "#define MILLING_SAFE ${MILLING_SAFE}" >> ./include/define.h)
	$(shell echo "#define MILLING_START ${MILLING_START}" >> ./include/define.h)
	$(shell echo "#define MILLING_DEEP ${MILLING_DEEP}" >> ./include/define.h)
	$(shell echo "#define MILLING_STEP ${MILLING_STEP}" >> ./include/define.h)
	$(shell echo "#define MILLING_FEED ${MILLING_FEED}" >> ./include/define.h)
	$(shell echo "#define CONFIGPATH \"${CONFIGPATH}\"" >> ./include/define.h)

.PHONY: clean
clean:
	rm ${BUILDDIR}/main.o ${BUILDDIR}/geom.o ${BUILDDIR}/context.o ${BUILDDIR}/reflist.o ${BUILDDIR}/point.o ${BUILDDIR}/line.o ${BUILDDIR}/arc.o ${BUILDDIR}/test ${BUILDDIR}/testmain.o ${BUILDDIR}/cont.o ${BUILDDIR}/milling.o ${BUILDDIR}/mergcont.o ${BUILDDIR}/gerber.o ${BUILDDIR}/excellon.o ${BUILDDIR}/gcode.o ${BUILDDIR}/export.o ${BUILDDIR}/conf.o ${BUILDDIR}/apply_config.o ${BUILDDIR}/${BINNAME} 2> /dev/null || true

.PHONY: checkbuilddir
checkbuilddir:
	-test -d ${BUILDDIR} || mkdir -p ${BUILDDIR}

.PHONY: checkworkdir
checkworkdir:
	-test -d ${WORKDIR} || mkdir -p ${WORKDIR}

.PHONY: pull
pull: clean
	git pull origin master

.PHONY: push
push: clean
	git commit -a
	git push origin master

.PHONY: install
install: preinstall
	install ${BUILDDIR}/${BINNAME} /usr/bin/${BINNAME}
	-test -f ${CONFIGPATH} || install -m 0644 -o ${USR} -g ${USR} ./templates/conf.example ${CONFIGPATH}
	-test -f ${WORKDIR}/convert.template || install -m 0644 -o ${USR} -g ${USR} ./templates/convert.template ${WORKDIR}/convert.template

.PHONY: preinstall
preinstall: checkworkdir
	@echo "-=INSTALL=-"

${BUILDDIR}/testmain.o: checkbuilddir tests/test.c
	gcc -o ${BUILDDIR}/testmain.o -c tests/test.c

.PHONY: test
test:     clean checkbuilddir ${BUILDDIR}/geom.o ${BUILDDIR}/context.o ${BUILDDIR}/testmain.o ${BUILDDIR}/reflist.o ${BUILDDIR}/point.o ${BUILDDIR}/line.o ${BUILDDIR}/arc.o ${BUILDDIR}/cont.o ${BUILDDIR}/milling.o ${BUILDDIR}/mergcont.o ${BUILDDIR}/gcode.o ${BUILDDIR}/export.o ${BUILDDIR}/conf.o ${BUILDDIR}/apply_config.o
	gcc -o   ${BUILDDIR}/test ${BUILDDIR}/geom.o ${BUILDDIR}/context.o ${BUILDDIR}/testmain.o ${BUILDDIR}/reflist.o ${BUILDDIR}/point.o ${BUILDDIR}/line.o ${BUILDDIR}/arc.o ${BUILDDIR}/cont.o ${BUILDDIR}/milling.o ${BUILDDIR}/mergcont.o ${BUILDDIR}/gcode.o ${BUILDDIR}/export.o ${BUILDDIR}/conf.o ${BUILDDIR}/apply_config.o -lm
	${BUILDDIR}/test

# --show-leak-kinds=all
.PHONY: check
check: test
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all -s ${BUILDDIR}/test

