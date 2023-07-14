// Name: Conner Estes
// Date: 04/20/23
// ASGNT: Semester Project
// COURSE: CMPS 3600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <sys/file.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <math.h>
int pid;
typedef double Myfloat;
int limit;
int check_x_click(XEvent *e);
int get_arguement_count(char **);
struct Global {
    Display *dpy;
    Window win;
    GC gc;
    int xres, yres;
    unsigned int foreground_color;
    Atom wm_delete_window;
    Myfloat box[4][2];
    int childE;

} g;




typedef struct {
    int flag;
    int box_x;
    int box_y;
} SharedData;

//declare variables in struct for box
struct SharedBox {
    int box_x, box_y;
    int h, w;
    int hover;
    int kclick;
    int flag;
    int spin;
    double speed;
    Myfloat rotation_angle;
}  shbox;

struct Box {
    int x, y;
    int h, w;
    int hover;
} box;

int child = 0;

void x11_cleanup_xwindows(void);
void x11_init_xwindows(void);
void x11_clear_window(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void render(void);
void create_child();
void moveWindow(int rx, int ry);
void* update_child_position();



struct SharedBox * shared;
int shmid;

char **myargv;
char **myenvp;

int render_flag = 0;
int main(int argc, char *argv[], char *envp[])
{


    // set up coordinates for rotating box
    g.box[0][0] = -51.0;
    g.box[1][0] = -51.0;
    g.box[2][0] = 51.0;
    g.box[3][0] = 51.0;
    g.box[0][1] = -51.0;
    g.box[1][1] = 51.0;
    g.box[2][1] = 51.0;
    g.box[3][1] = -51.0;

    shared = &shbox;
    int time = -1;
    argc = 0;
    myargv = argv;
    myenvp = envp;
    // get the arguement count
    argc = get_arguement_count(argv);

  
    if(argc > 1) {
        if (strcmp(argv[1], "child") == 0)
            child = 1;
    }

    if(argc > 1 && child == 0) {
        if (argv[1] != NULL) {
        time = atoi(argv[1]);
        }
        if (time == 0)
            time = -1;
    }
    
    

    if (child == 0) {
        //initialize rotation angle
        shared->rotation_angle = 0.0;
        

        key_t key = ftok("foo", 80);
        shmid = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);
       
        //creating a shared memory segment
        shared = shmat(shmid, (void *) 0,0);
        //shared = shmat(shmid, (void *) 0, 0); //attaching to the segment.
        printf("parent's shmid: %i\n", shmid);

    }
    if (child == 1) {
        
        shared = shmat(atoi(argv[2]), (void *) 0, 0); //attaching to the segment.

        pthread_t update_thread; //get a thread pid.
        pthread_create(&update_thread, NULL, update_child_position, (void*)0); //starting a thread
        //This SHOULD match the parent's shmid value. If not, then they will not share the same segment.
        printf("child's shmid: %i\n", atoi(argv[2])); //it uses the second argument that was passed.
    }
    //initialize speed and spin for rotating box
    shared->speed =0.0;
    shared->spin = 0;
  
    // init variables to detect what window you are in
    shared->hover = 0;
    shared->flag = 0;    

    XEvent e; 
    g.foreground_color = 0x00ff00ff;
    int done = 0;
    x11_init_xwindows();
    box.x = g.xres/2;
    box.y = g.yres/2;
    box.w = box.h = 40;
    shared->box_x = box.x;
    shared->box_y = box.y;
    while (!done) {
        if (time !=-1 && time > 0)
            time --;
        if (time == 0)
	    done=1;
        /* Check the event queue */
        while (XPending(g.dpy)) {
            XNextEvent(g.dpy, &e);
            check_mouse(&e);
            done |= check_keys(&e);
            done |= check_x_click(&e);
            render();
            render_flag = 1; 
        }
        usleep(1000);
        if (!child && shared->spin == 1)
            render_flag = 1;
        if (render_flag !=0) {
            render();
            render_flag = 0;
        }
    }
    x11_cleanup_xwindows();
    shmdt(shared);              /* detach from segment   */
    if (!child) {
	    shmctl(shmid, IPC_RMID, 0); /* remove shared segment */
    }
    return 0;
}

int get_arguement_count(char **arr){
   
    int i = 0;
    while (arr[i] != NULL) {
        ++i;  
    }
    printf("%i arguements\n",i);
    fflush(stdout);
    return i;
}

void x11_cleanup_xwindows(void)
{
    XDestroyWindow(g.dpy, g.win);
    XCloseDisplay(g.dpy);
}



