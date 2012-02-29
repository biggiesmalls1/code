/*  Spectrum Analyzer Visualization Plugin for Audacious
 *
 *  Copyright (C) 2006 William Pitcock
 *  Copyright (C) 1998-2001 V�gv�lgyi Attila, Peter Alm, Mikael Alm,
 *    Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// Choose which media player we are compiling for:
#define XMMS
// #define AUDACIOUS
// TODO: There might be an already existing way to find out!

// #define AIRFLOW

// Without transparency, skipping every other frame is fine for XMMS, but visibly slow in Audacious.
// If MASK_TRANSPARENCY is enabled for Audacious, then I must skip AT LEAST 2 frames to keep my system stable.
#define SKIP_FRAMES 2

//// MASK_TRANSPARENCY works in XMMS, but is way too slow in Audacious.

//// For debugging: does not make the tops of the bars spikey, so user can see the original spectrum bars.
// #define BARS

#define DEBUG(X,Y); 
// TODO: #define DEBUG(X,Y); fprintf(stdout,X,Y);

//// Pick one or none, not both!
//// Neither tested for Audacious
#define MASK_TRANSPARENCY
// #define PSEUDO_TRANSPARENCY
// WARNING! This can create a lot of X refresh events, and make your X unresponsive!
// Wow this works and it was not really too hard! :D
// But it's still a bit heavy on CPU.  It would be good to make
// PSEUDO_TRANSPARENCY, which would take a snapshot of the desktop/window
// beneath us at the beginning of each song, and use this is the backing bitmap
// instead of using a mask.
// This would save X from constantly forcing windows behind the fire to redraw,
// which can be a real drain on CPU.
// We could even fade the two bitmaps together, to make the tops of the flames
// semi-transparent.
// I don't actually know how to grab a screenshot.  I know amarok does it for
// its OSD.  xosd uses only a mask.  We could steal an example from xscreensaver.
// Hmm xscreensaver uses X libraries, audacious uses gtk2.
// Occasionally you may see the windows behind the flame flickering, or
// pixels above the flame flicker black when the flame drops.  This is not my
// fault!  I think that's X not re-drawing those pixels quickly enough.  ;)

// TODO: We need a way to reduce load on the system.
// Maybe we can do a little sleep, to give the rest of the system some CPU.
// Or maybe we should detect when CPU is overloaded, and skip rendering a frame.

#include "config.h"

#include <gtk/gtk.h>
#include <math.h>
#include <gdk/gdkx.h> // Provides GDK_ROOT_WINDOW() for PSEUDO_TRANSPARENCY.

#ifdef AUDACIOUS
#include <audacious/plugin.h>
#include <audacious/i18n.h>
#include "logo.xpm"
#endif

#ifdef XMMS
#include "xmms/plugin.h"
#include "libxmms/util.h"
#include "xmms_logo.xpm"
#include "xmms/i18n.h"
#define logo_xpm sanalyzer_xmms_logo_xpm
#endif

#ifdef AIRFLOW
	// One of these provides random():
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

/* This isn't just pretty.  It is still a spectrum analyser.  It retains the
 * same values as the original plugin, adding colour. */

// DONE dirtily: interpolation when bar is >= 2 pixels
// TODO: smooth (fade to smoke?) the tops of lines?  (May not be needed after interpolation.)
// TODO: flame is too boring on soft tracks, and maybe a little bit too excited on some crazy tracks
//       we don't want to fully normalise this, since the tracks do deserve it, we just want to reduce the inter-track variance
//       the spikes are desirable on very soft tracks
// TODO: unsmoothed tiny spikes are ok but unrealistic along the whole length of the flame
//       but well smoothed spikes are equally unrealistic.  we need a mixture!
//       maybe vary the smoothing as we cross the flame, creating some regions of spikiness, others smooth.
//       But if there is very little going on, we really want to display any spikes that do exist at full resolution
// TODO: make palette, and window size, user configurable

/* SPECWIDTH should be kept 256, this is the hardwired resolution of the
   spectrum given by XMMS. */
#define SPECWIDTH 256

/* FLAMEHEIGHT scales the height of the flames, and the colour buffer.
   I have seen flames actually reach 1.7*FLAMEHEIGHT pixels in height.
   If you increase FLAMEHEIGHT without increasing WINHEIGHT, the tops of noisy
   flames may get clipped. */
#define FLAMEHEIGHT 128
// #define FLAMEHEIGHT (WINHEIGHT/1.75)

