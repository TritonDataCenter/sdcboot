/*  HTML Parse Stuff

    Copyright (c) Express Software 1998-2002
    All Rights Reserved.

    Created by: Brian E. Reifsnyder, Paul Hsieh and Robert Platt
*/

#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED

/*************************/
/* Tag substition tables */
/*************************/

#define TST_ENTRY(b, a) {(b), (a)}

struct tagSubsEntryStruct
{
  char *before;
  char *after;
};

typedef struct tagSubsEntryStruct tagSubsEntry;

static tagSubsEntry tagSubsTable1[] = {
  TST_ENTRY ("<h1", "<b><h1"),
  TST_ENTRY ("</h1>", "</h1></b><br>"),
  TST_ENTRY (0, 0)
};

static tagSubsEntry tagSubsTable2[] = {
  TST_ENTRY ("<hr>",
	     "\n\n 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴 \n\n"),
  TST_ENTRY ("<strong", "<b"),
  TST_ENTRY ("</strong>", "</b>"),
  TST_ENTRY ("<code", "<b"),
  TST_ENTRY ("</code>", "</b>"),
  TST_ENTRY ("<br>", "\n"),
  TST_ENTRY ("<p>", "\n\n"),
  TST_ENTRY ("</p>", "\n\n"),
  TST_ENTRY ("<ol>", "\n\n"),
  TST_ENTRY ("</ol>", "\n\n"),
  TST_ENTRY ("<ul>", "\n\n"),
  TST_ENTRY ("</ul>", "\n\n"),
  TST_ENTRY ("<li>", " \n\007 "),
  TST_ENTRY ("<h2", "\n\n<b"),
  TST_ENTRY ("</h2>", "</b>\n"),
  TST_ENTRY ("<h3", "\n\n<b"),
  TST_ENTRY ("</h3>", "</b>\n"),
  TST_ENTRY ("<pre", "\n<pre"),
  TST_ENTRY ("</pre>", "\n</pre>"),
  TST_ENTRY ("<em", "<i"),
  TST_ENTRY ("</em>", "</i>"),
  TST_ENTRY ("<tt", "<i"),
  TST_ENTRY ("</tt>", "</i>"),
  TST_ENTRY (0, 0)
};

static tagSubsEntry plainToHTMLsubs[] = {
  TST_ENTRY ("<", "&lt;"),
  TST_ENTRY (">", "&gt;"),
  TST_ENTRY (0, 0)
};

#undef TST_ENTRY

#ifdef _PARSE_C_

/**********************/
/* Escape Code tables */
/**********************/

#define AST_ENTRY(b, a) {(b), (a)}

enum entityTableType { ascii, unicode };

/* Characters <= 0x7F */
struct ampCharEntryStruct
{
  char *before;
  int after;
}
static asciiEntityTable[] = {
  AST_ENTRY ("&para;", 20),
  AST_ENTRY ("&sect;", 21),
  AST_ENTRY ("&lt;", '<'),
  AST_ENTRY ("&gt;", '>'),
  AST_ENTRY ("&amp;", '&'),
  AST_ENTRY ("&nbsp;", ' '),
  AST_ENTRY ("&quot;", '\"'),
  AST_ENTRY ("&ldquo;", '\"'),
  AST_ENTRY ("&rdquo;", '\"'),
  AST_ENTRY ("&rsquo;", '\''),
  AST_ENTRY ("&lsquo;", '`'),
  AST_ENTRY ("&circ;", '^'),
  AST_ENTRY ("&tilde;", '~'),
  AST_ENTRY ("&spades;", 6),
  AST_ENTRY ("&clubs;", 5),
  AST_ENTRY ("&hearts;", 3),
  AST_ENTRY ("&diams;", 4),
  AST_ENTRY (0, 0)
};

/* Characters > 0x7F, which vary from codepage to codepage.
   Convert to unicode then use the unicode -> DOS codepage converter. */
