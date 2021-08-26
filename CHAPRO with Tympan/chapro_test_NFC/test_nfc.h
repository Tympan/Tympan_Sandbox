 
 //code extracted from CHA tst_nfc.c

#define MAX_MSG 256

typedef struct
{
    char *ifn, *ofn, *dfn, mat, nrep;
    double rate;
    float *iwav, *owav;
    int32_t cs;
    int32_t *siz;
    int32_t iod, nwav, nsmp, mseg, nseg, oseg, pseg;
    void **out;
} I_O;

/***********************************************************/

//static char msg[MAX_MSG] = {0};
static double srate = 24000; // sampling rate (Hz)
static int chunk = 32;       // chunk size
static int prepared = 0;
//static int io_wait = 40;
//static struct
//{
//    char *ifn, *ofn, nfc, mat, nrep, play;
//    double f1, f2;
//    int nw;
//} args;
static CHA_NFC nfc = {0};

/***********************************************************/

static void
prepare_nfc(CHA_PTR cp)
{
  //Serial.println("test_nfc.h: prepare_nfc()...");
  
    // prepare NFC
    cha_nfc_prepare(cp, &nfc);  //see CHAPRO library "nfc_prepare.c"
}

// prepare signal processing

static void
prepare(I_O *io, CHA_PTR cp)
{
  //Serial.println("test_nfc.h: prepare()...");
    //prepare_io(io);
    //srate = io->rate;
    //chunk = io->cs;
    prepare_nfc(cp);
    prepared++;
}

/***********************************************************/


static void
configure_nfc()
{
    // NFC parameters
    nfc.cs = chunk; // chunk size
    nfc.f1 = 3000;  // compression-lower-bound frequency
    nfc.f2 = 4000;  // compression-upper-bound frequency
    nfc.nw = 128;   // window size
    nfc.sr = srate; // sampling rate
//    // update args
//    if (args.f1 > 0)
//        nfc.f1 = args.f1;
//    if (args.f2 > 0)
//        nfc.f2 = args.f2;
//    if (args.nw > 0)
//        nfc.nw = args.nw;
}



static void
configure(I_O *io)
{
//    static char *ifn = "test/cat.wav";
//    static char *wfn = "test/tst_nfc.wav";
//    static char *mfn = "test/tst_nfc.mat";

    // initialize CHAPRO variables
    configure_nfc();
//    // initialize I/O
//#ifdef ARSCLIB_H
//    io->iod = ar_find_dev(ARSC_PREF_SYNC); // find preferred audio device
//#endif                                     // ARSCLIB_H
//    io->iwav = NULL;
//    io->owav = NULL;
//    io->ifn  = args.ifn  ? args.ifn : ifn;
//    io->ofn  = args.play ? args.ofn : wfn; 
//    io->dfn  = mfn; 
//    io->mat  = args.mat;
//    io->nrep = (args.nrep < 1) ? 1 : args.nrep;
}

/***********************************************************/


// /////////////////////////////////// Note from WEA (Creare)
//
// Here is the main() for this program.  It creates the main data array ("cp") and
// it creates a secondary data structure ("io") for I/O.  In his main, he passes
// these two structures to all the piece of the overall program.
//
// For Arduino, there is no "main".  Instead, there is a "setup()" and a "loop()".
// The question is where do we create "cp" and "io".  I have chosen to create
// then in the global space so that they are most easily accesible by both setup()
// and loop().  There are other ways that this could be done.
//
// I then comment out all of the other actions because these will be done
// in setup() and in loop().

//int main(int ac, char *av[])
//{
    static void *cp[NPTR] = {0};
    static I_O io;
//
//    parse_args(ac, av);
//    configure(&io);
//    report();
//    prepare(&io, cp);
//    process(&io, cp);
//    cleanup(&io, cp);
//    return (0);
//}
//#endif