/* Width 550 fits nicely over a double-size amp.  274 over normal size amp.
   TODO: Make this user configurable, either in preferences or by resizing the
   window manually, so they can place the flame over a single size-amp, or
   elsewhere on their desktop at their desired width.  If we do this, we should
   also disable ORGANIC_INTERPOLATION because it fails at high-res (until we
   add interpolation to it!  We should do something because hi-res looks too
   ... interpolated. ;P)
   If we increase WINWIDTH we should probably increase LOOKAHEAD also, and the
   way heatHere is calculated. */
// #define WINWIDTH 256
// #define WINWIDTH 274
#define WINWIDTH 550
// #define WINWIDTH 1024
// #define WINWIDTH 1200

/* Height 224 should prevent clipping of the tops of flames when FLAMEHEIGHT=128.
   TODO: Users may wish to change the window's height.  If they do I'm guessing
   they would like us to scale FLAMEHEIGHT for them also. */
#define WINHEIGHT 224

/* Linearity of the amplitude scale (0.5 for linear, keep in [0.1, 0.9]) */
#define d 0.33

/* Time factor of the band dinamics. 3 means that the coefficient of the
   last value is half of the current one's. (see source) */
#define tau 3

/* Factor used for the diffusion. 4 means that half of the height is
   added to the neighbouring bars */
#define dif 4

/* Parameters and functions for fire colouring. */
//// I actually prefer the non-true interpolation because it feels more organic, but it does not work at higher resolutions.  (At low-res it puts a slight curve on the flames, but as high-res it exhibits the flat graph underneath, unless we interpolate that input.)
#define ORGANIC_INTERPOLATION
//// XSCALE() Converts WINWIDTH to SPECWIDTH (maps window x onto bar_heights[]):
// #define XSCALE(i) (int)(i*(float)SPECWIDTH/(float)WINWIDTH)
// #define XSCALE(i) (int)(i*(float)SPECWIDTH/(float)WINWIDTH*0.7)
#define XSCALE(i) (int)((float)SPECWIDTH*dropEnds(doLog((float)(i)/(float)WINWIDTH)))
//// Modify the x scale?
#define doLog(x) (x)
// #define doLog(x) pow(x,1.6)
#define dropEnds(f) (f*0.7)
// #define dropEnds(f) (f)
// #define dropEnds(f) (0.2+0.6*(float)(f))

static GtkWidget *window = NULL,*area;
static GdkPixmap *bg_pixmap = NULL, *draw_pixmap = NULL, *bar = NULL;
static GdkGC *gc = NULL;
static gint16 bar_heights[SPECWIDTH];
/*static gint timeout_tag;*/
static gdouble scale, x00, y00;
static gdouble heatNow;
#ifdef MASK_TRANSPARENCY
static GdkGC *mgc = NULL; // gc for mask
GdkBitmap *mask;
#endif
#ifdef PSEUDO_TRANSPARENCY
// static XImage *orig_map;
// static Drawable *orig_image;
// static GdkDrawable *orig_image;
static GdkPixmap *background;
#endif

#define FitInt(I,MAX) ( I<0 ? 0 : I>MAX ? MAX : (int)I )

#ifdef SKIP_FRAMES
static gint frameCount;
#endif

#ifdef AIRFLOW
// Amount of air received from left/right/above/below cell at this moment, for each pixel:
static float airflow_left[WINWIDTH][WINHEIGHT];
static float airflow_right[WINWIDTH][WINHEIGHT];
static float airflow_up[WINWIDTH][WINHEIGHT];
static float airflow_down[WINWIDTH][WINHEIGHT];
static float current_heat[WINWIDTH][WINHEIGHT];
// Derived:
// #define airflow_pressure(x,y) ( x>=1 && x<WINWIDTH-1 && y>=1 && y<WINHEIGHT-1 ? airflow_weight[WINWIDTH][WINHEIGHT]
#define airflow_pressure(x,y) ( airflow_weight[FitInt(x,WINWIDTH)][FitInt(y,WINHEIGHT)]
/* AirflowStats airflow_stats = { .  } */
#endif

#ifdef XMMS
#define fsanalyzer_vp              sanalyzer_vp
#define fsanalyzer_init            sanalyzer_init
#define fsanalyzer_cleanup         sanalyzer_cleanup
#define fsanalyzer_playback_start  sanalyzer_playback_start
#define fsanalyzer_playback_stop   sanalyzer_playback_stop
#define fsanalyzer_render_freq     sanalyzer_render_freq
#define fsanalyzer_destroy_cb      sanalyzer_destroy_cb
#endif

static void fsanalyzer_init(void);
static void fsanalyzer_cleanup(void);
static void fsanalyzer_playback_start(void);
static void fsanalyzer_playback_stop(void);
static void fsanalyzer_render_freq(gint16 data[2][256]);

