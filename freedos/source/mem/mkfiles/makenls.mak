!include "mkfiles\generic.mak"

mkfiles\nls.mak: makenls.pl
    $(PERL) makenls.pl > mkfiles\nls.mak
