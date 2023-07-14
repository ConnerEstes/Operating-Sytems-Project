//Name: Conner Estes
//Date: 04/24/23
//ASSGN: Project Phase 2
//File Name: xyproj2.c


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <X11/Xatom.h>
//#include <sys/types.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>



//int pid;
//int limit;
struct Global {
    Display *dpy;
    Window win;
    GC gc;
    int xres, yres;
    int pos[2];
    int mbox;
    int childE;
    Atom wm_delete_window;


} g;


/*typedef struct {
    int flag;
    int box_x;
    int box_y;
} SharedData;

struct SharedBox {
    int box_x, box_y;
    int h, w;
    int hover;
    int kclick;
    int flag;
}  shbox;*/



struct Box {
    int x, y;
    int h, w;
    int hover;
} box;

#define NUM_THRDS 1
int pid;
pthread_t callThd[NUM_THRDS];
int check_x_click(XEvent *e);
void x11_cleanup_xwindows(void);
void x11_init_xwindows(void);
void x11_clear_window(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void render(void);
int get_arguement_count(char **);
void *thread(void *arg);

char **my_argv;
char **my_envp;

int child = 0;
int pipefd[2];
int fill;


void *thread(void *arg )
{

    struct Global tmp;
    while(read(pipefd[0], &tmp, sizeof(tmp))> 1){
       g.pos[0] = tmp.pos[0];
       g.pos[1] = tmp.pos[1];
       g.mbox = tmp.mbox;

    }
       if (child){
           g.pos[0] = tmp.pos[0];
           g.pos[1] = tmp.pos[1];
           render();
       }

    pthread_exit((void *)0);
    return 0;
}
int main(int argc, char  *argv[], char *envp[])
{   

    //shared = &shbox;
    int counter =0;
    int time = -1;
    argc = 0;
    my_argv = argv;
    my_envp = envp;
    // get the arguement count
    argc = get_arguement_count(argv);
    g.mbox = 1;
    my_argv = argv;
    my_envp = envp;
    if (argc >= 4) {
        printf("\n I am the Child! \n");
        child = 1;
        pipefd[0] = atoi(argv[2]);
        pipefd[1] = atoi(argv[3]);
        pthread_t child_thread;
        pthread_create(&child_thread, NULL, thread, NULL);
        
    }
    

    if(argc > 1 && child == 0) {
        time = atoi(argv[1]);
        if (time == 0)
            time =-1;
    }

   /* if (child == 0) {



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
    } */






    //shared->hover = 0;
    //shared->flag = 0;

    XEvent e;
    int done = 0;
    x11_init_xwindows();
    extern void init_pos(int *, int, int); //Prototype
    init_pos(g.pos, g.xres, g.yres);
    box.x = g.xres/2;
    box.y = g.yres/2;
    box.w = box.h = 40;
    //shared->box_x = box.x;
    //shared->box_y = box.y;

    while (!done) {
        if (time != -1 && time > 0)
            time--;
        if (time == 0)
            done = 1;
        /* Check the event queue */
        while (XPending(g.dpy)) {
            XNextEvent(g.dpy, &e);
            check_mouse(&e);
            done |= check_keys(&e);
            done |= check_x_click(&e);
            render();
        }
        render();
        usleep(4000);
        counter++;
    }
    x11_cleanup_xwindows();
    if (pid == 0)
        printf("\n main ending for child\n");
    return 0;
}

void init_pos(int *pos, int xres, int yres) {
    pos[0] = xres / 2;
    pos[1] = yres / 2;
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


void drawRect(int x, int y, int w, int h )
{ 
    XFillRectangle(g.dpy, g.win, g.gc, x, y, w, h);
}
void Text(int x, int y, const char *str)
{
    XDrawString(g.dpy, g.win, g.gc, x, y, str, strlen(str));
}
void check_mouse(XEvent *e)
{
    static int offsetx,offsety =0;
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
                //inside =1;
                offsetx = mx - g.pos[0];
                offsety = my - g.pos[1];
            }

            
        }

        }
        if (e->xbutton.button==3) {
            int mx = e->xbutton.x;
            int my = e->xbutton.y;
            int inside = (mx > g.pos[0] && mx < g.pos[0] + box.w) && (my > g.pos[1] && my < g.pos[1] + box.h);
            if (inside && !child && g.childE) {
                kill(pid, SIGTERM);
                g.childE = 0;
            }
       
       
        }
    


    if (e->type == MotionNotify) {
        if (savex != mx || savey != my) {
            /*mouse moved*/
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
                    if(!child) {
                        g.pos[0] = mx;  //was =mx
                        g.pos[1] = my;  //was =my
                        write(pipefd[1], &g, sizeof(g));
            }

                   
            }

           

            
            inside = 0; //you are not inside the box
            box.hover = 0; //therefore you are not hovering over it
            if (mx > g.pos[0] && mx < g.pos[0] + box.w) {
                if (my > g.pos[1] && my < g.pos[1] + box.h) { 
                    //inside = 1;
                    //you are inside the box.
                    if (++count > 20) {
                        printf("b");
                        fflush(stdout);
                        count = 0;
                    }
                    inside = 1; //you are inside
                    box.hover = 1;
                    //let render know you are inside
                    //shared->hover = 1; 
                }
                
            }
     
        
   

}
}
}