VisPlugin fsanalyzer_vp = {
	.description = "Fiery Spectrum Analyzer",
	.num_pcm_chs_wanted = 0,
	.num_freq_chs_wanted = 1,
	.init = fsanalyzer_init, /* init */
	.cleanup = fsanalyzer_cleanup, /* cleanup */
	.playback_start = fsanalyzer_playback_start, /* playback_start */
	.playback_stop = fsanalyzer_playback_stop, /* playback_stop */
	.render_freq = fsanalyzer_render_freq  /* render_freq */
};

#ifdef XMMS
VisPlugin *get_vplugin_info(void)
{
	sanalyzer_vp.description =
		g_strdup_printf(_("Fiery spectrum analyzer %s"), VERSION);
	return &sanalyzer_vp;
}
#endif

VisPlugin *spectrum_vplist[] = { &fsanalyzer_vp, NULL };

#ifdef AUDACIOUS
DECLARE_PLUGIN(spectrum, NULL, NULL, NULL, NULL, NULL, NULL, spectrum_vplist,NULL);
#endif

static void fsanalyzer_destroy_cb(GtkWidget *w,gpointer data) {
	fsanalyzer_vp.disable_plugin(&fsanalyzer_vp);
}

/*
static int max(int a,int b) {
	return ( a>b ? a : b );
}

static int min(int a,int b) {
	return ( a<b ? a : b );
}

static float fclamp(float val, float min, float max) {
	return ( val<min ? min : val>max ? max : val );
}
*/

#ifdef MASK_TRANSPARENCY
GdkBitmap *create_transparency_mask(GdkWindow *window) {
	GdkBitmap *mask = NULL;
	GdkColor pattern;
	// GdkPoint *gpoints;
	// gint numpoints;

	mask = gdk_pixmap_new(window, WINWIDTH, WINHEIGHT, 1);

	mgc = gdk_gc_new(mask);

	/*

	pattern.pixel = 0;
	gdk_gc_set_foreground(mgc, &pattern);
	gdk_draw_rectangle(mask, mgc, TRUE, 0, 0, WINWIDTH, WINHEIGHT);

	numpoints = 4;
	gpoints = g_malloc(numpoints * sizeof (GdkPoint));
	gpoints[0].x = 0          ; gpoints[0].y = 0;
	gpoints[1].x = WINWIDTH   ; gpoints[1].y = 0;
	gpoints[2].x = WINWIDTH   ; gpoints[2].y = WINHEIGHT;
	gpoints[3].x = 0          ; gpoints[3].y = WINHEIGHT;
	// gpoints[4].x = 0 ; gpoints[4].y = 0;

	pattern.pixel = 1;
	gdk_gc_set_foreground(mgc, &pattern);
	gdk_draw_polygon(mask, mgc, TRUE, gpoints, numpoints);

	g_free(gpoints);

	*/

	pattern.pixel = 1;
	gdk_gc_set_foreground(mgc, &pattern);
	gdk_draw_rectangle(mask, mgc, TRUE, 0, 0, WINWIDTH, WINHEIGHT);

	return mask;
}
#endif

#ifdef PSEUDO_TRANSPARENCY
// Nabbed from Audacious - may be gtk2.
static Pixmap* take_snapshot() {
	/*
	Pixmap pixmap;
	GC gc;
	// create a pixmap to hold the screenshot.
	pixmap = XCreatePixmap(window->dpy, window, WINWIDTH, WINHEIGHT,
			DefaultDepth(window->dpy, DefaultScreen(window->dpy)));
	// then copy the screen into the pixmap.
	gc = XCreateGC(window->dpy, pixmap, 0, NULL);
	XSetSubwindowMode(window->dpy, gc, IncludeInferiors);
	XCopyArea(window->dpy, DefaultRootWindow(window->dpy), pixmap, gc,
			window->x, window->y, WINWIDTH, WINHEIGHT,
			0, 0);
	XSetSubwindowMode(window->dpy, gc, ClipByChildren);
	XFreeGC(window->dpy, gc);
	return pixmap;
	*/

	//// We grab a snapshot of the desktop/windows behind our window, to use as background.
	// orig_map = XGetImage(dpy, window, 0, 0, WINWIDTH, WINHEIGHT, ~0L, ZPixmap);
	// orig_image = ((GdkWindow*)window->window)->get_image(0,0,WINWIDTH,WINHEIGHT);
	background = gdk_pixmap_new(window->window,WINWIDTH,WINHEIGHT,gdk_rgb_get_visual()->depth);
	//// gdk_draw_pixmap(background, gc, GTK_WINDOW(window)->get_default_root_window(), 0, 0, 0, 0, WINWIDTH, WINHEIGHT);
	//// gdk_draw_pixmap(background, gc, Gdk::Window.default_root_window, 0, 0, 0, 0, WINWIDTH, WINHEIGHT);
	//// gdk_draw_pixmap(background, gc, GTK_WINDOW(window)->RootWindow(), 0, 0, 0, 0, WINWIDTH, WINHEIGHT);
	//// gdk_draw_pixmap(background, gc, DefaultRootWindow(display), 0, 0, 0, 0, WINWIDTH, WINHEIGHT);
	gdk_draw_pixmap(background, gc, draw_pixmap, 0, 0, 0, 0, WINWIDTH, WINHEIGHT);
	// gdk_draw_pixmap(background, gc, gdk_window_foreign_new(GDK_ROOT_WINDOW()), 0, 0, 0, 0, WINWIDTH, WINHEIGHT);

	// TODO: OK so we grabbed something, but it's not the desktop!
	//       I see a copy of the default equalizer skin bitmap!
	//       Yeah we seem to be grabbing bits of memory, sometimes dirty.
	// I tried completely masking the vis window before copying the snapshot, but
	// we still ended up with bad bitmap data.
	return ((Pixmap*)((int)background));
}
#endif