void* update_child_position() {
    //SharedData* shared_data = (SharedData*)arg;

    while (1) {
        box.hover = 0; //reset the hover?
        if (shared->flag == 1) {
            //grab the shared memory values
            box.x = shared->box_x; //what is the x position?
            box.y = shared->box_y; //what is the y position?
            box.hover = shared->hover; //Is the mouse hovering over the box?
            shared->flag = 0; //reset the flag
            //render();//call render
            render_flag = 1;
        }
        usleep(10000);  // Sleep for 100 milliseconds
       
    }
}

void x11_init_xwindows(void)
{
    int scr;

    if (!(g.dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "ERROR: could not open display!\n");
        exit(EXIT_FAILURE);
    }
    scr = DefaultScreen(g.dpy);
    g.xres = 400;
    g.yres = 200;
    g.win = XCreateSimpleWindow(g.dpy, RootWindow(g.dpy, scr), 1, 1,
                            g.xres, g.yres, 0, 0x00ffffff, 0x00000000);
    XStoreName(g.dpy, g.win, "cs3600 xwin sample"); 
    g.gc = XCreateGC(g.dpy, g.win, 0, NULL);
    XMapWindow(g.dpy, g.win);
    XSelectInput(g.dpy, g.win, ExposureMask | StructureNotifyMask |
                                PointerMotionMask | ButtonPressMask |
                                ButtonReleaseMask | KeyPressMask);
    g.wm_delete_window = XInternAtom(g.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g.dpy, g.win, &g.wm_delete_window, 1);
}


void check_mouse(XEvent *e)
{

   
    static int offsetx, offsety = 0;
    static int inside = 0;
    static int lbuttondown = 0;
    static int count = 0;
    static int savex = 0;
    static int savey = 0;
    int mx = e->xbutton.x;
    int my = e->xbutton.y;

    if (e->type != ButtonPress
        && e->type != ButtonRelease
        && e->type != MotionNotify)
        return;
    if (e->type == ButtonRelease) {
        lbuttondown = 0;
    }
    if (e->type == ButtonPress) {
        if (e->xbutton.button==1) {
            if (inside) {
                lbuttondown = 1;
                offsetx = mx - box.x;
                offsety = my - box.y;
            }
        }
        // set up operations for right click on mouse
        if (e->xbutton.button==3) {

            // right click inside parent box kills child
             if (inside && !child && g.childE) {
                kill(pid, SIGTERM);
                g.childE = 0;
            }
        
            //if inside child box, start rotation
            if (inside && child == 1 && shared->spin == 0){
                shared->spin = 1;
                
            }
            else {
                //if pressed again, stop rotation
                if (inside && child ==1 && shared->spin == 1){
                    shared->spin = 0;
                }

            }
        }
    }
    if (e->type == MotionNotify) {
        if (savex != mx || savey != my) {
            /*mouse :moved*/
            savex = mx;
            savey = my;

            if (!child) {
                if (++count > 20) {
                    printf("p");
                    fflush(stdout); // this is so it outputs live, not saved in buffer
                    count = 0;
                }
            }
            if (child) {
                shared->speed = mx / 50.0;
                if (++count > 20) {
                    printf("c");
                    fflush(stdout);
                    count = 0;
                }
            }
            

	        if (lbuttondown) {
                //if you left click the mouse and the mouse moved.
                    //this saves the box's offset position in respect to the mouse.
                    box.x = mx - offsetx;
                    box.y = my - offsety;
                    if (!child) {
                        //share the parent's box with shared memory so that the child can read them
                        shared->box_x = box.x;
                        shared->box_y = box.y;
                        shared->flag = 1; //tell the child to update?
                    }
                    //return;
               


            }
                   

            inside = 0; //you are not inside the box
            box.hover = 0; //therefore you are not hovering over it
            if (mx > box.x && my < box.x + box.w) {
                if (mx > box.y && my < box.y + box.h) {
                    //you are inside the box.
                    if (++count > 20) {
                        printf("b");
                        fflush(stdout);
                        count = 0;
                    }
                    inside = 1; //you are inside
                    box.hover = 1;
                    //let render know you are inside
                    shared->hover = 1; //tell the child to update?
                }
            }
            else {
                shared->hover = 0; //you are still not inside
            }
        }
    }
    shared->flag = 1; //tell the child to update
}

