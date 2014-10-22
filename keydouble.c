#define _BSD_SOURCE

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <X11/Xlibint.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/record.h>
#include <X11/keysym.h>

#define ARTIFICIAL_TIMEOUT 600
#define SLEEP_MICROSEC 100*1000
#define MAX_CODE 256
#define CODE_UNDEF -1
#define PAIR_SEP ":"

typedef enum {
    false,
    true
} bool;

void setup(void);
void loop(void);
void stop(int signum);
void evtcallback(XPointer priv, XRecordInterceptData *hook);
void die(const char *errstr, ...);
int deltamsec(struct timeval t1, struct timeval t2);

Display *ctldpy, *datdpy;
XRecordContext reccontext;
XRecordRange *recrange;
XRecordClientSpec reccspec;

/* maps a natural to an artificial keycode */
int natart[MAX_CODE];
bool running = true;

/* from libxnee */
typedef union {
    unsigned char type;
    xEvent event;
    xResourceReq req;
    xGenericReply reply;
    xError error;
    xConnSetupPrefix setup;
} XRecordDatum;

void setup(void)
{
    int event, error, major, minor;
    /*
      We're gonna fetch two display objects; one, we'll be stealing events
      from, and the other, we'll be sending events to. This prevents an
      infinite loop.
    */
    if (!(ctldpy = XOpenDisplay(NULL)) || !(datdpy = XOpenDisplay(NULL)))
        die("cannot open display\n");

    /*
      We have to synchronize the control display to ensure that the
      events we *send* get sent immediately; because we're not doing
      anything but sending key events, it should not result in a
      significant reduction in speed.
    */
    XSynchronize(ctldpy, true);

    /*
       Now we have to fetch the XRecord context; some sanity checking,
       first, then grab a context off of the 'from' display.
    */
    if (!XTestQueryExtension(ctldpy, &event, &error, &major, &minor))
        die("the xtest extension is not loaded\n");

    if (!XRecordQueryVersion(ctldpy, &major, &minor))
        die("the record extension is not loaded\n");

    if (!(recrange = XRecordAllocRange()))
        die("could not alloc the record range object\n");

    recrange->device_events.first = KeyPress;
    recrange->device_events.last = ButtonPress;
    reccspec = XRecordAllClients;

    if (!(reccontext = XRecordCreateContext(datdpy, 0, &reccspec, 1, &recrange, 1)))
        die("could not create a record context");

    /* Finally, start listening for events. */
    if (!XRecordEnableContextAsync(datdpy, reccontext, evtcallback, NULL))
        die("cannot enable record context\n");
}

void loop(void)
{
    while (running) {
        XRecordProcessReplies(datdpy);
        usleep(SLEEP_MICROSEC);
    }
}

void evtcallback(XPointer priv, XRecordInterceptData *hook)
{
    if (hook->category != XRecordFromServer) {
        XRecordFreeData(hook);
        return;
    }

    XRecordDatum *data = (XRecordDatum *) hook->data;

    static unsigned int numnat;
    static bool natdown[MAX_CODE], keycomb[MAX_CODE];
    static struct timeval startwait[MAX_CODE], endwait[MAX_CODE];

    int code = data->event.u.u.detail;
    int evttype = data->event.u.u.type;

    if (evttype == KeyPress) {
        /* a natural key was pressed */
        if (!natdown[code] && natart[code] != CODE_UNDEF) {
            natdown[code] = true;
            numnat++;
            gettimeofday(&startwait[code], NULL);
        } else if (numnat > 0) {
            int i;
            for (i = 0; i < MAX_CODE; i++)
                keycomb[i] = natdown[i];
        }
    } else if (evttype == KeyRelease) {
        /* a natural key was released */
        if (natart[code] != CODE_UNDEF) {
            natdown[code] = false;
            numnat--;
            if (!keycomb[code]) {
                gettimeofday(&endwait[code], NULL);
                /* if the timeout wasn't reached since natural was pressed */
                if (deltamsec(endwait[code], startwait[code]) < ARTIFICIAL_TIMEOUT ) {
                    /* we send key Press/Release events for the artificial keycode */
                    XTestFakeKeyEvent(ctldpy, natart[code], true, CurrentTime);
                    XTestFakeKeyEvent(ctldpy, natart[code], false, CurrentTime);
                }
            }
            keycomb[code] = false;
        }
    } else if (evttype == ButtonPress && numnat > 0) {
        int i;
        for (i = 0; i < MAX_CODE; i++)
            keycomb[i] = natdown[i];
    }
    XRecordFreeData(hook);
}

void die(const char *errstr, ...)
{
    va_list ap;
    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

int deltamsec(struct timeval t1, struct timeval t2)
{
    return (((t1.tv_sec - t2.tv_sec) * 1000000)
            + (t1.tv_usec - t2.tv_usec)) / 1000;
}

void stop(int signum)
{
    running = false;
}

void addpair(char *na)
{
    char *natural, *artificial;
    int natcode, artcode;
    if (!(natural = strtok(na, PAIR_SEP)) || !(artificial = strtok(NULL, PAIR_SEP)))
        die("could not parse natart pair\n");
    natcode = atoi(natural);
    artcode = atoi(artificial);
    natart[natcode] = artcode;
}

int main(int argc, char *argv[])
{
    int i;
    if (argc < 2)
        die("usage: %s NAT:ART ...\n", argv[0]);
    for (i = 0; i < MAX_CODE; i++)
        natart[i] = CODE_UNDEF;
    for (i = 1; i < argc; i++)
        addpair(argv[i]);

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

    return 0;
}