static void fsanalyzer_init(void) {
	GdkColor palette[5];
	GdkColor color;
	int i;

	if(window)
		return;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Spectrum Analyzer"));
	gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
	// NEW! Joey's stuff:
	// gtk_window_set_resizable(GTK_WINDOW(window), TRUE); // worked ok, but isn't desirable until we support it!
	// gtk_window_set_decorated(GTK_WINDOW(window), FALSE); // failed, the window still has a titlebar+frame.
	/*
	//// TODO:
	//// I wanted to set the initial position of the fire spectrum window to WINHEIGHT pixels above the main window.
	//// But I could not work out how to get the main window!
	//// Ah well I finally managed to get my window manager to remember it's position, so this is less important now.  I hope other users can achieve it that way too.
	// parent = GTK_WINDOW(window)->parent;
	// parent = gtk_widget_get_parent_window(GTK_WINDOW(window));
	// parent = gtk_window_get_transient_for(GTK_WINDOW(window));
	GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(window));
	GList* glist = gdk_screen_get_toplevel_windows(GDK_SCREEN(screen));
	parent = (GtkWindow*)g_list_first(glist);
	parent = glist->data;
	// parent = g_list_next(glist)->data;
	// parent = gdk_get_default_root_window();
	// parent = gtk_widget_get_parent_window(GTK_WIDGET(mainwin));
	if (parent==0) {
		g_printf("Failed to get parent window!\n");
	}
	// g_printf("fsanalyzer_vp.mainwin = %d\n",fsanalyzer_vp.mainwin);
	// } else {
		// gtk_window_get_position(GTK_WINDOW(parent), &wx, &wy);
		// gdk_window_get_position(GDK_WINDOW(parent), &wx, &wy);
		gdk_window_get_origin(GDK_WINDOW(parent), &wx, &wy);
		g_printf("got x=%d y=%d\n",wx,wy);
		gtk_window_move(GTK_WINDOW(window), wx, wy-WINHEIGHT);
	// }
	*/

#ifdef MASK_TRANSPARENCY
	mask = create_transparency_mask(window->window);
	gtk_widget_shape_combine_mask(window,mask,0,0);
#endif

#ifdef PSEUDO_TRANSPARENCY
	take_snapshot();
#endif

	gtk_widget_realize(window);

	bg_pixmap = gdk_pixmap_create_from_xpm_d(window->window,NULL,NULL,logo_xpm);
	gdk_window_set_back_pixmap(window->window,bg_pixmap,0);
#ifdef XMMS
	gtk_signal_connect(GTK_OBJECT(window),"destroy",GTK_SIGNAL_FUNC(sanalyzer_destroy_cb),NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &window);
	gtk_widget_set_usize(window, WINWIDTH, WINHEIGHT);
#else
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(fsanalyzer_destroy_cb),NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_widget_destroyed), &window);
	gtk_widget_set_size_request(GTK_WIDGET(window), WINWIDTH, WINHEIGHT);
