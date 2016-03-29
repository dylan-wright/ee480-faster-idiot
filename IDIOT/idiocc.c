/*	idiocc.c

	Instruction Definition In Our C Compiler.

	Idiocc, pronounced "id-EE-OH-see-see," is a small C
	subset compiler created for the Spring EE480 idiot
	(Instruction Definition In Our Target) assembly language
	and processor implementations.  It's a 16-bit
	word-oriented machine, but includes non-IEEE-compliant
	16-bit floating point.  See
	http://aggregate.org/EE480/idiot.html for details.

	2016 by Hank Dietz, http://aggregate.org/hankd
*/

#define	VERSION	20160226

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

//	#define	LINETRACE	1
#define	HOISTADDR	1

int	haveinput = 0;

char	*prelab = "_";
int	callerreg, calleereg;
int	scope = 0;
int	labnum = 0;	/* next compiler-generated label */
int	lexsym;

int	beginlab, endlab;	/* for function begin/end code */
int	gpoffset, fpoffset;	/* for globals/locals */
int	isleaf;			/* is this a leaf procedure? */
int	linestart = 0;		/* start of current line */

#define	DATABASE	0x8000	/* where data starts */

#define	INT	'a'
#define	IF	'b'
#define	ELSE	'c'
#define	WHILE	'd'
#define	RETURN	'e'
#define	FUNC	'f'
#define	WORD	'g'
#define	NUM	'h'
#define	DO	'i'
#define	VAR	'j'
#define	AFP	'k'
#define	ASP	'l'
#define	SHORT	'n'
#define	CHAR	'o'
#define RETVAL	'p'
#define	MULBY4	'q'
#define	MULBY2	'r'
#define	STRING	's'
#define	FOR	't'
#define	GOTO	'u'
#define	TARGET	'v'

#define	EQ	'A'
#define	NE	'B'
#define	GE	'C'
#define	LE	'D'
#define	SL	'E'
#define	PP	'F'
#define	MM	'G'
#define	OE	'H'
#define	XE	'I'
#define	AE	'J'
#define	PE	'K'
#define	ME	'L'
#define	TE	'M'
#define	DE	'N'
#define	RE	'O'
#define	OO	'P'
#define	AA	'Q'
#define	NEG	'R'
#define	SR	'S'
#define	SUBR	'T'	/* Subtract reversed */
#define	MYEOF	'Z'

#define	MAXINPUT	(1024*1024)
char	input[MAXINPUT];
int	eof;
int	ipos;

char	*myname;	/* name of this command */

int	nextt;		/* next token */
int	lexnum;		/* lexical number value */
int	lexstr;		/* lexical string ipos */
int	lineno = 1;	/* current line number */

#define	STACKSIZE	64
int	objsize[STACKSIZE];
int	sp = 0;

int	highwater = 0;

#define	sym	struct _sym
sym {
	int	ipos;
	int	type;
	int	scope;
	int	base;
	int	size;
	int	dim;
} symtab[513];		/* symbol table */
int	symsp = 0;

void	expr(void);
void	decl(void);


int
isnamechar(register int t)
{
	return(((t >= '0') && (t <= '9')) ||
	       ((t >= 'A') && (t <= 'Z')) ||
	       ((t >= 'a') && (t <= 'z')) ||
	       (t == '_'));
}

char *
namestring(register int ipos)
{
	static char name[256];
	register int i = 0;

	while (isnamechar(name[i] = input[ipos+i])) ++i;
	name[i] = 0;
	return(&(name[0]));
}

void
warn(fmt, a, b, c)
char *fmt;
int a, b, c;
{
	fprintf(stderr, "#line %d: ", lineno);
	fprintf(stderr, fmt, a, b, c);
	fprintf(stderr, "\n");
}

void
error(fmt, a, b, c)
char *fmt;
int a, b, c;
{
	warn(fmt, a, b, c);
	fprintf(stderr,
"#compilation terminated on this error\n"
		);
	exit(1);
}


/*	Code generation stuff
*/

void
incsp(void)
{
	++sp;
}

void
decsp(void)
{
	--sp;
}

void
prchar(register int c)
{
	putchar(c);
}

void
pr(register char *s,
register int len)
{
	while (--len >= 0) {
		putchar(*s);
		++s;
	}
}

typedef enum {
	ZERO,	ONE,	SIGN,	ALL,	SP,	FP,	RA,	RV,
	U0,	U1,	U2,	U3,	U4,	U5,	U6,	U7,
	U8,	U9,	U10,	U11,	U12,	U13,	U14,	U15,
	U16,	U17,	U18,	U19,	U20,	U21,	U22,	U23,
	U24,	U25,	U26,	U27,	U28,	U29,	U30,	U31,
	U32,	U33,	U34,	U35,	U36,	U37,	U38,	U39,
	U40,	U41,	U42,	U43,	U44,	U45,	U46,	U47,
	U48,	U49,	U50,	U51,	U52,	U53,	U54,	U55
} reg_t;

