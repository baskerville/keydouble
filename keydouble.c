/*
     __                   __             __    __
    |  |--.-----.--.--.--|  .-----.--.--|  |--|  .-----.
    |    <|  -__|  |  |  _  |  _  |  |  |  _  |  |  -__|
    |__|__|_____|___  |_____|_____|_____|_____|__|_____|
                |_____|

    baskerville at lavabit dot com

    Refs: https://github.com/r0adrunner/Space2Ctrl
          https://github.com/tomykaira/pianistic
*/

#define _BSD_SOURCE
#define ARTIFICIAL_TIMEOUT 600
#define SLEEP_MICROSEC 1000
#define MAX_CODE 256
#define CODE_UNDEF -1
#define PAIR_SEP ":"

enum {False = 0, True = 1};

#include <X11/Xlibint.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static void setup(void);
static void loop(void);
static void stop(int signum);
static void evtcallback(XPointer priv, XRecordInterceptData *hook);
static void die(const char *errstr, ...); 
static int deltamsec(struct timeval t1, struct timeval t2);

static Display *ctldpy, *datdpy;
static XRecordContext reccontext;
static XRecordRange *recrange;
static XRecordClientSpec reccspec;

/* natural and artificial keycodes */
static int natart[MAX_CODE];
static int running = True;

/* from libxnee */
typedef union {
    unsigned char type;
    xEvent event;
    xResourceReq req;
    xGenericReply reply;
    xError error;
    xConnSetupPrefix setup;
} XRecordDatum;

void
setup(void) {
    int event, error, major, minor;

    if (!(ctldpy = XOpenDisplay(NULL)) || !(datdpy = XOpenDisplay(NULL)))
        die("cannot open display\n");

    XSynchronize(ctldpy, True); /* don't remove this line */

    if (!XTestQueryExtension(ctldpy, &event, &error, &major, &minor))
        die("the xtest extension is not loaded\n");

    if (!XRecordQueryVersion(ctldpy, &major, &minor))
        die("the record extension is not loaded\n");

    if (!(recrange = XRecordAllocRange()))
        die("could not alloc record range object!\n");

    recrange->device_events.first = KeyPress;
    recrange->device_events.last = ButtonPress;
    reccspec = XRecordAllClients;

    if (!(reccontext = XRecordCreateContext(ctldpy, 0, &reccspec, 1, &recrange, 1)))
        die("could not create a record context");
}

void
loop(void) {
    if (!XRecordEnableContextAsync(datdpy, reccontext, evtcallback, NULL))
        die("cannot enable record context\n");
    while (running) {
        XRecordProcessReplies(datdpy);
        usleep(SLEEP_MICROSEC);
    }
}

/* called from Xserver when new events occurs */
void
evtcallback(XPointer priv, XRecordInterceptData *hook) {
    if (hook->category != XRecordFromServer) {
        XRecordFreeData(hook);
        return;
    }

    XRecordDatum *data = (XRecordDatum *) hook->data;
    static int natdown[MAX_CODE];
    static int keycomb[MAX_CODE]; 
    static struct timeval startwait[MAX_CODE], endwait[MAX_CODE];

    int code = data->event.u.u.detail;
    int evttype = data->event.u.u.type;

    if (evttype == KeyPress) {
        /* a natural key was pressed */
        if (!natdown[code] && natart[code] != CODE_UNDEF) {
            natdown[code] = True;
            gettimeofday(&startwait[code], NULL);
        } 
        else {
            int i;
            for (i = 0; i < MAX_CODE; i++)
                keycomb[i] = natdown[i];
        }
    }
    else if (evttype == KeyRelease) {
        /* a natural key was released */
        if (natart[code] != CODE_UNDEF) {
            printf("%d\n", natart[code]);
            natdown[code] = False;	
            if (!keycomb[code]) {
                gettimeofday(&endwait[code], NULL);
                /* if the timeout wasn't reached since natural was pressed */
                if (deltamsec(endwait[code], startwait[code]) < ARTIFICIAL_TIMEOUT ) {
                    XTestFakeKeyEvent(ctldpy, natart[code], True, CurrentTime);
                    XTestFakeKeyEvent(ctldpy, natart[code], False, CurrentTime);
                }
            }
            keycomb[code] = False;
        } 
    }
    else if (evttype == ButtonPress) {
        int i;
        for (i = 0; i < MAX_CODE; i++)
            keycomb[i] = natdown[i];
    }
    XRecordFreeData(hook);
}

/* from dwm */
void
die(const char *errstr, ...) {
	va_list ap;
	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

int
deltamsec(struct timeval t1, struct timeval t2) {
    return (((t1.tv_sec - t2.tv_sec) * 1000000)
             + (t1.tv_usec - t2.tv_usec)) / 1000;
}

void
stop(int signum) {
    running = False;
}

void
addpair(char *na) {
    char *natural, *artificial;
    int natcode, artcode;
    if (!(natural = strtok(na, PAIR_SEP)) || !(artificial = strtok(NULL, PAIR_SEP)))
        die("could not parse natart pair\n");
    natcode = atoi(natural);
    artcode = atoi(artificial);
    natart[natcode] = artcode;
}

int
main(int argc, char *argv[]) {
    int i;
    if (argc < 2) {
        printf("usage: keydouble NAT:ART ...\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < MAX_CODE; i++)
        natart[i] = CODE_UNDEF;
    for (i = 1; i < argc; i++)
        addpair(argv[i]);
    for (i = 0; i < MAX_CODE; i++)
        printf("%d:%d ", i, natart[i]);
    printf("\n");
    signal(SIGINT, stop);
    signal(SIGTERM, stop);
    signal(SIGHUP, stop);
    setup();
    loop();
    if(!XRecordDisableContext(ctldpy, reccontext))
        die("could not disable record context\n");
    XRecordFreeContext(ctldpy, reccontext);
    XFlush(ctldpy);
    XFree(recrange);
    XCloseDisplay(datdpy);
    XCloseDisplay(ctldpy);
    return EXIT_SUCCESS;
}