#endif
	gc = gdk_gc_new(window->window);
	draw_pixmap = gdk_pixmap_new(window->window,WINWIDTH,WINHEIGHT,gdk_rgb_get_visual()->depth);

	// Our new bar pixel buffer is 3.5x as tall as the target window.
   // The first FLAMEHEIGHT pixels are black, and probably no longer accessed.
   // The second FLAMEHEIGHT pixels are our flame palette.
   // The third FLAMEHEIGHT*1.5 pixels are buffer pixels beyond the end of the
   // palette, when the flame is full white.
	bar = gdk_pixmap_new(window->window,25, FLAMEHEIGHT*3.5, gdk_rgb_get_visual()->depth);

	#define palScale 0.9
	//// Red and orange flame
	#define stages 5
	// A hint of blue in the bright "white" makes it even brighter.  Although my eyes cannot see the blue, they actually notice a red stripe where yellow meets white.
	// palette[0].red = 0xFF44; palette[0].green = 0xFF44; palette[0].blue = 0xFFFF;
	palette[0].red = 0xFF77; palette[0].green = 0xFF77; palette[0].blue = 0xCCCC;
	palette[1].red = 0xFF77; palette[1].green = 0xEEEE; palette[1].blue = 0x4444;
	palette[2].red = 0xFF77; palette[2].green = 0xBBBB; palette[2].blue = 0x0000;
	palette[3].red = 0xDDDD; palette[3].green = 0x5555; palette[3].blue = 0x0000;
	palette[4].red = 0x8888; palette[4].green = 0x0888; palette[4].blue = 0x0000;
	// We want a lick of red, then orange quickly moving to a strong yellow
	// But I think I have the scales wrong, I always have a significant band of dark orange.
	// The alternative to increasing MINCOL:
	#define palDelta 0.3
	// Unfortunately, now that we are using the whole range, we do not get the bright white candle areas!
	// This makes the last 0.3 of the palette static!

	/*
	//// Blue flame
	#define palScale 0.9
	//// This palette may prefer MINCOL = FLAMEHEIGHT*0.4, EXPLOSION=1.0, with a lower LOOKAHEAD.
	#define stages 4
	palette[0].red = 0xFFFF; palette[0].green = 0xFFFF; palette[0].blue = 0xEEEE;
	palette[1].red = 0x0000; palette[1].green = 0x8888; palette[1].blue = 0xCCCC;
	// palette[2].red = 0x0000; palette[2].green = 0x4444; palette[2].blue = 0x8888;
	// palette[2].red = 0x0077; palette[2].green = 0x2222; palette[2].blue = 0x7777;
	// palette[3].red = 0x0000; palette[3].green = 0x0000; palette[3].blue = 0x1111;
	palette[2].red = 0x0000; palette[2].green = 0x0000; palette[2].blue = 0x1111;
	palette[3].red = 0x0000; palette[3].green = 0x0000; palette[3].blue = 0x0011;
	#define palDelta 0.6
	*/

	/*
	//// blue-red-yellow-white - doesn't quite look right, especially the purple!
	#define stages 5
	palette[0].red = 0xFFFF; palette[0].green = 0xFFFF; palette[0].blue = 0xFFFF;
	palette[1].red = 0xEEEE; palette[1].green = 0xBBBB; palette[1].blue = 0x0000;
	palette[2].red = 0xBBBB; palette[2].green = 0x3333; palette[2].blue = 0x0000;
	palette[3].red = 0x5555; palette[3].green = 0x0000; palette[3].blue = 0x0000;
	palette[4].red = 0x0000; palette[4].green = 0x0000; palette[4].blue = 0x2222;
	#define palDelta 0.15
	*/

	for(i = 0; i < 3.5*FLAMEHEIGHT; i++) {
		float thruouter,thruinner;
		int pfrom,pto;
		//// palDelta is the proportion of the start of the palette which we drop - the rest is scaled to fit.
		//// This is because we usually don't really want very much of the first colour, just the end of its transition to the next.
		thruouter = palDelta + (1.0-palDelta)*(float)(i+1 - FLAMEHEIGHT)/(float)FLAMEHEIGHT / palScale; // the +1 because rather thruouter==1 than ==0!
		// if (thruouter<0) thruouter=0;
		// if (thruouter>1) thruouter=1;
		if (thruouter<=0) {
			// TODO: allocate these repeated colours only once - or will X handle this gracefully anyway?
			color.red = palette[stages-1].red;
			color.green = palette[stages-1].green;
			color.blue = palette[stages-1].blue;
		} else if (thruouter>=1.0) {
			color.red = palette[0].red;
			color.green = palette[0].green;
			color.blue = palette[0].blue;
		} else {
			thruouter = 1.0 - thruouter;
			thruinner = thruouter*(stages-1) - (int)(thruouter*(stages-1));
			pfrom = thruouter * (stages-1);
			pto = pfrom + 1;
			color.red = palette[pfrom].red + thruinner*(float)(palette[pto].red - palette[pfrom].red);
			color.green = palette[pfrom].green + thruinner*(float)(palette[pto].green - palette[pfrom].green);
			color.blue = palette[pfrom].blue + thruinner*(float)(palette[pto].blue - palette[pfrom].blue);
		}
		gdk_color_alloc(gdk_colormap_get_system(),&color);
		gdk_gc_set_foreground(gc,&color);
		gdk_draw_line(bar,gc,0,i,24,i);
	}

	scale = FLAMEHEIGHT / ( log((1 - d) / d) * 2 );
	x00 = d*d*32768.0/(2 * d - 1);
	y00 = -log(-x00) * scale;

