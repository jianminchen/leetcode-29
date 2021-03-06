/* hello-dnd.c : Simple tutorial for GTK+ Drag-N-Drop
 * Copyright (C) 2005 Ryan McDougall.
 * Copyright (C) 2015 Leslie Zhai <xiang.zhai@i-soft.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <string.h>

#include "moses-stackswitcher.h"

/******************************************************************************/
#define _BYTE   8
#define _WORD   16
#define _DWORD  32

enum {
    COL_ICON,
    COL_NAME,
    ICON_COLS,
};

/******************************************************************************/
/* Define a list of data types called "targets" that a destination widget will
 * accept. The string type is arbitrary, and negotiated between DnD widgets by
 * the developer. An enum or GQuark can serve as the integer target id. */
enum {
    TARGET_INT32,
    TARGET_STRING,
    TARGET_ROOTWIN
};

/* datatype (string), restrictions on DnD (GtkTargetFlags), datatype (int) */
static GtkTargetEntry target_list[] = {
    { "INTEGER",    0, TARGET_INT32 },
    { "STRING",     0, TARGET_STRING },
    { "text/plain", 0, TARGET_STRING },
    { "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};

static guint n_targets = G_N_ELEMENTS (target_list);
static GtkIconTheme *icon_theme;
static GtkWidget *well_dest;
static GtkWidget *stack;

/******************************************************************************/
/* Signal receivable by destination */

/* Emitted when the data has been received from the source. It should check
 * the GtkSelectionData sent by the source, and do something with it. Finally
 * it needs to finish the operation by calling gtk_drag_finish, which will emit
 * the "data-delete" signal if told to. */
static void drag_data_received_handl(GtkWidget *widget, 
                                     GdkDragContext *context, 
                                     gint x, 
                                     gint y, 
                                     GtkSelectionData *selection_data, 
                                     guint target_type, 
                                     guint time, 
                                     gpointer data)
{
    glong   *_idata;
    gchar   *_sdata;

    gboolean dnd_success = FALSE;
    gboolean delete_selection_data = FALSE;

    const gchar *name = gtk_widget_get_name(widget);
    g_print("%s: drag_data_received_handl\n", name);

    if (strcmp(name, "GtkIconView") == 0) {
        g_signal_stop_emission_by_name(widget, "drag-data-received");
    }

    /* Deal with what we are given from source */
    if (selection_data != NULL && 
       gtk_selection_data_get_length(selection_data) >= 0) {
        if (gdk_drag_context_get_suggested_action(context) == GDK_ACTION_ASK) {
            /* Ask the user to move or copy, then set the context action. */
        }

        if (gdk_drag_context_get_suggested_action(context) == GDK_ACTION_MOVE)
            delete_selection_data = TRUE;

        /* Check that we got the format we can use */
        g_print(" Receiving ");
        switch (target_type) {
        case TARGET_INT32:
            if (strcmp(name, "GtkIconView") == 0) {
                _sdata = (gchar*)gtk_selection_data_get_data(selection_data);
                g_print("string: %s", _sdata);
                
                GtkTreeModel *model = gtk_icon_view_get_model(GTK_ICON_VIEW(widget));
                GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(icon_theme,
                    _sdata, 48, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
                GtkTreeIter iter;

                gtk_list_store_append(GTK_LIST_STORE(model), &iter);
                gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                    COL_ICON, pixbuf, COL_NAME, _sdata, -1);
            } else {
                _idata = (glong*)gtk_selection_data_get_data(selection_data);
                g_print("integer: %ld", *_idata);
            }
            dnd_success = TRUE;
            break;

        case TARGET_STRING:
            _sdata = (gchar*)gtk_selection_data_get_data(selection_data);
            g_print("string: %s", _sdata);
            dnd_success = TRUE;
            if (strcmp(name, "GtkLabel") == 0)
                gtk_label_set_label(GTK_LABEL(well_dest), _sdata);
            
            break;

        default:
            g_print("nothing good");
        }

        g_print (".\n");
    }

    if (dnd_success == FALSE) {
        g_print ("DnD data transfer failed!\n");
    }

    gtk_drag_finish(context, dnd_success, delete_selection_data, time);
}

/* Emitted when a drag is over the destination */
static gboolean drag_motion_handl(GtkWidget *widget, 
                                  GdkDragContext *context, 
                                  gint x, 
                                  gint y, 
                                  guint time, 
                                  gpointer user_data)
{
    const gchar *name = gtk_widget_get_name(widget);
    gchar *stack_child_name = NULL;
    GtkWidget *stack_child = NULL;
    unsigned int index = 0;
    char buf[16] = {'\0'};
    GtkAllocation allocation;

    if (strcmp(name, "GtkIconView") != 0)
        return FALSE;

    gtk_widget_get_allocation(widget, &allocation);

    stack_child_name = gtk_stack_get_visible_child_name(stack);
    if (stack_child_name == NULL)
        return FALSE;

    if (x < 40)
        index = atoi(stack_child_name) - 1;
    else if (x > allocation.width - 40)
        index = atoi(stack_child_name) + 1;

    if (index) {
        snprintf(buf, sizeof(buf) - 1, "%d", index);
        stack_child = gtk_stack_get_child_by_name(stack, buf);
        if (stack_child)
            gtk_stack_set_visible_child(stack, stack_child);
    }

    return FALSE;
}

/* Emitted when a drag leaves the destination */
static void drag_leave_handl(GtkWidget *widget, 
                             GdkDragContext *context, 
                             guint time, 
                             gpointer user_data)
{
    const gchar *name = gtk_widget_get_name(widget);
    g_print("%s: drag_leave_handl\n", name);
}

/* Emitted when the user releases (drops) the selection. It should check that
 * the drop is over a valid part of the widget (if its a complex widget), and
 * itself to return true if the operation should continue. Next choose the
 * target type it wishes to ask the source for. Finally call gtk_drag_get_data
 * which will emit "drag-data-get" on the source. */
static gboolean drag_drop_handl(GtkWidget *widget, 
                                GdkDragContext *context, 
                                gint x, 
                                gint y, 
                                guint time, 
                                gpointer user_data)
{
    gboolean        is_valid_drop_site;
    GdkAtom         target_type;

    const gchar *name = gtk_widget_get_name(widget);
    g_print("%s: drag_drop_handl\n", name);

    /* Check to see if (x,y) is a valid drop site within widget */
    is_valid_drop_site = TRUE;

    /* If the source offers a target */
    if (gdk_drag_context_list_targets(context)) {
        /* Choose the best target type */
        target_type = GDK_POINTER_TO_ATOM(
            g_list_nth_data(gdk_drag_context_list_targets(context), 
            TARGET_STRING));

        /* Request the data from the source. */
        gtk_drag_get_data
        (
            widget,         /* will receive 'drag-data-received' signal */
            context,        /* represents the current state of the DnD */
            target_type,    /* the target type we want */
            time            /* time stamp */
        );
    }
    /* No target offered by source => error */
    else {
        is_valid_drop_site = FALSE;
    }

    return  is_valid_drop_site;
}


/******************************************************************************/
/* Signals receivable by source */

/* Emitted after "drag-data-received" is handled, and gtk_drag_finish is called
 * with the "delete" parameter set to TRUE (when DnD is GDK_ACTION_MOVE). */
static void
drag_data_delete_handl
(GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
        // We aren't moving or deleting anything here
        const gchar *name = gtk_widget_get_name (widget);
        g_print ("%s: drag_data_delete_handl\n", name);
}

/* Emitted when the destination requests data from the source via
 * gtk_drag_get_data. It should attempt to provide its data in the form
 * requested in the target_type passed to it from the destination. If it cannot,
 * it should default to a "safe" type such as a string or text, even if only to
 * print an error. Then use gtk_selection_data_set to put the source data into
 * the allocated selection_data object, which will then be passed to the
 * destination. This will cause "drag-data-received" to be emitted on the
 * destination. GdkSelectionData is based on X's selection mechanism which,
 * via X properties, is only capable of storing data in blocks of 8, 16, or
 * 32 bit units. */
static void drag_data_get_handl(GtkWidget *widget, 
                                GdkDragContext *context, 
                                GtkSelectionData *selection_data, 
                                guint target_type, 
                                guint time, 
                                gpointer user_data)
{
    const gchar *name = gtk_widget_get_name(widget);
    const gchar *string_data = "This is data from the source.";
    const glong integer_data = 42;

    g_print("%s: drag_data_get_handl\n", name);
    g_assert(selection_data != NULL);

    if (strcmp(name, "GtkButton") == 0)
        string_data = gtk_button_get_label(GTK_BUTTON(widget));

    if (strcmp(name, "GtkIconView") == 0) {
        GtkTreeModel *model = gtk_icon_view_get_model(GTK_ICON_VIEW(widget));
        GList *items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(widget));
        GtkTreeIter iter;

        for (GList *i = items; i; i = g_list_next(i)) {
            char *name = NULL;

            gtk_tree_model_get_iter(model, &iter, i->data);
            gtk_tree_model_get(model, &iter, COL_NAME, &name, -1);
            string_data = name;
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        }
    }

    g_print(" Sending ");
    switch (target_type) {
        /* case TARGET_SOME_OBJECT:
         * Serialize the object and send as a string of bytes.
         * Pixbufs, (UTF-8) text, and URIs have their own convenience
         * setter functions */

    case TARGET_INT32:
        if (strcmp(name, "GtkIconView") == 0) {
            g_print("string: %s", string_data);
            gtk_selection_data_set
            (
                selection_data, 
                gtk_selection_data_get_target(selection_data),
                _BYTE,
                (gchar*) string_data, 
                strlen(string_data)
            );
        } else {
            g_print("integer: %ld", integer_data);
            gtk_selection_data_set
            (
                selection_data,         /* Allocated GdkSelectionData object */
                gtk_selection_data_get_target(selection_data), /* target type */
                _DWORD,                 /* number of bits per 'unit' */
                (guchar*) &integer_data,/* pointer to data to be sent */
                sizeof(integer_data)   /* length of data in units */
            );
        }
        break;

    case TARGET_STRING:
        g_print("string: %s", string_data);
        gtk_selection_data_set
        (
            selection_data,
            gtk_selection_data_get_target(selection_data),
            _BYTE,
            (guchar*) string_data,
            strlen(string_data)
        );
        break;

    case TARGET_ROOTWIN:
        g_print("Dropped on the root window!\n");
        break;

    default:
        /* Default to some a safe target instead of fail. */
        g_assert_not_reached();
    }

    g_print(".\n");
}

/* Emitted when DnD begins. This is often used to present custom graphics. */
static void
drag_begin_handl
(GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
        const gchar *name = gtk_widget_get_name (widget);
        g_print ("%s: drag_begin_handl\n", name);
}

/* Emitted when DnD ends. This is used to clean up any leftover data. */
static void
drag_end_handl
(GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
        const gchar *name = gtk_widget_get_name (widget);
        g_print ("%s: drag_end_handl\n", name);
}

static void m_store_add_item(GtkListStore *store, char *id) 
{
    GdkPixbuf *pixbuf;
    GtkTreeIter iter;

    gtk_list_store_append(store, &iter);
    pixbuf = gtk_icon_theme_load_icon(icon_theme, id, 48,
        GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    gtk_list_store_set(store, &iter, COL_ICON, pixbuf, COL_NAME, id, -1);
}

/******************************************************************************/
int
main(int argc, char **argv)
{
    GdkScreen       *screen;
    GtkWidget       *window;
    GtkWidget       *vbox;
    GtkWidget       *hbox;
    GtkWidget       *coin_source;
    GtkWidget       *directions_label;
    GtkWidget       *stack_switcher;
    GtkWidget       *icon_view_source;
    GtkWidget       *icon_view_dest;
    guint           win_xsize       = 800;
    guint           win_ysize       = 600;
    guint           spacing         = 5;
    GtkListStore    *store;

    /* Always start GTK+ first! */
    gtk_init (&argc, &argv);


    /* Create the widgets */
    window  = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    vbox    = gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
    hbox    = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);

    coin_source     = gtk_button_new_with_label("[coins]");
    well_dest       = gtk_label_new("[a well]");

    directions_label = gtk_label_new("drag a coin and drop it in the well");

    screen = gdk_display_get_default_screen(gdk_display_get_default());
    icon_theme = gtk_icon_theme_get_for_screen(screen);
    store = gtk_list_store_new(ICON_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
    m_store_add_item(store, "microphone-sensitivity-high");
    m_store_add_item(store, "task-due");
    m_store_add_item(store, "edit-clear-all");
    m_store_add_item(store, "phone");
    m_store_add_item(store, "audio-headphones");
    m_store_add_item(store, "application-certificate");
    m_store_add_item(store, "x-office-calendar");
    m_store_add_item(store, "face-kiss");
    m_store_add_item(store, "face-laugh");
    m_store_add_item(store, "face-monkey");

    stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), 
        GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);

    stack_switcher = moses_stack_switcher_new(); 
    gtk_widget_set_name(stack_switcher, "moses-stackswitcher");
    gtk_widget_set_halign(stack_switcher, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(stack_switcher, GTK_ALIGN_END);
    moses_stack_switcher_set_stack(MOSES_STACK_SWITCHER(stack_switcher),
        GTK_STACK(stack));

    icon_view_source = gtk_icon_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_set_size_request(icon_view_source, 800, 500);
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(icon_view_source), COL_NAME);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(icon_view_source), COL_ICON);
    gtk_stack_add_titled(GTK_STACK(stack), icon_view_source, "1", "Page 1");

    icon_view_dest = gtk_icon_view_new_with_model(
        GTK_TREE_MODEL(gtk_list_store_new(ICON_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING)));
    gtk_widget_set_size_request(icon_view_dest, 800, 500);
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(icon_view_dest), COL_NAME);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(icon_view_dest), COL_ICON);
    gtk_stack_add_titled(GTK_STACK(stack), icon_view_dest, "2", "Page 2");

    /* Pack the widgets */
    gtk_container_add (GTK_CONTAINER (window), vbox);

    gtk_container_add (GTK_CONTAINER (hbox), coin_source);
    gtk_container_add (GTK_CONTAINER (hbox), directions_label);
    gtk_container_add (GTK_CONTAINER (hbox), well_dest);

    gtk_container_add(GTK_CONTAINER(vbox), hbox);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
    gtk_container_add(GTK_CONTAINER(hbox), stack);

    gtk_container_add(GTK_CONTAINER(vbox), hbox);
    gtk_container_add(GTK_CONTAINER(vbox), stack_switcher);

    /* Make the window big enough for some DnD action */
    gtk_window_set_default_size (GTK_WINDOW(window), win_xsize, win_ysize);


    /* Make the "well label" a DnD destination. */
    gtk_drag_dest_set
    (
        well_dest,              /* widget that will accept a drop */
        GTK_DEST_DEFAULT_MOTION /* default actions for dest on DnD */
        | GTK_DEST_DEFAULT_HIGHLIGHT,
        target_list,            /* lists of target to support */
        n_targets,              /* size of list */
        GDK_ACTION_COPY         /* what to do with data after dropped */
    );

    gtk_drag_dest_set(
        icon_view_dest, 
        GTK_DEST_DEFAULT_ALL, 
        target_list, 
        n_targets, 
        GDK_ACTION_COPY);

    gtk_drag_dest_set(
        icon_view_source, 
        GTK_DEST_DEFAULT_ALL, 
        target_list, 
        n_targets, 
        GDK_ACTION_COPY
    );

    /* Make the "coin button" a DnD source.
     * Why doesn't GtkLabel work here? 
     * See Caveat Window above
     */
    gtk_drag_source_set
    (
        coin_source,            /* widget will be drag-able */
        GDK_BUTTON1_MASK,       /* modifier that will start a drag */
        target_list,            /* lists of target to support */
        n_targets,              /* size of list */
        GDK_ACTION_COPY         /* what to do with data after dropped */
    );

    gtk_drag_source_set
    (
        icon_view_source,
        GDK_BUTTON1_MASK,
        target_list,
        n_targets,
        GDK_ACTION_COPY
    );

    gtk_drag_source_set
    (
        icon_view_dest,
        GDK_BUTTON1_MASK,
        target_list,
        n_targets,
        GDK_ACTION_COPY
    );

    /* Connect the signals */
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    /* All possible destination signals */
    g_signal_connect (well_dest, "drag-data-received",
                G_CALLBACK(drag_data_received_handl), NULL);

    g_signal_connect (well_dest, "drag-leave",
                G_CALLBACK (drag_leave_handl), NULL);

    g_signal_connect (well_dest, "drag-motion",
                G_CALLBACK (drag_motion_handl), NULL);

    g_signal_connect (well_dest, "drag-drop",
                G_CALLBACK (drag_drop_handl), NULL);

    g_signal_connect(icon_view_dest, "drag-data-received", 
        G_CALLBACK(drag_data_received_handl), NULL);

    g_signal_connect(icon_view_source, "drag-data-received", 
        G_CALLBACK(drag_data_received_handl), NULL);

    g_signal_connect(icon_view_dest, "drag-motion", 
        G_CALLBACK(drag_motion_handl), NULL);

    g_signal_connect(icon_view_source, "drag-motion", 
        G_CALLBACK(drag_motion_handl), NULL);

    /* All possible source signals */
    g_signal_connect (coin_source, "drag-data-get",
                G_CALLBACK (drag_data_get_handl), NULL);

    g_signal_connect (coin_source, "drag-data-delete",
                G_CALLBACK (drag_data_delete_handl), NULL);

    g_signal_connect (coin_source, "drag-begin",
                G_CALLBACK (drag_begin_handl), NULL);

    g_signal_connect (coin_source, "drag-end",
                G_CALLBACK (drag_end_handl), NULL);
    
    g_signal_connect(icon_view_source, "drag-data-get", 
            G_CALLBACK(drag_data_get_handl), NULL);

    g_signal_connect(icon_view_dest, "drag-data-get", 
        G_CALLBACK(drag_data_get_handl), NULL);

    /* Show the widgets */
    gtk_widget_show_all (window);

    /* Start the even loop */
    gtk_main ();

    return 0;
}
