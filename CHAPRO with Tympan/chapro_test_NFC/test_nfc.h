 
 //code extracted from CHA test_nfc.c

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

static char msg[MAX_MSG] = {0};
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
    //Serial.println("test_nfc.h: configure_nfc()...");
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
  //Serial.println("test_nfc.h: configure()...");
  
//    static char *ifn = "test/cat.wav";
//    static char *wfn = "test/tst_nfc.wav";

    // initialize CHAPRO variables
    configure_nfc();
    /*
    // initialize I/O
#ifdef ARSCLIB_H
    io->iod = ar_find_dev(ARSC_PREF_SYNC); // find preferred audio device
#endif                                     // ARSCLIB_H
    io->iwav = NULL;
    io->owav = NULL;
    io->ifn = args.ifn ? args.ifn : ifn;
    io->ofn = args.play ? args.ofn : wfn;
    io->dfn = 0;
    io->mat = 0;
    io->nrep = (args.nrep < 1) ? 1 : args.nrep;
    */
}

/***********************************************************/

//int main(int ac, char *av[])
//{
    static void *cp[NPTR] = {0};
    static I_O io;

//    parse_args(ac, av);
//    configure(&io);
//    report();
//    prepare(&io, cp);
//    process(&io, cp);
//    cleanup(&io, cp);
//    return (0);
//}