/* d=0.2, FLAMEHEIGHT=128
	scale = 46.1662413084;
	x00 = -2184.53333333;
	y00 = -354.979500941;
*/

	heatNow = 0.0;

	gdk_color_black(gdk_colormap_get_system(),&color);
	gdk_gc_set_foreground(gc,&color);

	area = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window),area);
	gtk_widget_realize(area);
	gdk_window_set_back_pixmap(area->window,bg_pixmap,0);

	gtk_widget_show(area);
	gtk_widget_show(window);
	gdk_window_clear(window->window);
	gdk_window_clear(area->window);

}

static void fsanalyzer_cleanup(void) {
	if(window) {
		gtk_widget_destroy(window);
	}
	if(gc) {
		gdk_gc_unref(gc);
		gc = NULL;
	}
#ifdef MASK_TRANSPARENCY
	if(mgc) {
		gdk_gc_destroy(mgc);
		// gdk_gc_unref(mgc); // failed - i guess we can't do both ;)
		mgc = NULL;
	}
#endif
	if(bg_pixmap) {
		gdk_pixmap_unref(bg_pixmap);
		bg_pixmap = NULL;
	}
	if(draw_pixmap) {
		gdk_pixmap_unref(draw_pixmap);
		draw_pixmap = NULL;
	}
	if(bar) {
		gdk_pixmap_unref(bar);
		bar = NULL;
	}
}