#define	ARG(N)	(U55-(N))

#define	TOS	(U0 + (sp-1))
#define	NOS	(U0 + (sp-2))
#define	TMP	(U0 + sp)
#define	TMP2	(U0 + (sp+1))

char *regname[64] = {
	"zero",	"one",	"sign",	"all",	"sp",	"fp",	"ra",	"rv",
	"u0",	"u1",	"u2",	"u3",	"u4",	"u5",	"u6",	"u7",
	"u8",	"u9",	"u10",	"u11",	"u12",	"u13",	"u14",	"u15",
	"u16",	"u17",	"u18",	"u19",	"u20",	"u21",	"u22",	"u23",
	"u24",	"u25",	"u26",	"u27",	"u28",	"u29",	"u30",	"u31",
	"u32",	"u33",	"u34",	"u35",	"u36",	"u37",	"u38",	"u39",
	"u40",	"u41",	"u42",	"u43",	"u44",	"u45",	"u46",	"u47",
	"u48",	"u49",	"u50",	"u51",	"u52",	"u53",	"u54",	"u55"
};


#define	MKREGOP(OP) \
static inline void \
idiot_##OP(reg_t d, reg_t s) \
{ \
	printf("\t" #OP "\t$%s,$%s\n", regname[d&63], regname[s&63]); \
}

MKREGOP(add)
MKREGOP(addf)
MKREGOP(and)
MKREGOP(any)
MKREGOP(dup)
MKREGOP(jz)
MKREGOP(ld)
MKREGOP(mulf)
MKREGOP(or)
MKREGOP(shr)
MKREGOP(st)
MKREGOP(xor)

static inline void
idiot_sz(reg_t d)
{
	printf("\tsz\t$%s\n", regname[d&63]);
}

static inline void
idiot_sys(void)
{
	printf("\tsys\n");
}

static inline void
idiot_li(reg_t d, int i)
{
	i &= 0xffff;
	switch (i) {
	case 0:		idiot_dup(d, ZERO); break;
	case 1:		idiot_dup(d, ONE); break;
	case 0x8000:	idiot_dup(d, SIGN); break;
	case 0xffff:	idiot_dup(d, ALL); break;
	default:
		printf("\tli\t$%s,0x%04x\n", regname[d&63], i);
	}
}

static inline void
idiot_la(reg_t d, char *s)
{
	printf("\tli\t$%s,%s\n", regname[d&63], s);
}

static inline void
idiot_la_(reg_t d, int i)
{
	printf("\tli\t$%s,%s%d\n", regname[d&63], prelab, i);
}


void
idiot_space(register int n)
{
	printf("\t.space\t%d\n", n);
}

void
idiot_text(void)
{
	printf("\t.text\n");
}

void
idiot_data(int n)
{
	printf("\t.data\n\t.origin\t%d\n", n);
}

void
idiot_label(register char *s)
{
	printf("%s:\n", s);
}

void
idiot_label_(register int n)
{
	printf("_%d:\n", n);
}

void
label(register int a)
{
	idiot_label_(a);
}

void
idiot_prelabel(register char *s)
{
	printf("_%s:\n", s);
}

void
pushnum(register int n)
{
	incsp();
	idiot_li(TOS, n);
	objsize[sp-1] = 0;
}

void
pushgpoff(register int off)
{
	incsp();
	idiot_li(TOS, off);
	objsize[sp-1] = 0;
}

void
pushfpoff(register int off)
{
	incsp();
	idiot_li(TOS, off);
	idiot_add(TOS, FP);
	objsize[sp-1] = 0;
}

void
pushdup(void)
{
	incsp();
	idiot_dup(TOS, NOS);
	objsize[sp-1] = objsize[sp-2];
}

void
pusharg(register int argno)
{
	incsp();
	idiot_dup(TOS, ARG(argno));
	objsize[sp-1] = 0;
}

void
lval(register int size)
{
	objsize[sp-1] = size;
}

void
loadtos(void)
{
	/* Need TOS loaded from memory? */

	switch (objsize[sp-1]) {
	case 0:	return;
	case 1: idiot_ld(TOS, TOS); break;
	default:
		error("cannot load %d-byte object",
		      objsize[sp-1]);
	}

	objsize[sp-1] = 0;
}

void
loadnostos(void)
{
	/* Need either NOS or TOS loaded from memory? */

	decsp();
	loadtos();
	incsp();
	loadtos();
}

void
loadnos(void)
{
	/* Need NOS loaded from memory? */

	decsp();
	loadtos();
	incsp();
}

void
setarg(register int argno)
{
	loadtos();
	idiot_dup(ARG(argno), TOS);
	decsp();
}

void
pushop(register int op)
{
	int lab;

	switch (op) {
	case NEG:
		loadtos();
		idiot_xor(TOS, ALL);
		idiot_add(TOS, ONE);
		break;
	case '!':
		loadtos();
		idiot_any(TOS, TOS);
		idiot_xor(TOS, ONE);
		++labnum;
		break;
	case '~':
		loadtos();
		idiot_xor(TOS, ALL);
		break;
	case AFP:
		loadtos();
		idiot_add(TOS, FP);
		break;
	case ASP:
		loadtos();
		idiot_add(TOS, SP);
		break;
	case RETVAL:
		loadtos();
		idiot_dup(RV, TOS);
		decsp();
		break;
	case '+':
		loadnostos();
		idiot_add(NOS, TOS);
		decsp();
		break;
	case '-':
		/* NOS += (~TOS) + 1; NOS -= TOS */
		loadnostos();
		idiot_xor(TOS, ALL);
		idiot_add(TOS, ONE);
		idiot_add(NOS, TOS);
		decsp();
		break;
	case SUBR:
		/* NOS = (~NOS) + 1 + TOS = TOS - NOS */
		loadnostos();
		idiot_xor(NOS, ALL);
		idiot_add(NOS, ONE);
		idiot_add(NOS, TOS);
		decsp();
		break;
	case SL:
		/* Do this as a loop */
		loadnostos();
		idiot_la_(TMP, labnum);
		idiot_la_(TMP2, labnum+1);
		idiot_jz(ZERO, TMP2);
		label(labnum);
		idiot_add(NOS, NOS);
		idiot_add(TOS, ALL);
		label(labnum+1);
		idiot_any(TMP2, TOS);
		idiot_xor(TMP2, ONE);
		idiot_jz(TMP2, TMP2);
		decsp();
		labnum += 2;
		break;
	case SR:
		/* Do this as a loop */
		loadnostos();
		idiot_la_(TMP, labnum);
		idiot_la_(TMP2, labnum+1);
		idiot_jz(ZERO, TMP2);
		label(labnum);
		idiot_shr(NOS, NOS);
		idiot_add(TOS, ALL);
		label(labnum+1);
		idiot_any(TMP2, TOS);
		idiot_xor(TMP2, ONE);
		idiot_jz(TMP2, TMP2);
		decsp();
		labnum += 2;
		break;
	case LE:
		/* recursive using '>' */
		pushop('>');
		idiot_xor(TOS, ONE);
		break;
	case '<':
		/* any(sign(TOS-NOS)) */
		pushop('-');
		idiot_and(TOS, SIGN);
		idiot_any(TOS, TOS);
		break;
	case GE:
		/* recursive using '<' */
		pushop('<');
		idiot_xor(TOS, ONE);
		break;
	case '>':
		/* any(sign(NOS-TOS)) */
		pushop(SUBR);
		idiot_and(TOS, SIGN);
		idiot_any(TOS, TOS);
		break;
	case EQ:
		pushop(NE);
		idiot_xor(TOS, ONE);
		break;
	case NE:
		loadnostos();
		idiot_xor(NOS, TOS);
		idiot_any(NOS, NOS);
		decsp();
		break;
	case '&':
		loadnostos();
		idiot_and(NOS, TOS);
		decsp();
		break;
	case '^':
		loadnostos();
		idiot_xor(NOS, TOS);
		decsp();
		break;
	case '|':
		loadnostos();
		idiot_or(NOS, TOS);
		decsp();
		break;
	default:
		error("cannot yet handle op (%c)", op);
	}
}

void
store(register int prop)
{
	/* Store TOS into NOS */

	loadtos();
	switch (objsize[sp-2]) {
	case 0: error("lvalue required");
	case 1: idiot_st(TOS, NOS); break;
	default:
		error("cannot store %d-byte object",
		      objsize[sp-2]);
	}

	if (prop) {
		/* Just in case we have x=(y=z)...
		   Strictly speaking, x=(y=z) sets x=((typeof(y))z),
		   but I don't think we have to be that precise here
		*/
		idiot_dup(NOS, TOS);
		decsp();
		objsize[sp-1] = 0;
	} else {
		decsp();
		decsp();
	}
}

void
jumpreg(register int r)
{
	idiot_jz(ZERO, r);
}

void
jump(register int a)
{
	/* Compiler-generated labels are nearby...
	   so we could use a branch (if we had one)
	*/

	idiot_la_(TMP, a);
	jumpreg(TMP);
}

void
jumpfreg(register int r)
{
	loadtos();
	idiot_jz(TOS, r);
	decsp();
}

void
jumpf(register int a)
{
	idiot_la_(TMP, a);
	jumpfreg(TMP);
}

void
jumptreg(register int r)
{
	loadtos();
	idiot_any(TOS, TOS);
	idiot_xor(TOS, ONE);
	idiot_jz(TOS, r);
	decsp();
}

void
jumpt(register int a)
{
	idiot_la_(TMP, a);
	jumptreg(TMP);
}


void
ghoto(register char *s)
{
	idiot_la(TMP, s);
	idiot_jz(ZERO, TMP);
}

void
target(register char *s)
{
	idiot_prelabel(s);
}

void
startup(void)
{
	idiot_text();
    //original
	//idiot_la(TMP, "main");
	idiot_la(TMP, "_main");
	idiot_jz(ZERO, TMP);
	idiot_label("_exit");
	idiot_sys();
}

void
call(register int mysym)
{
	register char *n = namestring(symtab[mysym].ipos);
	register int i;

	/* We're not a leaf procedure.... */
	isleaf = 0;

	/* Save registers */
	if (sp > 0) {
		for (i=U0; i<=TOS; ++i) {
			idiot_st(i, SP);
			idiot_add(SP, ALL);
		}
	}

	/* Call the function */
	idiot_la(TMP, n);
	idiot_jz(ZERO, TMP);

	/* Restore registers */
	if (sp > 0) {
		for (i=TOS; i>=U0; --i) {
			idiot_add(SP, ONE);
			idiot_st(i, SP);
		}
	}

	/* Copy the return value to someplace useful */
	incsp();
	idiot_dup(TOS, RV);
	objsize[sp-1] = 0;
}


void
funcbegin(register int mysym)
{
	register char *n = namestring(symtab[mysym].ipos);

	fpoffset = -4;
	highwater = 0;
	beginlab = labnum++;
	endlab = labnum++;

	/* For now, functions always return an int */
	symtab[mysym].type = FUNC;
	symtab[mysym].base = -4;
	symtab[mysym].size = 4;
	symtab[mysym].dim = 1;

	++scope;

	/* So far, we could be a leaf procedure.... */
	isleaf = 1;

	idiot_text();
	label(beginlab);
}


void
funcend(register int mysym)
{
	register char *n = namestring(symtab[mysym].ipos);

	/* For now, functions always return an int */
	symtab[mysym].type = FUNC;
	symtab[mysym].base = -1;
	symtab[mysym].size = 1;
	symtab[mysym].dim = 1;

	/* Now the common end return code...
	   but special-case leaf procedures
	*/
	if ((isleaf == 0) || (highwater != 0)) {
		/* Not a leaf; do full stack frame...
		   Stack looks like:
		   [old fp] [ret addr] [locals]
		   we do this here because that's when we know
		   how much space we need for locals...
		*/

		/* Add space for fp and return address */
		highwater += 2;	

		label(endlab);
		idiot_dup(RA, FP);		/* ra = mem[fp-1] */
		idiot_add(RA, ALL);
		idiot_ld(RA, RA);
		idiot_dup(SP, FP);		/* sp = fp */
		idiot_ld(FP, SP);		/* fp = mem[sp] (old fp) */
		idiot_add(SP, ONE);		/* sp = old sp */
		idiot_jz(ZERO, RA);		/* return */

		idiot_prelabel(n);

		idiot_dup(TMP, SP);		/* save sp value */
		idiot_li(TMP2, -highwater);	/* sp -= highwater */
		idiot_add(SP, TMP2);
		idiot_add(TMP, ALL);		/* mem[sp+highwater-1] = fp */
		idiot_st(FP, TMP);
		idiot_add(TMP, ALL);		/* mem[fp-1] = ra */
		idiot_st(RA, TMP);
	} else {
		/* A leaf without locals... */

		label(endlab);
		idiot_jz(ZERO, RA);		/* return */

		idiot_prelabel(n);
	}

	jump(beginlab);

	--scope;
}

void
def(register int mysym)
{
	register char *n = namestring(symtab[mysym].ipos);
	register int asize;

	asize = (symtab[mysym].size * symtab[mysym].dim);

	symtab[mysym].type = VAR;
	if ((symtab[mysym].scope = scope) == 0) {
		/* Offset from $gp */
		symtab[mysym].base = gpoffset;
		gpoffset += asize;

		idiot_data(DATABASE + symtab[mysym].base);
		idiot_label(n);
		idiot_space(asize);
	} else {
		/* Offset from $fp */
		fpoffset -= asize;
		symtab[mysym].base = fpoffset;

		if (highwater < -fpoffset) {
			highwater = -fpoffset;
		}
	}
}


int
defstr(register int spos)
{
	register int num;
	register int asize;

	idiot_data(DATABASE + gpoffset);

	asize = 0;
	while (input[++spos] != '"') {
		if (++asize > 1) {
			if ((asize & 7) == 1) {
				printf("\n\t.word\t");
			} else {
				printf(", ");
			}
		} else {
			printf("\t.word\t");
		}
		if (input[spos] == '\\') {
			switch (input[++spos]) {
			case 't':	printf("%3d", '\t'); break;
			case 'n':	printf("%3d", '\n'); break;
			case 'r':	printf("%3d", '\r'); break;
			case 'b':	printf("%3d", '\b'); break;
			case '0':	case '1':	case '2':
			case '3':	case '4':	case '5':
			case '6':	case '7':
				num = (input[spos] - '0');
				while ((input[spos+1] >= '0') &&
				       (input[spos+1] <= '7')) {
					num *= 8;
					num += (input[++spos] - '0');
				}
				printf("%3d", num);
				break;
			default:
				printf("%3d", input[spos]);
			}
		} else {
			printf("%3d", input[spos]);
		}
	}

	++asize;
	printf("\n"
	       "\t.word\t0\n"
	       "\n"
	       "\t.text\n");

	gpoffset += asize;
	return(DATABASE + gpoffset - asize);
}



/*	Lexicals...
*/

int
prefixis(register char *p)
{
	register int i = 0;

	for (;;) {
		register int t = p[i];

		if (t == 0) {
			ipos += i;
			return(1);
		}
		if (t != input[ipos + i]) {
			return(0);
		}
		++i;
	}
}

int
nameis(register char *p)
{
	register int i = 0;

	for (;;) {
		register int t = p[i];

		if (!isnamechar(t)) {
			if (!isnamechar(input[ipos + i])) {
				ipos += i;
				return(1);
			} else {
				return(0);
			}
		}
		if (t != input[ipos + i]) {
			return(0);
		}
		++i;
	}
}

int
lexhelp(void)
{
	register int base = 10;

again:

	/* Recognize all the non-name stuff */
	switch (input[ipos]) {

	/* Handle whitespace, etc. */
	case '\n':
#ifdef	LINETRACE
		printf("#line\t%d:\t", lineno);
		while (linestart <= ipos) {
			putchar(input[linestart]);
			++linestart;
		}
#endif
		++lineno;
		/* Fall through... */
	case ' ':	case '\t':	case '\r':
		++ipos;
		goto again;
	case '\000':
		return(MYEOF);

	/* Handling of punctuation... */
	case '=':	case '!':
	case '<':	case '>':
	case '+':	case '-':	case '~':
	case '*':	case '/':	case '%':
	case '|':	case '&':
		if (prefixis("==")) return(EQ);
		if (prefixis("!=")) return(NE);
		if (prefixis(">=")) return(GE);
		if (prefixis("<=")) return(LE);
		if (prefixis("<<")) return(SL);
		if (prefixis("++")) return(PP);
		if (prefixis("--")) return(MM);
		if (prefixis("|=")) return(OE);
		if (prefixis("^=")) return(XE);
		if (prefixis("&=")) return(AE);
		if (prefixis("+=")) return(PE);
		if (prefixis("-=")) return(ME);
		if (prefixis("*=")) return(TE);
		if (prefixis("/=")) return(DE);
		if (prefixis("%=")) return(RE);
		if (prefixis("||")) return(OO);
		if (prefixis("&&")) return(AA);
		/* Fall through... */
	case ',':	case '?':	case ':':
	case '{':	case '}':	case '^':
	case '[':	case ']':
	case '(':	case ')':
	case ';':
		return(input[ipos++]);

	/* Handling of numbers... */
	case '0':
		base = 8;
		switch (input[++ipos]) {
		case 'b': base = 2; ++ipos; break;
		case 'x': base = 16; ++ipos; break;
		}
	case '1':	case '2':	case '3':
	case '4':	case '5':	case '6':
	case '7':	case '8':	case '9':
		lexnum = 0;
		for (;;) {
			register int t = input[ipos];

			if ((t >= '0') && (t <= '9')) {
				t -= '0';
			} else if (((t |= ('a'-'A')) >= 'a') &&
				   (t <= 'f')) {
				t -= ('a' - 10);
			} else {
				return(NUM);
			}

			if (t >= base) {
				error("invalid digit");
			}

			lexnum = (lexnum * base) + t;
			++ipos;
		}
	case '\'':
		++ipos;
		lexnum = input[ipos++];
		if (input[ipos++] != '\'') {
			error("ill-formed character constant");
		}
		return(NUM);
	case '"':
		lexstr = ipos;
		do {
			++ipos;
			if (input[ipos] == 0) {
				error("string ends in end of input");
			}
		} while ((input[ipos] != '"') ||
			 (input[ipos-1] == '\\'));
		++ipos;
		return(STRING);
	default:
		if (!isnamechar(input[ipos])) {
			error("illegal character 0x%02x (%c)",
			      input[ipos],
			      input[ipos]);
			++ipos;
			goto again;
		}
	}

	/* Must be a name... */
	if (nameis("int")) return(INT);
	if (nameis("short")) return(SHORT);
	if (nameis("char")) return(CHAR);
	if (nameis("if")) return(IF);
	if (nameis("else")) return(ELSE);
	if (nameis("while")) return(WHILE);
	if (nameis("do")) return(DO);
	if (nameis("return")) return(RETURN);
	if (nameis("for")) return(FOR);
	if (nameis("goto")) return(GOTO);

	/* Find it in the symbol table */
	for (lexsym=(symsp-1); lexsym>=0; --lexsym) {
		if (nameis(&(input[symtab[lexsym].ipos]))) {
			return(symtab[lexsym].type);
		}
	}

	/* Make a new symbol table entry */
	symtab[lexsym = (symsp++)].ipos = ipos;
	nameis(&(input[ipos]));
	return(symtab[lexsym].type = ((input[ipos] == ':') ?
				      TARGET :
				      WORD));
}

int
lex(void)
{
	nextt = lexhelp();
	return(nextt);
}

int
match(register int t)
{
	if (nextt == t) {
		lex();
		return(1);
	}
	return(0);
}

int
assume(register int t)
{
	if (!match(t)) {
		warn("missing %c assumed", t);
		return(0);
	}
	return(1);
}



/*	Parsing...
*/

void
memaddr(register int mysym)
{
	/* Base address */
	if (symtab[mysym].scope != 0) {
		pushfpoff(symtab[mysym].base);
	} else {
		pushgpoff(symtab[mysym].base);
	}

	lex();
	if (match('[')) {
		/* subscripted */
		expr();		/* Index value */
		assume(']');

		/* Multiply by element size */
		switch (symtab[mysym].size) {
		case 2:	pushop(MULBY2); break;
		case 1: break;
		default:
			pushop(MULBY4);
		}

		pushop('+');	/* Add to base address */
	}

	lval(symtab[mysym].size);
}

void
unary(void)
{
	register int mysym;
	register int args = 0;

	switch (nextt) {
	case PP:
		lex();
		unary();
		pushdup();
		pushnum(1);
		pushop('+');
		store(1);
		break;
	case MM:
		lex();
		unary();
		pushdup();
		pushnum(-1);
		pushop('+');
		store(1);
		break;
	case '(':
		lex();
		expr();
		assume(')');
		break;
	case '-':
		lex();
		unary();
		pushop(NEG);
		break;
	case '!':
		lex();
		unary();
		pushop('!');
		break;
	case '~':
		lex();
		unary();
		pushop('~');
		break;
	case VAR:
		memaddr(lexsym);
		break;
	case WORD:
		symtab[lexsym].type = FUNC;
		symtab[lexsym].base = 0;
		/* Fall through... */
	case FUNC:
		/* Function call */
		mysym = lexsym;
		lex();
		if (!match('(')) {
			error("undefined variable %s",
			      namestring(symtab[mysym].ipos));
		}
		args = 0;
		while (!match(')')) {
			expr();
			setarg(args++);
			match(',');
		}
		call(mysym);
		break;
	case NUM:
		pushnum(lexnum);
		lex();
		break;
	case STRING:
		pushnum(defstr(lexstr));
		lex();
		break;
	default:
		error("malformed expression");
	}

	/* Suffix operation */
	switch (nextt) {
	case PP:
		lex();
		pushdup();
		loadnos();
		pushdup();
		pushnum(1);
		pushop('+');
		store(0);
		break;
	case MM:
		lex();
		pushdup();
		loadnos();
		pushdup();
		pushnum(1);
		pushop('+');
		store(0);
		break;
	}
}

void
mul(void)
{
	register int t;

	unary();
	for (;;) {
		switch (nextt) {
		case '*':
		case '/':
		case '%':
			t = nextt;
			lex();
			unary();
			pushop(t);
		default:
			return;
		}
	}
}

void
add(void)
{
	register int t;

	mul();
	for (;;) {
		switch (nextt) {
		case '+':
		case '-':
			t = nextt;
			lex();
			mul();
			pushop(t);
		default:
			return;
		}
	}
}

void
slsr(void)
{
	register int t;

	add();
	for (;;) {
		switch (nextt) {
		case SL:
		case SR:
			t = nextt;
			lex();
			add();
			pushop(t);
		default:
			return;
		}
	}
}

void
leltgegt(void)
{
	register int t;

	slsr();
	for (;;) {
		switch (nextt) {
		case LE:
		case '<':
		case GE:
		case '>':
			t = nextt;
			lex();
			slsr();
			pushop(t);
		default:
			return;
		}
	}
}

void
eqne(void)
{
	register int t;

	leltgegt();
	for (;;) {
		switch (nextt) {
		case EQ:
		case NE:
			t = nextt;
			lex();
			leltgegt();
			pushop(t);
		default:
			return;
		}
	}
}

void
and(void)
{
	eqne();
	while (match('&')) {
		eqne();
		pushop('&');
	}
}

void
xor(void)
{
	and();
	while (match('^')) {
		and();
		pushop('^');
	}
}

void
or(void)
{
	xor();
	while (match('|')) {
		xor();
		pushop('|');
	}
}

void
andand(void)
{
	register int lab;

	or();
	if (match(AA)) {
		lab = labnum;
		labnum += 3;

		do {
			jumpf(lab);
			++labnum;
			or();
		} while (match(AA));

		jumpt(lab+1);
		label(lab);
		pushnum(0);
		decsp();
		jump(lab+2);
		label(lab+1);
		pushnum(1);
		label(lab+2);
	}
}

void
oror(void)
{
	register int lab;

	andand();
	if (match(OO)) {
		lab = labnum;
		labnum += 3;

		do {
			jumpt(lab);
			++labnum;
			andand();
		} while (match(OO));

		jumpf(lab+1);
		label(lab);
		pushnum(1);
		decsp();
		jump(lab+2);
		label(lab+1);
		pushnum(0);
		label(lab+2);
	}
}

void
cond(void)
{
	register int lab;

	oror();
	if (match('?')) {
		lab = labnum;
		labnum += 2;

		jumpf(lab);
		expr();
		decsp();
		jump(lab+1);
		assume(':');
		label(lab);
		cond();
		label(lab+1);
	}
}

void
assign(void)
{
	register int t;

	cond();
	switch (nextt) {
	case '=':
		lex();
		assign();
		store(1);
		break;
	case OE:
		lex();
		pushdup();
		assign();
		pushop('|');
		store(1);
		break;
	case XE:
		lex();
		pushdup();
		assign();
		pushop('^');
		store(1);
		break;
	case AE:
		lex();
		pushdup();
		assign();
		pushop('&');
		store(1);
		break;
	case PE:
		lex();
		pushdup();
		assign();
		pushop('+');
		store(1);
		break;
	case ME:
		lex();
		pushdup();
		assign();
		pushop('-');
		store(1);
		break;
	case TE:
		lex();
		pushdup();
		assign();
		pushop('*');
		store(1);
		break;
	case DE:
		lex();
		pushdup();
		assign();
		pushop('/');
		store(1);
		break;
	case RE:
		lex();
		pushdup();
		assign();
		pushop('%');
		store(1);
		break;
	}
}

void
expr(void)
{
	assign();
	while (match(',')) {
		decsp();
		assign();
	}
}

int
newsym(void)
{
	/* Create a new symbol table entry */
	register int mysym;

	switch (nextt) {
	case WORD:
		mysym = lexsym;
		break;
	case VAR:
	case FUNC:
		if (scope == symtab[lexsym].scope) {
			warn("redefinition of identifier");
		}
		symtab[mysym = (symsp++)].ipos = symtab[lexsym].ipos;
		break;
	default:
		error("ill-formed declaration of %s",
		      namestring(symtab[mysym].ipos));
	}
	lex();
	symtab[mysym].scope = scope;
	return(mysym);
}

void
stat(void)
{
	register int scopesymsp, scopeoffset;
	register int lab;
	register int mysym;
	register int labreg, labreg1, labreg2, labreg3;

	switch (nextt) {
	case '{':
		lex();
		scopesymsp = symsp;
		scopeoffset = fpoffset;

		decl();
		while (!match('}')) {
			stat();
		}

		symsp = scopesymsp;
		fpoffset = scopeoffset;
		break;
	case IF:
		lex();
		expr();
		lab = labnum;
		labnum += 2;
		jumpf(lab);
		stat();
		if (nextt == ELSE) {
			lex();
			jump(lab+1);
			label(lab);
			stat();
			label(lab+1);
		} else {
			label(lab);
		}
		break;
	case FOR:
		printf("; for loop\n");
		lex();
		assume('(');
		lab = labnum;
		labnum += 4;
		if (!match(';')) {
			expr();
			decsp();
			match(';');
		}
#ifdef	HOISTADDR
		incsp();
		idiot_la_((labreg = TOS), lab);
		incsp();
		idiot_la_((labreg1 = TOS), lab+1);
		incsp();
		idiot_la_((labreg2 = TOS), lab+2);
		incsp();
		idiot_la_((labreg3 = TOS), lab+3);

		printf("; for condition\n");
		label(lab);
		if (!match(';')) {
			expr();
			jumpfreg(labreg1);
			match(';');
		}
		jumpreg(labreg2);
		printf("; for increment\n");
		label(lab+3);
		if (!match(')')) {
			expr();
			decsp();
			match(')');
		}
		jumpreg(labreg);
		printf("; for body\n");
		label(lab+2);
		stat();
		jumpreg(labreg3);
		label(lab+1);

		decsp();
		decsp();
		decsp();
		decsp();
#else
		label(lab);
		if (!match(';')) {
			expr();
			jumpf(lab+1);
			match(';');
		}
		jump(lab+2);
		label(lab+3);
		if (!match(')')) {
			expr();
			decsp();
			match(')');
		}
		jump(lab);
		label(lab+2);
		stat();
		jump(lab+3);
		label(lab+1);

#endif
		break;
	case WHILE:
		lex();
		lab = labnum;
		labnum += 2;
#ifdef	HOISTADDR
		incsp();
		idiot_la_((labreg = TOS), lab);
		incsp();
		idiot_la_((labreg1 = TOS), lab+1);
		label(lab);
		expr();
		jumpfreg(labreg1);
		stat();
		jumpreg(labreg);
		decsp();
		decsp();
#else
		label(lab);
		expr();
		jumpf(lab+1);
		stat();
		jump(lab);
#endif
		label(lab+1);
		break;
	case DO:
		lex();
		lab = (labnum++);
#ifdef	HOISTADDR
		incsp();
		idiot_la_((labreg = TOS), lab);
		label(lab);
		stat();
		if (match(WHILE)) {
			error("do missing while");
		}
		expr();
		assume(';');
		jumptreg(labreg);
		decsp();
#else
		label(lab);
		stat();
		if (match(WHILE)) {
			error("do missing while");
		}
		expr();
		assume(';');
		jumpt(lab);
#endif
		break;
	case RETURN:
		lex();
		if (nextt != ';') {
			expr();
			pushop(RETVAL);
		}
		match(';');
		break;
	case GOTO:
		lex();
		ghoto(namestring(symtab[newsym()].ipos));
		--symsp;
		break;
	case TARGET:
		target(namestring(symtab[symsp-1].ipos));
		--symsp;
		lex();
		assume(':');
		break;
	case ';':
		lex();
		break;
	default:
		expr();
		assume(';');
		decsp();
		break;
	}
}

int
ctype(void)
{
	switch (nextt) {
	case INT:	lex(); return(1);	/* All things are 16 bit units */
	case SHORT:	lex(); return(1);
	case CHAR:	lex(); return(1);
	case WORD:	if (scope == 0) {
				warn("missing int keyword assumed");
				return(1);
			}
	}
	return(0);
}

void
decl(void)
{
	register int scopeoffset;
	register int mysym, argsym;
	register int size, args;

	while ((size = ctype()) != 0) {
moredecls:
		mysym = newsym();
		symtab[mysym].size = size;

		switch (nextt) {
		case '[':
			lex();
			symtab[mysym].type = VAR;
			if (nextt != NUM) {
				error("non-constant dim for %s",
				      namestring(symtab[mysym].ipos));
			}
			symtab[mysym].dim = lexnum;
			def(mysym);
			lex();
			assume(']');
			if (match(',')) goto moredecls;
			assume(';');
			break;
		case ';':
			symtab[mysym].dim = 1;
			def(mysym);
			lex();
			break;
		case ',':
			symtab[mysym].dim = 1;
			def(mysym);
			lex();
			goto moredecls;
		case '(':
			if (scope != 0) {
				error("nested definition of function %s",
				      namestring(symtab[mysym].ipos));
			}
			lex();

			funcbegin(mysym);

			args = 0;
			while ((size = ctype()) != 0) {
				argsym = newsym();
				symtab[argsym].type = VAR;
				symtab[argsym].size = 1;
				symtab[argsym].dim = 1;
				def(argsym);

				/* Copy arg from register to local */
				pushnum(symtab[argsym].base);
				pushop(AFP);
				lval(1);
				pusharg(args++);
				store(0);

				if (match('[')) {
					error("array arguments currently not supported");
				}
				match(',');
			}
			if (!match(')')) {
				warn("missing ) in function argument declaration");
			}
			stat();
			funcend(mysym);

			break;
		default:
			error("declaration missing ; or argument list");
		}
	}
}

int
main(int arc, char **argv)
{
	int c;

	eof = 0;
	while ((c = getchar()) != EOF) input[eof++] = c;
	input[eof] = 0;
	ipos = 0;
	nextt = lex();
	startup();
	decl();
}