void create_child()
{

    

    if (g.childE ==0){
        g.childE = 1;
        pid = fork();
    }

    if (pid == 0) {
        child = 1;
        //main();
        //exit(0);
        //we neeed to pass the shmid to the child
        char str[16];
        sprintf(str, "%i", shmid); //convert shmid to a string
        printf("child is recieceving shmid: %s\n", str); //print the shmid that is being passed
        char *arr[4] = {myargv[0], "child", str, NULL}; //add it as the second argument
        execve(myargv[0], arr, myenvp); //and start a child process
    } else {
       

    }
}

int check_keys(XEvent *e)
{
    int key;
    if (e->type != KeyPress && e->type != KeyRelease)
        return 0;
    key = XLookupKeysym(&e->xkey, 0);
    if (e->type == KeyPress) {
        switch (key) {
            case XK_Right:
                // right arrow was pressed
                if (!child)
                    moveWindow(10, 0);
                break;
            case XK_Left:
                if (!child)
                    moveWindow(-10, 0);
                break;
            case XK_c:
                create_child();
                break;
            case XK_g:
                g.foreground_color = 0x00ffbb33;
                break;
            case XK_b:
                g.foreground_color = 0x000000ff;
                break;
            case XK_Escape:
                return 1;
        }
    }
    return 0;
}
void moveWindow(int rx, int ry)
{
    // get current position
    XWindowAttributes xwa;
    Window child;
    Window root = DefaultRootWindow(g.dpy);
    int x, y;
    XTranslateCoordinates(g.dpy, g.win, root, 0, 0, &x, &y, &child);
    XGetWindowAttributes(g.dpy, g.win, &xwa);
    XMoveWindow(g.dpy, g.win, x + rx, y + ry);
}

void drawText(int x, int y, char *str)
{
    XDrawString(g.dpy, g.win, g.gc, x, y, str, strlen(str));
}

void drawLine(int x1, int y1, int x2, int y2)
{

    XDrawLine(g.dpy, g.win, g.gc, x1, y1, x2, y2);

}

void render(void)
{
    // set the foreground color
    XSetForeground(g.dpy, g.gc, g.foreground_color);    // 0x00rrggbb = red, green, blue. we want red
    if(child) {
        XSetForeground(g.dpy, g.gc, 0x00ffbb55);
       
    }
   
    // fill the screen with a rectangle
    XFillRectangle(g.dpy, g.win, g.gc, 0, 0, g.xres, g.yres);

    // display text
    XSetForeground(g.dpy, g.gc, 0x00000000);
    //drawText(10, 16, "This is lab-1");
    if (child == 0) {
        drawText(10, 15, "This is Project Phase-3");
        drawText(10, 30, "Parent window");
        drawText(10, 50, "Drag box with left mouse button");
        drawText(10, 65, "Right-click inside box to close the child.");
        int x = g.xres/3;
        int y = g.yres/3;

        static Myfloat angle = 0.0;
        Myfloat mat[2][2] = {{cos(angle), -sin(angle)}, {sin(angle), cos(angle)}};

        int i;
        Myfloat b[4][2];
        memcpy(b, g.box, sizeof(Myfloat)*4*2);
        for (i =0; i<4; i++){
            b[i][0] = g.box[i][0]*mat[0][0] + g.box[i][1]*mat[0][1];
            b[i][1] = g.box[i][0]*mat[1][0] + g.box[i][1]*mat[1][1];
}

        for (i =0; i<4; i++){
            int j = (i+1) % 4;

        drawLine(b[i][0]+x, b[i][1]+y, b[j][0]+x, b[j][1]+y);
}
        if (shared->spin == 1){
            if(shared->speed >= 0.0)
                angle += (0.001*shared->speed);
        }

    }
    if (child == 1) {
        drawText(10, 15, "This is Project Phase-3");
        drawText(10, 30, "Child window");
        drawText(10, 70, "Right click inside box to start/stop rotation");
        drawText(10, 100, "Move mouse left / right to speed up or slow down spin");

    }
    
    // draw box
    XSetForeground(g.dpy, g.gc, 0x00ffffff);
    XDrawRectangle(g.dpy, g.win, g.gc, box.x, box.y, box.w, box.h);
    //if (box.hover == 1) {
        //XFillRectangle(g.dpy, g.win, g.gc, box.x, box.y, box.w, box.h);
        //XFillRectangle(g.dpy, g.win, g.gc, box.x, box.y, box.w, box.h);
    //}


}

int check_x_click(XEvent *e)
{
    //Code donated courtesy of Taylor Hooser Spring 2023.
    if (e->type != ClientMessage)
        return 0;
    //if x button clicked, DO NOT CLOSE WINDOW
    //instead, overwrite default handler
    if ((Atom)e->xclient.data.l[0] == g.wm_delete_window)
        return 1;
    return 0;
}