static struct ampCharEntryStruct unicodeEntityTable[] = {
/* Latin-1 (U+0080 - U+00FF) */
  AST_ENTRY ("&nbsp;", 160),
  AST_ENTRY ("&iexcl;", 161),
  AST_ENTRY ("&cent;", 162),
  AST_ENTRY ("&pound;", 163),
  AST_ENTRY ("&curren;", 164),
  AST_ENTRY ("&yen;", 165),
  AST_ENTRY ("&brvbar;", 166),
  AST_ENTRY ("&sect;", 167),
  AST_ENTRY ("&uml;", 168),
  AST_ENTRY ("&copy;", 169),
  AST_ENTRY ("&ordf;", 170),
  AST_ENTRY ("&laquo;", 171),
  AST_ENTRY ("&not;", 172),
  AST_ENTRY ("&shy;", 173),
  AST_ENTRY ("&reg;", 174),
  AST_ENTRY ("&macr;", 175),
  AST_ENTRY ("&deg;", 176),
  AST_ENTRY ("&plusmn;", 177),
  AST_ENTRY ("&sup2;", 178),
  AST_ENTRY ("&sup3;", 179),
  AST_ENTRY ("&acute;", 180),
  AST_ENTRY ("&micro;", 181),
  AST_ENTRY ("&para", 182),
  AST_ENTRY ("&middot;", 183),
  AST_ENTRY ("&cedil;", 184),
  AST_ENTRY ("&sup1;", 185),
  AST_ENTRY ("&ordm;", 186),
  AST_ENTRY ("&raquo;", 187),
  AST_ENTRY ("&frac14;", 188),
  AST_ENTRY ("&frac12;", 189),
  AST_ENTRY ("&frac34;", 190),
  AST_ENTRY ("&iquest;", 191),
  AST_ENTRY ("&Agrave;", 192),
  AST_ENTRY ("&Aacute;", 193),
  AST_ENTRY ("&Acirc;", 194),
  AST_ENTRY ("&Atilde;", 195),
  AST_ENTRY ("&Auml;", 196),
  AST_ENTRY ("&Aring;", 197),
  AST_ENTRY ("&AElig;", 198),
  AST_ENTRY ("&Ccedil;", 199),
  AST_ENTRY ("&Egrave;", 200),
  AST_ENTRY ("&Eacute;", 201),
  AST_ENTRY ("&Ecirc;", 202),
  AST_ENTRY ("&Euml;", 203),
  AST_ENTRY ("&Igrave;", 204),
  AST_ENTRY ("&Iacute;", 205),
  AST_ENTRY ("&Icirc;", 206),
  AST_ENTRY ("&Iuml;", 207),
  AST_ENTRY ("&ETH;", 208),
  AST_ENTRY ("&Ntilde;", 209),
  AST_ENTRY ("&Ograve;", 210),
  AST_ENTRY ("&Oacute;", 211),
  AST_ENTRY ("&Ocirc;", 212),
  AST_ENTRY ("&Otilde;", 213),
  AST_ENTRY ("&Ouml;", 214),
  AST_ENTRY ("&times;", 215),
  AST_ENTRY ("&Oslash;", 216),
  AST_ENTRY ("&Ugrave;", 217),
  AST_ENTRY ("&Uacute;", 218),
  AST_ENTRY ("&Ucirc;", 219),
  AST_ENTRY ("&Uuml;", 220),
  AST_ENTRY ("&Yacute;", 221),
  AST_ENTRY ("&THORN;", 222),
  AST_ENTRY ("&szlig;", 223),
  AST_ENTRY ("&agrave;", 224),
  AST_ENTRY ("&aacute;", 225),
  AST_ENTRY ("&acirc;", 226),
  AST_ENTRY ("&atilde;", 227),
  AST_ENTRY ("&auml;", 228),
  AST_ENTRY ("&aring;", 229),
  AST_ENTRY ("&aelig;", 230),
  AST_ENTRY ("&ccedil;", 231),
  AST_ENTRY ("&egrave;", 232),
  AST_ENTRY ("&eacute;", 233),
  AST_ENTRY ("&ecirc;", 234),
  AST_ENTRY ("&euml;", 235),
  AST_ENTRY ("&igrave;", 236),
  AST_ENTRY ("&iacute;", 237),
  AST_ENTRY ("&icirc;", 238),
  AST_ENTRY ("&iuml;", 239),
  AST_ENTRY ("&eth;", 240),
  AST_ENTRY ("&ntilde;", 241),
  AST_ENTRY ("&ograve;", 242),
  AST_ENTRY ("&oacute;", 243),
  AST_ENTRY ("&ocirc;", 244),
  AST_ENTRY ("&otilde;", 245),
  AST_ENTRY ("&ouml;", 246),
  AST_ENTRY ("&divide;", 247),
  AST_ENTRY ("&oslash;", 248),
  AST_ENTRY ("&ugrave;", 249),
  AST_ENTRY ("&uacute;", 250),
  AST_ENTRY ("&ucirc;", 251),
  AST_ENTRY ("&uuml;", 252),
  AST_ENTRY ("&yacute;", 253),
  AST_ENTRY ("&thorn;", 254),
  AST_ENTRY ("&yuml;", 255),
/* Latin Extended-A (U+0100 - U+017F) */
  AST_ENTRY ("&inodot;", 305),
  AST_ENTRY ("&OElig;", 338),
  AST_ENTRY ("&oelig;", 339),
  AST_ENTRY ("&Scaron;", 352),
  AST_ENTRY ("&scaron;", 353),
  AST_ENTRY ("&Yuml;", 376),
/* Latin Extended-B (U+0180 - U+024F) */
  AST_ENTRY ("&fnof;", 402),
/* Space Modifiers (U+02B0 - U+02FF) */
  AST_ENTRY ("&circ;", 710),
  AST_ENTRY ("&tilde;", 732),
/* Greek (U+0370 - U+03FF) */
  AST_ENTRY ("&Alpha;", 913),
  AST_ENTRY ("&Beta;", 914),
  AST_ENTRY ("&Gamma;", 915),
  AST_ENTRY ("&Delta;", 916),
  AST_ENTRY ("&Epsilon;", 917),
  AST_ENTRY ("&Zeta;", 918),
  AST_ENTRY ("&Eta;", 919),
  AST_ENTRY ("&Theta;", 920),
  AST_ENTRY ("&Iota;",  921),
  AST_ENTRY ("&Kappa;", 922),
  AST_ENTRY ("&Lambda;", 923),
  AST_ENTRY ("&Mu;", 924),
  AST_ENTRY ("&Nu;", 925),
  AST_ENTRY ("&Xi;", 926),
  AST_ENTRY ("&Omicron;", 927),
  AST_ENTRY ("&Pi;", 928),
  AST_ENTRY ("&Rho;", 929),
  AST_ENTRY ("&Sigma;", 931),
  AST_ENTRY ("&Tau;", 932),
  AST_ENTRY ("&Upsilon;", 933),
  AST_ENTRY ("&Phi;", 934),
  AST_ENTRY ("&Chi;", 935),
  AST_ENTRY ("&Psi;", 936),
  AST_ENTRY ("&Omega;", 937),
  AST_ENTRY ("&alpha;", 945),
  AST_ENTRY ("&beta;", 946),
  AST_ENTRY ("&gamma;", 947),
  AST_ENTRY ("&delta;", 948),
  AST_ENTRY ("&epsilon;", 949),
  AST_ENTRY ("&zeta;", 950),
  AST_ENTRY ("&eta;", 951),
  AST_ENTRY ("&theta;", 952),
  AST_ENTRY ("&iota;",  953),
  AST_ENTRY ("&kappa;", 954),
  AST_ENTRY ("&lambda;", 955),
  AST_ENTRY ("&mu;", 956),
  AST_ENTRY ("&nu;", 957),
  AST_ENTRY ("&xi;", 958),
  AST_ENTRY ("&omicron;", 959),
  AST_ENTRY ("&pi;", 960),
  AST_ENTRY ("&rho;", 961),
  AST_ENTRY ("&sigmaf;", 962),
  AST_ENTRY ("&sigma;", 963),
  AST_ENTRY ("&tau;", 964),
  AST_ENTRY ("&upsilon;", 965),
  AST_ENTRY ("&phi;", 966),
  AST_ENTRY ("&chi;", 967),
  AST_ENTRY ("&psi;", 968),
  AST_ENTRY ("&omega;", 969),
  AST_ENTRY ("&theyasym;", 977),
  AST_ENTRY ("&upsih;", 978),
  AST_ENTRY ("&piv;", 982),
/* General Punctuation (U+2000 - U+206F) */
  AST_ENTRY ("&ensp;", 8194),
  AST_ENTRY ("&emsp;", 8195),
  AST_ENTRY ("&thinsp;", 8201),
  AST_ENTRY ("&zwnj;", 8204),
  AST_ENTRY ("&zwj;", 8205),
  AST_ENTRY ("&lrm;", 8206),
  AST_ENTRY ("&rlm;", 8207),
  AST_ENTRY ("&ndash;", 8211),
  AST_ENTRY ("&mdash;", 8212),
  AST_ENTRY ("&lsquo;", 8216),
  AST_ENTRY ("&rsquo;", 8217),
  AST_ENTRY ("&sbquo;", 8218),
  AST_ENTRY ("&ldquo;", 8220),
  AST_ENTRY ("&rdquo;", 8221),
  AST_ENTRY ("&bdquo;", 8222),
  AST_ENTRY ("&dagger;", 8224),
  AST_ENTRY ("&Dagger;", 8225),
  AST_ENTRY ("&bull;", 8226),
  AST_ENTRY ("&hellip;", 8230),
  AST_ENTRY ("&permil;", 8240),
  AST_ENTRY ("&prime;", 8242),
  AST_ENTRY ("&Prime;", 8243),
  AST_ENTRY ("&lsaquo;", 8249),
  AST_ENTRY ("&rsaquo;", 8250),
  AST_ENTRY ("&oline;", 8254),
  AST_ENTRY ("&frasl;", 8260),
/* Currency Symbols (U+20A0 - U+20CF) */
  AST_ENTRY ("&euro;", 8364),
/* Letterlike Symbols (U+2100 - U+214F) */
  AST_ENTRY ("&image;", 8465),
  AST_ENTRY ("&weierp;", 8472),
  AST_ENTRY ("&real;", 8476),
  AST_ENTRY ("&trade;", 8482),
  AST_ENTRY ("&alefsym;", 8501),
/* Arrows (U+2100 - U+214F) */
  AST_ENTRY ("&larr;", 8592),
  AST_ENTRY ("&uarr;", 8593),
  AST_ENTRY ("&rarr;", 8594),
  AST_ENTRY ("&darr;", 8595),
  AST_ENTRY ("&harr;", 8596),
  AST_ENTRY ("&crarr;", 8629),
  AST_ENTRY ("&lArr;", 8656),
  AST_ENTRY ("&uArr;", 8657),
  AST_ENTRY ("&rArr;", 8658),
  AST_ENTRY ("&dArr;", 8659),
  AST_ENTRY ("&hArr;", 8660),
/* Mathematical Operators (U+2200 - U+22FF)*/
  AST_ENTRY ("&forall;", 8704),
  AST_ENTRY ("&part;", 8706),
  AST_ENTRY ("&exist;", 8707),
  AST_ENTRY ("&empty;", 8709),
  AST_ENTRY ("&nabla;", 8711),
  AST_ENTRY ("&isin;", 8712),
  AST_ENTRY ("&notin;", 8713),
  AST_ENTRY ("&ni;", 8715),
  AST_ENTRY ("&prod;", 8719),
  AST_ENTRY ("&sum;", 8721),
  AST_ENTRY ("&minus;", 8722),
  AST_ENTRY ("&lowast;", 8727),
  AST_ENTRY ("&radic;", 8730),
  AST_ENTRY ("&prop;", 8733),
  AST_ENTRY ("&infin;", 8734),
  AST_ENTRY ("&ang;", 8736),
  AST_ENTRY ("&and;", 8743),
  AST_ENTRY ("&or;", 8744),
  AST_ENTRY ("&cap;", 8745),
  AST_ENTRY ("&cup;", 8746),
  AST_ENTRY ("&int;", 8747),
  AST_ENTRY ("&there4;", 8756),
  AST_ENTRY ("&sim;", 8764),
  AST_ENTRY ("&cong;", 8773),
  AST_ENTRY ("&asymp;", 8776),
  AST_ENTRY ("&ne;", 8800),
  AST_ENTRY ("&equiv;", 8801),
  AST_ENTRY ("&le;", 8804),
  AST_ENTRY ("&ge;", 8805),
  AST_ENTRY ("&sub;", 8834),
  AST_ENTRY ("&sup;", 8835),
  AST_ENTRY ("&nsub;", 8836),
  AST_ENTRY ("&sube;", 8838),
  AST_ENTRY ("&supe;", 8839),
  AST_ENTRY ("&oplus;", 8853),
  AST_ENTRY ("&otimes;", 8855),
  AST_ENTRY ("&perp;", 8869),
  AST_ENTRY ("&sdot;", 8901),
/* Miscellaneous Technical (U+2300 - U+23FF)*/
  AST_ENTRY ("&lceil;", 8968),
  AST_ENTRY ("&rceil;", 8969),
  AST_ENTRY ("&lfloor;", 8970),
  AST_ENTRY ("&rfloor;", 8971),
  AST_ENTRY ("&lang;", 9001),
  AST_ENTRY ("&rang;", 9002),
/* Geometric Shapes (U+25A0 - U+25FF) */
  AST_ENTRY ("&loz;", 9674),
/* Miscellaneous Symbols (U+2600 - U+26FF) */
  AST_ENTRY ("&spades;", 9824),
  AST_ENTRY ("&clubs;", 9827),
  AST_ENTRY ("&hearts;", 9829),
  AST_ENTRY ("&diams;", 9830),
/* End the entity - unicode table */
  AST_ENTRY (0, 0)
};

#endif /* _PARSE_C_ */

void tags2lwr (char *text);
void simpleTagSubstitutions (struct eventState *pes,
                             tagSubsEntry * tagSubsTable);
void headerTagSubstitution (struct eventState *pes);
void preformatTrim (char *text);
void wordwrap (char *text);
void sensiblebreaks (char *text);
int ampSubs (struct eventState *pes);
int link_length (char *link_text);
/* stristr is taken from my WHATIS program - Rob Platt */
char *stristr (const char *s1, const char *s2);

#endif /* PARSE_H_INCLUDED */