/*void* update_child_position() {


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
        usleep(10000);  // Sleep for 10000 milliseconds

    }
} */


void make_child_process()
{
    if (g.childE == 0) {
        g.childE = 1;
        pipe(pipefd);
        pid = fork();
        if (pid == 0) {
            //child
            char fd1[16];
            char fd2[16];
            sprintf(fd1,"%i", pipefd[0]);
            sprintf(fd2,"%i", pipefd[1]);
            char *chargv[5] = {my_argv[0],"C",fd1, fd2, NULL};
            execve(my_argv[0], chargv, my_envp);
            render();

        } else {
            //parent
            //Thread Creation inside the parent
            for(long i = 0; i < NUM_THRDS; i++) {
                pthread_create(&callThd[i], NULL, thread, (void *)i);
            }
        }
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
            case XK_1:
                break;
            case XK_c:
                if(!child) {
                    if(!g.childE) {
                    make_child_process();
                    } else {
                        kill(pid, SIGTERM);
                        g.childE = 0;
                    }
                }
                break;
            case XK_Escape:
                return 1;
        }
    }
    return 0;
}

void render(void)
{


    
    int y = 16;
    if(child) {
        XSetForeground(g.dpy, g.gc, 0x00ffbb55);
        //drawRect(0,0,g.xres, g.yres);
        XFillRectangle(g.dpy, g.win, g.gc, 0, 0, g.xres, g.yres);
        XSetForeground(g.dpy, g.gc, 0xffffffff);
        Text(12, y, "This is Project Phase-2");
        y += 12;
        Text(12, y, "Child Window");
        y += 12;

    }
    else {
        XSetForeground(g.dpy, g.gc, 0x00ff00ff);
        //drawRect(0,0,g.xres, g.yres);
        //XDrawRectangle(g.dpy,g.win,g.gc,box.x,box.y,box.w,box.h);
        XFillRectangle(g.dpy, g.win, g.gc, 0, 0, g.xres, g.yres);
        XSetForeground(g.dpy, g.gc, 0xffffffff);
        Text(12, y, "This is Project Phase-2");
        y += 12;
        Text(12, y, "Parent window");
        y += 12;
        Text(12, y, "Press 'C' for child window");
        y += 12;
        Text(12, y, "Drag box with left mouse click");
        y += 12;
        Text(12, y, "Right click inside box to close child");
        y += 12;

        

    }


    
    

    XSetForeground(g.dpy, g.gc, 0xffffffff);
    XDrawRectangle(g.dpy, g.win, g.gc, g.pos[0], g.pos[1], box.w,box.h );
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

