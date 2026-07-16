#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <X11/Xlib.h>
#include <Imlib2.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>

void get_last_used(char *last_used, int size) {
    char *home = getenv("HOME");
    if (home == NULL) {
	printf("Could not determine home directory. Stopping...\n");
	return;
    }
    char statefile[512];
    snprintf(statefile, sizeof(statefile), "%s/.cache/random-walls-last", home);
    FILE *fr = fopen(statefile, "r");
    if (fr == NULL) {
	return;
    }   
    fgets(last_used, size, fr);
    fclose(fr);
}

void save_last_used(char *saved) {
    char *home = getenv("HOME");
    if (home == NULL) {
	printf("Could not determine home directory. Stopping...\n");
	return;
    }
    char statefile[512];
    snprintf(statefile, sizeof(statefile), "%s/.cache/random-walls-last", home);
    FILE *fw = fopen(statefile, "w");
    if (fw == NULL) {
	return;
    }
    fputs(saved, fw);
    fclose(fw);
}

int main() {
    char *home = getenv("HOME");
    if (home == NULL) {
	printf("Could not determine home directory. Stopping...\n");
	return 1;
    }

    srand(time(NULL));

    char wallpapers[512];
    snprintf(wallpapers, sizeof(wallpapers), "%s/Pictures/wallpapers", home);
    DIR *dir = opendir(wallpapers);
    if (dir == NULL) {
	printf("Wallpaper directory does not exist. Shutting down...");
	return 1;
    }

    struct dirent *entry;
    char images[200][256];
    int count = 0;
    
    char last_used[512] = "";
    get_last_used(last_used, sizeof(last_used));
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, last_used) == 0) {
	   continue;
        }
	char *dot = strrchr(entry->d_name, '.');
        if (dot != NULL && (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".png") == 0 || strcmp(dot, ".jpeg") == 0 || strcmp(dot, ".webp") == 0)) {
	    strcpy(images[count], entry->d_name);
	    count++;	    
        }
    }
    closedir(dir);
    
    if (count == 0) {
	printf("No wallpapers are available (folder empty, or only the last used image was found.");
	return 1;
    }

    int random_index = rand() % count;
    save_last_used(images[random_index]);

    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        printf("Could not open X display.\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);
    int num_monitors;
    XineramaScreenInfo *monitors = XineramaQueryScreens(display, &num_monitors);

    imlib_context_set_display(display);
    imlib_context_set_visual(DefaultVisual(display, screen));
    imlib_context_set_colormap(DefaultColormap(display, screen));
    
    char full_path[768];
    snprintf(full_path, sizeof(full_path), "%s/%s", wallpapers, images[random_index]);
    Imlib_Image image = imlib_load_image(full_path);
    if (image == NULL) {
        printf("Could not load image.\n");
        XCloseDisplay(display);
        return 1;
    }

    imlib_context_set_image(image);
    int img_width = imlib_image_get_width();
    int img_height = imlib_image_get_height();
    Pixmap pixmap = XCreatePixmap(display, root, width, height, DefaultDepth(display, screen));
    
    imlib_context_set_drawable(pixmap);
    
    int src_h, src_w, src_x, src_y;

    if (num_monitors < 1) {
	imlib_render_image_on_drawable_at_size(0, 0, width, height);
    }
    else {
	for (int i = 0; i < num_monitors; i++) {
	    float image_aspect = (float)img_width / img_height;
	    float monitor_aspect = (float)monitors[i].width / monitors[i].height;
	    if (image_aspect > monitor_aspect) {
	        src_h = img_height;
	        src_w = (int)(monitor_aspect * img_height);
	        src_x = (img_width - src_w) / 2;
	        src_y = 0;
	    }
	    else {
	        src_w = img_width;
	        src_h = (int)(img_width / monitor_aspect);
	        src_x = 0;
	        src_y = (img_height - src_h) / 2;
	    }
	    imlib_render_image_part_on_drawable_at_size(
	        src_x, src_y, src_w, src_h,
	        monitors[i].x_org, monitors[i].y_org, monitors[i].width, monitors[i].height
	    );
	}
    }
    XSetWindowBackgroundPixmap(display, root, pixmap);
    
    Atom prop_root = XInternAtom(display, "_XROOTPMAP_ID", False);
    Atom prop_esetroot = XInternAtom(display, "ESETROOT_PMAP_ID", False);
    
    XChangeProperty(display, root, prop_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&pixmap, 1);
    XChangeProperty(display, root, prop_esetroot, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&pixmap, 1);

    XClearWindow(display, root);
    XFlush(display);

    imlib_free_image();
    XFree(monitors);

    XSetCloseDownMode(display, RetainPermanent);
    XCloseDisplay(display);
    return 0;
}