static gint draw_func(gpointer data) {
	gint i;
	gdouble heatHere;
	gint lasty;
#ifdef MASK_TRANSPARENCY
	GdkColor pattern;
#endif

#ifdef SKIP_FRAMES
	frameCount++;
	if (frameCount%SKIP_FRAMES > 0) {
		return TRUE;
	}
#endif

	/* FIXME: should allow spare redrawing like the vis. in the main window */
	if(!window) {
/*		timeout_tag = 0;*/
		return FALSE;
	}

	GDK_THREADS_ENTER();

#ifndef AIRFLOW
#ifdef PSEUDO_TRANSPARENCY
	gdk_draw_pixmap(draw_pixmap, gc, background, 0, 0, 0, 0, WINWIDTH, WINHEIGHT);
#else
	gdk_draw_rectangle(draw_pixmap, gc, TRUE, 0, 0, WINWIDTH, WINHEIGHT);
	// TODO: for efficiency, we don't really need to do this when using MASK_TRANSPARENCY either.
#endif
#endif

#ifdef MASK_TRANSPARENCY
	pattern.pixel = 0;
	gdk_gc_set_foreground(mgc, &pattern);
	gdk_draw_rectangle(mask, mgc, TRUE, 0, 0, WINWIDTH, WINHEIGHT);
	pattern.pixel = 1;
	gdk_gc_set_foreground(mgc, &pattern);
#endif

	// heatHere = FLAMEHEIGHT/4;
	// heatHere = (bar_heights[0] + bar_heights[4] + bar_heights[8] + bar_heights[12]) / 4;
	heatHere = ( // cheeky initial "average" from the first half of the spectrum
			+ bar_heights[SPECWIDTH*0/16] + bar_heights[SPECWIDTH*1/16] + bar_heights[SPECWIDTH*2/16] + bar_heights[SPECWIDTH*3/16]
			+ bar_heights[SPECWIDTH*4/16] + bar_heights[SPECWIDTH*5/16] + bar_heights[SPECWIDTH*6/16] + bar_heights[SPECWIDTH*7/16]
		) / 8;

	// around 3 seconds to update ???  What are units and limits? :P
	//
	// Should we be using oldHeatHere instead of heatNow on the RHS?
	heatNow = heatNow*0.99 + (-FLAMEHEIGHT/32+1.5*heatHere)*0.01;

	// heatHere = 0;
	// heatHere = FLAMEHEIGHT/16; // When using true heatHere mean method

	lasty = WINHEIGHT-1;
	for(i = 0; i < WINWIDTH; i++) {

		// Black vertical stripes:
		// if ((i%8) == 0)
			// continue;

		//// I tried to make the bars taller on the left.
		// #define scaleybyx(x) ( 1.5 - 0.5*(float)x/(float)WINWIDTH )
		//// But what I really want to do is make the very leftmost bar stronger.
		//// This is the one which represents the real low bass, and is just unrepresentative to the sound when it is really small.
		#define scaleybyx(x) 1

		int y,cy;
		y = WINHEIGHT-1 - bar_heights[XSCALE(i)]*scaleybyx(i) - 2;
		// TODO: I would rather y went from 0.  We can do the WINHEIGHT-1 - y later!

		#ifdef ORGANIC_INTERPOLATION

			/*
			if (i==0)
				// lasty = y + (random()%31)-15;
				lasty = WINHEIGHT-1 - bar_heights[XSCALE(i)]/2 - 2;
			*/

			float spikiness;
			spikiness = 0.5;

			#ifdef BARS
				spikiness = 1.0;
			#endif

			y = lasty*(1.0-spikiness) + y*spikiness;
			lasty = y;
			//// TODO:
			//// Attempts to fiddle around with interpolation have a nasty habit of exhibiting artefacts resulting from the face that the input is a set of bars (not interpolated yet).
			//// Again we are using the dirty lop-sided averaging, and this time with heights not colours.
			//// Maybe better to calculate the true mean here.

		#else

			// True interpolation (albeit inefficient :P )
			int left = (int)((float)SPECWIDTH*0.7*(float)(i)/(float)WINWIDTH);
			int right = left + 1;
			float thru = ((float)SPECWIDTH*0.7*(float)(i)/(float)WINWIDTH) - left;
			float yleft = WINHEIGHT-1 - bar_heights[left] - 3;
			float yright = WINHEIGHT-1 - bar_heights[right] - 3;
			y = yleft*(1.0-thru) + yright*thru;

		#endif

		//// Update heatHere:

		//// This is a cheap way to approximate the heatHere mean, but it produces good results (localised and spread):
		//// If you increase LOOKAHEAD, you should also reduce GAIN accordingly, to calibrate phase on the x-axis.
		// #define LOOKAHEAD 24
		// #define GAIN 0.005
		#define LOOKAHEAD 6
		#define GAIN 0.04
		// #define LOOKAHEAD 3
		// #define GAIN 0.07

		#ifdef BARS
			#undef LOOKAHEAD
			#undef GAIN
			#define LOOKAHEAD 0
			#define GAIN 1.0
		#endif

		if (i+LOOKAHEAD<WINWIDTH)
			heatHere = heatHere*(1.0-GAIN) + GAIN*(float)bar_heights[XSCALE(i+LOOKAHEAD)];
		// CONSIDER: Occasionally (with strong contrast colours like blue and cyan) you can actually see
		// that the bar_heights[] have flat tops over i=n..n+2.  We could fix this by interpolating like we did with y.
		#define MINCOL 0
		#define EXPLOSION 1.4


		// Color height:

		// cy = FLAMEHEIGHT + MINCOL - (WINHEIGHT-y) + heatHere*EXPLOSION;
		cy = FLAMEHEIGHT - 6 + MINCOL - (WINHEIGHT-y)*0.6 /*MINCOL*/ + heatHere*EXPLOSION*0.8;
		// cy = FLAMEHEIGHT + MINCOL + (0.75*heatHere+0.25*heatNow)*EXPLOSION - (WINHEIGHT-y);
		// cy = FLAMEHEIGHT + MINCOL + heatNow*EXPLOSION - (WINHEIGHT-y);
		//// heatNow varies at a gentle rate over time
		//// It was intended to stabilise the overall brightness when the spectrum is bouncing up and down rapidly.
		//// But it doesn't work, we haven't eliminated the bounce, only dampened it.
		//// We should be trying to normalise the average targetCol?
		//// Or with a better buffer, we could copy a stretch bar to fix the lower col.
		//// As it was, this acted too strongly on phat spectrums, and flattened the desirable colour spikes (could be fixed by tweaking other values).




		DEBUG("cy=%i\n",cy);
		gdk_draw_pixmap(draw_pixmap, gc, bar, 0, cy, i, y, 1, WINHEIGHT-y);

#ifdef MASK_TRANSPARENCY
		// gdk_draw_rectangle(mask, mgc, TRUE, i, y, 1, WINHEIGHT-y);
		gdk_draw_line(mask, mgc, i, y, i, WINHEIGHT-1);
#endif



#ifdef AIRFLOW

	{ int j;

	for (j=0;j<cy;j++) {

		//// TODO

#define flameHeight bar_heights[XSCALE(i)]

		//// Heat contribution from fire (music, spectrum).
#define fadeout ( (float)j / (float)WINHEIGHT )
#define hotness ( fadeout*0.5*flameHeight )

/*
		//// The current_heat for this cell is adjusted by airflow.
		//// Airflow for this cell (and surrounding(future?)?) is updated.
		airflow_left[i][j] = airflow_left[i][FitInt(j+upward_speed,WINHEIGHT)];
		// new_airflow_for_this_cell =
		//   a_bit_upwards +
		//   upwards_due_to_heat +
		//   upwards_due_to_flow(downwards) +
		//   speed_due_to_expansion_to_compensate_for_pressure
		// airflow_up[i][j] = 
		// airflow.cell[i][j].velocity

#define always_up 0.1
		airflow.cell[i][j].velocity.y = 
			always_up +
*/

		//// Pressure for this cell (and surrounding(future?)?) is applied.
		//// Pressure for this cell (and surrounding(future?)?) is updated.

		//// Temperature of this cell is calculated
		//// Colour of fire is generated, and plotted if needed.
		//// Transparency/fading/masking is applied.

		// Current total action of window?
		// Total variance?
#define outColor FitInt(hotness,FLAMEHEIGHT)

		// heatHere = outColor;
		// heatHere = FitInt(heatHere,FLAMEHEIGHT*2);
		// cy = FitInt(hotness*3,FLAMEHEIGHT*3);

		// float dx,dy;
		dx = -3 + (random()%7);
		dy = -3 + (random()%7);
		dy = dy + 1;

		// TODO: blur the pixel with those near it, to create airflow effect
		//       see rgb_buf in blur_score.c

		#define cellColor (3*hotness)
		DEBUG("cellColor=%f\n",(float)(cellColor));
		gdk_draw_pixmap(draw_pixmap, gc, bar, 0, cellColor, i, j, 1, 1);
		// We get full white followed by a few pixels of black.  So cellColor ~=> 3*FLAMEHEIGHT ?

	}

	}

#endif

	}



#ifndef PSEUDO_TRANSPARENCY
#ifdef MASK_TRANSPARENCY
	gtk_widget_shape_combine_mask(window,mask,0,0);
#endif
#endif

	gdk_window_clear(area->window);
	GDK_THREADS_LEAVE();

	return TRUE;
}

