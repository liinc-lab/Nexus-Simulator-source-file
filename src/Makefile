#
# src/Makefile
#

OUTPUT			=	nexus

#PGN_FUNCTIONS		=	pgn_functions
PGN_FUNCTIONS		=	shape_functions

METHOD_FLAGS		=	-DNEXUS_BP \
				-DNEXUS_HEBB \
				-DNEXUS_INVERSION \
				-DNEXUS_RBF

INCLUDE_DIR             =       ../tcltk-8.3.3-69-headers/generic

NEXUS_LIBRARY		=	/usr/local/lib/nexus

PGN_SRC			=	$(PGN_FUNCTIONS).c
PGN_OBJ			=	$(PGN_FUNCTIONS).o

SOURCES			=	batch.c connect.c extern_functions.c \
				functions.c graphics.c input_output.c \
				lex_build.c main.c memory.c run.c \
				\
				nexus_opengl.c font.c tkGLx.c \
				tcl_main.c tcl_modules.c tcl_variables.c \
				\
				bp.c hebb.c inversion.c rbf.c

OBJECTS			=	$(SOURCES:.c=.o) $(PGN_OBJ)

CFLAGS_DEBUG		=	-O
#CFLAGS_DEBUG		=	-g -Wall

CFLAGS			=	-DHAVE_UNISTD_H=1 \
				-DNEXUS_LIBRARY=\"$(NEXUS_LIBRARY)\" \
				-DNEXUS_LINUX \
                                -I. -I$(INCLUDE_DIR) \
				$(CFLAGS_DEBUG) \
				$(METHOD_FLAGS)

LIBS			=	-ltix -ltk -ltcl \
				-lGL -lm -ll



LDFLAGS			=


all:				$(OUTPUT)

$(OUTPUT):			$(OBJECTS)
	rm -f $@
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(PGN_OBJ):			$(PGN_SRC)
	$(CC) $(CFLAGS) -o $(PGN_OBJ) -c $(PGN_SRC)

lex_build.c:  lex_build.l
	lex -n -t lex_build.l > lex_build.c

clean:
	-rm -f $(OUTPUT)
	-rm -f $(OBJECTS)
	-rm -f lex_build.c
	-rm -f *~
