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
#define SLEEP_MICROSEC 100*1000

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
static int natcode, artcode;
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
    static int natdown = False;
    static int keycomb = False; 
    static struct timeval startwait, endwait;
    int code = data->event.u.u.detail;
    int evttype = data->event.u.u.type;

    if (evttype == KeyPress) {
        /* natural key pressed */
        if (!natdown && code == natcode) {
            natdown = True;
            gettimeofday(&startwait, NULL);
        } 
        else
            keycomb = natdown;
    }
    else if (evttype == KeyRelease) {
        /* natural key released */
        if (code == natcode){
            natdown = False;	
            if (!keycomb) {
                gettimeofday(&endwait, NULL);
                /* if the timeout wasn't reached since natural was pressed */
                if (deltamsec(endwait, startwait) < ARTIFICIAL_TIMEOUT ) {
                    XTestFakeKeyEvent(ctldpy, artcode, True, CurrentTime);
                    XTestFakeKeyEvent(ctldpy, artcode, False, CurrentTime);
                }
            }
            keycomb = False;
        } 
    }
    else if (evttype == ButtonPress)
        keycomb = natdown;
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

int
main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("usage: keydouble <natural-keycode> <artificial-keycode>\n");
        exit(EXIT_FAILURE);
    }
    natcode = atoi(argv[1]);
    artcode = atoi(argv[2]);
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