static void fsanalyzer_playback_start(void) {
	if(window) {
		gdk_window_set_back_pixmap(area->window,draw_pixmap,0);
		gdk_window_clear(area->window);
	}
}


static void fsanalyzer_playback_stop(void) {
	if(GTK_WIDGET_REALIZED(area)) {
		gdk_window_set_back_pixmap(area->window,bg_pixmap,0);
		gdk_window_clear(area->window);
	}
}

// From Audacious:
static void fsanalyzer_render_freq(gint16 data[2][256]) {
	gint i;
	gdouble y;

	if(!window)
		return;

	/* FIXME: can anything taken out of the main thread? */
	for (i = 0; i < SPECWIDTH; i++) {
		y = (gdouble)data[0][i] * (i + 1); /* Compensating the energy */
		y = ( log(y - x00) * scale + y00 ); /* Logarithmic amplitude */

		y = ( (dif-2)*y + /* FIXME: conditionals should be rolled out of the loop */
			(i==0           ? y : bar_heights[i-1]) +
			(i==SPECWIDTH-1 ? y : bar_heights[i+1])) / dif; /* Add some diffusion */
		y = ((tau-1)*bar_heights[i] + y) / tau; /* Add some dynamics */
		bar_heights[i] = (gint16)y;
	}
	draw_func(NULL);
	return;
}

// From XMMS:
static void sanalyzer_render_freq_DISABLED(gint16 data[2][256])
{
	gint i,c;
	gint y;

	// gint xscale[] = {0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255};
	// #define xscale(x) xscale[x]
	// #define xscale(x) XSCALE(x)
	// #define xscale(x) ((int)(x*SPECWIDTH/WINWIDTH))
	// #define xscale(x) ((int)(SPECWIDTH*pow((float)x/(float)WINWIDTH,0.5)))
	// #define xscale(x) ((int)(SPECWIDTH*pow((float)x/(float)SPECWIDTH,1.0)))
	#define xscale(x) (x/2)

	if(!window)
		return;
	for(i = 0; i < SPECWIDTH; i++)
	{
		for(c = xscale(i), y = 0; c < xscale(i + 1); c++)
		{
			if(data[0][c] > y)
				y = data[0][c];
		}
		y >>= 7;
		if(y != 0)
		{
			y = (gint)(log(y) * scale);
			if(y > WINHEIGHT - 1)
				y = WINHEIGHT - 1;
		}

		if(y > bar_heights[i])
			bar_heights[i] = y;
		else if(bar_heights[i] > 4)
			bar_heights[i] -= 4;
		else
			bar_heights[i] = 0;
		bar_heights[i] = bar_heights[i] / 2;
	}
	draw_func(NULL);
	return;
}

